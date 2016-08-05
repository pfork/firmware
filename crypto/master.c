/**
  ************************************************************************************
  * @file    master.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides functions to return the master password
  ************************************************************************************
  */

#include <string.h>
#include <stdint.h>
#include "crypto_secretbox.h"
#include "crypto_generichash.h"
#include "dual.h"
#include "usb.h"
#include "keys.h"
#include "delay.h"
#include "oled.h"
#include "itoa.h"
#include "stm32f.h"
#include "widgets.h"
#include "pitchfork.h"
#include "systimer.h"
#include "storage.h"
#include "pbkdf2_generichash.h"

#define KEY_TIMEOUT 30*1000 // milliseconds / 30s
static uint8_t masterkey[crypto_secretbox_KEYBYTES];
volatile uint8_t pitchfork_hot;
static unsigned long long ts; // when was last accessed, for auto-expiry

// todo store unique device random into OTP[2] in init user (or in user record)

// reads in max 64 digits composing a 32 byte string.
// reads the next digit on pressing the enter button
// premature exiting can be achieved by pressing enter button longer for the last digit
static int chord(uint8_t *dst) {
  uint8_t keys, lastbyte=0;
  int n=0, i=0;
  char prev=0;
  while(i<64) {
    keys = keys_pressed();
    if(keys & BUTTON_ENTER) {
      n++;
      if(n>400) {
        if(i>0)
            dst[(i-1)/2]=lastbyte; // undo the last enter-ed digit
        return i/2;
      }
    } else {
      n=0;
    }

    if(keys & BUTTON_ENTER && prev!=keys) {
      lastbyte=dst[i/2]; // in case this is the enter for finishing the chords
      // accept entry
      if(i%2==0) {
        dst[i/2] = (dst[i/2] & 0xf) | ((keys&0xf) << 4);
      } else {
        dst[i/2] = (dst[i/2] & 0xf0) | (keys&0xf);
      }
      i++;
      char tmp[16];
      itos(tmp,i);
      oled_print(7*8,32,tmp,Font_8x8);
      mDelay(200);
    }
    prev=keys;
  }
  return i/2;
}

unsigned char* get_master_key(void) {
  ts=sysctr; // update last used timestamp
  if(pitchfork_hot!=0) // unlocked, use key in store
    return masterkey;
  uint8_t passcode[crypto_secretbox_KEYBYTES];
  int passlen=0;

  if(dual_usb_mode == DISK) {
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  } else {
    if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  }

  oled_clear();
  oled_print_inv(0,0,"unlock key", Font_8x8);
  oled_print(0,0,"unlock key", Font_8x8);
  // todo get and display keyid/
  memset(passcode,0,sizeof(passcode));
  passlen=chord(passcode); // read passcode
  oled_print(0,9,"deriving key", Font_8x8);

  // derive key from chords + unique device random
  // todo generichash is not a kdf per se
  UserRecord *userdata = get_userrec();
  if(userdata==NULL) {
    oled_clear();
    oled_print(0,0,"uninitalized",Font_8x8);
    oled_print(0,9,"pls reboot and", Font_8x8);
    oled_print(0,18,"initialize",Font_8x8);
    while(1);
  }

  // xor user salt with device salt
  uint8_t salt[32];
  int i, *salt4 = (int*) salt,
      *a = (int*) userdata->salt,
      *b = (int*) (OTP_START_ADDR + 2 * OTP_BYTES_IN_BLOCK);
  for(i=0;i<8;i++)
    salt4[i] = a[i] ^ b[i];

  pbkdf2_generichash(masterkey,
                     passcode, passlen,
                     salt);

  oled_print(0,9,"derived key  ", Font_8x8);
  memset(passcode,0,sizeof(passcode));
  pitchfork_hot=1;
  gui_refresh=1;
  if(dual_usb_mode == DISK) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
  else {
    if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 0);
    if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
  }
  return masterkey;
}

void erase_master_key(void) {
  memset(masterkey,0, sizeof(masterkey));
  pitchfork_hot=0;
}

void expire_master_key(void) {
  if(pitchfork_hot!=0 &&
     ts+KEY_TIMEOUT<sysctr &&
     dual_usb_mode != DISK &&
     modus == PITCHFORK_CMD_STOP ) {

    erase_master_key();
    gui_refresh=1;
  }
}
