/**
  ************************************************************************************
  * @file    pitchfork_irq.c
  * @author  stf
  * @brief   This file provides PITCHFORK usb irq handlers
  ************************************************************************************
  */
/* TOC                                                                */
/*        ----===== typedefs =====----                                */
/*        ----===== imported globals =====----                        */
/*        ----===== local globals =====----                           */
/*        ----===== exported globals =====----                        */
/*        ----===== helper functions =====----                        */
/*        ----===== handlers for specific operations =====----        */
/*        ----===== IRQ handlers for CTRL and DATA EPs =====----      */
/*        ----===== userspace handler for CTRL and DATA EPs =====---- */
/*        ----===== main userspace handler =====----                  */

#include "usb.h"
#include "stm32f.h"
#include <utils.h>
#include <string.h>
#include "stfs.h"
#include "dma.h"
#include "master.h"
#include "oled.h"
#include "keys.h"
#include "delay.h"
#include "widgets.h"
#include "pitchfork.h"
#include "pf_store.h"

#include "axolotl.h"
#include "xeddsa.h"

#include <crypto_generichash.h>
#include "randombytes_pitchfork.h"
#include "crypto_scalarmult_curve25519.h"

#include "pqcrypto_sign.h"

#define outstart32 (outbuf+crypto_secretbox_ZEROBYTES)

/*        ----===== typedefs =====----        */
/**
  * @brief  CMD_Buffer: USB read buffer for commands
  */
typedef struct {
  Buffer_State state;                                        /* buffer state (i/o/c) */
  int size;                                                  /* size of buffer */
  unsigned char buf[128+64];                                 /* Buffer */
} CMD_Buffer;

/*        ----===== imported globals =====----        */

extern usbd_device *usbd_dev;

/*        ----===== local globals =====----        */

/**
  * @brief  active_buf: index in bufs to active input buffer
  */
static unsigned char active_buf = 0;

/**
  * @brief  modus: state of PITCHFORK, off is PITCHFORK_CMD_STOP
  */
CRYPTO_CMD modus=PITCHFORK_CMD_STOP;

/**
  * @brief  hash_state: for conserving hash_states across multiple input buf
  */
static crypto_generichash_state hash_state;

/**
  * @brief  blocked: 1 if stalled.
  */
unsigned char blocked = 0;

static CMD_Buffer cmd_buf;
char cmd_blocked=0;
static unsigned char nonce[crypto_secretbox_NONCEBYTES];

/**
  * @brief  params: for passing params from ctl to buf handlers
  *         ctl is in irq, while buf handler is from mainloop
  */
static unsigned char params[128+64];

/*        ----===== exported globals =====----        */
/**
  * @brief  bufs: input double buffering
  */
Buffer bufs[2];

/**
  * @brief  outbuf: output buffer
  */
unsigned char outbuf[crypto_secretbox_ZEROBYTES+BUF_SIZE];

/*        ----===== helper functions =====----        */

/**
  * @brief  clears the buffer for commands, releases locks
  * @retval None
  */
static void cmd_clear(void) {
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  cmd_buf.state=INPUT;
  cmd_buf.size=0;
  if(cmd_blocked!=0) {
    cmd_blocked=0;
  }
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
}

/**
  * @brief  toggle_buf: tries to switch double buffering
  * @param  None
  * @retval ptr to active_buf, or -1 on error
  */
static int toggle_buf(void) {
  if(bufs[!active_buf].state == INPUT && bufs[!active_buf].size==0) {
    if(bufs[active_buf].state == INPUT)
      bufs[active_buf].state = OUTPUT;
    active_buf = !active_buf;
    return active_buf;
  }
  return -1;
}

/**
  * @brief  pf_reset: resets PITCHFORK mode
  * @param  None
  * @retval None
  */
static void pf_reset(void) {
  unsigned int i;
  modus = PITCHFORK_CMD_STOP;
  bufs[0].size = 0;
  bufs[1].size = 0;
  bufs[0].state = INPUT;
  bufs[1].state = INPUT;
  blocked = 0;
  for (i=0;i<(sizeof(params)>>2);i++) ((unsigned int*) params)[i]=0;
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  cmd_clear();
  oled_print(40,56, "           ", Font_8x8);
}

static int query_user(char* op) {
  int res=0, retries=16384, samples;
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  oled_clear();
  oled_print_inv(0,18,op, Font_8x8);
  oled_print_inv(0,36,"<", Font_8x8);
  oled_print(16,36,"reject", Font_8x8);
  oled_print_inv(0,45,">", Font_8x8);
  oled_print(16,45,"allow", Font_8x8);
  uint8_t keys;
  while(retries>0) {
    if((retries%128)==0) {
      if((retries%256)==0) {
        oled_print(0,0,"client wants to", Font_8x8);
      } else {
        oled_print_inv(0,0,"client wants to", Font_8x8);
      }
    }
    //while((keys=keys_pressed())==0);
    //keys=keys_pressed();
    for(samples=32,keys=0;samples>0 && keys==0;keys=keys_pressed(), samples--);
    if(keys & BUTTON_LEFT) {
      res=0;
      break;
    } else if(keys & BUTTON_RIGHT) {
      res=1;
      usb_write((unsigned char*) "ok", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
      break;
    }
    retries--;
  }
  if(res==0) {
    cmd_clear();
    usb_write((unsigned char*) "err: rejected", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
  }
  gui_refresh=1;
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  mDelay(200);
  statusline();
  return res;
}

static void pf_send(uint8_t *buf, int size, CRYPTO_CMD m) {
  int len, i;
  for(i=0;i<size && (modus == m);i+=len) {
    len = (size-i)>=64?64:(size-i);
    /* usb_write(buf+i,len,32,USB_CRYPTO_EP_DATA_OUT); */
    while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, buf+i, len) == 0) &&
          (modus == m))
      usbd_poll(usbd_dev); // todo why are we polling here?
  }
  if(size%64==0) {
    uDelay(100);
    usb_write(NULL,0,32,USB_CRYPTO_EP_DATA_OUT);
  }
}

/*        ----===== handlers for specific operations =====----        */

static void incnonce() {
  int i;
  uint32_t *ptr = (uint32_t *) nonce;
  for(i=crypto_secretbox_NONCEBYTES/4-1;i>=0;i--) {
    ptr[i]++;
    if(ptr[i]!=0) break;
  }
}

/**
  * @brief  encrypt_block: handler for encrypt op
  * @param  buf: ptr one of the input buffer structs
  *         encrypts buffer and sends ciphertext back via usb
  * @retval None
  */
static void encrypt_block(Buffer *buf) {
  int i, len, size = buf->size;
  // zero out beginning of plaintext as demanded by nacl
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf->buf)[i]=0;
  // encrypt (key is stored in beginning of params)
  crypto_secretbox(outbuf, buf->buf, size+crypto_secretbox_ZEROBYTES, nonce, params);
  size+=crypto_secretbox_MACBYTES; // add mac size to total size
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    irq_disable(NVIC_OTG_FS_IRQ);
    usb_write(outbuf+16+i, len, 0, USB_CRYPTO_EP_DATA_OUT);
    irq_enable(NVIC_OTG_FS_IRQ);
  }
  incnonce();
}

/**
  * @brief  decrypt_block: handler for decrypt op,
  *         decrypts buffer and sends plaintext back via usb
  * @param  buf: ptr one of the input buffer structs
  * @retval None
  */
static void decrypt_block(Buffer* buf) {
  int i, len;
  // substract nonce size from total size (16B)
  int size = buf->size - crypto_secretbox_MACBYTES;
  // zero out crypto_secretbox_BOXZEROBYTES preamble
  // overwriting the end of the nonce
  memset(buf->start - crypto_secretbox_BOXZEROBYTES,0,crypto_secretbox_BOXZEROBYTES);
  // decrypt (key is stored in beginning of params)
  if(-1 == crypto_secretbox_open(outbuf,                                       // m
                                 (buf->start) - crypto_secretbox_BOXZEROBYTES, // c + preamble
                                 size+crypto_secretbox_ZEROBYTES,              // clen = len(plain)+2x(boxzerobytes)
                                 nonce,                                        // n
                                 params)) {
    usb_write((unsigned char*) "err: corrupt", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    irq_disable(NVIC_OTG_FS_IRQ); // todo why disable irqs, and why in-loop?
    usb_write(outstart32+i, len, 0, USB_CRYPTO_EP_DATA_OUT);
    irq_enable(NVIC_OTG_FS_IRQ); // todo why reenable irqs, and why in-loop?
  }
  incnonce();
}

/**
  * @brief  hash_block: handler for sign/verify ops
  * @param  buf: ptr one of the input buffer structs
  * @retval None
  */
static void hash_block(const Buffer *buf) {
  crypto_generichash_update(&hash_state, buf->start, buf->size);
}

/**
  * @brief  kex_start: starts a key-exchange over USB
  * @param  None
  * @retval None
  */
void kex_start() {
  Axolotl_KeyPair kp;
  if(load_ltkeypair(&kp)==0) {
    usb_write((unsigned char*) "err: no pubkey", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }

  // generate prekey
  Axolotl_PreKey my_pk;
  Axolotl_prekey_private my_sk;
  axolotl_prekey(&my_pk, &my_sk, &kp);
  memset((uint8_t*) &kp, 0, sizeof(kp)); // clear keymaterial
  // store my_sk in /prekeys/
  if(0!=store_key((uint8_t*) &my_sk, sizeof(my_sk), "/prekeys/", my_pk.ephemeralkey, NULL, 0)) {
    usb_write((unsigned char*) "err: store pk", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }
  // send off my_pk
  modus = PITCHFORK_CMD_KEX_START;
  usb_write((unsigned char*) "tx", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
  pf_send((uint8_t*) &my_pk, sizeof(my_pk), PITCHFORK_CMD_KEX_START);
  pf_reset();
}

int kex_resp(Buffer *buf) {
  if(buf->size!=sizeof(Axolotl_PreKey)) {
    return -1;
  }
  // respond with handshake with prekey
  Axolotl_Resp my_pk;
  Axolotl_PreKey *o_pk = (Axolotl_PreKey*) buf->start;
  Axolotl_prekey_private my_sk;

  // load lt private key and derive pubkey from it
  Axolotl_KeyPair kp;
  if(load_ltkeypair(&kp)==0) {
    usb_write((unsigned char*) "err: no pubkey", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
    cmd_clear();
    return -1;
  }

  // init own prekey
  axolotl_kexresp(&my_pk, &my_sk, &kp);

  Axolotl_ctx ctx;
  if(axolotl_handshake(&ctx, &my_pk, o_pk, &my_sk)!=0) {
    usb_write((unsigned char*) "err: fail", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    cmd_clear();
    return -1;
  }
  // save ctx (and other keys)
  if(0!=save_ax(&ctx, o_pk->identitykey, params+1, params[0])) {
    usb_write((unsigned char*) "err: save fail", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
    cmd_clear();
    return -1;
  }
  // send off my_pk
  memcpy(&my_pk.prekeyid,o_pk->ephemeralkey,16); // to remind the initiator what he used
  pf_send((uint8_t*)&my_pk, sizeof(my_pk), PITCHFORK_CMD_KEX_RESPOND);

  return 0;
}

int kex_end(Buffer *buf) {
  if(buf->size!=sizeof(Axolotl_Resp)) {
    return -1;
  }
  Axolotl_Resp *o_pk = (Axolotl_Resp*) buf->start;
  // load my_sk from /prekeys
  uint8_t prekey[]="/prekeys/                                ";
  stohex(prekey+9, o_pk->prekeyid, 16);
  Axolotl_prekey_private my_sk;
  if(cread(prekey, (uint8_t*) &my_sk, sizeof(my_sk))!= sizeof(my_sk)) {
    memset((uint8_t*) &my_sk,0,sizeof(my_sk));
    return -1;
  }

  Axolotl_ctx ctx;
  if(axolotl_handshake_resp(&ctx, o_pk, &my_sk)!=0) {
    usb_write((unsigned char*) "err: fail", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    cmd_clear();
    return -1;
  }
  // store ctx (and other keys)
  if(0!=save_ax(&ctx, o_pk->identitykey, params+1, params[0])) {
    usb_write((unsigned char*) "err: save fail", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
    cmd_clear();
    return -1;
  }
  // delete prekey from /prekeys
  stfs_unlink(prekey);
  usb_write((unsigned char*) "OK", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
  return 0;
}

/**
  * @brief  ax_send_init()
  * @param  None
  * @retval None
  */
int ax_send_init(uint8_t *peer, uint32_t peerlen) {
  // todo merge the innards of this back into axolotl.c
  if( peerlen == 0 || peerlen >= PEER_NAME_MAX) {
    usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }

  // try to load ax session context
  Axolotl_ctx ctx;
  uint8_t cpath[]="/ax/                                /                                ";
  uint8_t peerid[STORAGE_ID_LEN];
  if(topeerid(peerid, peer, peerlen)!=0) return -1;
  stohex(cpath+4, peerid, STORAGE_ID_LEN);
  if(0!=load_key(cpath, 36, (uint8_t*) &ctx, sizeof(ctx))) {
    memset((uint8_t*) &ctx,0,sizeof(ctx));
    return -1;
  }

  uint8_t *hnonce=outbuf;
  int i,j;
  // check if we have a DHRs
  for(i=0,j=0;i<crypto_secretbox_KEYBYTES;i++) if(ctx.dhrs.sk[i]==0) j++;
  if(j==crypto_secretbox_KEYBYTES) { // if not, generate one, and reset counter
    randombytes_buf(ctx.dhrs.sk,crypto_scalarmult_curve25519_BYTES);
    crypto_scalarmult_curve25519_base(ctx.dhrs.pk, ctx.dhrs.sk);
    ctx.pns=ctx.ns;
    ctx.ns=0;
  }
  // derive message key
  crypto_generichash(params, crypto_secretbox_KEYBYTES,  // output
                     ctx.cks, crypto_secretbox_KEYBYTES, // msg
                     (uint8_t*) "MK", 2);                // "MK")
  // hnonce
  randombytes_buf(hnonce,crypto_secretbox_NONCEBYTES);

  // calculate Enc(HKs, Ns || PNs || DHRs)
  uint8_t header[PADDEDHCRYPTLEN]; // includes nacl padding
  memset(header,0,sizeof(header));
  // concat ns || pns || dhrs
  memcpy(header+32,&ctx.ns, sizeof(long long));
  memcpy(header+32+sizeof(long long),&ctx.pns, sizeof(long long));
  memcpy(header+32+sizeof(long long)*2, ctx.dhrs.pk, crypto_scalarmult_curve25519_BYTES);

  uint8_t header_enc[PADDEDHCRYPTLEN]; // also nacl padded
  // encrypt them
  crypto_secretbox(header_enc, header, sizeof(header), hnonce, ctx.hks);

  // unpad to output buf
  memcpy(hnonce+crypto_secretbox_NONCEBYTES, header_enc+16, sizeof(header_enc)-16);
  // send off headers
  pf_send(outbuf, crypto_secretbox_NONCEBYTES+sizeof(header_enc)-16, PITCHFORK_CMD_AX_SEND);

  ctx.ns++;
  crypto_generichash(ctx.cks, crypto_scalarmult_curve25519_BYTES, // output
                     ctx.cks, crypto_scalarmult_curve25519_BYTES, // msg
                     (uint8_t*) "CK", 2);                          // no key

  // save ax session ctx
  if(0!=write_enc(cpath, (uint8_t*) &ctx, sizeof(ctx))) {
    memset(params,0,crypto_secretbox_KEYBYTES);
    memset((uint8_t*) &ctx,0,sizeof(ctx));
    return -1;
  }

  return 0;
}

static void ax_recv_init(Buffer *buf) {
  uint32_t out_len;

  Axolotl_ctx ctx;
  uint8_t cpath[]="/ax/                                /                                ";
  uint8_t peerid[STORAGE_ID_LEN];
  if(topeerid(peerid, params+1, params[0])!=0) {
    return;
  }
  stohex(cpath+4, peerid, STORAGE_ID_LEN);
  // todo try with all existing ctx for peer
  if(0!=load_key(cpath, 36, (uint8_t*) &ctx, sizeof(ctx))) {
    memset((uint8_t*) &ctx,0,sizeof(ctx));
    usb_write((unsigned char*) "err: no ctx", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }

  if(0!=ax_recv(&ctx, outbuf, &out_len, cmd_buf.buf+1, nonce,
                cmd_buf.buf+1+crypto_secretbox_NONCEBYTES,
                buf->start-16, buf->size+16, params)) {
    usb_write((unsigned char*) "err: corrupt", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }
  // save ax session ctx
  if(0!=write_enc(cpath, (uint8_t*) &ctx, sizeof(ctx))) {
    memset(params,0,crypto_secretbox_KEYBYTES);
    memset((uint8_t*) &ctx,0,sizeof(ctx));
    return;
  }
  memset((uint8_t*) &ctx,0,sizeof(ctx));
  // send usb packet sized result
  usb_write((unsigned char*) "tx", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
  pf_send(outstart32, out_len, PITCHFORK_CMD_AX_RECEIVE);
  memset(outstart32,0,out_len);

  if(out_len==32768) {
    modus=PITCHFORK_CMD_DECRYPT;
    incnonce();
  }
}

/**
  * @brief  sign_msg: final handler for sign ops
  * @param  None
  * @retval None
  */
static void sign_msg(void) {
  uint8_t h[32];

  crypto_generichash_final(&hash_state, h, 32);
  // todo output doc hash for verification with host doc hash

  // load ltkey, perform xeddsa with it on the finalhash
  Axolotl_KeyPair kp;
  if(0==load_ltkeypair(&kp)) {
    usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }
  // sign with xeddsa, send back sig
  uint8_t sig[64], random[64];
  randombytes_buf(random,sizeof(random));
  if(0!=xed25519_sign(sig, kp.sk, h, sizeof(h), random)) {
    usb_write((unsigned char*) "err: sign", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }
  memset((uint8_t*) &kp,0,sizeof(kp));

  pf_send(sig, sizeof(sig), PITCHFORK_CMD_SIGN);
  memset(sig,0,sizeof(sig));
}

/**
  * @brief  verify_msg: final handler for sign ops
  * @param  None
  * @retval None
  */
static void verify_msg(void) {
  uint8_t h[32];

  crypto_generichash_final(&hash_state, h, 32);
  // verify with xeddsa, send bool
  oled_clear();
  oled_print(0,23,"     message", Font_8x8);
  if(0!=xed25519_verify(params /* 64 bytes */, params+64 /* 32 bytes */, h, sizeof(h))) {
    oled_print(0,32,"     invalid", Font_8x8);
    usb_write((unsigned char*) "0", 1, 32,USB_CRYPTO_EP_DATA_OUT);
  } else {
    oled_print(0,32,"       ok", Font_8x8);
    usb_write((unsigned char*) "1", 1, 32,USB_CRYPTO_EP_DATA_OUT);
  }
}

/**
  * @brief  pqsign_msg: final handler for sign ops
  * @param  None
  * @retval None
  */
static void pqsign_msg(void) {
  uint8_t h[32];

  crypto_generichash_final(&hash_state, h, 32);
  // todo output doc hash for verification with host doc hash

  uint8_t sk[PQCRYPTO_SECRETKEYBYTES];
  uint8_t path[]="/sph/                                ";
  if(load_key(path, 4, sk, PQCRYPTO_SECRETKEYBYTES)==-1) {
    usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
    pf_reset();
    return;
  }

  oled_clear();
  oled_print(0,23,"Please Wait", Font_8x8);
  oled_print(0,32," doing pq magic ", Font_8x8);
  // save bufs
  uint8_t* olds1 = bufs[0].start, *olds2=bufs[1].start;
  // sign with sphincs, send back sig
  pqcrypto_sign((uint8_t*) bufs, h, sk);
  memset(sk,0,PQCRYPTO_SECRETKEYBYTES);

  pf_send((uint8_t*) bufs, PQCRYPTO_BYTES, PITCHFORK_CMD_PQSIGN);

  // restore bufs
  bufs[0].start=olds1; bufs[1].start=olds2;
}

/**
  * @brief  rng_handler: handler for ~
  * @param  None
  * @retval None
  */
static void rng_handler(void) {
  int i;
  randombytes_buf((void *) outbuf, BUF_SIZE);
  for(i=0;i<BUF_SIZE && (modus == PITCHFORK_CMD_RNG);i+=64) {
    while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf+i, 64) == 0) &&
          (modus == PITCHFORK_CMD_RNG))
      usbd_poll(usbd_dev); // todo why do we poll here? is this some artifact?
  }
  oled_print(40,56, "           ", Font_8x8);
}

/**
  * @brief  _listkeys: helper function lists all keys or keys belonging to a peerid
  * @param  
  * @retval amounts of bytes output to outptr, emits results via usb.
  */
static int _listkeys(uint8_t *path, int pathlen, int keysize, uint8_t *outptr) {
  const Inode_t *inode;
  ReaddirCTX kctx;
  int ret=0;
  path[pathlen-34]=0;
  stfs_opendir(path, &kctx);
  path[pathlen-34]='/';
  while((inode=stfs_readdir(&kctx))!=0) {
    if(inode->name_len!=32 || inode->type!=File) {
      continue; // todo flag error?
    }
    uint8_t keyid[STORAGE_ID_LEN];
    if(unhex(keyid, inode->name, STORAGE_ID_LEN*2)==-1) {
      // fail invalid hex digit in filename, ignore and skip
      continue;
    }

    // try to open key
    uint8_t sk[keysize];
    int retries=3, len;
    memcpy(path+pathlen-33, inode->name, 32);
    path[pathlen-1]=0;
    while((len=cread(path, sk, keysize))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }
    memset(sk,0,keysize); //clear key
    if(len<1) {
      // corrupt
      *outptr++=3;
    } else {
      // correct key
      *outptr++=2;
    }
    ret+=1+sizeof(keyid);
    memcpy(outptr,keyid,sizeof(keyid));
    outptr+=sizeof(keyid);
  }
  return ret;
}

/**
  * @brief  listkeys: lists all keys or keys belonging to a peerid
  * @param  peer: pointer to peer name or 0 if all keys
  * @retval None, emits results via usb.
  */
static void listkeys(const PF_KeyType type, const unsigned char *peer) {
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);

  uint8_t *prefix, prefixlen=-1, haspeer=-1;
  int keysize=0;
  switch(type) {
  case PF_KEY_LONGTERM: {
    prefix=(uint8_t*) "/lt/"; prefixlen=4; keysize=crypto_scalarmult_curve25519_BYTES; haspeer=0;
    break;
  }
  case PF_KEY_AXOLOTL: {
    prefix=(uint8_t*) "/ax/"; prefixlen=4; keysize=sizeof(Axolotl_ctx); haspeer=1;
    break;
  }
  case PF_KEY_SPHINCS: {
    prefix=(uint8_t*) "/sph/"; prefixlen=5; keysize=PQCRYPTO_SECRETKEYBYTES; haspeer=0;
    break;
  }
  case PF_KEY_SHARED: {
    prefix=(uint8_t*) "/keys/"; prefixlen=6; keysize=crypto_secretbox_KEYBYTES; haspeer=1;
    break;
  }
  case PF_KEY_PUBCURVE: {
    prefix=(uint8_t*) "/pub/"; prefixlen=5; keysize=crypto_scalarmult_curve25519_BYTES; haspeer=1;
    break;
  }
  case PF_KEY_PREKEY: {
    prefix=(uint8_t*) "/prekeys/"; prefixlen=9; keysize=sizeof(Axolotl_prekey_private); haspeer=0;
    break;
  }
  }
  int len=0, pathlen=prefixlen+32+1;
  if(haspeer>0) pathlen+=33;
  uint8_t path[pathlen];
  if(keysize==0 || haspeer==-1 || prefixlen==-1) {
    goto exit;
  }
  memcpy(path,prefix,prefixlen);

  unsigned char *outptr = outbuf, *tptr;

  if(haspeer==0) { // simplest case _listkeys for path
      outptr+=_listkeys(path, pathlen, keysize, outptr);
  } else if(peer!=NULL) {
    // easy case, we have a peer, and a type,
    // lets compose the path and list all keyids for the peer
    len=strlen((char*)peer);
    // assertion
    if(len<1 || len>PEER_NAME_MAX) {
      //fail, should never happen since the dispatcher handles this
      goto exit;
    }
    // we have a peer, set the peerid in the path
    uint8_t peerid[STORAGE_ID_LEN];
    if(topeerid(peerid, peer, len)!=0) {
      // fail no user initialized
      goto exit;
    }
    stohex(path+prefixlen,peerid, sizeof(peerid));
    path[prefixlen+32]='/';
    outptr+=_listkeys(path, pathlen, keysize, outptr);
  } else { // list keys for all peers
    ReaddirCTX pctx;
    path[prefixlen-1]=0;
    // iterate through all entries in prefix dir
    if(stfs_opendir(path, &pctx)!=0) {
      // fail
      goto exit;
    }
    path[prefixlen-1]='/';

    const Inode_t *inode;
    while((inode=stfs_readdir(&pctx))!=0) {
      if(inode->name_len!=32 || inode->type!=Directory) {
        continue; // todo flag error?
      }
      // resolve peer and call _listkeys on each key
      uint8_t peerpath[]="/peers/                                ";
      memcpy(peerpath+7,inode->name, inode->name_len);
      int retries=3;
      uint8_t cpeer[PEER_NAME_MAX];
      while((len=cread(peerpath, cpeer, sizeof(cpeer)))==-2 && retries-->=0) {
        erase_master_key();
        get_master_key("bad key");
      }
      if(len<1) {
        len=inode->name_len;
        *outptr++=1; // uknown user
        *outptr++=(uint8_t) len;
        memcpy(outptr,inode->name, len); // peerid
      } else {
        *outptr++=0; // known user
        *outptr++=(uint8_t) len;
        memcpy(outptr,cpeer,len); // username
      }
      outptr+=len;
      // iter over all keys with peer
      memcpy(path+(prefixlen),inode->name, inode->name_len);
      outptr+=_listkeys(path, pathlen, keysize, outptr);
    }
  }

  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);

  // write out result
  tptr=outbuf;
  // write out outbuf
  while(tptr+64<outptr) {
    usb_write(tptr, 64, 0, USB_CRYPTO_EP_DATA_OUT);
    tptr+=64;
  }
  // write out last packet
  if(tptr<outptr) {// short pkt
    usb_write(tptr, outptr-tptr, 0, USB_CRYPTO_EP_DATA_OUT);
  } else if(tptr==outptr) { // zlp
    usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 0);
  }

 exit:
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  pf_reset();
}

/*        ----===== IRQ handlers for CTRL and DATA EPs =====----        */
/**
  * @brief  usb rx callback for the ctl end point
  * @param  usbd_dev: pointer to usbd (libopencm3 style)
  * @param  ep: end point (libopencm3 style)
  * @retval None
  */
void handle_ctl(usbd_device *usbd_dev, const uint8_t ep) {
  if(cmd_buf.state!=INPUT) { // if not input userspace is still parsing the previous cmd
    uint8_t tmp[64];
    // peek into pkt and sink it
    int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, tmp, 64);
    if(len==1 && tmp[0] == PITCHFORK_CMD_STOP) {
      cmd_buf.buf[0] = PITCHFORK_CMD_STOP;
      memset(tmp,0,sizeof(tmp));
    } else {
      // ignore packet
      memset(tmp,0,sizeof(tmp));
      usb_write((uint8_t*) "err: mode", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    }

    cmd_blocked = 1;
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    return;
  }
  if(cmd_buf.size+64>sizeof(cmd_buf.buf)) { // should not happen, as we set output in state before
    // overflowing - also illegal to send cmds longer than 195 bytes
    usb_write((uint8_t*) "err: oflow", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
    // abort
    cmd_clear();
    return;
  }
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, cmd_buf.buf+cmd_buf.size, 64);
  cmd_buf.size+=len;
  if(len<64 || // packet is short, end of cmd
     cmd_buf.size+64>=sizeof(cmd_buf.buf)) {// cmd_buf is full, can't read anymore
    cmd_buf.state=OUTPUT;
  }
}

/**
  * @brief  usb rx callback for the data end point
  * @param  usbd_dev: pointer to usbd (libopencm3 style)
  * @param  ep: end point (libopencm3 style)
  * @retval None
  */
void handle_data(usbd_device *usbd_dev, const uint8_t ep) {
  int len;
  if((modus & PITCHFORK_CMD_BUFFERED) == 0) {
    // we are not in any modus that needs a buffer
    unsigned char tmpbuf[64];
    usb_read(tmpbuf); // sink it
    // overwrite this so attacker cannot get reliably data written to stack
    memset(tmpbuf,0,sizeof(tmpbuf));
    usb_write((unsigned char*) "err: no op", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
    return;
  }
  if(bufs[active_buf].state != INPUT) {
    if(toggle_buf() == -1) { // if other buffer yet unavailable
      // throttle input
      blocked = 1;
      usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
      usb_write((unsigned char*) "err: oflow", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
      return;
    }
    // or read into the fresh buffer
  }
  // read into buffer
  len = usb_read(bufs[active_buf].start+bufs[active_buf].size);
  // adjust buffer size
  bufs[active_buf].size+=len;
  // todo test this instead of below: if(len<64)
  if(len<64 && (modus!=PITCHFORK_CMD_DECRYPT || bufs[active_buf].size!=BUF_SIZE+16 )) {
    // short buffer read finish off reading
    bufs[active_buf].state = CLOSED;
    toggle_buf();
  } else if(bufs[active_buf].size >= BUF_SIZE) {
    if(modus!=PITCHFORK_CMD_DECRYPT || (bufs[active_buf].size > BUF_SIZE)) {
      // buffer full mark it ...
      bufs[active_buf].state = OUTPUT;
    }
    // ... and try to switch to other buffer
    if((bufs[active_buf].state == OUTPUT) && toggle_buf() == -1) { // if other buffer yet unavailable
      // throttle input
      blocked = 1;
      usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
    }
  }
}

/*        ----===== userspace handler for CTRL and DATA EPs =====----        */

static void handle_cmd(void) {
  if(cmd_buf.size<=0 || cmd_buf.state!=OUTPUT) {
    if(cmd_buf.size<0) { // very strange
      // todo log occurence, abort?
      cmd_clear();
    }
    return;
  }

  // shall we stop whatever we're doing?
  if(cmd_buf.buf[0]==PITCHFORK_CMD_STOP) {
    if(modus == PITCHFORK_CMD_RNG) {
      // finish rng with zlp
      usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 0);
    }
    // stop whatever we're doing
    pf_reset();
    cmd_clear();
    oled_print(40,56, "           ",Font_8x8);
    return;
  }

  if(modus!=PITCHFORK_CMD_STOP) {
    // we are already in a mode, ignore the cmd;
    usb_write((unsigned char*) "err: mode", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    oled_print_inv(40,56, "    bad cmd", Font_8x8);
    cmd_clear();
    return;
  }

  // what is the cmd?
  switch(cmd_buf.buf[0] & 0x1f) {

  case PITCHFORK_CMD_ENCRYPT: {
    if(query_user("encrypt")==0) { // expects peer name
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1) {
      usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }

    //if(peer_to_seed(params, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
    if(peer2seed(params, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
      usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // get nonce
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    // send nonce back
    usb_write(nonce, sizeof(nonce), 32, USB_CRYPTO_EP_DATA_OUT);

    oled_print_inv(40,56, "    encrypt", Font_8x8);
    modus = PITCHFORK_CMD_ENCRYPT;
    break;
  }

  case PITCHFORK_CMD_DECRYPT: { // expects keyid, nonce
    if(query_user("decrypt")==0) {
      return;
    }
    if(cmd_buf.size!=EKID_SIZE+1+crypto_secretbox_NONCEBYTES) {
      usb_write((unsigned char*) "err: inv param", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // keyid 2 seed
    if(ekid2key(params, (unsigned char*) cmd_buf.buf+1)!=0) {
      usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // get nonce
    memcpy(nonce, cmd_buf.buf+1+EKID_SIZE, crypto_secretbox_NONCEBYTES);
    oled_print_inv(40,56, "    decrypt", Font_8x8);
    modus = PITCHFORK_CMD_DECRYPT;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }

  // kex functions
  case PITCHFORK_CMD_KEX_START: {
    if(query_user("start kex")==0) {
      return;
    }
    // send prekey
    // load lt private key and derive pubkey from it
    modus = PITCHFORK_CMD_KEX_START;
    kex_start();
    break;
  }

  case PITCHFORK_CMD_KEX_RESPOND: {
    if(query_user("kex respond")==0) { // expects peer name
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1 || cmd_buf.size<2) {
      usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    modus = PITCHFORK_CMD_KEX_RESPOND;
    oled_print_inv(40,56, "   kex resp", Font_8x8);
    params[0]=cmd_buf.size-1;
    memcpy(params+1,cmd_buf.buf+1,cmd_buf.size-1);
    params[cmd_buf.size]=0;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }

  case PITCHFORK_CMD_KEX_END: {
    if(query_user("kex end")==0) { // expects peer name
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1 || cmd_buf.size<2) {
      usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    oled_print_inv(40,56, "    kex end", Font_8x8);
    params[0]=cmd_buf.size-1;
    memcpy(params+1,cmd_buf.buf+1,cmd_buf.size-1);
    params[cmd_buf.size]=0;
    modus = PITCHFORK_CMD_KEX_END;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }

  case PITCHFORK_CMD_DECRYPT_ANON: { // expects nonce, epk
                                     // key is scalarmult(epk,mypub)
    if(cmd_buf.size!=1+crypto_secretbox_NONCEBYTES+crypto_scalarmult_curve25519_BYTES) {
      usb_write((unsigned char*) "err: inv param", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // get nonce
    memcpy(nonce, cmd_buf.buf+1, crypto_secretbox_NONCEBYTES);
    // derive key
    Axolotl_KeyPair kp;
    if(1!=load_ltkeypair(&kp)) {
      usb_write((unsigned char*) "err: no ltkey", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    if(0!=crypto_scalarmult_curve25519(params, kp.sk, cmd_buf.buf+1+crypto_secretbox_NONCEBYTES)) {
      usb_write((unsigned char*) "err: inv param", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    modus = PITCHFORK_CMD_DECRYPT;
    if(query_user("anon decrypt")==0) {
      return;
    }
    oled_print_inv(40,56, "    decrypt", Font_8x8);
    break;
  }

  case PITCHFORK_CMD_AX_SEND: { // expects peer name
    if(query_user("ax encrypt")==0) {
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1 || cmd_buf.size<2) {
      usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }

    oled_print_inv(40,56, " ax encrypt", Font_8x8);
    modus = PITCHFORK_CMD_AX_SEND;
    //if(peer_to_seed(params, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
    //if(ax_send_init(params, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
    if(0!=ax_send_init(cmd_buf.buf+1,cmd_buf.size-1)) {
      usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // get nonce
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    // send nonce back to host
    usb_write(nonce, sizeof(nonce), 32, USB_CRYPTO_EP_DATA_OUT);

    modus = PITCHFORK_CMD_ENCRYPT;
    break;
  }

  case PITCHFORK_CMD_AX_RECEIVE: { // expects hnonce, headers, mnonce, peer
    if(query_user("ax decrypt")==0) {
      return;
    }
    int fixsize=PADDEDHCRYPTLEN-16+crypto_secretbox_NONCEBYTES*2+1;
    if(cmd_buf.size>fixsize+PEER_NAME_MAX || cmd_buf.size<fixsize+1) {
      usb_write((unsigned char*) "err: bad params", 16, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    memcpy(nonce, cmd_buf.buf+1+PADDEDHCRYPTLEN-16+crypto_secretbox_NONCEBYTES, crypto_secretbox_NONCEBYTES);
    params[0]=cmd_buf.size-fixsize;
    memcpy(params+1, cmd_buf.buf+fixsize, cmd_buf.size-fixsize);

    oled_print_inv(40,56, " ax decrypt", Font_8x8);
    modus = PITCHFORK_CMD_AX_RECEIVE;

    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }

  case PITCHFORK_CMD_SIGN: {
    crypto_generichash_init(&hash_state, NULL, 0, 32);
    if(query_user("sign")==0) {
      return;
    }
    oled_print_inv(40,56, "       sign", Font_8x8);
    modus = PITCHFORK_CMD_SIGN;
    break;
  }

  case PITCHFORK_CMD_PQSIGN: {
    if(query_user("pqsign")==0) {
      return;
    }
    crypto_generichash_init(&hash_state, NULL, 0, 32);
    modus = PITCHFORK_CMD_PQSIGN;
    oled_print_inv(40,56, "     pqsign", Font_8x8);
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }

  case PITCHFORK_CMD_VERIFY: { // expects signature || peer
    if(query_user("verify")==0) {
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1+64 || cmd_buf.size<2+64) {
      usb_write((unsigned char*) "err: bad name", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    memcpy(params, cmd_buf.buf+1, 64); // copy sig
    // get pubkey aftersig
    if(1!=peer2pub(params+64,cmd_buf.buf+64+1, cmd_buf.size-65)) {
      usb_write((unsigned char*) "err: no pub", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    gui_refresh=0;
    crypto_generichash_init(&hash_state, NULL, 0, 32);
    modus = PITCHFORK_CMD_VERIFY;
    oled_print_inv(40,56, "     verify", Font_8x8);
    break;
  }

  case PITCHFORK_CMD_LIST_KEYS: {
    if(query_user("list keys")==0) {
      return;
    }
    uint8_t peerid[PEER_NAME_MAX];
    if(cmd_buf.size>2 && cmd_buf.size<PEER_NAME_MAX+2) {
      PF_KeyType type=cmd_buf.buf[1];
      listkeys(type, peerid); // list keys
    } else if(cmd_buf.size==2) {
      PF_KeyType type=cmd_buf.buf[1];
      listkeys(type, 0); // list keys
    } else {
      usb_write((unsigned char*) "err: param", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    break;
  }

  case PITCHFORK_CMD_RNG: {
    modus = PITCHFORK_CMD_RNG;
    oled_print_inv(40,56, "        rng", Font_8x8);
    break;
  }

  case PITCHFORK_CMD_DUMP_PUB: { // expects type
    if(cmd_buf.size!=2) {
      usb_write((unsigned char*) "err: inv param", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    if(query_user("dump pub")==0) {
      return;
    }
    if(cmd_buf.buf[1]==0) {
      // derive key
      Axolotl_KeyPair kp;
      if(1!=load_ltkeypair(&kp)) {
        usb_write((unsigned char*) "err: no ltkey", 14, 32,USB_CRYPTO_EP_CTRL_OUT);
        cmd_clear();
        return;
      }
      memset(kp.sk,0,32);
      usb_write(kp.pk, 32, 32,USB_CRYPTO_EP_DATA_OUT);
    } else if(cmd_buf.buf[1]==1) {
      uint8_t sk[PQCRYPTO_SECRETKEYBYTES];
      uint8_t path[]="/sph/                                ";
      if(load_key(path, 4, sk, PQCRYPTO_SECRETKEYBYTES)==-1) {
        usb_write((unsigned char*) "err: no key", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
        pf_reset();
        return;
      }
      uint8_t pk[PQCRYPTO_PUBLICKEYBYTES];
      pqcrypto_sign_public_key(pk, sk);
      memset(sk,0,sizeof(sk));
      modus = PITCHFORK_CMD_DUMP_PUB;
      pf_send(pk,sizeof(pk),PITCHFORK_CMD_DUMP_PUB);
    } else {
      usb_write((unsigned char*) "err: inv param", 15, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    break;
  }

  default: {
    usb_write((unsigned char*) "err: bad cmd", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    oled_print_inv(40,56, "    bad cmd", Font_8x8);
  }
  }

  // clear cmd_buffer
  cmd_clear();
}

/**
  * @brief  handle_buf: executes operations
  * @param  None
  * @retval None
  */
static void handle_buf(void) {
  Buffer *buf = 0;
  if(modus == PITCHFORK_CMD_STOP) return; // nothing to process
  oled_print_inv(40,56, "*", Font_8x8);
  if(modus == PITCHFORK_CMD_RNG) {
    oled_print_inv(40,56, "        rng", Font_8x8);
    rng_handler(); // produce rng pkts
    return;
  }

  // guard against invalid modus, whitelist allowed actions
  // all other ops don't need an input buffer
  switch(modus) {
  case PITCHFORK_CMD_ENCRYPT:
  case PITCHFORK_CMD_SIGN:
  case PITCHFORK_CMD_PQSIGN:
  case PITCHFORK_CMD_VERIFY:
  case PITCHFORK_CMD_DECRYPT_ANON:
  case PITCHFORK_CMD_KEX_RESPOND:
  case PITCHFORK_CMD_KEX_END:
  case PITCHFORK_CMD_AX_SEND:
  case PITCHFORK_CMD_AX_RECEIVE:
  case PITCHFORK_CMD_DECRYPT: { ; break; }
  default: { return; }
  }

  if(((bufs[!active_buf].state!=INPUT) && (bufs[!active_buf].size>0)) ||
     ((bufs[!active_buf].state==CLOSED) && (bufs[!active_buf].size==0))) {
    buf = &bufs[!active_buf]; // alias default active buf
  } else {
      // inactive buffer has nothing to process, check the active one
      // this should only happen in the case when we became stalled.
      // in which case this branch is taken on the second invocation
      // of this function from the mainloop
    if(((bufs[active_buf].state!=INPUT) && (bufs[active_buf].size>0)) ||
       ((bufs[active_buf].state==CLOSED) && (bufs[active_buf].size==0))) {
        buf = &bufs[active_buf]; // alias other inactive buf
    } else {
      return; // nothing to do
    }
  }

  // finally do the processing
  if(buf->size>0) {
     switch(modus) {
        /* case PITCHFORK_CMD_AX_SEND: { */
        /*   oled_print_inv(40,56, " ax encrypt", Font_8x8); */
        /*   // todo implement ax_send here */
        /*   encrypt_block(buf); */
        /*   break; */
        /* } */
        case PITCHFORK_CMD_ENCRYPT: {
          oled_print_inv(40,56, "    encrypt", Font_8x8);
          encrypt_block(buf);
          break;
        }
        case PITCHFORK_CMD_AX_RECEIVE: {
          oled_print_inv(40,56, " ax decrypt", Font_8x8);
          ax_recv_init(buf);
          break;
        }
        case PITCHFORK_CMD_DECRYPT_ANON: {
          oled_print_inv(40,56, " an decrypt", Font_8x8);
          decrypt_block(buf);
          break;
        }
        case PITCHFORK_CMD_DECRYPT: {
          oled_print_inv(40,56, "    decrypt", Font_8x8);
          decrypt_block(buf);
          break;
        }
        case PITCHFORK_CMD_KEX_RESPOND: {
          oled_print_inv(40,56, "   kex resp", Font_8x8);
          kex_resp(buf);
          break;
        }
        case PITCHFORK_CMD_KEX_END: {
          oled_print_inv(40,56, "    kex end", Font_8x8);
          kex_end(buf);
          break;
        }
        case PITCHFORK_CMD_SIGN: {
          oled_print_inv(40,56, "     sign", Font_8x8);
          hash_block(buf);
          break;
        }
        case PITCHFORK_CMD_PQSIGN: {
          oled_print_inv(40,56, "     pqsign", Font_8x8);
          hash_block(buf);
          break;
        }
        case PITCHFORK_CMD_VERIFY: {
          oled_print_inv(40,56, "   verify", Font_8x8);
          hash_block(buf);
          break;
        }
        default: { /* should never get here */ while(1); } // todo error handling/reporting
     }
  }
  // some final loose ends to tend to
  if(buf->state == CLOSED ) {
    if(modus == PITCHFORK_CMD_SIGN) sign_msg();
    else if(modus == PITCHFORK_CMD_PQSIGN) pqsign_msg();
    else if(modus == PITCHFORK_CMD_VERIFY) verify_msg();
    oled_print(40,56, "           ", Font_8x8);
    pf_reset();
  } else { // buf->state == OUTPUT
    buf->size=0;
    buf->state=INPUT;
    if(blocked==1) {
      // now that there is an empty buf, handle postponed pkts
      blocked = 0;
      usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
    }
  }
}

/*        ----===== main userspace handler =====----        */

/**
  * @brief  pitchfork_main: dispatches PITCHFORK operations
  *         this function should be called from the
  *         mainloop when in PITCHFORK mode
  * @param  None
  * @retval None
  */
void pitchfork_main(void) {
  // process cmd_buf
  handle_cmd();
  handle_buf();
}
