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

unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
unsigned char* outstart = outbuf+crypto_secretbox_BOXZEROBYTES;
unsigned char buf[2][BUF_SIZE+crypto_secretbox_ZEROBYTES];
unsigned char* bufstart[2] = { buf[0] + crypto_secretbox_ZEROBYTES,
                               buf[1] + crypto_secretbox_ZEROBYTES};
int buf_size[2];
unsigned char full[2];
unsigned char active_buf;
crypto_generichash_state hash_state;
unsigned char signature[crypto_generichash_BYTES];
unsigned char modus;
unsigned char lastmodus;
extern void (*op_cb)(void);

void crypto_mode_reset(void) {
  lastmodus = USB_CRYPTO_CMD_STOP;
  modus = USB_CRYPTO_CMD_STOP;
  active_buf = 0;
  buf_size[0] = 0;
  buf_size[1] = 0;
  full[0] = 0;
  full[1] = 0;
  op_cb = 0;
}

unsigned int data_read(unsigned char* dst) {
  unsigned int len;
  reset_write_led;
  len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, dst, 64);
  set_write_led;
  return len;
}

void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep) {
  if(retries == 0) {
    // blocking
    while(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0);
  } else {
    for(;(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0) && retries>0;retries--);
  }
}

void encrypt_block(unsigned char bidx) {
  int i, len, size = buf_size[bidx];
  unsigned char key[crypto_secretbox_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) key)[i]=0;
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf[bidx])[i]=0;
  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf[bidx], size+crypto_secretbox_ZEROBYTES, outbuf, key);
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

void decrypt_block(unsigned char bidx) {
}

void hash_init(void) {
  unsigned int i;
  unsigned char k[crypto_generichash_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) k)[i]=0;
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

void sign_block(unsigned char bidx) {
  crypto_generichash_update(&hash_state, bufstart[bidx], buf_size[bidx]);
  if(lastmodus==USB_CRYPTO_CMD_SIGN || modus==USB_CRYPTO_CMD_SIGN) {
    crypto_mode_reset();
    crypto_generichash_final(&hash_state, outbuf, 32);
    usb_write(outbuf, 32, 0, USB_CRYPTO_EP_DATA_OUT);
  }
}

void verify_block(unsigned char bidx) {
  reset_read_led;
  crypto_generichash_update(&hash_state, bufstart[bidx], buf_size[bidx]);
  if(lastmodus==USB_CRYPTO_CMD_VERIFY || modus==USB_CRYPTO_CMD_VERIFY) {
    crypto_mode_reset();
    crypto_generichash_final(&hash_state, outbuf, 32);
    outbuf[0] = (sodium_memcmp(signature, outbuf, 32) != -1);
    usb_write(outbuf, 1, 0, USB_CRYPTO_EP_DATA_OUT);
  }
}

void rng_handler(void) {
  set_write_led;
  randombytes_salsa20_random_buf((void *) buf[active_buf], 64);
  irq_disable(NVIC_OTG_FS_IRQ);
  while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, buf, 64) == 0) && (modus == USB_CRYPTO_CMD_RNG)) usbd_poll(usbd_dev);
  irq_enable(NVIC_OTG_FS_IRQ);
  reset_write_led;
}

void (*ops[])(unsigned char) = {
  &encrypt_block,
  &decrypt_block,
  &sign_block,
  &verify_block,
};

void handle_buf(void) {
  unsigned char curbuf;
  if(modus==USB_CRYPTO_CMD_STOP && lastmodus==USB_CRYPTO_CMD_STOP) return;
  curbuf = (active_buf==0);
  if(full[curbuf] == 0) {
    // inactive buf is not full, why?
    usb_write((unsigned char*) "err: buffer underrun", 20, 32,USB_CRYPTO_EP_CTRL_OUT);
    return; // try again in next main loop iteration
  }
  set_write_led;
  if(modus<=USB_CRYPTO_CMD_VERIFY) {
    ops[modus](curbuf);
  } else if(lastmodus <= USB_CRYPTO_CMD_VERIFY) {
    ops[lastmodus](curbuf);
    lastmodus = USB_CRYPTO_CMD_STOP;
  }
  reset_write_led;
  if(full[active_buf]!=1) {
    buf_size[curbuf]=0;
    full[curbuf]=0;
    op_cb = 0;
  } else {
    buf_size[curbuf]=0;
    full[curbuf]=0;
    active_buf=curbuf;
  }
}

void handle_data(void) {
  int len;
  unsigned char tmpbuf[64];
  set_read_led;
  if(modus>USB_CRYPTO_CMD_VERIFY) {
    // we are not in any modus that needs a buffer
    len = data_read(tmpbuf); // sink it
    usb_write((unsigned char*) "err: no op", 10, 32,USB_CRYPTO_EP_CTRL_OUT);
    reset_read_led;
    return;
  }
  if(buf_size[active_buf]+64<=BUF_SIZE) {
    // read into buffer
    len = data_read(bufstart[active_buf]+buf_size[active_buf]);
  } else {
    len = data_read(tmpbuf);
    if(buf_size[active_buf]+len>BUF_SIZE || full[active_buf]==1) {
      usb_write((unsigned char*) "err: buffer overflow", 22, 32,USB_CRYPTO_EP_CTRL_OUT);
      modus = USB_CRYPTO_CMD_STOP;
      buf_size[active_buf] = 0;
      reset_read_led;
      return;
    } else if(len>0) {
      memcpy(bufstart[active_buf]+buf_size[active_buf], tmpbuf, len);
    }
  }
  buf_size[active_buf]+=len;
  if(buf_size[active_buf] >= BUF_SIZE) {
    full[active_buf] = 1;
    if(full[active_buf^1]==0)
      active_buf ^= 1;
    op_cb = handle_buf;
  }
  reset_read_led;
}

void handle_ctl(void) {
  char buf[64];
  buf[0]=0;
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, buf, 64);
  if(len>0) {
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
          //for(i=0;i<crypto_generichash_BYTES;i++) signature[i] = buf[i+1];
          memcpy(signature, buf+1, crypto_generichash_BYTES);
          modus = USB_CRYPTO_CMD_VERIFY;
        } else {
          usb_write((unsigned char*) "err: no sig", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
        }
        break;
      }

      case USB_CRYPTO_CMD_RNG: {
        op_cb = &rng_handler;
        modus = USB_CRYPTO_CMD_RNG;
        break;
      }
      case USB_CRYPTO_CMD_ABORT: {
        crypto_mode_reset();
        break;
      }
      /* case USB_CRYPTO_CMD_STORAGE: { */
      /*   if(cmd_fn!=0) { */
      /*     irq_mode(); */
      /*   } */
      /*   dual_usb_mode = DISK; */
      /*   break; */
      /* } */

      case USB_CRYPTO_CMD_STOP: {
        if(modus == USB_CRYPTO_CMD_RNG) {
          modus = USB_CRYPTO_CMD_STOP;
          op_cb = 0;
        } else if(modus<=USB_CRYPTO_CMD_VERIFY) {
            // run one last time
            lastmodus = modus;
            modus = USB_CRYPTO_CMD_STOP;
            if(op_cb == 0) {
              // set this buf to full
              full[active_buf]=1;
              if(full[active_buf==0]==0)
                // switch to other buf if it's empty
                active_buf = (active_buf==0);
            }
            op_cb = handle_buf;
        }
        break;
      }
      default: {
        //usb_puts("err: invalid cmd");
      }
    }
  }
}
