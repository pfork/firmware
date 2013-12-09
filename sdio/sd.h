#ifndef sd_h
#define sd_h

/**
  ******************************************************************************
  * @file    sd.c
  * @author  stf
  * @version V0.0.1
  * @date    02-December-2013
  * @brief   This file contains all the functions prototypes for the SD Card
  *          stm322xg_eval_sdio_sd driver firmware library.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f.h"

typedef enum {
/**
  * @brief  SDIO specific error defines
  */
  SD_CMD_CRC_FAIL                    = (1), /*!< Command response received (but CRC check failed) */
  SD_DATA_CRC_FAIL                   = (2), /*!< Data bock sent/received (CRC check Failed) */
  SD_CMD_RSP_TIMEOUT                 = (3), /*!< Command response timeout */
  SD_DATA_TIMEOUT                    = (4), /*!< Data time out */
  SD_TX_UNDERRUN                     = (5), /*!< Transmit FIFO under-run */
  SD_RX_OVERRUN                      = (6), /*!< Receive FIFO over-run */
  SD_START_BIT_ERR                   = (7), /*!< Start bit not detected on all data signals in widE bus mode */
  SD_CMD_OUT_OF_RANGE                = (8), /*!< CMD's argument was out of range.*/
  SD_ADDR_MISALIGNED                 = (9), /*!< Misaligned address */
  SD_BLOCK_LEN_ERR                   = (10), /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
  SD_ERASE_SEQ_ERR                   = (11), /*!< An error in the sequence of erase command occurs.*/
  SD_BAD_ERASE_PARAM                 = (12), /*!< An Invalid selection for erase groups */
  SD_WRITE_PROT_VIOLATION            = (13), /*!< Attempt to program a write protect block */
  SD_LOCK_UNLOCK_FAILED              = (14), /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
  SD_COM_CRC_FAILED                  = (15), /*!< CRC check of the previous command failed */
  SD_ILLEGAL_CMD                     = (16), /*!< Command is not legal for the card state */
  SD_CARD_ECC_FAILED                 = (17), /*!< Card internal ECC was applied but failed to correct the data */
  SD_CC_ERROR                        = (18), /*!< Internal card controller error */
  SD_GENERAL_UNKNOWN_ERROR           = (19), /*!< General or Unknown error */
  SD_STREAM_READ_UNDERRUN            = (20), /*!< The card could not sustain data transfer in stream read operation. */
  SD_STREAM_WRITE_OVERRUN            = (21), /*!< The card could not sustain data programming in stream mode */
  SD_CID_CSD_OVERWRITE               = (22), /*!< CID/CSD overwrite error */
  SD_WP_ERASE_SKIP                   = (23), /*!< only partial address space was erased */
  SD_CARD_ECC_DISABLED               = (24), /*!< Command has been executed without using internal ECC */
  SD_ERASE_RESET                     = (25), /*!< Erase sequence was cleared before executing because an out of erase sequence command was received */
  SD_AKE_SEQ_ERROR                   = (26), /*!< Error in sequence of authentication. */
  SD_INVALID_VOLTRANGE               = (27),
  SD_ADDR_OUT_OF_RANGE               = (28),
  SD_SWITCH_ERROR                    = (29),
  SD_SDIO_DISABLED                   = (30),
  SD_SDIO_FUNCTION_BUSY              = (31),
  SD_SDIO_FUNCTION_FAILED            = (32),
  SD_SDIO_UNKNOWN_FUNCTION           = (33),

/**
  * @brief  Standard error defines
  */
  SD_INTERNAL_ERROR,
  SD_NOT_CONFIGURED,
  SD_REQUEST_PENDING,
  SD_REQUEST_NOT_APPLICABLE,
  SD_INVALID_PARAMETER,
  SD_UNSUPPORTED_FEATURE,
  SD_UNSUPPORTED_HW,
  SD_ERROR,
  SD_OK = 0
} SD_Error;

/**
  * @brief  SDIO Transfer state
  */
typedef enum {
  SD_TRANSFER_OK  = 0,
  SD_TRANSFER_BUSY = 1,
  SD_TRANSFER_ERROR
} SDTransferState;

/**
  * @brief  SD Card States
  */
typedef enum {
  SD_CARD_READY                  = ((unsigned int)0x00000001),
  SD_CARD_IDENTIFICATION         = ((unsigned int)0x00000002),
  SD_CARD_STANDBY                = ((unsigned int)0x00000003),
  SD_CARD_TRANSFER               = ((unsigned int)0x00000004),
  SD_CARD_SENDING                = ((unsigned int)0x00000005),
  SD_CARD_RECEIVING              = ((unsigned int)0x00000006),
  SD_CARD_PROGRAMMING            = ((unsigned int)0x00000007),
  SD_CARD_DISCONNECTED           = ((unsigned int)0x00000008),
  SD_CARD_ERROR                  = ((unsigned int)0x000000FF)
} SDCardState;


/**
  * @brief  Card Specific Data: CSD Register
  */
typedef struct {
  volatile unsigned char  CSDStruct;            /*!< CSD structure */
  volatile unsigned char  SysSpecVersion;       /*!< System specification version */
  volatile unsigned char  Reserved1;            /*!< Reserved */
  volatile unsigned char  TAAC;                 /*!< Data read access-time 1 */
  volatile unsigned char  NSAC;                 /*!< Data read access-time 2 in CLK cycles */
  volatile unsigned char  MaxBusClkFrec;        /*!< Max. bus clock frequency */
  volatile unsigned short CardComdClasses;      /*!< Card command classes */
  volatile unsigned char  RdBlockLen;           /*!< Max. read data block length */
  volatile unsigned char  PartBlockRead;        /*!< Partial blocks for read allowed */
  volatile unsigned char  WrBlockMisalign;      /*!< Write block misalignment */
  volatile unsigned char  RdBlockMisalign;      /*!< Read block misalignment */
  volatile unsigned char  DSRImpl;              /*!< DSR implemented */
  volatile unsigned char  Reserved2;            /*!< Reserved */
  volatile unsigned int   DeviceSize;           /*!< Device Size */
  volatile unsigned char  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min */
  volatile unsigned char  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max */
  volatile unsigned char  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min */
  volatile unsigned char  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max */
  volatile unsigned char  DeviceSizeMul;        /*!< Device size multiplier */
  volatile unsigned char  EraseGrSize;          /*!< Erase group size */
  volatile unsigned char  EraseGrMul;           /*!< Erase group size multiplier */
  volatile unsigned char  WrProtectGrSize;      /*!< Write protect group size */
  volatile unsigned char  WrProtectGrEnable;    /*!< Write protect group enable */
  volatile unsigned char  ManDeflECC;           /*!< Manufacturer default ECC */
  volatile unsigned char  WrSpeedFact;          /*!< Write speed factor */
  volatile unsigned char  MaxWrBlockLen;        /*!< Max. write data block length */
  volatile unsigned char  WriteBlockPaPartial;  /*!< Partial blocks for write allowed */
  volatile unsigned char  Reserved3;            /*!< Reserved */
  volatile unsigned char  ContentProtectAppli;  /*!< Content protection application */
  volatile unsigned char  FileFormatGrouop;     /*!< File format group */
  volatile unsigned char  CopyFlag;             /*!< Copy flag (OTP) */
  volatile unsigned char  PermWrProtect;        /*!< Permanent write protection */
  volatile unsigned char  TempWrProtect;        /*!< Temporary write protection */
  volatile unsigned char  FileFormat;           /*!< File Format */
  volatile unsigned char  ECC;                  /*!< ECC code */
  volatile unsigned char  CSD_CRC;              /*!< CSD CRC */
  volatile unsigned char  Reserved4;            /*!< always 1*/
} SD_CSD;

/**
  * @brief  Card Identification Data: CID Register
  */
typedef struct {
  volatile unsigned char  ManufacturerID;       /*!< ManufacturerID */
  volatile unsigned short OEM_AppliID;          /*!< OEM/Application ID */
  volatile unsigned int ProdName1;            /*!< Product Name part1 */
  volatile unsigned char  ProdName2;            /*!< Product Name part2*/
  volatile unsigned char  ProdRev;              /*!< Product Revision */
  volatile unsigned int ProdSN;               /*!< Product Serial Number */
  volatile unsigned char  Reserved1;            /*!< Reserved1 */
  volatile unsigned short ManufactDate;         /*!< Manufacturing Date */
  volatile unsigned char  CID_CRC;              /*!< CID CRC */
  volatile unsigned char  Reserved2;            /*!< always 1 */
} SD_CID;

/**
  * @brief SD Card Status
  */
typedef struct {
  volatile unsigned char DAT_BUS_WIDTH;
  volatile unsigned char SECURED_MODE;
  volatile unsigned short SD_CARD_TYPE;
  volatile unsigned int SIZE_OF_PROTECTED_AREA;
  volatile unsigned char SPEED_CLASS;
  volatile unsigned char PERFORMANCE_MOVE;
  volatile unsigned char AU_SIZE;
  volatile unsigned short ERASE_SIZE;
  volatile unsigned char ERASE_TIMEOUT;
  volatile unsigned char ERASE_OFFSET;
} SD_CardStatus;


/**
  * @brief SD Card information
  */
typedef struct {
  SD_CSD SD_csd;
  SD_CID SD_cid;
  unsigned long long CardCapacity;  /*!< Card Capacity */
  unsigned int CardBlockSize; /*!< Card Block Size */
  unsigned short RCA;
  unsigned char CardType;
} SD_CardInfo;

/**
  * @brief SDIO Commands  Index
  */
#define SD_CMD_GO_IDLE_STATE                       ((unsigned char)0)
#define SD_CMD_SEND_OP_COND                        ((unsigned char)1)
#define SD_CMD_ALL_SEND_CID                        ((unsigned char)2)
#define SD_CMD_SET_REL_ADDR                        ((unsigned char)3) /*!< SDIO_SEND_REL_ADDR for SD Card */
#define SD_CMD_SET_DSR                             ((unsigned char)4)
#define SD_CMD_SDIO_SEN_OP_COND                    ((unsigned char)5)
#define SD_CMD_HS_SWITCH                           ((unsigned char)6)
#define SD_CMD_SEL_DESEL_CARD                      ((unsigned char)7)
#define SD_CMD_HS_SEND_EXT_CSD                     ((unsigned char)8)
#define SD_CMD_SEND_CSD                            ((unsigned char)9)
#define SD_CMD_SEND_CID                            ((unsigned char)10)
#define SD_CMD_READ_DAT_UNTIL_STOP                 ((unsigned char)11) /*!< SD Card doesn't support it */
#define SD_CMD_STOP_TRANSMISSION                   ((unsigned char)12)
#define SD_CMD_SEND_STATUS                         ((unsigned char)13)
#define SD_CMD_HS_BUSTEST_READ                     ((unsigned char)14)
#define SD_CMD_GO_INACTIVE_STATE                   ((unsigned char)15)
#define SD_CMD_SET_BLOCKLEN                        ((unsigned char)16)
#define SD_CMD_READ_SINGLE_BLOCK                   ((unsigned char)17)
#define SD_CMD_READ_MULT_BLOCK                     ((unsigned char)18)
#define SD_CMD_HS_BUSTEST_WRITE                    ((unsigned char)19)
#define SD_CMD_WRITE_DAT_UNTIL_STOP                ((unsigned char)20) /*!< SD Card doesn't support it */
#define SD_CMD_SET_BLOCK_COUNT                     ((unsigned char)23) /*!< SD Card doesn't support it */
#define SD_CMD_WRITE_SINGLE_BLOCK                  ((unsigned char)24)
#define SD_CMD_WRITE_MULT_BLOCK                    ((unsigned char)25)
#define SD_CMD_PROG_CID                            ((unsigned char)26) /*!< reserved for manufacturers */
#define SD_CMD_PROG_CSD                            ((unsigned char)27)
#define SD_CMD_SET_WRITE_PROT                      ((unsigned char)28)
#define SD_CMD_CLR_WRITE_PROT                      ((unsigned char)29)
#define SD_CMD_SEND_WRITE_PROT                     ((unsigned char)30)
#define SD_CMD_SD_ERASE_GRP_START                  ((unsigned char)32) /*!< To set the address of the first write
                                                                  block to be erased. (For SD card only) */
#define SD_CMD_SD_ERASE_GRP_END                    ((unsigned char)33) /*!< To set the address of the last write block of the
                                                                  continuous range to be erased. (For SD card only) */
#define SD_CMD_ERASE_GRP_START                     ((unsigned char)35) /*!< To set the address of the first write block to be erased.
                                                                  (For MMC card only spec 3.31) */

#define SD_CMD_ERASE_GRP_END                       ((unsigned char)36) /*!< To set the address of the last write block of the
                                                                  continuous range to be erased. (For MMC card only spec 3.31) */

#define SD_CMD_ERASE                               ((unsigned char)38)
#define SD_CMD_FAST_IO                             ((unsigned char)39) /*!< SD Card doesn't support it */
#define SD_CMD_GO_IRQ_STATE                        ((unsigned char)40) /*!< SD Card doesn't support it */
#define SD_CMD_LOCK_UNLOCK                         ((unsigned char)42)
#define SD_CMD_APP_CMD                             ((unsigned char)55)
#define SD_CMD_GEN_CMD                             ((unsigned char)56)
#define SD_CMD_NO_CMD                              ((unsigned char)64)

/**
  * @brief Following commands are SD Card Specific commands.
  *        SDIO_APP_CMD should be sent before sending these commands.
  */
#define SD_CMD_APP_SD_SET_BUSWIDTH                 ((unsigned char)6)  /*!< For SD Card only */
#define SD_CMD_SD_APP_STAUS                        ((unsigned char)13) /*!< For SD Card only */
#define SD_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((unsigned char)22) /*!< For SD Card only */
#define SD_CMD_SD_APP_OP_COND                      ((unsigned char)41) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CLR_CARD_DETECT          ((unsigned char)42) /*!< For SD Card only */
#define SD_CMD_SD_APP_SEND_SCR                     ((unsigned char)51) /*!< For SD Card only */
#define SD_CMD_SDIO_RW_DIRECT                      ((unsigned char)52) /*!< For SD I/O Card only */
#define SD_CMD_SDIO_RW_EXTENDED                    ((unsigned char)53) /*!< For SD I/O Card only */

/**
  * @brief Following commands are SD Card Specific security commands.
  *        SDIO_APP_CMD should be sent before sending these commands.
  */
#define SD_CMD_SD_APP_GET_MKB                      ((unsigned char)43) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_MID                      ((unsigned char)44) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RN1                  ((unsigned char)45) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RN2                  ((unsigned char)46) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RES2                 ((unsigned char)47) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RES1                 ((unsigned char)48) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((unsigned char)18) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((unsigned char)25) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_ERASE                 ((unsigned char)38) /*!< For SD Card only */
#define SD_CMD_SD_APP_CHANGE_SECURE_AREA           ((unsigned char)49) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MKB             ((unsigned char)48) /*!< For SD Card only */

/* Uncomment the following line to select the SDIO Data transfer mode */
#if !defined (SD_DMA_MODE) && !defined (SD_POLLING_MODE)
#define SD_DMA_MODE                                ((unsigned int)0x00000000)
/*#define SD_POLLING_MODE                            ((unsigned int)0x00000002)*/
#endif

/**
  * @brief  SD detection on its memory slot
  */
#define SD_PRESENT                                 ((unsigned char)0x01)
#define SD_NOT_PRESENT                             ((unsigned char)0x00)

/**
  * @brief Supported SD Memory Cards
  */
#define SDIO_STD_CAPACITY_SD_CARD_V1_1             ((unsigned int)0x00000000)
#define SDIO_STD_CAPACITY_SD_CARD_V2_0             ((unsigned int)0x00000001)
#define SDIO_HIGH_CAPACITY_SD_CARD                 ((unsigned int)0x00000002)
#define SDIO_MULTIMEDIA_CARD                       ((unsigned int)0x00000003)
#define SDIO_SECURE_DIGITAL_IO_CARD                ((unsigned int)0x00000004)
#define SDIO_HIGH_SPEED_MULTIMEDIA_CARD            ((unsigned int)0x00000005)
#define SDIO_SECURE_DIGITAL_IO_COMBO_CARD          ((unsigned int)0x00000006)
#define SDIO_HIGH_CAPACITY_MMC_CARD                ((unsigned int)0x00000007)

void SD_DeInit(void);
SD_Error SD_Init(void);
SDTransferState SD_GetStatus(void);
SDCardState SD_GetState(void);
unsigned char SD_Detect(void);
SD_Error SD_PowerON(void);
SD_Error SD_PowerOFF(void);
SD_Error SD_InitializeCards(void);
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo);
SD_Error SD_GetCardStatus(SD_CardStatus *cardstatus);
SD_Error SD_EnableWideBusOperation(unsigned int WideMode);
SD_Error SD_SelectDeselect(unsigned int addr);
SD_Error SD_ReadBlock(unsigned int *readbuff, unsigned int ReadAddr, unsigned short BlockSize);
SD_Error SD_ReadMultiBlocks(unsigned char *readbuff, unsigned int ReadAddr, unsigned int BlockSize, unsigned int NumberOfBlocks);
SD_Error SD_WriteBlock(unsigned int *writebuff, unsigned int WriteAddr, unsigned short BlockSize);
SD_Error SD_WriteMultiBlocks(unsigned char *writebuff, unsigned int WriteAddr, unsigned int BlockSize, unsigned int NumberOfBlocks);
SDTransferState SD_GetTransferState(void);
SD_Error SD_StopTransfer(void);
SD_Error SD_Erase(unsigned int startaddr, unsigned int endaddr);
SD_Error SD_SendStatus(unsigned int *pcardstatus);
SD_Error SD_SendSDStatus(unsigned int *psdstatus);
SD_Error SD_ProcessIRQSrc(void);
void SD_ProcessDMAIRQ(void);
SD_Error SD_WaitReadOperation(void);
SD_Error SD_WaitWriteOperation(void);

#endif
