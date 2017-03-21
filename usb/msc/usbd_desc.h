/**
  ******************************************************************************
  * @file    usbd_desc.h
  * @date    05-December-2013
  * @brief   header file for the usbd_desc.c file
  ******************************************************************************
  */

#ifndef usb_desc_h
#define usb_desc_h

#include "usbd_def.h"
#include "usbd_conf.h"

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05
#define USB_SIZ_DEVICE_DESC                     18
#define USB_SIZ_STRING_LANGID                   4

extern  unsigned char USBD_DeviceDesc  [USB_SIZ_DEVICE_DESC];
extern  unsigned char USBD_StrDesc[USB_MAX_STR_DESC_SIZ];
extern  unsigned char USBD_OtherSpeedCfgDesc[USB_LEN_CFG_DESC];
extern  unsigned char USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];
extern  unsigned char USBD_LangIDDesc[USB_SIZ_STRING_LANGID];
extern  USBD_DEVICE USR_desc;

unsigned char *     USBD_USR_DeviceDescriptor( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_LangIDStrDescriptor( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_ManufacturerStrDescriptor ( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_ProductStrDescriptor ( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_SerialStrDescriptor( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_ConfigStrDescriptor( unsigned char speed , unsigned short *length);
unsigned char *     USBD_USR_InterfaceStrDescriptor( unsigned char speed , unsigned short *length);

#ifdef USB_SUPPORT_USER_STRING_DESC
unsigned char *     USBD_USR_USRStringDesc (unsigned char speed, unsigned char idx , unsigned short *length);
#endif /* USB_SUPPORT_USER_STRING_DESC */

#endif /* usbd_desc_h */
