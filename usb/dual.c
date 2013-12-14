#include "dual.h"
#include "usbd_msc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include <libopencm3/usb/usbd.h>
#include "delay.h"
#include "usb_crypto.h"
#include "led.h"

USB_OTG_CORE_HANDLE USB_OTG_dev;
usbd_device *usbd_dev;
unsigned int dual_usb_mode = CRYPTO;

void set_usb_mode(unsigned int newstate) {
  usbd_disconnect(usbd_dev, true);
  mDelay(1);
  dual_usb_mode = newstate;
  usbd_disconnect(usbd_dev, false);
}

void crypto_mode(void) {
  usb_start();
  set_usb_mode(CRYPTO);
  reset_status1_led;
}

void storage_mode(void) {
  USBD_Init(&USB_OTG_dev, &USR_desc, &USBD_MSC_cb, &USR_cb);
  set_usb_mode(DISK);
  set_status1_led;
}

