#include "dual.h"
#include "usb_crypto.h"
#include "crypto/usb_handler.h"
#include "stm32f.h"
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include <crypto_generichash.h>
#include <crypto_secretbox.h>
#include <utils.h>
#include <string.h>
#include "led.h"
#include "ecdho.h"

extern usbd_device *usbd_dev;

CRYPTO_CMD modus=USB_CRYPTO_CMD_STOP;
Buffer bufs[2];
unsigned char active_buf = 0;
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
unsigned char* outstart = outbuf+crypto_secretbox_BOXZEROBYTES;
unsigned char* outstart32 = outbuf+crypto_secretbox_ZEROBYTES;
crypto_generichash_state hash_state;
unsigned char blocked = 0;
unsigned char params[128];

void reset(void) {
  modus = USB_CRYPTO_CMD_STOP;
  bufs[0].size = 0;
  bufs[1].size = 0;
  bufs[0].state = INPUT;
  bufs[1].state = INPUT;
  blocked = 0;
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
}

unsigned int data_read(unsigned char* dst) {
  unsigned int len;
  set_read_led;
  len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, dst, 64);
  reset_read_led;
  return len;
}

void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep) {
  set_write_led;
  if(retries == 0) {
    // blocking
    while(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0);
  } else {
    for(;(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0) && retries>0;retries--);
  }
  reset_write_led;
}

void encrypt_block(Buffer *buf) {
  int i, len, size = buf->size;
  unsigned char key[crypto_secretbox_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) key)[i]=0;
  // zero out beginning of plaintext as demanded by nacl
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf->buf)[i]=0;
  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf->buf, size+crypto_secretbox_ZEROBYTES, outbuf, key);
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
    // final packet
    if(len<64) {
      break;
    }
  }
}

void decrypt_block(Buffer* buf) {
  int i, len;
  // substract nonce size from total size (40B)
  int size = buf->size - (crypto_secretbox_NONCEBYTES + crypto_secretbox_BOXZEROBYTES);
  unsigned char key[crypto_secretbox_KEYBYTES];
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) key)[i]=0;
  // get nonce from beginning of input buffer
  memcpy(nonce, buf->start, crypto_secretbox_NONCEBYTES);
  //for(i=0;i<(crypto_secretbox_NONCEBYTES>>2);i++)
  //  ((unsigned int*) nonce)[i] = ((unsigned int*) buf->start)[i];
  // zero out crypto_secretbox_BOXZEROBYTES preamble
  // overwriting tne end of the nonce
  for(i=((crypto_secretbox_NONCEBYTES-crypto_secretbox_BOXZEROBYTES)>>2);
      i<(crypto_secretbox_NONCEBYTES>>2);
      i++)
    ((unsigned int*) buf->start)[i]=0;
  // decrypt
  if(-1 == crypto_secretbox_open(outbuf,  // m
                                 (buf->start) + (crypto_secretbox_NONCEBYTES - crypto_secretbox_BOXZEROBYTES), // c + preamble
                                 size+crypto_secretbox_ZEROBYTES,  // clen = len(plain)+2x(boxzerobytes)
                                 nonce, // n
                                 key)) {
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
    // final packet
    if(len<64) {
      break;
    }
  }
}

void hash_init(void) {
  unsigned int i;
  unsigned char k[crypto_generichash_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) k)[i]=0;
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

void hash_block(Buffer *buf) {
  crypto_generichash_update(&hash_state, buf->start, buf->size);
}

void sign_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  usb_write(outbuf, 32, 0, USB_CRYPTO_EP_DATA_OUT);
}

void verify_msg(void) {
  crypto_generichash_final(&hash_state, outbuf, 32);
  outbuf[0] = (sodium_memcmp(params, outbuf, 32) != -1);
  usb_write(outbuf, 1, 0, USB_CRYPTO_EP_DATA_OUT);
}

void rng_handler(void) {
  int i;
  set_write_led;
  randombytes_salsa20_random_buf((void *) outbuf, BUF_SIZE);
  irq_disable(NVIC_OTG_FS_IRQ);
  for(i=0;i<BUF_SIZE && (modus == USB_CRYPTO_CMD_RNG);i+=64) {
    while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf+i, 64) == 0) &&
          (modus == USB_CRYPTO_CMD_RNG))
      usbd_poll(usbd_dev);
  }
  irq_enable(NVIC_OTG_FS_IRQ);
  reset_write_led;
}

void ecdh_start_handler(void) {
  ECDH_Start_Params* args = (ECDH_Start_Params*) params;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char keyid[16];
  start_ecdh(args->name, args->len, pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,pub,crypto_scalarmult_curve25519_BYTES);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}
void ecdh_respond_handler(void) {
  ECDH_Response_Params* args = (ECDH_Response_Params*) params;
  unsigned char keyid[16];
  respond_ecdh(args->name, args->len, args->pub, keyid);
  // output keyid, pub
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  memcpy(outbuf+STORAGE_ID_LEN,args->pub,crypto_scalarmult_curve25519_BYTES);
  usb_write(outbuf, STORAGE_ID_LEN+crypto_scalarmult_curve25519_BYTES, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

void ecdh_end_handler(void) {
  unsigned char peer[32];
  unsigned char keyid[16];
  ECDH_End_Params* args = (ECDH_End_Params*) params;
  SeedRecord* seedptr = get_seedrec(SEED,0,args->keyid);
  unsigned char peer_len = get_peer(peer, (unsigned char*) seedptr->peerid);
  if(seedptr > 0 && peer_len > 0)
    finish_ecdh(peer, peer_len, args->keyid, args->pub, keyid);
  // output keyid
  memcpy(outbuf,keyid,STORAGE_ID_LEN);
  usb_write(outbuf, STORAGE_ID_LEN, 0, USB_CRYPTO_EP_DATA_OUT);
  reset();
}

void (*ops[])(Buffer* buf) = {
  &encrypt_block,
  &decrypt_block,
  &hash_block,
  &hash_block,
};

void handle_buf(void) {
  Buffer *buf = 0;
  if(modus  > USB_CRYPTO_CMD_RNG) return; // nothing to process
  if(modus == USB_CRYPTO_CMD_RNG) {
    rng_handler(); // produce rng pkts
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_START) {
    ecdh_start_handler(); // produce rng pkts
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_RESPOND) {
    ecdh_respond_handler(); // produce rng pkts
    return;
  } else if(modus == USB_CRYPTO_CMD_ECDH_END) {
    ecdh_end_handler(); // produce rng pkts
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

int toggle_buf(void) {
  if(bufs[!active_buf].state == INPUT && bufs[!active_buf].size==0) {
    if(bufs[active_buf].state == INPUT)
      bufs[active_buf].state = OUTPUT;
    active_buf = !active_buf;
    return active_buf;
  }
  return -1;
}

void handle_data(void) {
  int len;
  unsigned char tmpbuf[64];
  if(modus>USB_CRYPTO_CMD_VERIFY) {
    // we are not in any modus that needs a buffer
    len = data_read(tmpbuf); // sink it
    // todo overwrite this so attacker cannot get reliably data written to stack?
    usb_write((unsigned char*) "err: no op", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    return;
  }
  if(bufs[active_buf].state != INPUT) {
    if(toggle_buf() == -1) { // if other buffer yet unavailable
      // throttle input
      usb_write((unsigned char*) "err: overflow", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
      return;
    }
    // or read into the fresh buffer
  }
  // read into buffer
  len = data_read(bufs[active_buf].start+bufs[active_buf].size);
  // adjust buffer size
  bufs[active_buf].size+=len;
  if(len<64 && (modus!=USB_CRYPTO_CMD_DECRYPT || (len==40 && bufs[active_buf].size<BUF_SIZE+40) )) {
    // short buffer read finish off reading
    bufs[active_buf].state = CLOSED;
    toggle_buf();
  } else if(bufs[active_buf].size >= BUF_SIZE) {
    if(modus!=USB_CRYPTO_CMD_DECRYPT || (bufs[active_buf].size > BUF_SIZE)) {
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

void handle_ctl(void) {
  char buf[64];
  buf[0]=0;
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, buf, 64);
  if(len>0) {
    if(buf[0]==USB_CRYPTO_CMD_STOP) {
      if(modus == USB_CRYPTO_CMD_RNG) {
        // finish rng with zlp
        usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 0);
      }
      reset();
      return;
    }
    if(modus!=USB_CRYPTO_CMD_STOP) {
      // we are already in a mode
      usb_write((unsigned char*) "err: mode", 9, 32,USB_CRYPTO_EP_CTRL_OUT);
      return;
    }
    switch(buf[0] & 15) {
      case USB_CRYPTO_CMD_ENCRYPT: {
        modus = USB_CRYPTO_CMD_ENCRYPT;
        break;
      }
      case USB_CRYPTO_CMD_DECRYPT: {
        modus = USB_CRYPTO_CMD_DECRYPT;
        break;
      }
      case USB_CRYPTO_CMD_SIGN: {
        hash_init();
        modus = USB_CRYPTO_CMD_SIGN;
        break;
      }
      case USB_CRYPTO_CMD_VERIFY: {
        if(len==crypto_generichash_BYTES+1) {
          hash_init();
          memcpy(params, buf+1, crypto_generichash_BYTES);
          modus = USB_CRYPTO_CMD_VERIFY;
        } else {
          usb_write((unsigned char*) "err: no sig", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
        }
        break;
      }
      case USB_CRYPTO_CMD_RNG: {
        modus = USB_CRYPTO_CMD_RNG;
        break;
      }
      case USB_CRYPTO_CMD_ECDH_START: {
        if(len>1) {
          ECDH_Start_Params* args = (ECDH_Start_Params*) params;
          args->len = len-1;
          memcpy(args->name, buf+1, args->len);
          modus = USB_CRYPTO_CMD_ECDH_START;
        } else {
          usb_write((unsigned char*) "err: bad args", 12, 32,USB_CRYPTO_EP_CTRL_OUT);
        }
        break;
      }
      case USB_CRYPTO_CMD_ECDH_RESPOND: {
        if(len>crypto_scalarmult_curve25519_BYTES+1) {
          ECDH_Response_Params* args = (ECDH_Response_Params*) params;
          args->len=len-(crypto_scalarmult_curve25519_BYTES+1);
          memcpy(args->pub, buf+1, crypto_scalarmult_curve25519_BYTES);
          memcpy(args->name, buf+crypto_scalarmult_curve25519_BYTES+1, args->len);
          modus = USB_CRYPTO_CMD_ECDH_RESPOND;
        } else {
          usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
        }
        break;
      }
      case USB_CRYPTO_CMD_ECDH_END: {
        if(len==crypto_scalarmult_curve25519_BYTES+STORAGE_ID_LEN+1) {
          ECDH_End_Params* args = (ECDH_End_Params*) params;
          memcpy(args->pub, buf+1, crypto_scalarmult_curve25519_BYTES);
          memcpy(args->keyid, buf+1+crypto_scalarmult_curve25519_BYTES, STORAGE_ID_LEN);
          modus = USB_CRYPTO_CMD_ECDH_END;
        } else {
          usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
        }
        break;
      }
      /* case USB_CRYPTO_CMD_STORAGE: { */
      /*   if(cmd_fn!=0) { */
      /*     irq_mode(); */
      /*   } */
      /*   dual_usb_mode = DISK; */
      /*   break; */
      /* } */
      default: {
        //usb_puts("err: invalid cmd");
      }
    }
  }
}

