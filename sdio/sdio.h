/**
  ************************************************************************************
  * @file    sdio.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef sdio_h
#define sdio_h
#include "stm32f.h"

#define SD_DETECT_PIN                    GPIO_Pin_8                  /* PG.8 */
#define SD_DETECT_PIN_SOURCE             8                           /* PG.8 */
#define SD_DETECT_GPIO_PORT              GPIOG_BASE                  /* GPIOG */
#define SD_DETECT_GPIO_CLK               RCC_AHB1Periph_GPIOG

#define SDIO_FIFO_ADDRESS                ((unsigned int)0x40012C80)
/**
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define SDIO_INIT_CLK_DIV                ((unsigned char)0x76)
/**
  * @brief  SDIO Data Transfer Frequency (25MHz max)
  */
#define SDIO_TRANSFER_CLK_DIV            ((unsigned char)0x2)

#define SD_SDIO_DMA_STREAM3	           3
#define SD_SDIO_DMA_PORT                 DMA2
#define SD_SDIO_DMA_CLK                  RCC_AHB1Periph_DMA2
#define SD_SDIO_DMA_IRQn               NVIC_DMA2_STREAM3_IRQ
//#define SD_SDIO_DMA_STREAM6              6

#define SD_SDIO_DMA_STREAM            DMA_STREAM3
#define SD_SDIO_DMA_STREAM_REGS       ((DMA_Stream_Regs *) DMA2_Stream3_BASE)
#define SD_SDIO_DMA_CHANNEL           DMA_SxCR_CHSEL_4
/* #define SD_SDIO_DMA_FLAG_FEIF         (DMA_FEIF << DMA_ISR_OFFSET(DMA_STREAM3)) */
/* #define SD_SDIO_DMA_FLAG_DMEIF        (DMA_DMEIF << DMA_ISR_OFFSET(DMA_STREAM3)) */
/* #define SD_SDIO_DMA_FLAG_TEIF         (DMA_TEIF << DMA_ISR_OFFSET(DMA_STREAM3)) */
/* #define SD_SDIO_DMA_FLAG_HTIF         (DMA_HTIF << DMA_ISR_OFFSET(DMA_STREAM3)) */
/* #define SD_SDIO_DMA_FLAG_TCIF         (DMA_TCIF << DMA_ISR_OFFSET(DMA_STREAM3)) */
#define SD_SDIO_DMA_FLAG_FEIF         DMA_FLAG_FEIF3
#define SD_SDIO_DMA_FLAG_DMEIF        DMA_FLAG_DMEIF3
#define SD_SDIO_DMA_FLAG_TEIF         DMA_FLAG_TEIF3
#define SD_SDIO_DMA_FLAG_HTIF         DMA_FLAG_HTIF3
#define SD_SDIO_DMA_FLAG_TCIF         DMA_FLAG_TCIF3

typedef struct {
  unsigned int ClockEdge;            /*!< Specifies the clock transition on which the bit capture is made.
                                      This parameter can be a value of @ref SDIO_Clock_Edge */
  unsigned int ClockBypass;          /*!< Specifies whether the SDIO Clock divider bypass is
                                      enabled or disabled.
                                      This parameter can be a value of @ref SDIO_Clock_Bypass */
  unsigned int ClockPowerSave;       /*!< Specifies whether SDIO Clock output is enabled or
                                      disabled when the bus is idle.
                                      This parameter can be a value of @ref SDIO_Clock_Power_Save */
  unsigned int BusWidth;              /*!< Specifies the SDIO bus width.
                                      This parameter can be a value of @ref SDIO_Bus_Width */
  unsigned int HardwareFlowControl;  /*!< Specifies whether the SDIO hardware flow control is enabled or disabled.
                                           This parameter can be a value of @ref SDIO_Hardware_Flow_Control */
  unsigned char ClockDiv;              /*!< Specifies the clock frequency of the SDIO controller.
                                           This parameter can be a value between 0x00 and 0xFF. */
} SDIO_InitArgs;

typedef struct {
  unsigned int Argument;  /*!< Specifies the SDIO command argument which is sent
                           to a card as part of a command message. If a command
                           contains an argument, it must be loaded into this register
                           before writing the command to the command register */
  unsigned int CmdIndex;  /*!< Specifies the SDIO command index. It must be lower than 0x40. */
  unsigned int Response;  /*!< Specifies the SDIO response type.
                           This parameter can be a value of @ref SDIO_Response_Type */
  unsigned int Wait;      /*!< Specifies whether SDIO wait-for-interrupt request is enabled or disabled.
                           This parameter can be a value of @ref SDIO_Wait_Interrupt_State */
  unsigned int CPSM;      /*!< Specifies whether SDIO Command path state machine (CPSM)
                          is enabled or disabled.
                          This parameter can be a value of @ref SDIO_CPSM_State */
} SDIO_CmdInitArgs;

typedef struct {
  unsigned int DataTimeOut;    /*!< Specifies the data timeout period in card bus clock periods. */
  unsigned int DataLength;     /*!< Specifies the number of data bytes to be transferred. */
  unsigned int DataBlockSize;  /*!< Specifies the data block size for block transfer.
                                This parameter can be a value of @ref SDIO_Data_Block_Size */
  unsigned int TransferDir;    /*!< Specifies the data transfer direction, whether the transfer
                                is a read or write.
                                This parameter can be a value of @ref SDIO_Transfer_Direction */
  unsigned int TransferMode;   /*!< Specifies whether data transfer is in stream or block mode.
                                This parameter can be a value of @ref SDIO_Transfer_Type */
  unsigned int DPSM;           /*!< Specifies whether SDIO Data path state machine (DPSM)
                                is enabled or disabled.
                                This parameter can be a value of @ref SDIO_DPSM_State */
} SDIO_DataInitArgs;

#define SDIO_ClockEdge_Rising               ((unsigned int)0x00000000)
#define SDIO_ClockEdge_Falling              ((unsigned int)0x00002000)

#define SDIO_ClockBypass_Disable             ((unsigned int)0x00000000)
#define SDIO_ClockBypass_Enable              ((unsigned int)0x00000400)

#define SDIO_ClockPowerSave_Disable         ((unsigned int)0x00000000)
#define SDIO_ClockPowerSave_Enable          ((unsigned int)0x00000200)
#define SDIO_BusWidth_1b                     ((unsigned int)0x00000000)
#define SDIO_BusWidth_4b                     ((unsigned int)0x00000800)
#define SDIO_BusWidth_8b                     ((unsigned int)0x00001000)

#define SDIO_HardwareFlowControl_Disable    ((unsigned int)0x00000000)
#define SDIO_HardwareFlowControl_Enable     ((unsigned int)0x00004000)

#define SDIO_PowerState_OFF                 ((unsigned int)0x00000000)
#define SDIO_PowerState_ON                  ((unsigned int)0x00000003)

#define SDIO_IT_CCRCFAIL                    ((unsigned int)0x00000001)
#define SDIO_IT_DCRCFAIL                    ((unsigned int)0x00000002)
#define SDIO_IT_CTIMEOUT                    ((unsigned int)0x00000004)
#define SDIO_IT_DTIMEOUT                    ((unsigned int)0x00000008)
#define SDIO_IT_TXUNDERR                    ((unsigned int)0x00000010)
#define SDIO_IT_RXOVERR                     ((unsigned int)0x00000020)
#define SDIO_IT_CMDREND                     ((unsigned int)0x00000040)
#define SDIO_IT_CMDSENT                     ((unsigned int)0x00000080)
#define SDIO_IT_DATAEND                     ((unsigned int)0x00000100)
#define SDIO_IT_STBITERR                    ((unsigned int)0x00000200)
#define SDIO_IT_DBCKEND                     ((unsigned int)0x00000400)
#define SDIO_IT_CMDACT                      ((unsigned int)0x00000800)
#define SDIO_IT_TXACT                       ((unsigned int)0x00001000)
#define SDIO_IT_RXACT                       ((unsigned int)0x00002000)
#define SDIO_IT_TXFIFOHE                    ((unsigned int)0x00004000)
#define SDIO_IT_RXFIFOHF                    ((unsigned int)0x00008000)
#define SDIO_IT_TXFIFOF                     ((unsigned int)0x00010000)
#define SDIO_IT_RXFIFOF                     ((unsigned int)0x00020000)
#define SDIO_IT_TXFIFOE                     ((unsigned int)0x00040000)
#define SDIO_IT_RXFIFOE                     ((unsigned int)0x00080000)
#define SDIO_IT_TXDAVL                      ((unsigned int)0x00100000)
#define SDIO_IT_RXDAVL                      ((unsigned int)0x00200000)
#define SDIO_IT_SDIOIT                      ((unsigned int)0x00400000)
#define SDIO_IT_CEATAEND                    ((unsigned int)0x00800000)

#define SDIO_Response_No                    ((unsigned int)0x00000000)
#define SDIO_Response_Short                 ((unsigned int)0x00000040)
#define SDIO_Response_Long                  ((unsigned int)0x000000C0)

#define SDIO_Wait_No                        ((unsigned int)0x00000000) /*!< SDIO No Wait, TimeOut is enabled */
#define SDIO_Wait_IT                        ((unsigned int)0x00000100) /*!< SDIO Wait Interrupt Request */
#define SDIO_Wait_Pend                      ((unsigned int)0x00000200) /*!< SDIO Wait End of transfer */

#define SDIO_CPSM_Disable                    ((unsigned int)0x00000000)
#define SDIO_CPSM_Enable                     ((unsigned int)0x00000400)

#define SDIO_RESP1                          ((unsigned int)0x00000000)
#define SDIO_RESP2                          ((unsigned int)0x00000004)
#define SDIO_RESP3                          ((unsigned int)0x00000008)
#define SDIO_RESP4                          ((unsigned int)0x0000000C)

#define SDIO_DataBlockSize_1b               ((unsigned int)0x00000000)
#define SDIO_DataBlockSize_2b               ((unsigned int)0x00000010)
#define SDIO_DataBlockSize_4b               ((unsigned int)0x00000020)
#define SDIO_DataBlockSize_8b               ((unsigned int)0x00000030)
#define SDIO_DataBlockSize_16b              ((unsigned int)0x00000040)
#define SDIO_DataBlockSize_32b              ((unsigned int)0x00000050)
#define SDIO_DataBlockSize_64b              ((unsigned int)0x00000060)
#define SDIO_DataBlockSize_128b             ((unsigned int)0x00000070)
#define SDIO_DataBlockSize_256b             ((unsigned int)0x00000080)
#define SDIO_DataBlockSize_512b             ((unsigned int)0x00000090)
#define SDIO_DataBlockSize_1024b            ((unsigned int)0x000000A0)
#define SDIO_DataBlockSize_2048b            ((unsigned int)0x000000B0)
#define SDIO_DataBlockSize_4096b            ((unsigned int)0x000000C0)
#define SDIO_DataBlockSize_8192b            ((unsigned int)0x000000D0)
#define SDIO_DataBlockSize_16384b           ((unsigned int)0x000000E0)

#define SDIO_TransferDir_ToCard             ((unsigned int)0x00000000)
#define SDIO_TransferDir_ToSDIO             ((unsigned int)0x00000002)

#define SDIO_TransferMode_Block             ((unsigned int)0x00000000)
#define SDIO_TransferMode_Stream            ((unsigned int)0x00000004)

#define SDIO_DPSM_Disable                    ((unsigned int)0x00000000)
#define SDIO_DPSM_Enable                     ((unsigned int)0x00000001)

#define SDIO_FLAG_CCRCFAIL                  ((unsigned int)0x00000001)
#define SDIO_FLAG_DCRCFAIL                  ((unsigned int)0x00000002)
#define SDIO_FLAG_CTIMEOUT                  ((unsigned int)0x00000004)
#define SDIO_FLAG_DTIMEOUT                  ((unsigned int)0x00000008)
#define SDIO_FLAG_TXUNDERR                  ((unsigned int)0x00000010)
#define SDIO_FLAG_RXOVERR                   ((unsigned int)0x00000020)
#define SDIO_FLAG_CMDREND                   ((unsigned int)0x00000040)
#define SDIO_FLAG_CMDSENT                   ((unsigned int)0x00000080)
#define SDIO_FLAG_DATAEND                   ((unsigned int)0x00000100)
#define SDIO_FLAG_STBITERR                  ((unsigned int)0x00000200)
#define SDIO_FLAG_DBCKEND                   ((unsigned int)0x00000400)
#define SDIO_FLAG_CMDACT                    ((unsigned int)0x00000800)
#define SDIO_FLAG_TXACT                     ((unsigned int)0x00001000)
#define SDIO_FLAG_RXACT                     ((unsigned int)0x00002000)
#define SDIO_FLAG_TXFIFOHE                  ((unsigned int)0x00004000)
#define SDIO_FLAG_RXFIFOHF                  ((unsigned int)0x00008000)
#define SDIO_FLAG_TXFIFOF                   ((unsigned int)0x00010000)
#define SDIO_FLAG_RXFIFOF                   ((unsigned int)0x00020000)
#define SDIO_FLAG_TXFIFOE                   ((unsigned int)0x00040000)
#define SDIO_FLAG_RXFIFOE                   ((unsigned int)0x00080000)
#define SDIO_FLAG_TXDAVL                    ((unsigned int)0x00100000)
#define SDIO_FLAG_RXDAVL                    ((unsigned int)0x00200000)
#define SDIO_FLAG_SDIOIT                    ((unsigned int)0x00400000)
#define SDIO_FLAG_CEATAEND                  ((unsigned int)0x00800000)

#define SDIO_ReadWaitMode_CLK               ((unsigned int)0x00000000)
#define SDIO_ReadWaitMode_DATA2             ((unsigned int)0x00000001)

/*  Function used to set the SDIO configuration to the default reset state ****/
void SDIO_DeInit(void);

/* Initialization and Configuration functions *********************************/
void SDIO_Init(SDIO_InitArgs* args);
void SDIO_StructInit(SDIO_InitArgs* args);
void SDIO_ClockCmd(FunctionalState state);
void SDIO_SetPowerState(unsigned int SDIO_PowerState);
unsigned int SDIO_GetPowerState(void);

/* Command path state machine (CPSM) management functions *********************/
void SDIO_SendCommand(SDIO_CmdInitArgs *args);
void SDIO_CmdStructInit(SDIO_CmdInitArgs* args);
unsigned char SDIO_GetCommandResponse(void);
unsigned int SDIO_GetResponse(unsigned int SDIO_RESP);

/* Data path state machine (DPSM) management functions ************************/
void SDIO_DataConfig(SDIO_DataInitArgs* args);
void SDIO_DataStructInit(SDIO_DataInitArgs* args);
unsigned int SDIO_GetDataCounter(void);
unsigned int SDIO_ReadData(void);
void SDIO_WriteData(unsigned int Data);
unsigned int SDIO_GetFIFOCount(void);

/* SDIO IO Cards mode management functions ************************************/
void SDIO_StartSDIOReadWait(FunctionalState state);
void SDIO_StopSDIOReadWait(FunctionalState state);
void SDIO_SetSDIOReadWaitMode(unsigned int SDIO_ReadWaitMode);
void SDIO_SetSDIOOperation(FunctionalState state);
void SDIO_SendSDIOSuspendCmd(FunctionalState state);

/* CE-ATA mode management functions *******************************************/
void SDIO_CommandCompletionCmd(FunctionalState state);
void SDIO_CEATAITCmd(FunctionalState state);
void SDIO_SendCEATACmd(FunctionalState state);

/* DMA transfers management functions *****************************************/
void SDIO_DMACmd(FunctionalState state);

/* Interrupts and flags management functions **********************************/
void SDIO_ITConfig(unsigned int SDIO_IT, FunctionalState state);
FlagStatus SDIO_GetFlagStatus(unsigned int SDIO_FLAG);
void SDIO_ClearFlag(unsigned int SDIO_FLAG);
ITStatus SDIO_GetITStatus(unsigned int SDIO_IT);
void SDIO_ClearITPendingBit(unsigned int SDIO_IT);

void SD_LowLevel_DeInit(void);
void SD_LowLevel_Init(void);
void SD_LowLevel_DMA_TxConfig(unsigned int *BufferSRC, unsigned int BufferSize);
void SD_LowLevel_DMA_RxConfig(unsigned int *BufferDST, unsigned int BufferSize);
unsigned int SD_DMAEndOfTransferStatus(void);

#endif
