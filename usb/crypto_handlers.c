#include "dual.h"
#include "usb_crypto.h"
#include "crypto_handlers.h"
#include "stm32f.h"
#include "randombytes_salsa20_random.h"
#include <crypto_generichash.h>
#include <crypto_secretbox.h>
#include <utils.h>
#include <string.h>
#include "led.h"

#define BUF_SIZE 32768

extern usbd_device *usbd_dev;

Crypto_State state = INPUT;
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
unsigned char* outstart = outbuf+crypto_secretbox_BOXZEROBYTES;
unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES];
unsigned char* bufstart =  buf + crypto_secretbox_ZEROBYTES;
int buf_size;
crypto_generichash_state hash_state;
unsigned char signature[crypto_generichash_BYTES];
CRYPTO_CMD modus;

void crypto_mode_reset(void) {
  modus = USB_CRYPTO_CMD_STOP;
  buf_size = 0;
  state = INPUT;
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

void encrypt_block(void) {
  int i, len, size = buf_size;
  unsigned char key[crypto_secretbox_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) key)[i]=0;
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf)[i]=0;
  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf, size+crypto_secretbox_ZEROBYTES, outbuf, key);
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

void decrypt_block() {
}

void hash_init(void) {
  unsigned int i;
  unsigned char k[crypto_generichash_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) k)[i]=0;
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

void hash_block(void) {
  crypto_generichash_update(&hash_state, bufstart, buf_size);
}

void sign_msg(void) {
  crypto_mode_reset();
  crypto_generichash_final(&hash_state, outbuf, 32);
  usb_write(outbuf, 32, 0, USB_CRYPTO_EP_DATA_OUT);
}

void verify_msg(void) {
  crypto_mode_reset();
  crypto_generichash_final(&hash_state, outbuf, 32);
  outbuf[0] = (sodium_memcmp(signature, outbuf, 32) != -1);
  usb_write(outbuf, 1, 0, USB_CRYPTO_EP_DATA_OUT);
}

void rng_handler(void) {
  set_write_led;
  randombytes_salsa20_random_buf((void *) outbuf, 64);
  irq_disable(NVIC_OTG_FS_IRQ);
  while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 64) == 0) && (modus == USB_CRYPTO_CMD_RNG)) usbd_poll(usbd_dev);
  irq_enable(NVIC_OTG_FS_IRQ);
  reset_write_led;
}

void (*ops[])(void) = {
  &encrypt_block,
  &decrypt_block,
  &hash_block,
  &hash_block,
};

void handle_buf(void) {
  if(modus==USB_CRYPTO_CMD_RNG) {
    rng_handler();
    return;
  }
  if(state == INPUT || modus>USB_CRYPTO_CMD_VERIFY) return;
  if(buf_size <= 0) {
    // buf is not full, why?
    usb_write((unsigned char*) "err: buffer underrun", 20, 32,USB_CRYPTO_EP_CTRL_OUT);
    return; // try again in next main loop iteration
  }
  set_write_led;
  ops[modus]();
  if(state == OUTPUT) {
    buf_size=0;
    state=INPUT;
  } else if(state == CLOSED) {
    if(modus == USB_CRYPTO_CMD_SIGN) sign_msg();
    else if(modus == USB_CRYPTO_CMD_VERIFY) verify_msg();
    crypto_mode_reset();
  }
  reset_write_led;
}

void handle_data(void) {
  int len;
  unsigned char tmpbuf[64];
  if(modus>USB_CRYPTO_CMD_VERIFY) {
    // we are not in any modus that needs a buffer
    len = data_read(tmpbuf); // sink it
    usb_write((unsigned char*) "err: no op", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    return;
  }
  if(buf_size+64>BUF_SIZE || state != INPUT) {
    // wouldn't fit, careful
    len = data_read(tmpbuf);
    if(buf_size+len>BUF_SIZE || state!=INPUT) {
      usb_write((unsigned char*) "err: buffer overflow", 20, 32,USB_CRYPTO_EP_CTRL_OUT);
      crypto_mode_reset();
      return;
    } else if(len>0 && buf_size+len<=BUF_SIZE) {
      // this should not happen, it means we previously had
      // 32kB - n, n<64 already in the buffer. that means we
      // already had a short pkt and should've gone into CLOSED
      // mode earlier
      memcpy(bufstart+buf_size, tmpbuf, len);
    }
  } else {
    // read into buffer
    len = data_read(bufstart+buf_size);
  }
  buf_size+=len;
  if(len<64 && buf_size<BUF_SIZE) {
    state = CLOSED;
  } else if(buf_size >= BUF_SIZE) {
    state = OUTPUT;
  }
}

void handle_ctl(void) {
  char buf[64];
  buf[0]=0;
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, buf, 64);
  if(len>0) {
    if(buf[0]==USB_CRYPTO_CMD_STOP) {
        crypto_mode_reset();
        return;
    }
    switch(buf[0] & 7) {
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
          memcpy(signature, buf+1, crypto_generichash_BYTES);
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

