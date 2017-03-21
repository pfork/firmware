/**
  ******************************************************************************
  * @file    usbd_usr.h
  * @date    05-December-2013
  * @brief   Header file for usbd_usr.c
  ******************************************************************************
  */

#ifndef usbd_usr_h
#define usbd_usr_h

#include "usbd_core.h"

extern USBD_Usr_cb_TypeDef USR_cb;
extern USBD_Usr_cb_TypeDef USR_FS_cb;

void USBD_USR_Init(void);
void USBD_USR_DeviceReset (unsigned char speed);
void USBD_USR_DeviceConfigured (void);
void USBD_USR_DeviceSuspended(void);
void USBD_USR_DeviceResumed(void);

void USBD_USR_DeviceConnected(void);
void USBD_USR_DeviceDisconnected(void);

void USBD_USR_FS_Init(void);
void USBD_USR_FS_DeviceReset (unsigned char speed);
void USBD_USR_FS_DeviceConfigured (void);
void USBD_USR_FS_DeviceSuspended(void);
void USBD_USR_FS_DeviceResumed(void);

void USBD_USR_FS_DeviceConnected(void);
void USBD_USR_FS_DeviceDisconnected(void);

#endif /*__USBD_USR_H__*/
