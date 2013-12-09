/**
  ******************************************************************************
  * @file    usbd_msc_bot.c
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   This file provides all the BOT protocol core functions.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_msc_bot.h"
#include "usbd_msc_scsi.h"
#include "usbd_ioreq.h"
#include "usbd_msc_mem.h"
#include "usbd_conf.h"
#include "stm32f.h"

unsigned short MSC_BOT_DataLen;
unsigned char  MSC_BOT_State;
unsigned char  MSC_BOT_Status;

__ALIGN_BEGIN unsigned char MSC_BOT_Data[MSC_MEDIA_PACKET] __ALIGN_END ;
__ALIGN_BEGIN MSC_BOT_CBW_TypeDef  MSC_BOT_cbw __ALIGN_END;
__ALIGN_BEGIN MSC_BOT_CSW_TypeDef  MSC_BOT_csw __ALIGN_END;

static void MSC_BOT_CBW_Decode (USB_OTG_CORE_HANDLE  *pdev);
static void MSC_BOT_SendData (USB_OTG_CORE_HANDLE  *pdev, unsigned char* pbuf, unsigned short len);
static void MSC_BOT_Abort(USB_OTG_CORE_HANDLE  *pdev);

/**
* @brief  MSC_BOT_Init
*         Initialize the BOT Process
* @param  pdev: device instance
* @retval None
*/
void MSC_BOT_Init (USB_OTG_CORE_HANDLE  *pdev) {
  MSC_BOT_State = BOT_IDLE;
  MSC_BOT_Status = BOT_STATE_NORMAL;
  USBD_STORAGE_fops->Init();

  DCD_EP_Flush(pdev, MSC_OUT_EP);
  DCD_EP_Flush(pdev, MSC_IN_EP);
  /* Prapare EP to Receive First BOT Cmd */
  DCD_EP_PrepareRx (pdev, MSC_OUT_EP, (unsigned char *)&MSC_BOT_cbw, BOT_CBW_LENGTH);
}

/**
* @brief  MSC_BOT_Reset
*         Reset the BOT Machine
* @param  pdev: device instance
* @retval  None
*/
void MSC_BOT_Reset (USB_OTG_CORE_HANDLE  *pdev) {
  MSC_BOT_State = BOT_IDLE;
  MSC_BOT_Status = BOT_STATE_RECOVERY;
  /* Prapare EP to Receive First BOT Cmd */
  DCD_EP_PrepareRx(pdev, MSC_OUT_EP, (unsigned char *)&MSC_BOT_cbw, BOT_CBW_LENGTH);
}

/**
* @brief  MSC_BOT_DeInit
*         Uninitialize the BOT Machine
* @param  pdev: device instance
* @retval None
*/
void MSC_BOT_DeInit (USB_OTG_CORE_HANDLE  *pdev) {
  MSC_BOT_State = BOT_IDLE;
}

/**
* @brief  MSC_BOT_DataIn
*         Handle BOT IN data stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval None
*/
void MSC_BOT_DataIn (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum) {
  switch (MSC_BOT_State) {
  case BOT_DATA_IN:
    if(SCSI_ProcessCmd(pdev, MSC_BOT_cbw.bLUN, &MSC_BOT_cbw.CB[0]) < 0) {
      MSC_BOT_SendCSW (pdev, CSW_CMD_FAILED);
    }
    break;
  case BOT_SEND_DATA:
  case BOT_LAST_DATA_IN:
    MSC_BOT_SendCSW (pdev, CSW_CMD_PASSED); break;
  default:
    break;
  }
}
/**
* @brief  MSC_BOT_DataOut
*         Proccess MSC OUT data
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval None
*/
void MSC_BOT_DataOut (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum) {
  switch (MSC_BOT_State) {
  case BOT_IDLE:
    MSC_BOT_CBW_Decode(pdev);
    break;

  case BOT_DATA_OUT:
    if(SCSI_ProcessCmd(pdev, MSC_BOT_cbw.bLUN, &MSC_BOT_cbw.CB[0]) < 0) {
      MSC_BOT_SendCSW (pdev, CSW_CMD_FAILED);
    }
    break;

  default:
    break;
  }
}

/**
* @brief  MSC_BOT_CBW_Decode
*         Decode the CBW command and set the BOT state machine accordingtly
* @param  pdev: device instance
* @retval None
*/
static void  MSC_BOT_CBW_Decode (USB_OTG_CORE_HANDLE  *pdev) {
  MSC_BOT_csw.dTag = MSC_BOT_cbw.dTag;
  MSC_BOT_csw.dDataResidue = MSC_BOT_cbw.dDataLength;

  if ((USBD_GetRxCount (pdev ,MSC_OUT_EP) != BOT_CBW_LENGTH) ||
      (MSC_BOT_cbw.dSignature != BOT_CBW_SIGNATURE)||
      (MSC_BOT_cbw.bLUN > 1) ||
      (MSC_BOT_cbw.bCBLength < 1) ||
      (MSC_BOT_cbw.bCBLength > 16)) {
    SCSI_SenseCode(MSC_BOT_cbw.bLUN,
                   ILLEGAL_REQUEST,
                   INVALID_CDB);
     MSC_BOT_Status = BOT_STATE_ERROR;
    MSC_BOT_Abort(pdev);
  } else {
    if(SCSI_ProcessCmd(pdev, MSC_BOT_cbw.bLUN, &MSC_BOT_cbw.CB[0]) < 0) {
      MSC_BOT_Abort(pdev);
    }
    /*Burst xfer handled internally*/
    else if ((MSC_BOT_State != BOT_DATA_IN) &&
             (MSC_BOT_State != BOT_DATA_OUT) &&
             (MSC_BOT_State != BOT_LAST_DATA_IN)) {
      if (MSC_BOT_DataLen > 0) {
        MSC_BOT_SendData(pdev, MSC_BOT_Data, MSC_BOT_DataLen);
      } else if (MSC_BOT_DataLen == 0) {
        MSC_BOT_SendCSW (pdev, CSW_CMD_PASSED);
      }
    }
  }
}

/**
* @brief  MSC_BOT_SendData
*         Send the requested data
* @param  pdev: device instance
* @param  buf: pointer to data buffer
* @param  len: Data Length
* @retval None
*/
static void  MSC_BOT_SendData(USB_OTG_CORE_HANDLE  *pdev, unsigned char* buf, unsigned short len) {
  len = MIN (MSC_BOT_cbw.dDataLength, len);
  MSC_BOT_csw.dDataResidue -= len;
  MSC_BOT_csw.bStatus = CSW_CMD_PASSED;
  MSC_BOT_State = BOT_SEND_DATA;

  DCD_EP_Tx (pdev, MSC_IN_EP, buf, len);
}

/**
* @brief  MSC_BOT_SendCSW
*         Send the Command Status Wrapper
* @param  pdev: device instance
* @param  status : CSW status
* @retval None
*/
void  MSC_BOT_SendCSW (USB_OTG_CORE_HANDLE  *pdev, unsigned char CSW_Status) {
  MSC_BOT_csw.dSignature = BOT_CSW_SIGNATURE;
  MSC_BOT_csw.bStatus = CSW_Status;
  MSC_BOT_State = BOT_IDLE;

  DCD_EP_Tx (pdev, MSC_IN_EP, (unsigned char *)&MSC_BOT_csw, BOT_CSW_LENGTH);
  /* Prapare EP to Receive next Cmd */
  DCD_EP_PrepareRx (pdev, MSC_OUT_EP, (unsigned char *)&MSC_BOT_cbw, BOT_CBW_LENGTH);
}

/**
* @brief  MSC_BOT_Abort
*         Abort the current transfer
* @param  pdev: device instance
* @retval status
*/

static void  MSC_BOT_Abort (USB_OTG_CORE_HANDLE  *pdev) {
  if ((MSC_BOT_cbw.bmFlags == 0) &&
      (MSC_BOT_cbw.dDataLength != 0) &&
      (MSC_BOT_Status == BOT_STATE_NORMAL) ) {
    DCD_EP_Stall(pdev, MSC_OUT_EP );
  }
  DCD_EP_Stall(pdev, MSC_IN_EP);

  if(MSC_BOT_Status == BOT_STATE_ERROR) {
    DCD_EP_PrepareRx (pdev, MSC_OUT_EP, (unsigned char *)&MSC_BOT_cbw, BOT_CBW_LENGTH);
  }
}

/**
* @brief  MSC_BOT_CplClrFeature
*         Complete the clear feature request
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval None
*/

void  MSC_BOT_CplClrFeature (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum) {
  if(MSC_BOT_Status == BOT_STATE_ERROR )/* Bad CBW Signature */ {
    DCD_EP_Stall(pdev, MSC_IN_EP);
    MSC_BOT_Status = BOT_STATE_NORMAL;
  }
  else if(((epnum & 0x80) == 0x80) && ( MSC_BOT_Status != BOT_STATE_RECOVERY)) {
    MSC_BOT_SendCSW (pdev, CSW_CMD_FAILED);
  }
}
