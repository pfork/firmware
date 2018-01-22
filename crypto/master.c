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
#include "delay.h"
#include "display.h"
#include "stm32f.h"
#include "widgets.h"
#include "pitchfork.h"
#include "systimer.h"
#include "user.h"
#include "pbkdf2_generichash.h"

#define KEY_TIMEOUT 30*1000 // milliseconds / 30s
static uint8_t masterkey[crypto_secretbox_KEYBYTES];
volatile uint8_t pitchfork_hot;
static unsigned long long ts; // when was last accessed, for auto-expiry

// todo store unique device random into OTP[2] in init user (or in user record)

unsigned char* get_master_key(char *msg) {
  ts=sysctr; // update last used timestamp
  if(pitchfork_hot!=0) // unlocked, use key in store
    return masterkey;
  uint8_t passcode[65];
  int passlen=0;

  if(dual_usb_mode == DISK) {
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  } else {
    if(!cmd_blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_CTRL_IN, 1);
    if(!blocked) usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
  }

  disp_clear();
  disp_print_inv(0,0,"unlock key");
  disp_print(0,DISPLAY_HEIGHT-8,msg);
  disp_print(0,0,"unlock key");
  // todo get and display keyid/
  memset(passcode,0,sizeof(passcode));
  passlen=get_passcode(passcode, sizeof(passcode)); // read passcode
  disp_print(0,9,"deriving key");

  // derive key from chords + unique device random

  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userdata=(UserRecord*) userbuf;
  if(get_user(userdata)==-1) {
    disp_clear();
    disp_print(0,0,"uninitalized");
    disp_print(0,9,"pls reboot and");
    disp_print(0,18,"initialize");
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

  disp_print(0,9,"derived key  ");
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
