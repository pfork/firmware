#include "usb_crypto.h"
#include "init.h"
#include "uart.h"
#include "main.h"
#include "systimer.h"
#include "led.h"
#include "keys.h"
#include "delay.h"

#include "usbd_msc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include <libopencm3/usb/usbd.h>

__ALIGN_BEGIN USB_OTG_CORE_HANDLE     USB_OTG_dev __ALIGN_END ;
extern usbd_device *usbd_dev;

unsigned int state = RNG;

void set_usb_mode(unsigned int newstate) {
  usbd_disconnect(usbd_dev, true);
  mDelay(1);
  state = newstate;
  usbd_disconnect(usbd_dev, false);
}

void rng(void) {
  usb_start();
  set_usb_mode(RNG);
  //usb_string("\n\rMode: rng");
}

void disk(void) {
  USBD_Init(&USB_OTG_dev, &USR_desc, &USBD_MSC_cb, &USR_cb);
  set_usb_mode(DISK);
}

int main ( void ) {
  unsigned char kmask;
  unsigned int recent_key;
  init();

  //set_status1_led;
  //set_status2_led;

  recent_key = 0;

  while(1) {
    kmask = key_handler();
    //if(kmask!=0) usb_hexstring(kmask,1);
    if(kmask & (1<<6) && recent_key==0) {
      recent_key = 100;
      switch(state) {
      case RNG: { disk(); set_status1_led; break; }
      case DISK: { rng(); reset_status1_led; break; }
      }
    }
    if(recent_key>0) recent_key--;
    led_handler();
  }
  return(0);
}
