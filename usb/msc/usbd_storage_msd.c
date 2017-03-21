/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @date    05-December-2013
  * @brief   This file provides the interface to the sdio device.
  ******************************************************************************
  */

#include "usbd_msc_mem.h"
#include "usb_conf.h"
#include "sd.h"

#include "stm32f.h"

#define STORAGE_LUN_NBR                  1

/* USB Mass storage Standard Inquiry Data */
const char  STORAGE_Inquirydata[] = { //36
  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,
  0x00,
  's', 't', 'f', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'm', 'i', 'c', 'r', 'o', 'S', 'D', ' ', /* Product      : 16 Bytes */
  'F', 'l', 'a', 's', 'h', ' ', ' ', ' ',
  '1', '.', '0' ,'0',                     /* Version      : 4 Bytes */
};

char STORAGE_Init(void);
char STORAGE_GetCapacity (unsigned int *block_num, unsigned int *block_size);
char STORAGE_IsReady(void);
char STORAGE_IsWriteProtected(void);
char STORAGE_Read(unsigned char *buf, unsigned int blk_addr, unsigned int blk_len);
char STORAGE_Write(unsigned char *buf, unsigned int blk_addr, unsigned int blk_len);
char STORAGE_GetMaxLun(void);

USBD_STORAGE_cb_TypeDef USBD_MICRO_SDIO_fops = {
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  (char *)STORAGE_Inquirydata,
};

USBD_STORAGE_cb_TypeDef  *USBD_STORAGE_fops = &USBD_MICRO_SDIO_fops;
extern SD_CardInfo SDCardInfo;
volatile unsigned int count = 0;

/**
  * @brief  Initialize the storage medium
  * @retval Status
  */

char STORAGE_Init (void) {
  NVIC_IPR(NVIC_SDIO_IRQn) = 0;
  irq_enable(NVIC_SDIO_IRQn);

  if( SD_Init() != 0) {
    return (-1);
  }

  return (0);
}

/**
  * @brief  return medium capacity and block size
  * @param  block_num :  number of physical block
  * @param  block_size : size of a physical block
  * @retval Status
  */
char STORAGE_GetCapacity (unsigned int *block_num, unsigned int *block_size) {
  if(SD_GetStatus() != 0 ) {
    return (-1);
  }

  *block_size =  512;
  *block_num =  SDCardInfo.CardCapacity / 512; // CardCapacity is uint64_t
  //*block_size =  SDCardInfo.CardBlockSize;
  //*block_num =  SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;

  return (0);
}

/**
  * @brief  check whether the medium is ready
  * @retval Status
  */
char  STORAGE_IsReady (void) {
  static char last_status = 0;

  if(last_status  < 0) {
    SD_Init();
    last_status = 0;
  }

  if(SD_GetStatus() != 0) {
    last_status = -1;
    return (-1);
  }

  return (0);
}

/**
  * @brief  check whether the medium is write-protected
  * @retval Status
  */
char  STORAGE_IsWriteProtected (void) {
  return  0;
}

/**
  * @brief  Read data from the medium
  * @param  buf : Pointer to the buffer to save data
  * @param  blk_addr :  address of 1st block to be read
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */
char STORAGE_Read (unsigned char *buf, unsigned int blk_addr, unsigned int blk_len) {
  if( SD_ReadMultiBlocks(buf, blk_addr, 512, blk_len) != 0) {
    return -1;
  }
  SD_WaitReadOperation();
  while (SD_GetStatus() != SD_TRANSFER_OK);
  return 0;
}
/**
  * @brief  Write data to the medium
  * @param  buf : Pointer to the buffer to write from
  * @param  blk_addr :  address of 1st block to be written
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */
char STORAGE_Write (unsigned char *buf, unsigned int blk_addr, unsigned int blk_len) {
  if( SD_WriteMultiBlocks(buf, blk_addr, 512, blk_len) != 0) {
    return -1;
  }
  SD_WaitWriteOperation();
  while (SD_GetStatus() != SD_TRANSFER_OK);
  return (0);
}

/**
  * @brief  Return number of supported logical unit
  * @param  None
  * @retval number of logical unit
  */
char STORAGE_GetMaxLun (void) {
  return (STORAGE_LUN_NBR - 1);
}

