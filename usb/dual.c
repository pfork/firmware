#include "dual.h"
#include "usbd_msc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include <libopencm3/usb/usbd.h>
#include "delay.h"
#include "usb.h"
#include "led.h"

USB_OTG_CORE_HANDLE USB_OTG_dev;
usbd_device *usbd_dev;
unsigned int dual_usb_mode = CRYPTO;

/**
  * @brief  set_usb_mode: sets the usb mode
  * @param  newstate: mode to set
  * @retval None
  */
void set_usb_mode(unsigned int newstate) {
  usbd_disconnect(usbd_dev, true);
  mDelay(1);
  dual_usb_mode = newstate;
  usbd_disconnect(usbd_dev, false);
}

/**
  * @brief  crypto_mode: resets usb for PITCHFORK mode
  * @param  None
  * @retval None
  */
void crypto_mode(void) {
  usb_start();
  set_usb_mode(CRYPTO);
  reset_status1_led;
}

/**
  * @brief  storage_mode: resets usb for PITCHFORK storage mode
  * @param  None
  * @retval None
  */
void storage_mode(void) {
  USBD_Init(&USB_OTG_dev, &USR_desc, &USBD_MSC_cb, &USR_cb);
  set_usb_mode(DISK);
  set_status1_led;
}

