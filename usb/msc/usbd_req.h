/**
  ******************************************************************************
  * @file    usbd_req.h
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   header file for the usbd_req.c file
  ******************************************************************************
  */

#ifndef usb_request_h
#define usb_request_h

#include  "usbd_def.h"
#include  "usbd_core.h"
#include  "usbd_conf.h"

#define USB_REQ_GET_STATUS			0
#define USB_REQ_CLEAR_FEATURE			1
/* Reserved for future use: 2 */
#define USB_REQ_SET_FEATURE			3
/* Reserved for future use: 3 */
#define USB_REQ_SET_ADDRESS			5
#define USB_REQ_GET_DESCRIPTOR			6
#define USB_REQ_SET_DESCRIPTOR			7
#define USB_REQ_GET_CONFIGURATION		8
#define USB_REQ_SET_CONFIGURATION		9
#define USB_REQ_GET_INTERFACE			10
#define USB_REQ_SET_INTERFACE			11
#define USB_REQ_SET_SYNCH_FRAME			12

USBD_Status USBD_StdDevReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status USBD_StdItfReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status USBD_StdEPReq  (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
void USBD_ParseSetupRequest( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
void USBD_CtlError( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
void USBD_GetString(unsigned char *desc, unsigned char *unicode, unsigned short *len);

#endif /* usb_request_h */
