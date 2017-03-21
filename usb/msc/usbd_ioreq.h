/**
  ******************************************************************************
  * @file    usbd_ioreq.h
  * @date    05-December-2013
  * @brief   header file for the usbd_ioreq.c file
  ******************************************************************************
  */

#ifndef usbd_ioreq_h
#define usbd_ioreq_h

#include  "usbd_def.h"
#include  "usbd_core.h"

USBD_Status  USBD_CtlSendData (USB_OTG_CORE_HANDLE  *pdev, unsigned char *buf, unsigned short len);
USBD_Status  USBD_CtlContinueSendData (USB_OTG_CORE_HANDLE  *pdev, unsigned char *pbuf, unsigned short len);
USBD_Status USBD_CtlPrepareRx (USB_OTG_CORE_HANDLE  *pdev, unsigned char *pbuf, unsigned short len);
USBD_Status  USBD_CtlContinueRx (USB_OTG_CORE_HANDLE  *pdev, unsigned char *pbuf, unsigned short len);
USBD_Status  USBD_CtlSendStatus (USB_OTG_CORE_HANDLE  *pdev);
USBD_Status  USBD_CtlReceiveStatus (USB_OTG_CORE_HANDLE  *pdev);
unsigned short  USBD_GetRxCount (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum);
#endif /* usbd_ioreq_h */
