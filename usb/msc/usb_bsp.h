/**
  ******************************************************************************
  * @file    usb_bsp.h
  * @date    05-December-2013
  * @brief   Specific api's relative to the used hardware platform
  ******************************************************************************
  */

#ifndef usb_bsb_h
#define usb_bsb_h

#include "usb_core.h"
#include "sdio.h"
#include "delay.h"

void BSP_Init(void);

//void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_Init (void);
//void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_EnableInterrupt (void);

#endif //usb_bsb_h
