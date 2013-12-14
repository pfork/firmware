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

unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES];
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
int buf_size;
crypto_generichash_state hash_state;
unsigned char signature[crypto_generichash_BYTES];

extern void (*op_cb)(void);
void (*cmd_fn)(void) = 0;
void (*next_op)(void) = 0;

void irq_mode(void) {
  cmd_fn = 0;
  op_cb = 0;
  next_op = 0;
  buf_size = 0;
  reset_read_led;
  reset_write_led;
  //irq_enable(NVIC_OTG_FS_IRQ);
}

void mode_start(void) {
  if(cmd_fn!=0) {
    irq_mode();
  }
  set_read_led;
  set_write_led;
}

void fill_buf(void) {
  int i, len;
  if(buf_size+64>BUF_SIZE || op_cb) {
    char null[64];
    len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, null, 64);
    if(op_cb) return;
    if(buf_size+len>BUF_SIZE) {
      for(i=32;(usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, "err: buffer overflow", 5) == 0) && i>0;i--);
      return;
    } else if(len>0) {
      memcpy(buf+buf_size+crypto_secretbox_ZEROBYTES, null, len);
    }
  } else {
    reset_write_led;
    len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, buf+buf_size+crypto_secretbox_ZEROBYTES, 64);
    set_write_led;
  }
  buf_size+=len;
  if(buf_size >= BUF_SIZE) {
    op_cb = next_op;
  }
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
  reset_write_led;
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf, size+crypto_secretbox_ZEROBYTES, outbuf, key);
  // reset buf_size
  buf_size = 0;
  // disable op_cb, encryption only runs once per block
  op_cb = 0;
  // move nonce over boxzerobytes - so it's
  // concated to the ciphertext for sending
  for(i=(crypto_secretbox_NONCEBYTES>>2)-1;i>=0;i--)
    ((unsigned int*) outbuf)[(crypto_secretbox_BOXZEROBYTES>>2)+i] = ((unsigned int*) outbuf)[i];
  size+=crypto_secretbox_NONCEBYTES + crypto_secretbox_ZEROBYTES -crypto_secretbox_BOXZEROBYTES ; // add nonce+mac size to total size
  reset_read_led;
  set_write_led;
  // send usb packet sized result
  for(i=0;i<size;i+=len) {
    len = (size-i)>=64?64:(size-i);
    while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf+crypto_secretbox_BOXZEROBYTES+i, len) == 0) ;
    // final packet
    if(len<64) {
      break;
    }
  }

  if(cmd_fn!=0) {
    set_read_led;
  } else {
    irq_mode();
  }
}

void decrypt_block(void) {
  if(cmd_fn!=0) {
    set_read_led;
  } else {
    irq_mode();
  }
}

void hash_init(void) {
  unsigned int i;
  unsigned char k[crypto_generichash_KEYBYTES];
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) k)[i]=0;
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

void sign_hash(void) {
  reset_read_led;
  crypto_generichash_update(&hash_state, buf+crypto_secretbox_ZEROBYTES, buf_size);
  // reset buf_size
  buf_size = 0;
  // disable op_cb, encryption only runs once per block
  op_cb = 0;
  // encrypt
  if(cmd_fn!=0) {
    set_read_led;
  } else {
    crypto_generichash_final(&hash_state, outbuf, 32);
    while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 32) == 0) ;
    irq_mode();
  }
}

void verify_hash(void) {
  reset_read_led;
  crypto_generichash_update(&hash_state, buf+crypto_secretbox_ZEROBYTES, buf_size);
  // reset buf_size
  buf_size = 0;
  // disable op_cb, encryption only runs once per block
  op_cb = 0;
  // encrypt
  if(cmd_fn!=0) {
    set_read_led;
  } else {
    crypto_generichash_final(&hash_state, outbuf, 32);
    outbuf[0] = (unsigned char) !sodium_memcmp(signature, outbuf, 32);
    while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, outbuf, 1) == 0) ;
    irq_mode();
  }
}

void rng_handler(void) {
  reset_write_led;
  randombytes_salsa20_random_buf((void *) buf, 64);
  reset_read_led;
  set_write_led;
  while((usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, buf, 64) == 0) && (next_op!=0)) ;
  if(next_op!=0) {
    set_read_led;
  } else {
    reset_write_led;
    reset_read_led;
  }
}

void handle_ctl(void) {
  char buf[64];
  buf[0]=0;
  int i, len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, buf, 64);
  if(len>0) {
    switch(buf[0] & 7) {
      case USB_CRYPTO_CMD_ENCRYPT: {
        mode_start();
        next_op = &encrypt_block;
        cmd_fn = &fill_buf;
        break;
      }
      case USB_CRYPTO_CMD_DECRYPT: {
        mode_start();
        next_op = &decrypt_block;
        cmd_fn = &fill_buf;
        break;
      }
      case USB_CRYPTO_CMD_SIGN: {
        mode_start();
        hash_init();
        next_op = &sign_hash;
        cmd_fn = &fill_buf;
        break;
      }
      case USB_CRYPTO_CMD_VERIFY: {
        if(len==crypto_generichash_BYTES+1) {
          mode_start();
          hash_init();
          //for(i=0;i<crypto_generichash_BYTES;i++) signature[i] = buf[i+1];
          memcpy(buf+1, signature, crypto_generichash_BYTES);
          next_op = &verify_hash;
          cmd_fn = &fill_buf;
        } else {
          for(i=32;(usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, "err: no sig", 5) == 0) && i>0;i--);
        }
        break;
      }
      case USB_CRYPTO_CMD_RNG: {
        mode_start();
        op_cb = &rng_handler;
        break;
      }
      case USB_CRYPTO_CMD_ABORT: {
        irq_mode();
        break;
      }
      case USB_CRYPTO_CMD_STORAGE: {
        if(cmd_fn!=0) {
          irq_mode();
        }
        dual_usb_mode = DISK;
        break;
      }

      case USB_CRYPTO_CMD_STOP: {
        op_cb = next_op;
        next_op = 0;
        cmd_fn = 0;
        break;
      }
      default: {
        //usb_puts("err: invalid cmd");
      }
    }
  }
}
