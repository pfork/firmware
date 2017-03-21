/**
  ******************************************************************************
  * @file    usbd_msc_scsi.c
  * @date    06-December-2013
  * @brief   This file provides all the USBD SCSI layer functions.
  ******************************************************************************
  */

#include "usbd_msc_bot.h"
#include "usbd_msc_scsi.h"
#include "usbd_msc_mem.h"
#include "usbd_msc_data.h"
#include "stm32f.h"

SCSI_Sense_TypeDef SCSI_Sense [SENSE_LIST_DEEPTH];
unsigned char   SCSI_Sense_Head;
unsigned char   SCSI_Sense_Tail;

unsigned int  SCSI_blk_size;
unsigned int  SCSI_blk_nbr;

unsigned int  SCSI_blk_addr;
unsigned int  SCSI_blk_len;

USB_OTG_CORE_HANDLE  *cdev;

static char SCSI_TestUnitReady(unsigned char lun, unsigned char *params);
static char SCSI_Inquiry(unsigned char lun, unsigned char *params);
static char SCSI_ReadFormatCapacity(unsigned char lun, unsigned char *params);
static char SCSI_ReadCapacity10(unsigned char lun, unsigned char *params);
static char SCSI_RequestSense (unsigned char lun, unsigned char *params);
static char SCSI_StartStopUnit(unsigned char lun, unsigned char *params);
static char SCSI_ModeSense6 (unsigned char lun, unsigned char *params);
static char SCSI_ModeSense10 (unsigned char lun, unsigned char *params);
static char SCSI_Write10(unsigned char lun , unsigned char *params);
static char SCSI_Read10(unsigned char lun , unsigned char *params);
static char SCSI_Verify10(unsigned char lun, unsigned char *params);
static char SCSI_CheckAddressRange (unsigned char lun , unsigned int blk_offset , unsigned short blk_nbr);
static char SCSI_ProcessRead (unsigned char lun);
static char SCSI_ProcessWrite (unsigned char lun);

/**
* @brief  SCSI_ProcessCmd
*         Process SCSI commands
* @param  pdev: device instance
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
char SCSI_ProcessCmd(USB_OTG_CORE_HANDLE  *pdev, unsigned char lun, unsigned char *params) {
  cdev = pdev;

  switch (params[0]) {
  case SCSI_TEST_UNIT_READY:
    return SCSI_TestUnitReady(lun, params);

  case SCSI_REQUEST_SENSE:
    return SCSI_RequestSense (lun, params);
  case SCSI_INQUIRY:
    return SCSI_Inquiry(lun, params);

  case SCSI_START_STOP_UNIT:
    return SCSI_StartStopUnit(lun, params);

  case SCSI_ALLOW_MEDIUM_REMOVAL:
    return SCSI_StartStopUnit(lun, params);

  case SCSI_MODE_SENSE6:
    return SCSI_ModeSense6 (lun, params);

  case SCSI_MODE_SENSE10:
    return SCSI_ModeSense10 (lun, params);

  case SCSI_READ_FORMAT_CAPACITIES:
    return SCSI_ReadFormatCapacity(lun, params);

  case SCSI_READ_CAPACITY10:
    return SCSI_ReadCapacity10(lun, params);

  case SCSI_READ10:
    return SCSI_Read10(lun, params);

  case SCSI_WRITE10:
    return SCSI_Write10(lun, params);

  case SCSI_VERIFY10:
    return SCSI_Verify10(lun, params);

  default:
    SCSI_SenseCode(lun, ILLEGAL_REQUEST, INVALID_CDB);
    return -1;
  }
}


/**
* @brief  SCSI_TestUnitReady
*         Process SCSI Test Unit Ready Command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_TestUnitReady(unsigned char lun, unsigned char *params) {
  /* case 9 : Hi > D0 */
  if (MSC_BOT_cbw.dDataLength != 0) {
    SCSI_SenseCode(MSC_BOT_cbw.bLUN, ILLEGAL_REQUEST, INVALID_CDB);
    return -1;
  }

  if(USBD_STORAGE_fops->IsReady() !=0 ) {
    SCSI_SenseCode(lun, NOT_READY, MEDIUM_NOT_PRESENT);
    return -1;
  }
  MSC_BOT_DataLen = 0;
  return 0;
}

/**
* @brief  SCSI_Inquiry
*         Process Inquiry command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char  SCSI_Inquiry(unsigned char lun, unsigned char *params) {
  unsigned char* pPage;
  unsigned short len;

  if (params[1] & 0x01)/*Evpd is set*/ {
    pPage = (unsigned char *)MSC_Page00_Inquiry_Data;
    len = LENGTH_INQUIRY_PAGE00;
  } else {
    pPage = (unsigned char *)&USBD_STORAGE_fops->pInquiry[lun * USBD_STD_INQUIRY_LENGTH];
    len = pPage[4] + 5;

    if (params[4] <= len) {
      len = params[4];
    }
  }
  MSC_BOT_DataLen = len;

  while (len) {
    len--;
    MSC_BOT_Data[len] = pPage[len];
  }
  return 0;
}

/**
* @brief  SCSI_ReadCapacity10
*         Process Read Capacity 10 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_ReadCapacity10(unsigned char lun, unsigned char *params) {

  if(USBD_STORAGE_fops->GetCapacity(&SCSI_blk_nbr, &SCSI_blk_size) != 0) {
    SCSI_SenseCode(lun, NOT_READY, MEDIUM_NOT_PRESENT);
    return -1;
  } else {
    MSC_BOT_Data[0] = (unsigned char)((SCSI_blk_nbr - 1) >> 24);
    MSC_BOT_Data[1] = (unsigned char)((SCSI_blk_nbr - 1) >> 16);
    MSC_BOT_Data[2] = (unsigned char)((SCSI_blk_nbr - 1) >>  8);
    MSC_BOT_Data[3] = (unsigned char)((SCSI_blk_nbr - 1));

    MSC_BOT_Data[4] = (unsigned char)(SCSI_blk_size >>  24);
    MSC_BOT_Data[5] = (unsigned char)(SCSI_blk_size >>  16);
    MSC_BOT_Data[6] = (unsigned char)(SCSI_blk_size >>  8);
    MSC_BOT_Data[7] = (unsigned char)(SCSI_blk_size);

    MSC_BOT_DataLen = 8;
    return 0;
  }
}
/**
* @brief  SCSI_ReadFormatCapacity
*         Process Read Format Capacity command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_ReadFormatCapacity(unsigned char lun, unsigned char *params) {
 unsigned int blk_size;
  unsigned int blk_nbr;
  unsigned short i;

  for(i=0 ; i < 12 ; i++) {
    MSC_BOT_Data[i] = 0;
  }

  if(USBD_STORAGE_fops->GetCapacity(&blk_nbr, &blk_size) != 0) {
    SCSI_SenseCode(lun, NOT_READY, MEDIUM_NOT_PRESENT);
    return -1;
  } else {
    MSC_BOT_Data[3] = 0x08;
    MSC_BOT_Data[4] = (unsigned char)((blk_nbr - 1) >> 24);
    MSC_BOT_Data[5] = (unsigned char)((blk_nbr - 1) >> 16);
    MSC_BOT_Data[6] = (unsigned char)((blk_nbr - 1) >>  8);
    MSC_BOT_Data[7] = (unsigned char)(blk_nbr - 1);

    MSC_BOT_Data[8] = 0x02;
    MSC_BOT_Data[9] = (unsigned char)(blk_size >>  16);
    MSC_BOT_Data[10] = (unsigned char)(blk_size >>  8);
    MSC_BOT_Data[11] = (unsigned char)(blk_size);

    MSC_BOT_DataLen = 12;
    return 0;
  }
}
/**
* @brief  SCSI_ModeSense6
*         Process Mode Sense6 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_ModeSense6 (unsigned char lun, unsigned char *params) {
  unsigned short len = 8 ;
  MSC_BOT_DataLen = len;

  while (len) {
    len--;
    MSC_BOT_Data[len] = MSC_Mode_Sense6_data[len];
  }
  return 0;
}

/**
* @brief  SCSI_ModeSense10
*         Process Mode Sense10 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_ModeSense10 (unsigned char lun, unsigned char *params) {
 unsigned short len = 8;
 MSC_BOT_DataLen = len;

 while (len) {
    len--;
    MSC_BOT_Data[len] = MSC_Mode_Sense10_data[len];
  }
  return 0;
}

/**
* @brief  SCSI_RequestSense
*         Process Request Sense command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/

static char SCSI_RequestSense (unsigned char lun, unsigned char *params) {
  unsigned char i;

  for(i=0 ; i < REQUEST_SENSE_DATA_LEN ; i++) {
    MSC_BOT_Data[i] = 0;
  }

  MSC_BOT_Data[0]	= 0x70;
  MSC_BOT_Data[7]	= REQUEST_SENSE_DATA_LEN - 6;

  if((SCSI_Sense_Head != SCSI_Sense_Tail)) {

    MSC_BOT_Data[2]     = SCSI_Sense[SCSI_Sense_Head].Skey;
    MSC_BOT_Data[12]    = SCSI_Sense[SCSI_Sense_Head].w.b.ASCQ;
    MSC_BOT_Data[13]    = SCSI_Sense[SCSI_Sense_Head].w.b.ASC;
    SCSI_Sense_Head++;

    if (SCSI_Sense_Head == SENSE_LIST_DEEPTH) {
      SCSI_Sense_Head = 0;
    }
  }
  MSC_BOT_DataLen = REQUEST_SENSE_DATA_LEN;

  if (params[4] <= REQUEST_SENSE_DATA_LEN) {
    MSC_BOT_DataLen = params[4];
  }
  return 0;
}

/**
* @brief  SCSI_SenseCode
*         Load the last error code in the error list
* @param  lun: Logical unit number
* @param  sKey: Sense Key
* @param  ASC: Additional Sense Key
* @retval none

*/
void SCSI_SenseCode(unsigned char lun, unsigned char sKey, unsigned char ASC) {
  SCSI_Sense[SCSI_Sense_Tail].Skey  = sKey;
  SCSI_Sense[SCSI_Sense_Tail].w.ASC = ASC << 8;
  SCSI_Sense_Tail++;
  if (SCSI_Sense_Tail == SENSE_LIST_DEEPTH) {
    SCSI_Sense_Tail = 0;
  }
}
/**
* @brief  SCSI_StartStopUnit
*         Process Start Stop Unit command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_StartStopUnit(unsigned char lun, unsigned char *params) {
  MSC_BOT_DataLen = 0;
  return 0;
}

/**
* @brief  SCSI_Read10
*         Process Read10 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/
static char SCSI_Read10(unsigned char lun , unsigned char *params) {
  if(MSC_BOT_State == BOT_IDLE)  /* Idle */ {
    /* case 10 : Ho <> Di */
    if ((MSC_BOT_cbw.bmFlags & 0x80) != 0x80) {
      SCSI_SenseCode(MSC_BOT_cbw.bLUN, ILLEGAL_REQUEST, INVALID_CDB);
      return -1;
    }

    if(USBD_STORAGE_fops->IsReady() !=0 ) {
      SCSI_SenseCode(lun, NOT_READY, MEDIUM_NOT_PRESENT);
      return -1;
    }

    SCSI_blk_addr = (params[2] << 24) | (params[3] << 16) | (params[4] <<  8) | params[5];
    SCSI_blk_len =  (params[7] <<  8) | params[8];

    if( SCSI_CheckAddressRange(lun, SCSI_blk_addr, SCSI_blk_len) < 0) {
      return -1; /* error */
    }

    MSC_BOT_State = BOT_DATA_IN;
    SCSI_blk_len  *= SCSI_blk_size;

    /* cases 4,5 : Hi <> Dn */
    if (MSC_BOT_cbw.dDataLength != SCSI_blk_len) {
      SCSI_SenseCode(MSC_BOT_cbw.bLUN, ILLEGAL_REQUEST, INVALID_CDB);
      return -1;
    }
  }
  MSC_BOT_DataLen = MSC_MEDIA_PACKET;

  return SCSI_ProcessRead(lun);
}

/**
* @brief  SCSI_Write10
*         Process Write10 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/

static char SCSI_Write10 (unsigned char lun , unsigned char *params) {
  if (MSC_BOT_State == BOT_IDLE) /* Idle */ {
    /* case 8 : Hi <> Do */
    if ((MSC_BOT_cbw.bmFlags & 0x80) == 0x80) {
      SCSI_SenseCode(MSC_BOT_cbw.bLUN, ILLEGAL_REQUEST, INVALID_CDB);
      return -1;
    }

    /* Check whether Media is ready */
    if(USBD_STORAGE_fops->IsReady() !=0 ) {
      SCSI_SenseCode(lun, NOT_READY, MEDIUM_NOT_PRESENT);
      return -1;
    }

    /* Check If media is write-protected */
    if(USBD_STORAGE_fops->IsWriteProtected() !=0 ) {
      SCSI_SenseCode(lun, NOT_READY, WRITE_PROTECTED);
      return -1;
    }


    SCSI_blk_addr = (params[2] << 24) | (params[3] << 16) | (params[4] <<  8) | params[5];
    SCSI_blk_len = (params[7] <<  8) | params[8];

    /* check if LBA address is in the right range */
    if(SCSI_CheckAddressRange(lun, SCSI_blk_addr, SCSI_blk_len) < 0) {
      return -1; /* error */
    }

    SCSI_blk_len  *= SCSI_blk_size;

    /* cases 3,11,13 : Hn,Ho <> D0 */
    if (MSC_BOT_cbw.dDataLength != SCSI_blk_len) {
      SCSI_SenseCode(MSC_BOT_cbw.bLUN, ILLEGAL_REQUEST, INVALID_CDB);
      return -1;
    }

    /* Prepare EP to receive first data packet */
    MSC_BOT_State = BOT_DATA_OUT;
    DCD_EP_PrepareRx (cdev, MSC_OUT_EP, MSC_BOT_Data, MIN (SCSI_blk_len, MSC_MEDIA_PACKET));
  } else /* Write Process ongoing */ {
    return SCSI_ProcessWrite(lun);
  }
  return 0;
}


/**
* @brief  SCSI_Verify10
*         Process Verify10 command
* @param  lun: Logical unit number
* @param  params: Command parameters
* @retval status
*/

static char SCSI_Verify10(unsigned char lun , unsigned char *params){
  if ((params[1]& 0x02) == 0x02) {
    SCSI_SenseCode (lun, ILLEGAL_REQUEST, INVALID_FIELED_IN_COMMAND);
    return -1; /* Error, Verify Mode Not supported*/
  }

  if(SCSI_CheckAddressRange(lun, SCSI_blk_addr, SCSI_blk_len) < 0) {
    return -1; /* error */
  }
  MSC_BOT_DataLen = 0;
  return 0;
}

/**
* @brief  SCSI_CheckAddressRange
*         Check address range
* @param  lun: Logical unit number
* @param  blk_offset: first block address
* @param  blk_nbr: number of block to be processed
* @retval status
*/
static char SCSI_CheckAddressRange (unsigned char lun , unsigned int blk_offset , unsigned short blk_nbr) {
  if ((blk_offset + blk_nbr) > SCSI_blk_nbr ) {
    SCSI_SenseCode(lun, ILLEGAL_REQUEST, ADDRESS_OUT_OF_RANGE);
    return -1;
  }
  return 0;
}

/**
* @brief  SCSI_ProcessRead
*         Handle Read Process
* @param  lun: Logical unit number
* @retval status
*/
static char SCSI_ProcessRead (unsigned char lun) {
  unsigned int len;

  len = MIN(SCSI_blk_len , MSC_MEDIA_PACKET);

  if( USBD_STORAGE_fops->Read(MSC_BOT_Data, SCSI_blk_addr, len / SCSI_blk_size) < 0) {
    SCSI_SenseCode(lun, HARDWARE_ERROR, UNRECOVERED_READ_ERROR);
    return -1;
  }

  DCD_EP_Tx (cdev, MSC_IN_EP, MSC_BOT_Data, len);

  SCSI_blk_addr   += len / SCSI_blk_size;
  SCSI_blk_len    -= len;

  /* case 6 : Hi = Di */
  MSC_BOT_csw.dDataResidue -= len;

  if (SCSI_blk_len == 0) {
    MSC_BOT_State = BOT_LAST_DATA_IN;
  }
  return 0;
}

/**
* @brief  SCSI_ProcessWrite
*         Handle Write Process
* @param  lun: Logical unit number
* @retval status
*/

static char SCSI_ProcessWrite (unsigned char lun) {
  unsigned int len;

  len = MIN(SCSI_blk_len , MSC_MEDIA_PACKET);
  if(USBD_STORAGE_fops->Write(MSC_BOT_Data, SCSI_blk_addr, len / SCSI_blk_size) < 0) {
    SCSI_SenseCode(lun, HARDWARE_ERROR, WRITE_FAULT);
    return -1;
  }

  SCSI_blk_addr  += len / SCSI_blk_size;
  SCSI_blk_len   -= len;

  /* case 12 : Ho = Do */
  MSC_BOT_csw.dDataResidue -= len;

  if (SCSI_blk_len == 0) {
    MSC_BOT_SendCSW (cdev, CSW_CMD_PASSED);
  } else {
    /* Prapare EP to Receive next packet */
    DCD_EP_PrepareRx (cdev, MSC_OUT_EP, MSC_BOT_Data, MIN (SCSI_blk_len, MSC_MEDIA_PACKET));
  }

  return 0;
}
