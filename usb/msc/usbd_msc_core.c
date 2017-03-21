/**
  ******************************************************************************
  * @file    usbd_msc_core.c
  * @date    06-December-2013
  * @brief   This file provides all the MSC core functions.
  *
  * @verbatim
  *
  * ===================================================================
  *                       MSC Class  Description
  * ===================================================================
  *  This module manages the MSC class V1.0 following the "Universal
  *  Serial Bus Mass Storage Class (MSC) Bulk-Only Transport (BOT) Version 1.0
  *  Sep. 31, 1999".
  *  This driver implements the following aspects of the specification:
  *    - Bulk-Only Transport protocol
  *    - Subclass : SCSI transparent command set (ref. SCSI Primary Commands - 3 (SPC-3))
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_msc_mem.h"
#include "usbd_msc_core.h"
#include "usbd_msc_bot.h"
#include "usbd_req.h"
#include "stm32f.h"

unsigned char  USBD_MSC_Init (void  *pdev, unsigned char cfgidx);
unsigned char  USBD_MSC_DeInit (void  *pdev, unsigned char cfgidx);
unsigned char  USBD_MSC_Setup (void  *pdev, USB_SETUP_REQ *req);
unsigned char  USBD_MSC_DataIn (void  *pdev, unsigned char epnum);
unsigned char  USBD_MSC_DataOut (void  *pdev, unsigned char epnum);
unsigned char  *USBD_MSC_GetCfgDesc (unsigned char speed, unsigned short *length);
unsigned char USBD_MSC_CfgDesc[USB_MSC_CONFIG_DESC_SIZ];

USBD_Class_cb_TypeDef  USBD_MSC_cb = {
  USBD_MSC_Init,
  USBD_MSC_DeInit,
  USBD_MSC_Setup,
  NULL, /*EP0_TxSent*/
  NULL, /*EP0_RxReady*/
  USBD_MSC_DataIn,
  USBD_MSC_DataOut,
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_MSC_GetCfgDesc,
};

/* USB Mass storage device Configuration Descriptor */
/* All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
__ALIGN_BEGIN unsigned char USBD_MSC_CfgDesc[USB_MSC_CONFIG_DESC_SIZ] __ALIGN_END = {
  0x09,   /* bLength: Configuation Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,   /* bDescriptorType: Configuration */
  USB_MSC_CONFIG_DESC_SIZ,

  0x00,
  0x01,   /* bNumInterfaces: 1 interface */
  0x01,   /* bConfigurationValue: */
  0x04,   /* iConfiguration: */
  0x80,   /* bmAttributes: */
  0x32,   /* MaxPower 100 mA */

  /********************  Mass Storage interface ********************/
  0x09,   /* bLength: Interface Descriptor size */
  0x04,   /* bDescriptorType: */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints*/
  0x08,   /* bInterfaceClass: MSC Class */
  0x06,   /* bInterfaceSubClass : SCSI transparent*/
  0x50,   /* nInterfaceProtocol */
  0x05,          /* iInterface: */
  /********************  Mass Storage Endpoints ********************/
  0x07,   /*Endpoint descriptor length = 7*/
  0x05,   /*Endpoint descriptor type */
  MSC_IN_EP,   /*Endpoint address (IN, address 1) */
  0x02,   /*Bulk endpoint type */
  LOBYTE(MSC_MAX_PACKET),
  HIBYTE(MSC_MAX_PACKET),
  0x00,   /*Polling interval in milliseconds */

  0x07,   /*Endpoint descriptor length = 7 */
  0x05,   /*Endpoint descriptor type */
  MSC_OUT_EP,   /*Endpoint address (OUT, address 1) */
  0x02,   /*Bulk endpoint type */
  LOBYTE(MSC_MAX_PACKET),
  HIBYTE(MSC_MAX_PACKET),
  0x00     /*Polling interval in milliseconds*/
};

__ALIGN_BEGIN static unsigned char  USBD_MSC_MaxLun  __ALIGN_END = 0;
__ALIGN_BEGIN static unsigned char  USBD_MSC_AltSet  __ALIGN_END = 0;

/**
* @brief  USBD_MSC_Init
*         Initialize  the mass storage configuration
* @param  pdev: device instance
* @param  cfgidx: configuration index
* @retval status
*/
unsigned char  USBD_MSC_Init (void  *pdev, unsigned char cfgidx) {
  USBD_MSC_DeInit(pdev , cfgidx );
  /* Open EP IN */
  DCD_EP_Open(pdev, MSC_IN_EP, MSC_EPIN_SIZE, USB_OTG_EP_BULK);
  /* Open EP OUT */
  DCD_EP_Open(pdev, MSC_OUT_EP, MSC_EPOUT_SIZE, USB_OTG_EP_BULK);
  /* Init the BOT  layer */
  MSC_BOT_Init(pdev);

  return USBD_OK;
}

/**
* @brief  USBD_MSC_DeInit
*         DeInitilaize  the mass storage configuration
* @param  pdev: device instance
* @param  cfgidx: configuration index
* @retval status
*/
unsigned char  USBD_MSC_DeInit (void  *pdev, unsigned char cfgidx) {
  /* Close MSC EPs */
  DCD_EP_Close (pdev , MSC_IN_EP);
  DCD_EP_Close (pdev , MSC_OUT_EP);

  /* Un Init the BOT layer */
  MSC_BOT_DeInit(pdev);
  return USBD_OK;
}
/**
* @brief  USBD_MSC_Setup
*         Handle the MSC specific requests
* @param  pdev: device instance
* @param  req: USB request
* @retval status
*/
unsigned char  USBD_MSC_Setup (void  *pdev, USB_SETUP_REQ *req) {
  switch (req->bmRequest & USB_REQ_TYPE_MASK) {
  /* Class request */
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest) {
    case BOT_GET_MAX_LUN :
      if((req->wValue  == 0) &&
         (req->wLength == 1) &&
         ((req->bmRequest & 0x80) == 0x80)) {
        USBD_MSC_MaxLun = USBD_STORAGE_fops->GetMaxLun();
        if(USBD_MSC_MaxLun > 0) {
           USBD_CtlSendData (pdev, &USBD_MSC_MaxLun, 1);
        } else {
          USBD_CtlError(pdev , req);
          return USBD_FAIL;
        }
      } else {
         USBD_CtlError(pdev , req);
         return USBD_FAIL;
      }
      break;

    case BOT_RESET :
      if((req->wValue  == 0) &&
         (req->wLength == 0) &&
        ((req->bmRequest & 0x80) != 0x80))
      {
         MSC_BOT_Reset(pdev);
      } else {
         USBD_CtlError(pdev , req);
         return USBD_FAIL;
      }
      break;

    default:
       USBD_CtlError(pdev , req);
       return USBD_FAIL;
    }
    break;

  /* Interface & Endpoint request */
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest) {
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        &USBD_MSC_AltSet,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      USBD_MSC_AltSet = (unsigned char)(req->wValue);
      break;

    case USB_REQ_CLEAR_FEATURE:
      /* Flush the FIFO and Clear the stall status */
      DCD_EP_Flush(pdev, (unsigned char)req->wIndex);

      /* Re-activate the EP */
      DCD_EP_Close (pdev , (unsigned char)req->wIndex);
      if((((unsigned char)req->wIndex) & 0x80) == 0x80) {
        DCD_EP_Open(pdev,
                    ((unsigned char)req->wIndex),
                    MSC_EPIN_SIZE,
                    USB_OTG_EP_BULK);
      } else {
        DCD_EP_Open(pdev,
                    ((unsigned char)req->wIndex),
                    MSC_EPOUT_SIZE,
                    USB_OTG_EP_BULK);
      }

      /* Handle BOT error */
      MSC_BOT_CplClrFeature(pdev, (unsigned char)req->wIndex);
      break;

    }
    break;

  default:
    break;
  }
  return USBD_OK;
}

/**
* @brief  USBD_MSC_DataIn
*         handle data IN Stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
unsigned char  USBD_MSC_DataIn (void  *pdev, unsigned char epnum) {
  MSC_BOT_DataIn(pdev , epnum);
  return USBD_OK;
}

/**
* @brief  USBD_MSC_DataOut
*         handle data OUT Stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
unsigned char  USBD_MSC_DataOut (void  *pdev, unsigned char epnum) {
  MSC_BOT_DataOut(pdev , epnum);
  return USBD_OK;
}

/**
* @brief  USBD_MSC_GetCfgDesc
*         return configuration descriptor
* @param  speed : current device speed
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
unsigned char  *USBD_MSC_GetCfgDesc (unsigned char speed, unsigned short *length) {
  *length = sizeof (USBD_MSC_CfgDesc);
  return USBD_MSC_CfgDesc;
}
