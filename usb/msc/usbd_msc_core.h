/**
  ******************************************************************************
  * @file    usbd_msc_core.h
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   header for the usbd_msc_core.c file
  ******************************************************************************
  */

#ifndef _USB_MSC_CORE_H_
#define _USB_MSC_CORE_H_

#include  "usbd_ioreq.h"

#define BOT_GET_MAX_LUN              0xFE
#define BOT_RESET                    0xFF
#define USB_MSC_CONFIG_DESC_SIZ      32

#define MSC_EPIN_SIZE  *(unsigned short *)(((USB_OTG_CORE_HANDLE *)pdev)->dev.pConfig_descriptor + 22)
#define MSC_EPOUT_SIZE *(unsigned short *)(((USB_OTG_CORE_HANDLE *)pdev)->dev.pConfig_descriptor + 29)

extern USBD_Class_cb_TypeDef  USBD_MSC_cb;

#endif  // _USB_MSC_CORE_H_
