/**
  ************************************************************************************
  * @file    pitchfork_irq.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides PITCHFORK usb irq handlers
  ************************************************************************************
  */

#include "usb.h"
#include "pitchfork.h"
#include "stm32f.h"
#include <utils.h>
#include <string.h>
#include "led.h"

/**
  * @brief  modus: state of PITCHFORK, of is USB_CRYPTO_CMD_STOP
  */
CRYPTO_CMD modus=USB_CRYPTO_CMD_STOP;
/**
  * @brief  params: for passing params from ctl to buf handlers
  *         ctl is in irq, while buf handler is from mainloop
  */
unsigned char params[128];

/**
  * @brief  reset: resets PITCHFORK mode
  * @param  None
  * @retval None
  */
void reset(void) {
  unsigned int i;
  modus = USB_CRYPTO_CMD_STOP;
  bufs[0].size = 0;
  bufs[1].size = 0;
  bufs[0].state = INPUT;
  bufs[1].state = INPUT;
  blocked = 0;
  for (i=0;i<(sizeof(params)>>2);i++) ((unsigned int*) params)[i]=0;
  usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
}

/**
  * @brief  check_seed emits key errors via usb
  * @param  sptr: return value form get_seed
  * @retval 0 on error, 1 on success
  */
unsigned char check_seed(unsigned int sptr) {
  if (sptr == 0) {
    usb_write((unsigned char*) "err: no key", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  if (sptr == -1) {
    usb_write((unsigned char*) "err: unavailable key", 11, 32,USB_CRYPTO_EP_CTRL_OUT);
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
unsigned char peer_to_seed(unsigned char* dst, unsigned char* src, unsigned char len) {
  if( len == 0 || len >= PEER_NAME_MAX) {
    usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  unsigned int sptr = get_peer_seed(dst, src, len);
  if(check_seed(sptr) == 0) return 0;
  usb_write( ((unsigned char*) ((SeedRecord*) sptr)->keyid), STORAGE_ID_LEN, 32, USB_CRYPTO_EP_CTRL_OUT);
  return 1;
}

/**
  * @brief  hash_init: intializer for sign/verify ops
  * @param  k: key to init the hash engine
  * @retval None
  */
void hash_init(unsigned char* k) {
  crypto_generichash_init(&hash_state, k, crypto_generichash_KEYBYTES, 32);
}

/**
  * @brief  toggle_buf: tries to switch double buffering
  * @param  None
  * @retval ptr to active_buf, or -1 on error
  */
int toggle_buf(void) {
  if(bufs[!active_buf].state == INPUT && bufs[!active_buf].size==0) {
    if(bufs[active_buf].state == INPUT)
      bufs[active_buf].state = OUTPUT;
    active_buf = !active_buf;
    return active_buf;
  }
  return -1;
}

/**
  * @brief  usb rx callback for the data end point
  * @param  usbd_dev: pointer to usbd (libopencm3 style)
  * @param  ep: end point (libopencm3 style)
  * @retval None
  */
void handle_data(usbd_device *usbd_dev, uint8_t ep) {
  int len;
  unsigned char tmpbuf[64];
  if(modus>USB_CRYPTO_CMD_VERIFY) {
    // we are not in any modus that needs a buffer
    len = usb_read(tmpbuf); // sink it
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
  len = usb_read(bufs[active_buf].start+bufs[active_buf].size);
  // adjust buffer size
  bufs[active_buf].size+=len;
  if(len<64 && (modus!=USB_CRYPTO_CMD_DECRYPT || bufs[active_buf].size!=BUF_SIZE+40 )) {
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


/**
  * @brief  usb rx callback for the ctl end point
  * @param  usbd_dev: pointer to usbd (libopencm3 style)
  * @param  ep: end point (libopencm3 style)
  * @retval None
  */
void handle_ctl(usbd_device *usbd_dev, uint8_t ep) {
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
        if(peer_to_seed(params, (unsigned char*) buf+1, len-1)==0)
          return;
        modus = USB_CRYPTO_CMD_ENCRYPT;
        break;
      }
      case USB_CRYPTO_CMD_DECRYPT: {
        if(len!=STORAGE_ID_LEN+1) {
          usb_write((unsigned char*) "err: no keyid", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
          return;
        }
        // keyid 2 seed
        unsigned int sptr = get_seed(params, 0, (unsigned char*) buf+1);
        if(check_seed(sptr) == 0) return;
        modus = USB_CRYPTO_CMD_DECRYPT;
        break;
      }
      case USB_CRYPTO_CMD_SIGN: {
        unsigned char seed [crypto_generichash_KEYBYTES];
        if(peer_to_seed(seed, (unsigned char*) buf+1, len-1)==0)
          return;
        hash_init(seed);
        modus = USB_CRYPTO_CMD_SIGN;
        break;
      }
      case USB_CRYPTO_CMD_VERIFY: {
        if(len!=crypto_generichash_BYTES+STORAGE_ID_LEN+1) {
          usb_write((unsigned char*) "err: bad args", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
          return;
        }
        // get seed via keyid
        unsigned char seed[crypto_generichash_KEYBYTES];
        if(check_seed(get_seed(seed,0,(unsigned char*) buf+1+crypto_generichash_BYTES)) == 0)
          return;
        hash_init(seed);
        // copy signature for final verification
        memcpy(params, buf+1, crypto_generichash_BYTES);
        modus = USB_CRYPTO_CMD_VERIFY;
        break;
      }
      case USB_CRYPTO_CMD_LIST_KEYS: {
        if(len>1 && len<PEER_NAME_MAX) {
          topeerid((unsigned char*) buf+1, len-1, params+1);
          params[0] = 1;
        } else {
          params[0] = 0;
        }
        modus = USB_CRYPTO_CMD_LIST_KEYS;
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
