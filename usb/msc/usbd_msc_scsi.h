/**
  ******************************************************************************
  * @file    usbd_msc_scsi.h
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   header for the usbd_msc_scsi.c file
  ******************************************************************************
  */

#ifndef usbd_msc_scsi_h
#define usbd_msc_scsi_h

#include "usbd_def.h"

#define SENSE_LIST_DEEPTH                          4

/* SCSI Commands */
#define SCSI_FORMAT_UNIT                            0x04
#define SCSI_INQUIRY                                0x12
#define SCSI_MODE_SELECT6                           0x15
#define SCSI_MODE_SELECT10                          0x55
#define SCSI_MODE_SENSE6                            0x1A
#define SCSI_MODE_SENSE10                           0x5A
#define SCSI_ALLOW_MEDIUM_REMOVAL                   0x1E
#define SCSI_READ6                                  0x08
#define SCSI_READ10                                 0x28
#define SCSI_READ12                                 0xA8
#define SCSI_READ16                                 0x88

#define SCSI_READ_CAPACITY10                        0x25
#define SCSI_READ_CAPACITY16                        0x9E

#define SCSI_REQUEST_SENSE                          0x03
#define SCSI_START_STOP_UNIT                        0x1B
#define SCSI_TEST_UNIT_READY                        0x00
#define SCSI_WRITE6                                 0x0A
#define SCSI_WRITE10                                0x2A
#define SCSI_WRITE12                                0xAA
#define SCSI_WRITE16                                0x8A

#define SCSI_VERIFY10                               0x2F
#define SCSI_VERIFY12                               0xAF
#define SCSI_VERIFY16                               0x8F

#define SCSI_SEND_DIAGNOSTIC                        0x1D
#define SCSI_READ_FORMAT_CAPACITIES                 0x23

#define NO_SENSE                                    0
#define RECOVERED_ERROR                             1
#define NOT_READY                                   2
#define MEDIUM_ERROR                                3
#define HARDWARE_ERROR                              4
#define ILLEGAL_REQUEST                             5
#define UNIT_ATTENTION                              6
#define DATA_PROTECT                                7
#define BLANK_CHECK                                 8
#define VENDOR_SPECIFIC                             9
#define COPY_ABORTED                               10
#define ABORTED_COMMAND                            11
#define VOLUME_OVERFLOW                            13
#define MISCOMPARE                                 14


#define INVALID_CDB                                 0x20
#define INVALID_FIELED_IN_COMMAND                   0x24
#define PARAMETER_LIST_LENGTH_ERROR                 0x1A
#define INVALID_FIELD_IN_PARAMETER_LIST             0x26
#define ADDRESS_OUT_OF_RANGE                        0x21
#define MEDIUM_NOT_PRESENT                          0x3A
#define MEDIUM_HAVE_CHANGED                         0x28
#define WRITE_PROTECTED                             0x27
#define UNRECOVERED_READ_ERROR			    0x11
#define WRITE_FAULT				    0x03

#define READ_FORMAT_CAPACITY_DATA_LEN               0x0C
#define READ_CAPACITY10_DATA_LEN                    0x08
#define MODE_SENSE10_DATA_LEN                       0x08
#define MODE_SENSE6_DATA_LEN                        0x04
#define REQUEST_SENSE_DATA_LEN                      0x12
#define STANDARD_INQUIRY_DATA_LEN                   0x24
#define BLKVFY                                      0x04

extern  unsigned char Page00_Inquiry_Data[];
extern  unsigned char Standard_Inquiry_Data[];
extern  unsigned char Standard_Inquiry_Data2[];
extern  unsigned char Mode_Sense6_data[];
extern  unsigned char Mode_Sense10_data[];
extern  unsigned char Scsi_Sense_Data[];
extern  unsigned char ReadCapacity10_Data[];
extern  unsigned char ReadFormatCapacity_Data [];

typedef struct _SENSE_ITEM {
  char Skey;
  union {
    struct _ASCs {
      char ASC;
      char ASCQ;
    } b;
    unsigned int	ASC;
    char *pData;
  } w;
} SCSI_Sense_TypeDef;

extern SCSI_Sense_TypeDef SCSI_Sense [SENSE_LIST_DEEPTH];
extern unsigned char   SCSI_Sense_Head;
extern unsigned char   SCSI_Sense_Tail;

char SCSI_ProcessCmd(USB_OTG_CORE_HANDLE  *pdev, unsigned char lun, unsigned char *cmd);
void SCSI_SenseCode(unsigned char lun, unsigned char sKey, unsigned char ASC);

#endif /* usbd_msc_scsi_h */
