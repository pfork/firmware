/**
  ************************************************************************************
  * @file    pitchfork_handler.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides PITCHFORK ops in "userspace"
  ************************************************************************************
  */

#include "usb.h"
#include "pitchfork.h"
#include "stm32f.h"
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include <crypto_generichash.h>
#include <crypto_secretbox.h>
#include <utils.h>
#include <string.h>
#include "led.h"
#include "dma.h"
#include "ecdho.h"
#include "storage.h"
#include "master.h"

extern usbd_device *usbd_dev;

/**
  * @brief  bufs: input double buffering
  */
Buffer bufs[2];

/**
  * @brief  active_buf: index in bufs to active input buffer
  */
unsigned char active_buf = 0;

/**
  * @brief  outbuf: output buffer
  */
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];

/**
  * @brief  hash_state: for conserving hash_states across multiple input buf
  */
crypto_generichash_state hash_state;

/**
  * @brief  blocked: 1 if stalled.
  */
unsigned char blocked = 0;

/**
  * @brief  encrypt_block: handler for encrypt op
  * @param  buf: ptr one of the input buffer structs
  *         encrypts buffer and sends ciphertext back via usb
  * @retval None
  */
void encrypt_block(Buffer *buf) {
  int i, len, size = buf->size;
  // zero out beginning of plaintext as demanded by nacl
  //for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf->buf)[i]=0;
  dmaset32(buf->buf, 0, crypto_secretbox_ZEROBYTES>>2);
  //dmawait();

  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt (key is stored in beginning of params)
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf->buf, size+crypto_secretbox_ZEROBYTES, outbuf, params);
  // move nonce over boxzerobytes - so it's
  // concated to the ciphertext for sending
  for(i=(crypto_secretbox_NONCEBYTES>>2)-1;i>=0;i--)
    ((unsigned int*) outbuf)[(crypto_secretbox_BOXZEROBYTES>>2)+i] = ((unsigned int*) outbuf)[i];
  size+=crypto_secretbox_NONCEBYTES + crypto_secretbox_ZEROBYTES -crypto_secretbox_BOXZEROBYTES ; // add nonce+mac size to total size
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    irq_disable(NVIC_OTG_FS_IRQ);
    usb_write(outstart+i, len, 0, USB_CRYPTO_EP_DATA_OUT);
    irq_enable(NVIC_OTG_FS_IRQ);
    // final packet TODO test without this! then remove
    //if(len<64) {
    //  break;
    //}
  }
}

/**
  * @brief  decrypt_block: handler for decrypt op,
  *         decrypts buffer and sends plaintext back via usb
  * @param  buf: ptr one of the input buffer structs
  * @retval None
  */
void decrypt_block(Buffer* buf) {
  int i, len;
  // substract nonce size from total size (40B)
  int size = buf->size - (crypto_secretbox_NONCEBYTES + crypto_secretbox_BOXZEROBYTES);
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  // get nonce from beginning of input buffer
  memcpy(nonce, buf->start, crypto_secretbox_NONCEBYTES);
  // zero out crypto_secretbox_BOXZEROBYTES preamble
  // overwriting tne end of the nonce
  for(i=((crypto_secretbox_NONCEBYTES-crypto_secretbox_BOXZEROBYTES)>>2);
      i<(crypto_secretbox_NONCEBYTES>>2);
      i++)
    ((unsigned int*) buf->start)[i]=0;
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
    // final packet TODO test without this! then remove
    //if(len<64) {
    //  break;
    //}
  }
}

/**
  * @brief  sign_msg: handler for sign/verify ops (see ops array)
  * @param  buf: ptr one of the input buffer structs
  * @retval None
  */
void hash_block(Buffer *buf) {
  crypto_generichash_update(&hash_state, buf->start, buf->size);
}

/**
  * @brief  sign_msg: final handler for sign ops
  * @param  None
  * @retval None
  */
void sign_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  usb_write(outbuf, 32, 0, USB_CRYPTO_EP_DATA_OUT);
}

/**
  * @brief  verify_msg: final handler for verify ops
  * @param  None
  * @retval None
  */
void verify_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  outbuf[0] = (sodium_memcmp(params, outbuf, 32) != -1);
  usb_write(outbuf, 1, 0, USB_CRYPTO_EP_DATA_OUT);
}

/**
  * @brief  rng_handler: handler for ~
  * @param  None
  * @retval None
  */
void rng_handler(void) {
  int i;
  //set_write_led;
  randombytes_salsa20_random_buf((void *) outbuf, BUF_SIZE);
  irq_disable(NVIC_OTG_FS_IRQ); // TODO needed?
  for(i=0;i<BUF_SIZE && (modus == USB_CRYPTO_CMD_RNG);i+=64) {
    while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf+i, 64) == 0) &&
          (modus == USB_CRYPTO_CMD_RNG))
      usbd_poll(usbd_dev);
  }
  irq_enable(NVIC_OTG_FS_IRQ);
  //reset_write_led;
}

/**
  * @brief  ecdh_start_handler: handler for ~
  * @param  None
  * @retval None
  */
void ecdh_start_handler(void) {
  ECDH_Start_Params* args = (ECDH_Start_Params*) params;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char keyid[STORAGE_ID_LEN];
  start_ecdh(args->name, args->len, pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,pub,crypto_scalarmult_curve25519_BYTES);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  ecdh_respond_handler: handler for ~
  * @param  None
  * @retval None
  */
void ecdh_respond_handler(void) {
  ECDH_Response_Params* args = (ECDH_Response_Params*) params;
  unsigned char keyid[STORAGE_ID_LEN];
  respond_ecdh(args->name, args->len, args->pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,args->pub,crypto_scalarmult_curve25519_BYTES);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  ecdh_end_handler: handler for ~
  * @param  None
  * @retval None
  */
void ecdh_end_handler(void) {
  unsigned char peer[32];
  unsigned char keyid[STORAGE_ID_LEN];
  ECDH_End_Params* args = (ECDH_End_Params*) params;
  SeedRecord* seedptr = get_seedrec(SEED,0,args->keyid, 0, 0);
  unsigned char peer_len = get_peer(peer, (unsigned char*) seedptr->peerid);
  if(seedptr > 0 && peer_len > 0)
    finish_ecdh(peer, peer_len, args->keyid, args->pub, keyid);
  // output keyid
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  usb_write(outbuf, STORAGE_ID_LEN, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

/**
  * @brief  listkeys: lists all keys or keys belonging to a peerid
  * @param  peerid: pointer to peerid or 0 if all keys
  * @retval None, emits results via usb.
  */
void listkeys(unsigned char *peerid) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  unsigned int ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, *tptr, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned short deleted = 0, corrupt = 0, unknown = 0;
  DeletedSeed* delrec;
  UserRecord *userdata = get_userrec();

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
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

    // check if deleted?
    delrec = (DeletedSeed*) get_seedrec(SEED | DELETED, 0,
                                        (unsigned char*) (((SeedRecord*) ptr)->keyid),
                                        ptr, 0);
    if(delrec!=0 &&
       memcmp(delrec->peerid, ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN)==0) {
      deleted++;
      goto endloop; // this seed has been deleted skip it.
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
                             get_master_key())      // key
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

/**
  * @brief  ops callback array for ops operating on the data buffer
  *         order is according to the CRYPTO_CMD enum (en,de,si,ve)
  */
void (*ops[])(Buffer* buf) = {
  &encrypt_block,
  &decrypt_block,
  &hash_block,
  &hash_block,
};

/**
  * @brief  handle_buf: dispatches PITCHFORK operations
  *         and manages data buffer
  *         this function should be called from the
  *         mainloop when in PITCHFORK mode
  * @param  None
  * @retval None
  */
void handle_buf(void) {
  Buffer *buf = 0;
  if(modus  > USB_CRYPTO_CMD_RNG) return; // nothing to process
  if(modus == USB_CRYPTO_CMD_RNG) {
    rng_handler(); // produce rng pkts
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_START) {
    ecdh_start_handler(); // generate a secret and a public
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_RESPOND) {
    ecdh_respond_handler(); // respond to an ecdh request
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_END) {
    ecdh_end_handler(); // finish ecdh request
    return;
  } else if(modus == USB_CRYPTO_CMD_LIST_KEYS) {
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
    } else return; // nothing to do
  }

  set_write_led;
  // finally do the processing
  if(buf->size>0) ops[modus](buf);
  // some final loose ends to tend to
  if(buf->state == CLOSED ) {
    if(modus == USB_CRYPTO_CMD_SIGN) sign_msg();
    else if(modus == USB_CRYPTO_CMD_VERIFY) verify_msg();
    reset();
  } else { // buf->state == OUTPUT
    buf->size=0;
    buf->state=INPUT;
    if(blocked==1)
      // now that there is an empty buf, handle postponed pkts
      blocked = 0;
      usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  }
  reset_write_led;
}
