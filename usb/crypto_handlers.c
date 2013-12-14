#include "dual.h"
#include "usb_crypto.h"
#include "crypto_handlers.h"
#include "stm32f.h"
#include "randombytes_salsa20_random.h"
#include <crypto_secretbox.h>
#include "led.h"

#define BUF_SIZE 32768

extern usbd_device *usbd_dev;


unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES];
unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
int buf_size;
extern void (*op_cb)(void);
void (*cmd_fn)(void) = 0;
void (*next_op)(void) = 0;

void irq_mode(void) {
  cmd_fn = 0;
  op_cb = 0;
  next_op = 0;
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
    if(len>0 || op_cb) {
      for(i=32;(usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, "abort", 5) == 0) && i>0;i--);
      // we abort due to buffer overflow
      return;
    }
  } else {
    len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, buf+buf_size+crypto_secretbox_ZEROBYTES, 64);
  }
  buf_size+=len;
  if(buf_size >= BUF_SIZE) {
    op_cb = next_op;
  }
}

void encrypt_block(void) {
  int i, len, size = buf_size;
  unsigned char key[crypto_secretbox_KEYBYTES];
  // reset buf_size
  buf_size = 0;
  // disable op_cb, encryption only runs once per block
  op_cb = 0;
  // get key TODO
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++) ((unsigned int*) key)[i]=0;
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*) buf)[i]=0;
  // get nonce
  randombytes_salsa20_random_buf(outbuf, crypto_secretbox_NONCEBYTES);
  // encrypt
  reset_write_led;
  crypto_secretbox(outbuf+crypto_secretbox_NONCEBYTES, buf, size+crypto_secretbox_ZEROBYTES, outbuf, key);
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

void sign_hash(void) {
  if(cmd_fn!=0) {
    set_read_led;
  } else {
    irq_mode();
  }
}

void verify_hash(void) {
  if(cmd_fn!=0) {
    set_read_led;
  } else {
    irq_mode();
  }
}

void rng_handler(void) {
  reset_write_led;
  randombytes_salsa20_random_buf((void *) buf, 64);
  reset_read_led;
  set_write_led;
  while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_DATA_OUT, buf, 64) == 0) ;
  if(next_op!=0) set_read_led;
}

void handle_ctl(void) {
  char buf[64];
  buf[0]=0;
  int len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_CTRL_IN, buf, 64);
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
        next_op = &sign_hash;
        cmd_fn = &fill_buf;
        break;
      }
      case USB_CRYPTO_CMD_VERIFY: {
        mode_start();
        next_op = &verify_hash;
        cmd_fn = &fill_buf;
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
