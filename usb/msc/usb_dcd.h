/**
  ******************************************************************************
  * @file    usb_dcd.h
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   Peripheral Device Interface Layer
  ******************************************************************************
  */

#ifndef usb_dcd_h
#define usb_dcd_h

#include "usb_core.h"

#define USB_OTG_EP_CONTROL                       0
#define USB_OTG_EP_ISOC                          1
#define USB_OTG_EP_BULK                          2
#define USB_OTG_EP_INT                           3
#define USB_OTG_EP_MASK                          3

/*  Device Status */
#define USB_OTG_DEFAULT                          1
#define USB_OTG_ADDRESSED                        2
#define USB_OTG_CONFIGURED                       3
#define USB_OTG_SUSPENDED                        4

typedef struct {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned char  bEndpointAddress;
  unsigned char  bmAttributes;
  unsigned short wMaxPacketSize;
  unsigned char  bInterval;
} EP_DESCRIPTOR, *PEP_DESCRIPTOR;

void         DCD_Init(USB_OTG_CORE_HANDLE *pdev);
void         DCD_EP_SetAddress(USB_OTG_CORE_HANDLE *pdev, unsigned char address);
unsigned int DCD_EP_Open(USB_OTG_CORE_HANDLE *pdev, unsigned char ep_addr, unsigned short ep_mps, unsigned char ep_type);
unsigned int DCD_EP_Close(USB_OTG_CORE_HANDLE *pdev, unsigned char  ep_addr);
unsigned int DCD_EP_PrepareRx(USB_OTG_CORE_HANDLE *pdev, unsigned char ep_addr, unsigned char *pbuf, unsigned short buf_len);
unsigned int DCD_EP_Tx (USB_OTG_CORE_HANDLE *pdev, unsigned char  ep_addr, unsigned char *pbuf, unsigned int buf_len);
unsigned int DCD_EP_Stall (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum);
unsigned int DCD_EP_ClrStall (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum);
unsigned int DCD_EP_Flush (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum);
unsigned int DCD_Handle_ISR(USB_OTG_CORE_HANDLE *pdev);
unsigned int DCD_GetEPStatus(USB_OTG_CORE_HANDLE *pdev, unsigned char epnum);
void         DCD_SetEPStatus (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum, unsigned int Status);

#endif //usb_dcd_h
