/**
  ******************************************************************************
  * @file    usbd_desc.c
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   This file provides the USBD descriptors and string formating method.
  ******************************************************************************
  */

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_conf.h"
#include "usb_regs.h"
#include "stm32f.h"
#include "main.h"

#define USBD_VID                   0x0483
#define USBD_PID                   0x5720

#define USBD_LANGID_STRING         0x409
#define USBD_MANUFACTURER_STRING   ((unsigned char*) "stf (c) 2013")


#define USBD_PRODUCT_FS_STRING        ((unsigned char*) "pocketsalt "VERSION" in FS Mass Storage Mode")
#define USBD_SERIALNUMBER_FS_STRING   ((unsigned char*) "0x01729a")
#define USBD_CONFIGURATION_FS_STRING  ((unsigned char*) "MSC Config")
#define USBD_INTERFACE_FS_STRING      ((unsigned char*) "MSC Interface")

USBD_DEVICE USR_desc = {
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor,
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN unsigned char USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END = {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_CFG_MAX_NUM            /*bNumConfigurations*/
} ; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
__ALIGN_BEGIN unsigned char USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN unsigned char USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END = {
     USB_SIZ_STRING_LANGID,
     USB_DESC_TYPE_STRING,
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING),
};

/**
* @brief  USBD_USR_DeviceDescriptor
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_DeviceDescriptor( unsigned char speed , unsigned short *length) {
  *length = sizeof(USBD_DeviceDesc);
  return USBD_DeviceDesc;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_LangIDStrDescriptor( unsigned char speed , unsigned short *length) {
  *length =  sizeof(USBD_LangIDDesc);
  return USBD_LangIDDesc;
}

/**
* @brief  USBD_USR_ProductStrDescriptor
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_ProductStrDescriptor( unsigned char speed , unsigned short *length) {
  USBD_GetString (USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_ManufacturerStrDescriptor( unsigned char speed , unsigned short *length) {
  USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_SerialStrDescriptor( unsigned char speed , unsigned short *length) {
  USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_ConfigStrDescriptor( unsigned char speed , unsigned short *length) {
  USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_InterfaceStrDescriptor
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
unsigned char *  USBD_USR_InterfaceStrDescriptor( unsigned char speed , unsigned short *length) {
  USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}
