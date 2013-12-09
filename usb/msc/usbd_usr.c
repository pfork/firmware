#include "usbd_usr.h"
#include <stdio.h>

USBD_Usr_cb_TypeDef USR_cb = {
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
};

#define USER_INFORMATION1  "INFO : Single Lun configuration"
#define USER_INFORMATION2  "INFO : microSD is used"

/**
* @brief  Displays the message on LCD on device lib initialization
*/
void USBD_USR_Init(void) {}

/**
* @brief  Displays the message on LCD on device reset event
* @param  speed : device speed
*/
void USBD_USR_DeviceReset (unsigned char speed) {}


/**
* @brief  Displays the message on LCD on device config event
*/
void USBD_USR_DeviceConfigured (void) {
  /* println("> MSC Interface started.\n"); */
}

/**
* @brief  Displays the message on LCD on device suspend event
*/
void USBD_USR_DeviceSuspended(void) {
  /* println("> Device In suspend mode.\n"); */
}

/**
* @brief  Displays the message on LCD on device resume event
*/
void USBD_USR_DeviceResumed(void) {}
