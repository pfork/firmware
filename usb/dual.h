/**
  ************************************************************************************
  * @file    dual.h
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef dual_h
#define dual_h

#include <libopencm3/usb/usbd.h>
#include "usb_core.h"

enum DUAL_USB_MODE {
  OFF = 0,
  CRYPTO,
  DISK
};

extern unsigned int dual_usb_mode;
extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern usbd_device *usbd_dev;

void crypto_mode(void);
void storage_mode(void);

#endif // dual_h
