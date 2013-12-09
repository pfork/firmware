/**
  ******************************************************************************
  * @file    usbd_msc_bot.h
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   header for the usbd_msc_bot.c file
  ******************************************************************************
  */

#include "usbd_core.h"

#ifndef usbd_msc_bot_h
#define usbd_msc_bot_h

#define BOT_IDLE                      0       /* Idle state */
#define BOT_DATA_OUT                  1       /* Data Out state */
#define BOT_DATA_IN                   2       /* Data In state */
#define BOT_LAST_DATA_IN              3       /* Last Data In Last */
#define BOT_SEND_DATA                 4       /* Send Immediate data */

#define BOT_CBW_SIGNATURE             0x43425355
#define BOT_CSW_SIGNATURE             0x53425355
#define BOT_CBW_LENGTH                31
#define BOT_CSW_LENGTH                13

/* CSW Status Definitions */
#define CSW_CMD_PASSED                0x00
#define CSW_CMD_FAILED                0x01
#define CSW_PHASE_ERROR               0x02

/* BOT Status */
#define BOT_STATE_NORMAL              0
#define BOT_STATE_RECOVERY            1
#define BOT_STATE_ERROR               2

#define DIR_IN                        0
#define DIR_OUT                       1
#define BOTH_DIR                      2

typedef struct _MSC_BOT_CBW {
  unsigned int dSignature;
  unsigned int dTag;
  unsigned int dDataLength;
  unsigned char  bmFlags;
  unsigned char  bLUN;
  unsigned char  bCBLength;
  unsigned char  CB[16];
} MSC_BOT_CBW_TypeDef;

typedef struct _MSC_BOT_CSW {
  unsigned int dSignature;
  unsigned int dTag;
  unsigned int dDataResidue;
  unsigned char  bStatus;
} MSC_BOT_CSW_TypeDef;

extern unsigned char  MSC_BOT_Data[];
extern unsigned short MSC_BOT_DataLen;
extern unsigned char  MSC_BOT_State;
extern unsigned char  MSC_BOT_BurstMode;
extern MSC_BOT_CBW_TypeDef  MSC_BOT_cbw;
extern MSC_BOT_CSW_TypeDef  MSC_BOT_csw;

void MSC_BOT_Init (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_Reset (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_DeInit (USB_OTG_CORE_HANDLE  *pdev);
void MSC_BOT_DataIn (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum);
void MSC_BOT_DataOut (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum);
void MSC_BOT_SendCSW (USB_OTG_CORE_HANDLE  *pdev, unsigned char CSW_Status);
void  MSC_BOT_CplClrFeature (USB_OTG_CORE_HANDLE  *pdev, unsigned char epnum);

#endif /* usbd_msc_bot_h */
