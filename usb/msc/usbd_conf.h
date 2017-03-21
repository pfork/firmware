/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @date    05-December-2013
  * @brief   USB Device configuration file
  ******************************************************************************
  */

#ifndef usbd_conf_h
#define usbd_conf_h

#define USBD_CFG_MAX_NUM           1
#define USBD_ITF_MAX_NUM           1
#define USB_MAX_STR_DESC_SIZ       64

/* Class Layer Parameter */
#define MSC_IN_EP                    0x81
#define MSC_OUT_EP                   0x01
#define MSC_MAX_PACKET               64

#define MSC_MEDIA_PACKET             4096

#endif // guard
