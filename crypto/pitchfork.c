/**
  ************************************************************************************
  * @file    pitchfork_irq.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides PITCHFORK usb irq handlers
  ************************************************************************************
  */
/* TOC                                                                */
/*        ----===== typedefs =====----                                */
/*        ----===== imported globals =====----                        */
/*        ----===== exported globals =====----                        */
/*        ----===== local globals =====----                           */
/*        ----===== helper functions =====----                        */
/*        ----===== handlers for specific operations =====----        */
/*        ----===== IRQ handlers for CTRL and DATA EPs =====----      */
/*        ----===== userspace handler for CTRL and DATA EPs =====---- */
/*        ----===== main userspace handler =====----                  */

#include "usb.h"
#include "stm32f.h"
#include <utils.h>
#include <string.h>
#include "storage.h"
#include "dma.h"
#include "ecdho.h"
#include "master.h"
#include "oled.h"
#include "keys.h"
#include "delay.h"
#include "widgets.h"
#include "pitchfork.h"

#include <crypto_generichash.h>
#include "randombytes_pitchfork.h"

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

/**
  * @brief ECDH_Start_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char len;
  unsigned char name[32];
} __attribute((packed)) ECDH_Start_Params;

/**
  * @brief ECDH_Response_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char len;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char name[32];
} __attribute((packed)) ECDH_Response_Params;

/**
  * @brief ECDH_End_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char keyid[STORAGE_ID_LEN];
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) ECDH_End_Params;

/*        ----===== imported globals =====----        */

extern usbd_device *usbd_dev;

/*        ----===== exported globals =====----        */
/**
  * @brief  bufs: input double buffering
  */
Buffer bufs[2];

/**
  * @brief  outbuf: output buffer
  */
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];

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

/**
  * @brief  params: for passing params from ctl to buf handlers
  *         ctl is in irq, while buf handler is from mainloop
  */
static unsigned char params[128+64];

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
  * @brief  reset: resets PITCHFORK mode
  * @param  None
  * @retval None
  */
void reset(void) {
  unsigned int i;
  modus = PITCHFORK_CMD_STOP;
  bufs[0].size = 0;
  bufs[1].size = 0;
  bufs[0].state = INPUT;
  bufs[1].state = INPUT;
  blocked = 0;
  for (i=0;i<(sizeof(params)>>2);i++) ((unsigned int*) params)[i]=0;
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  oled_print(40,56, "           ", Font_8x8);
}

/**
  * @brief  check_seed emits key errors via usb
  * @param  sptr: return value form get_seed
  * @retval 0 on error, 1 on success
  */
static unsigned char check_seed(unsigned int sptr) {
  if (sptr == 0) {
    usb_write((unsigned char*) "err: no key", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  return 1;
}

/**
  * @brief  peer_to_seed: retrieves key and
  *         returns keyid for crypt/sign operations via usb data ep
  * @param  dst: pointer to hold retrieved key
  * @param  src: pointer to peer name
  * @param  len: length of peer name
  * @retval 0 on error, 1 on success
  */
static unsigned char peer_to_seed(unsigned char* dst, unsigned char* src, const unsigned char len) {
  unsigned char ekid[EKID_LEN+EKID_NONCE_LEN];

  if( len == 0 || len >= PEER_NAME_MAX) {
    usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  SeedRecord* sptr = get_peer_seed(dst, src, len);
  if(check_seed((int)sptr) == 0) {
     return 0;
  }

  // calculate ephemeral keyid
  get_ekid(((unsigned char*) sptr->keyid), ekid+EKID_LEN, ekid);
  // and send it back immediately over the ctrl ep
  usb_write(ekid, sizeof(ekid), 32, USB_CRYPTO_EP_CTRL_OUT);
  return 1;
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
    usb_write((unsigned char*) "err: rejected", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    reset();
  }
  gui_refresh=1;
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  mDelay(200);
  statusline();
  return res;
}

/*        ----===== handlers for specific operations =====----        */

/**
  * @brief  hash_init: intializer for sign/verify ops
  * @param  k: key to init the hash engine
  * @retval None
  */
static void hash_init(const unsigned char* k) {
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

/**
  * @brief  encrypt_block: handler for encrypt op
  * @param  buf: ptr one of the input buffer structs
  *         encrypts buffer and sends ciphertext back via usb
  * @retval None
  */
static void encrypt_block(Buffer *buf) {
  int i, len, size = buf->size;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  // zero out beginning of plaintext as demanded by nacl
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf->buf)[i]=0;
  // get nonce
  // TODO make it incremental implicit nonces like in pbp!!
  randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
  // encrypt (key is stored in beginning of params)
  crypto_secretbox(outbuf+8, buf->buf, size+crypto_secretbox_ZEROBYTES, nonce, params);
  // move nonce over boxzerobytes - so it's
  // prepended to the ciphertext for sending
  memcpy(outbuf,nonce,crypto_secretbox_NONCEBYTES);
  size+=crypto_secretbox_NONCEBYTES + (crypto_secretbox_MACBYTES ); // add nonce+mac size to total size
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    irq_disable(NVIC_OTG_FS_IRQ);
    usb_write(outbuf+i, len, 0, USB_CRYPTO_EP_DATA_OUT);
    irq_enable(NVIC_OTG_FS_IRQ);
  }
}

/**
  * @brief  decrypt_block: handler for decrypt op,
  *         decrypts buffer and sends plaintext back via usb
  * @param  buf: ptr one of the input buffer structs
  * @retval None
  */
static void decrypt_block(Buffer* buf) {
  int i, len;
  // substract nonce size from total size (40B)
  int size = buf->size - (crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES);
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  // get nonce from beginning of input buffer
  // TODO make it incremental implicit nonces like in pbp!!
  memcpy(nonce, buf->start, crypto_secretbox_NONCEBYTES);
  // zero out crypto_secretbox_BOXZEROBYTES preamble
  // overwriting tne end of the nonce
  memset(buf->start + (crypto_secretbox_NONCEBYTES - crypto_secretbox_BOXZEROBYTES),0,crypto_secretbox_BOXZEROBYTES);
  // decrypt (key is stored in beginning of params)
  if(-1 == crypto_secretbox_open(outbuf,  // m
                                 (buf->start) + (crypto_secretbox_NONCEBYTES - crypto_secretbox_BOXZEROBYTES), // c + preamble
                                 size+crypto_secretbox_ZEROBYTES,  // clen = len(plain)+2x(boxzerobytes)
                                 nonce, // n
                                 params)) {
    usb_write((unsigned char*) "err: corrupt", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
    reset();
    return;
  }
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    irq_disable(NVIC_OTG_FS_IRQ);
    usb_write(outstart32+i, len, 0, USB_CRYPTO_EP_DATA_OUT);
    irq_enable(NVIC_OTG_FS_IRQ);
  }
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
  * @brief  sign_msg: final handler for sign ops
  * @param  None
  * @retval None
  */
static void sign_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  usb_write(outbuf, 32, 0, USB_CRYPTO_EP_DATA_OUT);
}

/**
  * @brief  verify_msg: final handler for verify ops
  * @param  None
  * @retval None
  */
static void verify_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  outbuf[0] = (sodium_memcmp(params, outbuf, 32) != -1);
  usb_write(outbuf, 1, 0, USB_CRYPTO_EP_DATA_OUT);
}

/**
  * @brief  rng_handler: handler for ~
  * @param  None
  * @retval None
  */
static void rng_handler(void) {
  int i;
  //set_write_led;
  randombytes_buf((void *) outbuf, BUF_SIZE);
  for(i=0;i<BUF_SIZE && (modus == PITCHFORK_CMD_RNG);i+=64) {
    while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf+i, 64) == 0) &&
          (modus == PITCHFORK_CMD_RNG))
      usbd_poll(usbd_dev);
  }
  oled_print(40,56, "           ", Font_8x8);
  //reset_write_led;
}

/**
  * @brief  ecdh_start_handler: handler for ~
  * @param  None
  * @retval None
  */
static void ecdh_start_handler(void) {
  ECDH_Start_Params* args = (ECDH_Start_Params*) params;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char keyid[STORAGE_ID_LEN];
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  start_ecdh(args->name, args->len, pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,pub,crypto_scalarmult_curve25519_BYTES);
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  ecdh_respond_handler: handler for ~
  * @param  None
  * @retval None
  */
static void ecdh_respond_handler(void) {
  ECDH_Response_Params* args = (ECDH_Response_Params*) params;
  unsigned char keyid[STORAGE_ID_LEN];
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  respond_ecdh(args->name, args->len, args->pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,args->pub,crypto_scalarmult_curve25519_BYTES);
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  ecdh_end_handler: handler for ~
  * @param  None
  * @retval None
  */
static void ecdh_end_handler(void) {
  unsigned char peer[32];
  unsigned char keyid[STORAGE_ID_LEN];
  ECDH_End_Params* args = (ECDH_End_Params*) params;

  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);

  SeedRecord* seedptr = get_seedrec(0,args->keyid, 0, 0);
  unsigned char peer_len = get_peer(peer, (unsigned char*) seedptr->peerid);
  if(seedptr > 0 && peer_len > 0)
    finish_ecdh(peer, peer_len, args->keyid, args->pub, keyid);
  // output keyid
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  usb_write(outbuf, STORAGE_ID_LEN, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  listkeys: lists all keys or keys belonging to a peerid
  * @param  peerid: pointer to peerid or 0 if all keys
  * @retval None, emits results via usb.
  */
static void listkeys(const unsigned char *peerid) {
  unsigned int ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, *tptr, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned short deleted = 0, corrupt = 0, unknown = 0;
  UserRecord *userdata = get_userrec();

  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    if(*((unsigned char*)ptr) == DELETED_SEED) {
      deleted++;
    }
    if(*((unsigned char*)ptr) != SEED) { // only seeds
        goto endloop; // this seed has been deleted skip it.
    }

    if(peerid!=0 && // list only keys of user
       memcmp((unsigned char*) (((SeedRecord*) ptr)->peerid),
              peerid,
              STORAGE_ID_LEN) != 0) {
      // skip other peers
      goto endloop;
    }

    // try to unmask name
    nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
    if(nlen==0 || nlen >= PEER_NAME_MAX) {
      // couldn't map peerid to name
      // what to do? write "unresolvable name"
      unknown++;
      nlen=0;
    }
    outptr[nlen] = 0; //terminate it
    nlen++;
    // test if seed can be decrypted

    // pad ciphertext with extra 16 bytes
    memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
           ((SeedRecord*) ptr)->mac,
           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
    memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
    // nonce
    crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                       (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                       (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
    // decrypt
    // todo erase plain in any case! leaks keymaterial
    if(crypto_secretbox_open(plain,                 // ciphertext output
                             cipher,                // plaintext input
                             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                             nonce,                 // nonce
                             get_master_key("usb list keys"))      // key
       == -1) {
      memset(plain, 0, crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES);
      // rewind name of corrupt seed
      corrupt++;
      goto endloop;
    }
    memset(plain, 0, crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES);
    outptr+=nlen;
    // copy keyid
    memcpy(outptr, ((SeedRecord*) ptr)->keyid, STORAGE_ID_LEN);
    outptr+=STORAGE_ID_LEN;

  endloop:
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }

  if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);

  if(outptr>outbuf) {
    // write out stats
    *((unsigned short*) outptr) = deleted;
    outptr+=2;
    *((unsigned short*) outptr) = corrupt;
    outptr+=2;
    *((unsigned short*) outptr) = unknown;
    outptr+=2;
    *((unsigned short*) outptr) = ptr - FLASH_BASE;
    outptr+=2;
    tptr=outbuf;
    // write out outbuf
    while(tptr+64<=outptr) {
      usb_write(tptr, 64, 0, USB_CRYPTO_EP_DATA_OUT);
      tptr+=64;
    }
    // write out last packet
    if(tptr<outptr) {
      // short
      usb_write(tptr, outptr-tptr, 0, USB_CRYPTO_EP_DATA_OUT);
    }
  }
  reset();
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
    cmd_blocked = 1;
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    return;
  }
  if(cmd_buf.size>sizeof(cmd_buf.buf)-64) { // should not happen, as we set output in state before
    // overflowing - also illegal, to send cmds longer than 195 bytes
    usb_write((unsigned char*) "err: oflow", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    // abort
    cmd_clear();
    return;
  }
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, cmd_buf.buf+cmd_buf.size, 64);
  cmd_buf.size+=len;
  if(len<64 || // packet is short, end of cmd
     cmd_buf.size<sizeof(cmd_buf.buf)-64) {// cmd_buf is full, can't read anymore
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
  if(modus>PITCHFORK_CMD_VERIFY) {
    unsigned char tmpbuf[64];
    // we are not in any modus that needs a buffer
    usb_read(tmpbuf); // sink it
    // todo overwrite this so attacker cannot get reliably data written to stack?
    usb_write((unsigned char*) "err: no op", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    return;
  }
  if(bufs[active_buf].state != INPUT) {
    if(toggle_buf() == -1) { // if other buffer yet unavailable
      // throttle input
      // where is the throttling? theres no nak_set called here // is it already throttled?
      usb_write((unsigned char*) "err: overflow", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      return;
    }
    // or read into the fresh buffer
  }
  // read into buffer
  len = usb_read(bufs[active_buf].start+bufs[active_buf].size);
  // adjust buffer size
  bufs[active_buf].size+=len;
  if(len<64 && (modus!=PITCHFORK_CMD_DECRYPT || bufs[active_buf].size!=BUF_SIZE+40 )) {
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
    reset();
    cmd_clear();
    oled_print(40,56, "           ",Font_8x8);
    return;
  }

  if(modus!=PITCHFORK_CMD_STOP) {
    // we are already in a mode, ignore the cmd;
    usb_write((unsigned char*) "err: mode", 9, 32,USB_CRYPTO_EP_CTRL_OUT);
    oled_print_inv(40,56, "    bad cmd", Font_8x8);
    cmd_clear();
    return;
  }

  // what is the cmd?
  switch(cmd_buf.buf[0] & 15) {
  case PITCHFORK_CMD_ENCRYPT: {
    if(query_user("encrypt")==0) {
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1) {
      usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    if(peer_to_seed(params, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
      cmd_clear();
      return;
    }
    oled_print_inv(40,56, "    encrypt", Font_8x8);
    modus = PITCHFORK_CMD_ENCRYPT;
    break;
  }
  case PITCHFORK_CMD_DECRYPT: {
    if(query_user("decrypt")==0) {
      return;
    }
    if(cmd_buf.size!=EKID_SIZE+1) {
      usb_write((unsigned char*) "err: no keyid", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // keyid 2 seed
    SeedRecord* sptr = get_seed(params, 0, (unsigned char*) cmd_buf.buf+1, 1);
    if(check_seed((int)sptr) == 0) {
      cmd_clear();
      return;
    }
    oled_print_inv(40,56, "    decrypt", Font_8x8);
    modus = PITCHFORK_CMD_DECRYPT;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }
  case PITCHFORK_CMD_SIGN: {
    if(query_user("sign")==0) {
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1) {
      usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    unsigned char seed [crypto_generichash_KEYBYTES];
    if(peer_to_seed(seed, (unsigned char*) cmd_buf.buf+1, cmd_buf.size-1)==0) {
      cmd_clear();
      return;
    }
    hash_init(seed);
    memset(seed,0,crypto_generichash_BYTES);
    oled_print_inv(40,56, "       sign", Font_8x8);
    modus = PITCHFORK_CMD_SIGN;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }
  case PITCHFORK_CMD_VERIFY: {
    if(query_user("verify")==0) {
      return;
    }
    if(cmd_buf.size!=crypto_generichash_BYTES+EKID_SIZE+1) {
      usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      cmd_clear();
      return;
    }
    // get seed via keyid
    unsigned char seed[crypto_generichash_KEYBYTES];
    if(check_seed((int) get_seed(seed,0,(unsigned char*) cmd_buf.buf+1+crypto_generichash_BYTES, 1)) == 0) {
      cmd_clear();
      return;
    }
    hash_init(seed);
    memset(seed,0,crypto_generichash_BYTES);
    // copy signature for final verification
    memcpy(params, cmd_buf.buf+1, crypto_generichash_BYTES);
    oled_print_inv(40,56, "     verify", Font_8x8);
    modus = PITCHFORK_CMD_VERIFY;
    usb_write((unsigned char*) "go", 2, 32,USB_CRYPTO_EP_CTRL_OUT);
    break;
  }
  case PITCHFORK_CMD_LIST_KEYS: {
    if(query_user("list keys")==0) {
      return;
    }
    if(cmd_buf.size>1 && cmd_buf.size<PEER_NAME_MAX+1) {
      topeerid((unsigned char*) cmd_buf.buf+1, cmd_buf.size-1, params+1);
      params[0] = 1;
    } else {
      params[0] = 0;
    }
    modus = PITCHFORK_CMD_LIST_KEYS;
    break;
  }
  case PITCHFORK_CMD_RNG: {
    modus = PITCHFORK_CMD_RNG;
    oled_print_inv(40,56, "        rng", Font_8x8);
    break;
  }
  case PITCHFORK_CMD_ECDH_START: {
    if(query_user("start ecdh")==0) {
      return;
    }
    if(cmd_buf.size>PEER_NAME_MAX+1) {
      usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    if(cmd_buf.size>1) {
      ECDH_Start_Params* args = (ECDH_Start_Params*) params;
      args->len = cmd_buf.size-1;
      memcpy(args->name, cmd_buf.buf+1, args->len);
      modus = PITCHFORK_CMD_ECDH_START;
      oled_print_inv(40,56, " ecdh start", Font_8x8);
    } else {
      usb_write((unsigned char*) "err: bad args", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    break;
  }
  case PITCHFORK_CMD_ECDH_RESPOND: {
    if(query_user("respond 2 ecdh")==0) {
      return;
    }
    if(cmd_buf.size>crypto_scalarmult_curve25519_BYTES+1) {
      if(cmd_buf.size-(crypto_scalarmult_curve25519_BYTES+1)>PEER_NAME_MAX) {
        usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      }
      ECDH_Response_Params* args = (ECDH_Response_Params*) params;
      args->len=cmd_buf.size-(crypto_scalarmult_curve25519_BYTES+1);
      memcpy(args->pub, cmd_buf.buf+1, crypto_scalarmult_curve25519_BYTES);
      memcpy(args->name, cmd_buf.buf+crypto_scalarmult_curve25519_BYTES+1, args->len);
      modus = PITCHFORK_CMD_ECDH_RESPOND;
      oled_print_inv(40,56, "  ecdh resp", Font_8x8);
    } else {
      usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    break;
  }
  case PITCHFORK_CMD_ECDH_END: {
    if(query_user("finish ecdh")==0) {
      return;
    }
    if(cmd_buf.size==crypto_scalarmult_curve25519_BYTES+STORAGE_ID_LEN+1) {
      ECDH_End_Params* args = (ECDH_End_Params*) params;
      memcpy(args->pub, cmd_buf.buf+1, crypto_scalarmult_curve25519_BYTES);
      memcpy(args->keyid, cmd_buf.buf+1+crypto_scalarmult_curve25519_BYTES, STORAGE_ID_LEN);
      modus = PITCHFORK_CMD_ECDH_END;
      oled_print_inv(40,56, "   ecdh end", Font_8x8);
    } else {
      usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    }
    break;
  }
    /* case PITCHFORK_CMD_STORAGE: { */
    /*   if(cmd_fn!=0) { */
    /*     irq_mode(); */
    /*   } */
    /*   dual_usb_mode = DISK; */
    /*   break; */
    /* } */
  default: {
    usb_write((unsigned char*) "err: bad cmd", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
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
  if(modus >= PITCHFORK_CMD_STOP) return; // nothing to process
  oled_print_inv(40,56, "*", Font_8x8);
  if(modus == PITCHFORK_CMD_RNG) {
    oled_print_inv(40,56, "        rng", Font_8x8);
    rng_handler(); // produce rng pkts
    return;
  } else if(modus == PITCHFORK_CMD_ECDH_START) {
    oled_print_inv(40,56, " ecdh start", Font_8x8);
    ecdh_start_handler(); // generate a secret and a public
    return;
  } else if(modus == PITCHFORK_CMD_ECDH_RESPOND) {
    oled_print_inv(40,56, "  ecdh resp", Font_8x8);
    ecdh_respond_handler(); // respond to an ecdh request
    return;
  } else if(modus == PITCHFORK_CMD_ECDH_END) {
    oled_print_inv(40,56, "   ecdh end", Font_8x8);
    ecdh_end_handler(); // finish ecdh request
    return;
  } else if(modus == PITCHFORK_CMD_LIST_KEYS) {
    if(params[0]==1) {
      listkeys(params+1); // list keys
    } else {
      listkeys(0); // list keys
    }
    return;
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
        case PITCHFORK_CMD_ENCRYPT: {
          oled_print_inv(40,56, "    encrypt", Font_8x8);
          encrypt_block(buf);
          break;
        }
        case PITCHFORK_CMD_DECRYPT: {
          oled_print_inv(40,56, "    decrypt", Font_8x8);
          decrypt_block(buf);
          break;
        }
        case PITCHFORK_CMD_SIGN: {
          oled_print_inv(40,56, "       sign", Font_8x8);
          hash_block(buf);
          break;
        }
        case PITCHFORK_CMD_VERIFY: {
          oled_print_inv(40,56, "     verify", Font_8x8);
          hash_block(buf);
          break;
        }
        default: { /* should never get here */ while(1); } // todo error handling/reporting
     }
  }
  // some final loose ends to tend to
  if(buf->state == CLOSED ) {
    if(modus == PITCHFORK_CMD_SIGN) sign_msg();
    else if(modus == PITCHFORK_CMD_VERIFY) verify_msg();
    oled_print(40,56, "           ", Font_8x8);
    reset();
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
