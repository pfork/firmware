/**
  ******************************************************************************
  * @file    usbd_core.h
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   Header file for usbd_core.c
  ******************************************************************************
  */

#ifndef usbd_core_h
#define usbd_core_h

#include "usb_dcd.h"
#include "usbd_def.h"

typedef enum {
  USBD_OK   = 0,
  USBD_BUSY,
  USBD_FAIL,
} USBD_Status;

void USBD_Init(USB_OTG_CORE_HANDLE *pdev,
               USBD_DEVICE *pDevice,
               USBD_Class_cb_TypeDef *class_cb,
               USBD_Usr_cb_TypeDef *usr_cb);
USBD_Status USBD_DeInit(USB_OTG_CORE_HANDLE *pdev);
USBD_Status USBD_ClrCfg(USB_OTG_CORE_HANDLE  *pdev, unsigned char cfgidx);
USBD_Status USBD_SetCfg(USB_OTG_CORE_HANDLE  *pdev, unsigned char cfgidx);

#endif /* usbd_core_h */
