/**
  ******************************************************************************
  * @file    usb_dcd_int.h
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   Peripheral Device Interface Layer
  ******************************************************************************
  */

#ifndef usb_dcd_int_h
#define usb_dcd_int_h

#include "usb_dcd.h"

typedef struct _USBD_DCD_INT {
  unsigned char (* DataOutStage) (USB_OTG_CORE_HANDLE *pdev , unsigned char epnum);
  unsigned char (* DataInStage)  (USB_OTG_CORE_HANDLE *pdev , unsigned char epnum);
  unsigned char (* SetupStage) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* SOF) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* Reset) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* Suspend) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* Resume) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* IsoINIncomplete) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* IsoOUTIncomplete) (USB_OTG_CORE_HANDLE *pdev);

  unsigned char (* DevConnected) (USB_OTG_CORE_HANDLE *pdev);
  unsigned char (* DevDisconnected) (USB_OTG_CORE_HANDLE *pdev);
} USBD_DCD_INT_cb_TypeDef;

extern USBD_DCD_INT_cb_TypeDef *USBD_DCD_INT_fops;

#define CLEAR_IN_EP_INTR(epnum,intr) \
  diepint.d32=0; \
  diepint.b.intr = 1; \
  WRITE_REG32(&pdev->regs.INEP_REGS[epnum]->DIEPINT,diepint.d32);

#define CLEAR_OUT_EP_INTR(epnum,intr) \
  doepint.d32=0; \
  doepint.b.intr = 1; \
  WRITE_REG32(&pdev->regs.OUTEP_REGS[epnum]->DOEPINT,doepint.d32);

unsigned int USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);

#endif // usb_dcd_int_h
