/**
  ******************************************************************************
  * @file    sd.c
  * @date    02-December-2013
  * @brief   This file provides a set of functions needed to manage SDIO SD
  *          Card memory.
  *
  *  @verbatim
  *
  *  ===================================================================
  *                           How to use this driver
  *  ===================================================================
  *  It implements a high level communication layer for read and write
  *  from/to this memory. The needed STM32 hardware resources (SDIO and
  *  GPIO) are defined in sd.h file, and the initialization is
  *  performed in SD_LowLevel_Init() function declared in sdio.c
  *  file.
  *  You can easily tailor this driver to any other hw setup,
  *  by just adapting the defines for hardware resources and
  *  SD_LowLevel_Init() function.
  *
  *  A - SD Card Initialization and configuration
  *  ============================================
  *    - To initialize the SD Card, use the SD_Init() function.  It
  *      Initializes the SD Card and put it into StandBy State (Ready
  *      for data transfer). This function provide the following operations:
  *
  *      1 - Apply the SD Card initialization process at 400KHz and check
  *          the SD Card type (Standard Capacity or High Capacity). You
  *          can change or adapt this frequency by adjusting the
  *          "SDIO_INIT_CLK_DIV" define inside the sd.h file.
  *          The SD Card frequency (SDIO_CK) is computed as follows:
  *
  *             +---------------------------------------------+
  *             | SDIO_CK = SDIOCLK / (SDIO_INIT_CLK_DIV + 2) |
  *             +---------------------------------------------+
  *
  *          In initialization mode and according to the SD Card standard,
  *          make sure that the SDIO_CK frequency don't exceed 400KHz.
  *
  *      2 - Get the SD CID and CSD data. All these information are
  *          managed by the SDCardInfo structure. This structure provide
  *          also ready computed SD Card capacity and Block size.
  *
  *      3 - Configure the SD Card Data transfer frequency. By Default,
  *          the card transfer frequency is set to 24MHz. You can change
  *          or adapt this frequency by adjusting the "SDIO_TRANSFER_CLK_DIV"
  *          define inside the sd.h file.
  *          The SD Card frequency (SDIO_CK) is computed as follows:
  *
  *             +---------------------------------------------+
  *             | SDIO_CK = SDIOCLK / (SDIO_INIT_CLK_DIV + 2) |
  *             +---------------------------------------------+
  *
  *          In transfer mode and according to the SD Card standard,
  *          make sure that the SDIO_CK frequency don't exceed 25MHz
  *          and 50MHz in High-speed mode switch.
  *          To be able to use a frequency higher than 24MHz, you should
  *          use the SDIO peripheral in bypass mode. Refer to the
  *          corresponding reference manual for more details.
  *
  *      4 -  Select the corresponding SD Card according to the address
  *           read with the step 2.
  *
  *      5 -  Configure the SD Card in wide bus mode: 4-bits data.
  *
  *  B - SD Card Read operation
  *  ==========================
  *   - You can read SD card by using the function: SD_ReadMultiBlocks()
  *     function. This function support only 512-byte block length.
  *   - The SD_ReadMultiBlocks() function read only mutli blocks (multiple
  *     of 512-byte).
  *   - Any read operation should be followed by two functions to check
  *     if the DMA Controller and SD Card status.
  *      - SD_ReadWaitOperation(): this function insure that the DMA
  *        controller has finished all data transfer.
  *      - SD_GetStatus(): to check that the SD Card has finished the
  *        data transfer and it is ready for data.
  *
  *   - The DMA transfer is finished by the SDIO Data End interrupt.
  *     User has to call the SD_ProcessIRQ() function inside the SDIO_IRQHandler()
  *     and SD_ProcessDMAIRQ() function inside the DMA2_Streamx_IRQHandler().
  *     Don't forget to enable the SDIO_IRQn and DMA2_Stream3_IRQn or
  *     DMA2_Stream6_IRQn interrupts using the NVIC controller.
  *
  *  C - SD Card Write operation
  *  ===========================
  *   - You can write SD card by using the function:
  *     SD_WriteMultiBlocks() functions. This function supports only
  *     512-byte block length.
  *   - The SD_WriteMultiBlocks() function write only mutli blocks (multiple
  *     of 512-byte).
  *   - Any write operation should be followed by two functions to check
  *     if the DMA Controller and SD Card status.
  *      - SD_ReadWaitOperation(): this function insure that the DMA
  *        controller has finished all data transfer.
  *      - SD_GetStatus(): to check that the SD Card has finished the
  *        data transfer and it is ready for data.
  *
  *   - The DMA transfer is finished by the SDIO Data End interrupt.
  *     User has to call the SD_ProcessIRQ() function inside the SDIO_IRQHandler()
  *     and SD_ProcessDMAIRQ() function inside the DMA2_Streamx_IRQHandler().
  *     Don't forget to enable the SDIO_IRQn and DMA2_Stream3_IRQn or
  *     DMA2_Stream6_IRQn interrupts using the NVIC controller.
  *
  *
  *  D - SD card status
  *  ==================
  *   - At any time, you can check the SD Card status and get the SD card
  *     state by using the SD_GetStatus() function. This function checks
  *     first if the SD card is still connected and then get the internal
  *     SD Card transfer state.
  *   - You can also get the SD card SD Status register by using the
  *     SD_SendSDStatus() function.
  *
  *  E - Programming Model (Selecting DMA for SDIO data Transfer)
  *  ============================================================
  *     Status = SD_Init(); // Initialization Step as described in section A
  *
  *     // SDIO Interrupt ENABLE
  *     NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  *     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  *     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  *     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  *     NVIC_Init(&NVIC_InitStructure);
  *     // DMA2 STREAMx Interrupt ENABLE
  *     NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
  *     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  *     NVIC_Init(&NVIC_InitStructure);
  *
  *     Status = SD_WriteMultiBlocks(buffer, address, 512, NUMBEROFBLOCKS);
  *     Status = SD_WaitWriteOperation();
  *     while(SD_GetStatus() != SD_TRANSFER_OK);
  *
  *     Status = SD_ReadMultiBlocks(buffer, address, 512, NUMBEROFBLOCKS);
  *     Status = SD_WaitReadOperation();
  *     while(SD_GetStatus() != SD_TRANSFER_OK);
  *
  *     - Add the SDIO and DMA2 StreamX (3 or 6) IRQ Handlers:
  *         void SDIO_IRQHandler(void)
  *         {
  *           SD_ProcessIRQ();
  *         }
  *         void SD_SDIO_DMA_IRQHANDLER(void)
  *         {
  *           SD_ProcessDMAIRQ();
  *         }
  *
  *  STM32 SDIO Pin assignment
  *  =========================
  *  +-----------------------------------------------------------+
  *  |                     Pin assignment                        |
  *  +-----------------------------+---------------+-------------+
  *  |  STM32 SDIO Pins            |     SD        |    Pin      |
  *  +-----------------------------+---------------+-------------+
  *  |      SDIO D2                |   D2          |    1        |
  *  |      SDIO D3                |   D3          |    2        |
  *  |      SDIO CMD               |   CMD         |    3        |
  *  |                             |   VCC         |    4 (3.3 V)|
  *  |      SDIO CLK               |   CLK         |    5        |
  *  |                             |   GND         |    6 (0 V)  |
  *  |      SDIO D0                |   D0          |    7        |
  *  |      SDIO D1                |   D1          |    8        |
  *  +-----------------------------+---------------+-------------+
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

#include "sdio.h"
#include "sd.h"
#include "led.h"

/**
  * @brief  SDIO Static flags, TimeOut, FIFO Address
  */
#define NULL 0
#define SDIO_STATIC_FLAGS               ((unsigned int)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((unsigned int)0x00010000)

/**
  * @brief  Mask for errors Card Status R1 (OCR Register)
  */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((unsigned int)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((unsigned int)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((unsigned int)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((unsigned int)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((unsigned int)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((unsigned int)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((unsigned int)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((unsigned int)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((unsigned int)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((unsigned int)0x00200000)
#define SD_OCR_CC_ERROR                 ((unsigned int)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((unsigned int)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((unsigned int)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((unsigned int)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((unsigned int)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((unsigned int)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((unsigned int)0x00004000)
#define SD_OCR_ERASE_RESET              ((unsigned int)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((unsigned int)0x00000008)
#define SD_OCR_ERRORBITS                ((unsigned int)0xFDFFE008)

/**
  * @brief  Masks for R6 Response
  */
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((unsigned int)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((unsigned int)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((unsigned int)0x00008000)

#define SD_VOLTAGE_WINDOW_SD            ((unsigned int)0x80100000)
#define SD_HIGH_CAPACITY                ((unsigned int)0x40000000)
#define SD_STD_CAPACITY                 ((unsigned int)0x00000000)
#define SD_CHECK_PATTERN                ((unsigned int)0x000001AA)

#define SD_MAX_VOLT_TRIAL               ((unsigned int)0x0000FFFF)
#define SD_ALLZERO                      ((unsigned int)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((unsigned int)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((unsigned int)0x00010000)
#define SD_CARD_LOCKED                  ((unsigned int)0x02000000)

#define SD_DATATIMEOUT                  ((unsigned int)0xFFFFFFFF)
#define SD_0TO7BITS                     ((unsigned int)0x000000FF)
#define SD_8TO15BITS                    ((unsigned int)0x0000FF00)
#define SD_16TO23BITS                   ((unsigned int)0x00FF0000)
#define SD_24TO31BITS                   ((unsigned int)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((unsigned int)0x01FFFFFF)

#define SD_HALFFIFO                     ((unsigned int)0x00000008)
#define SD_HALFFIFOBYTES                ((unsigned int)0x00000020)

/**
  * @brief  Command Class Supported
  */
#define SD_CCCC_LOCK_UNLOCK             ((unsigned int)0x00000080)
#define SD_CCCC_WRITE_PROT              ((unsigned int)0x00000040)
#define SD_CCCC_ERASE                   ((unsigned int)0x00000020)

/**
  * @brief  Following commands are SD Card Specific commands.
  *         SDIO_APP_CMD should be sent before sending these commands.
  */
#define SDIO_SEND_IF_COND               ((unsigned int)0x00000008)

static unsigned int CardType =  SDIO_STD_CAPACITY_SD_CARD_V1_1;
static unsigned int CSD_Tab[4], CID_Tab[4], RCA = 0;
static unsigned char SDSTATUS_Tab[80];
volatile unsigned int StopCondition = 0;
volatile SD_Error TransferError = SD_OK;
volatile unsigned int TransferEnd = 0, DMAEndOfTransfer = 0;
SD_CardInfo SDCardInfo;

SDIO_InitArgs SDIO_InitStructure;
SDIO_CmdInitArgs SDIO_CmdInitStructure;
SDIO_DataInitArgs SDIO_DataInitStructure;

static SD_Error CmdError(void);
static SD_Error CmdResp1Error(unsigned char cmd);
static SD_Error CmdResp7Error(void);
static SD_Error CmdResp3Error(void);
static SD_Error CmdResp2Error(void);
static SD_Error CmdResp6Error(unsigned char cmd, unsigned short *prca);
static SD_Error SDEnWideBus(FunctionalState NewState);
static SD_Error IsCardProgramming(unsigned char *pstatus);
static SD_Error FindSCR(unsigned short rca, unsigned int *pscr);
unsigned char convert_from_bytes_to_power_of_two(unsigned short NumberOfBytes);

/**
  * @brief  DeInitializes the SDIO interface.
  * @param  None
  * @retval None
  */
void SD_DeInit(void) {
  SD_LowLevel_DeInit();
}

/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for data
  *         transfer).
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Init(void) {
  volatile SD_Error errorstatus = SD_OK;

  /* SDIO Peripheral Low Level Init */
  SD_LowLevel_Init();

  SDIO_DeInit();

  errorstatus = SD_PowerON();

  if (errorstatus != SD_OK) {
    /*!< CMD Response TimeOut (wait for CMDSENT flag) */
    return(errorstatus);
  }

  errorstatus = SD_InitializeCards();

  if (errorstatus != SD_OK) {
    /*!< CMD Response TimeOut (wait for CMDSENT flag) */
    return(errorstatus);
  }

  /*!< Configure the SDIO peripheral */
  /*!< SDIO_CK = SDIOCLK / (SDIO_TRANSFER_CLK_DIV + 2) */
  /*!< on STM32F2xx devices, SDIOCLK is fixed to 48MHz */
  SDIO_InitStructure.ClockDiv = SDIO_TRANSFER_CLK_DIV;
  SDIO_InitStructure.ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStructure.ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStructure.BusWidth = SDIO_BusWidth_1b;
  SDIO_InitStructure.HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
  SDIO_Init(&SDIO_InitStructure);

  /*----------------- Read CSD/CID MSD registers ------------------*/
  errorstatus = SD_GetCardInfo(&SDCardInfo);

  if (errorstatus == SD_OK) {
    /*----------------- Select Card --------------------------------*/
    errorstatus = SD_SelectDeselect((unsigned int) (SDCardInfo.RCA << 16));
  }

  if (errorstatus == SD_OK) {
    errorstatus = SD_EnableWideBusOperation(SDIO_BusWidth_4b);
  }

  return(errorstatus);
}

/**
  * @brief  Gets the current sd card data transfer status.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be:
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetStatus(void) {
  SDCardState cardstate =  SD_CARD_TRANSFER;

  cardstate = SD_GetState();

  if(cardstate == SD_CARD_TRANSFER) return(SD_TRANSFER_OK);
  if(cardstate == SD_CARD_ERROR) return (SD_TRANSFER_ERROR);
  return(SD_TRANSFER_BUSY);
}

/**
  * @brief  Returns the current card's state.
  * @param  None
  * @retval SDCardState: SD Card Error or SD Card Current State.
  */
SDCardState SD_GetState(void) {
  unsigned int resp1 = 0;

  if(SD_Detect()== SD_PRESENT) {
    if (SD_SendStatus(&resp1) != SD_OK) return SD_CARD_ERROR;
    return (SDCardState)((resp1 >> 9) & 0x0F);
  }
  return SD_CARD_ERROR;
}

/**
 * @brief  Detect if SD card is correctly plugged in the memory slot.
 * @param  None
 * @retval Return if SD is detected or not
 */
unsigned char SD_Detect(void) {
  volatile unsigned char status = SD_PRESENT;
  /*!< Check GPIO to detect SD */
  if (gpio_get(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) != 0) {
    status = SD_NOT_PRESENT;
  }
  return status;
}

/**
  * @brief  Enquires cards about their operating voltage and configures
  *   clock controls.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerON(void) {
  volatile SD_Error errorstatus = SD_OK;
  unsigned int response = 0, count = 0, validvoltage = 0;
  unsigned int SDType = SD_STD_CAPACITY;

  /*!< Power ON Sequence -----------------------------------------------------*/
  /*!< Configure the SDIO peripheral */
  /*!< SDIO_CK = SDIOCLK / (SDIO_INIT_CLK_DIV + 2) */
  /*!< on STM32F2xx devices, SDIOCLK is fixed to 48MHz */
  /*!< SDIO_CK for initialization should not exceed 400 KHz */
  SDIO_InitStructure.ClockDiv = SDIO_INIT_CLK_DIV;
  SDIO_InitStructure.ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStructure.ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStructure.BusWidth = SDIO_BusWidth_1b;
  SDIO_InitStructure.HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
  SDIO_Init(&SDIO_InitStructure);

  /*!< Set Power State to ON */
  SDIO_SetPowerState(SDIO_PowerState_ON);

  /*!< Enable SDIO Clock */
  SDIO_ClockCmd(ENABLE);

  /*!< CMD0: GO_IDLE_STATE ---------------------------------------------------*/
  /*!< No CMD response required */
  SDIO_CmdInitStructure.Argument = 0x0;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_GO_IDLE_STATE;
  SDIO_CmdInitStructure.Response = SDIO_Response_No;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdError();

  if (errorstatus != SD_OK) {
    /*!< CMD Response TimeOut (wait for CMDSENT flag) */
    return(errorstatus);
  }

  /*!< CMD8: SEND_IF_COND ----------------------------------------------------*/
  /*!< Send CMD8 to verify SD card interface operating condition */
  /*!< Argument: - [31:12]: Reserved (shall be set to '0')
               - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
               - [7:0]: Check Pattern (recommended 0xAA) */
  /*!< CMD Response: R7 */
  SDIO_CmdInitStructure.Argument = SD_CHECK_PATTERN;
  SDIO_CmdInitStructure.CmdIndex = SDIO_SEND_IF_COND;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp7Error();

  if (errorstatus == SD_OK) {
    CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0; /*!< SD Card 2.0 */
    SDType = SD_HIGH_CAPACITY;
  } else {
    /*!< CMD55 */
    SDIO_CmdInitStructure.Argument = 0x00;
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
    SDIO_CmdInitStructure.Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);
    errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
  }
  /*!< CMD55 */
  SDIO_CmdInitStructure.Argument = 0x00;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

  /*!< If errorstatus is Command TimeOut, it is a MMC card */
  /*!< If errorstatus is SD_OK it is a SD card: SD card 2.0 (voltage range mismatch)
     or SD card 1.x */
  if (errorstatus == SD_OK) {
    /*!< SD CARD */
    /*!< Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
    while ((!validvoltage) && (count < SD_MAX_VOLT_TRIAL)) {

      /*!< SEND CMD55 APP_CMD with RCA as 0 */
      SDIO_CmdInitStructure.Argument = 0x00;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

      if (errorstatus != SD_OK) {
        return(errorstatus);
      }
      SDIO_CmdInitStructure.Argument = SD_VOLTAGE_WINDOW_SD | SDType;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_SD_APP_OP_COND;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp3Error();
      if (errorstatus != SD_OK) {
        return(errorstatus);
      }

      response = SDIO_GetResponse(SDIO_RESP1);
      validvoltage = (((response >> 31) == 1) ? 1 : 0);
      count++;
    }
    if (count >= SD_MAX_VOLT_TRIAL) {
      errorstatus = SD_INVALID_VOLTRANGE;
      return(errorstatus);
    }

    if (response &= SD_HIGH_CAPACITY) {
      CardType = SDIO_HIGH_CAPACITY_SD_CARD;
    }

  }/*!< else MMC Card */

  return(errorstatus);
}

/**
  * @brief  Turns the SDIO output signals off.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerOFF(void) {
  SD_Error errorstatus = SD_OK;
  /*!< Set Power State to OFF */
  SDIO_SetPowerState(SDIO_PowerState_OFF);

  return(errorstatus);
}

/**
  * @brief  Intialises all cards or single card as the case may be Card(s) come
  *         into standby state.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_InitializeCards(void) {
  SD_Error errorstatus = SD_OK;
  unsigned short rca = 0x01;

  if (SDIO_GetPowerState() == SDIO_PowerState_OFF) {
    errorstatus = SD_REQUEST_NOT_APPLICABLE;
    return(errorstatus);
  }

  if (SDIO_SECURE_DIGITAL_IO_CARD != CardType) {
    /*!< Send CMD2 ALL_SEND_CID */
    SDIO_CmdInitStructure.Argument = 0x0;
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_ALL_SEND_CID;
    SDIO_CmdInitStructure.Response = SDIO_Response_Long;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    errorstatus = CmdResp2Error();

    if (SD_OK != errorstatus) {
      return(errorstatus);
    }

    CID_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
    CID_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
    CID_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
    CID_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
  }
  if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) ||
      (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) ||
      (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == CardType) ||
      (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
    /*!< Send CMD3 SET_REL_ADDR with argument 0 */
    /*!< SD Card publishes its RCA. */
    SDIO_CmdInitStructure.Argument = 0x00;
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_REL_ADDR;
    SDIO_CmdInitStructure.Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    errorstatus = CmdResp6Error(SD_CMD_SET_REL_ADDR, &rca);

    if (SD_OK != errorstatus) {
      return(errorstatus);
    }
  }

  if (SDIO_SECURE_DIGITAL_IO_CARD != CardType) {
    RCA = rca;

    /*!< Send CMD9 SEND_CSD with argument as card's RCA */
    SDIO_CmdInitStructure.Argument = (unsigned int)(rca << 16);
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_SEND_CSD;
    SDIO_CmdInitStructure.Response = SDIO_Response_Long;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    errorstatus = CmdResp2Error();

    if (SD_OK != errorstatus) {
      return(errorstatus);
    }

    CSD_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
    CSD_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
    CSD_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
    CSD_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
  }

  errorstatus = SD_OK; /*!< All cards get intialized */

  return(errorstatus);
}

/**
  * @brief  Returns information about specific card.
  * @param  cardinfo: pointer to a SD_CardInfo structure that contains all SD card
  *         information.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo) {
  SD_Error errorstatus = SD_OK;
  unsigned char tmp = 0;

  cardinfo->CardType = (unsigned char)CardType;
  cardinfo->RCA = (unsigned short)RCA;

  /*!< Byte 0 */
  tmp = (unsigned char)((CSD_Tab[0] & 0xFF000000) >> 24);
  cardinfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
  cardinfo->SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
  cardinfo->SD_csd.Reserved1 = tmp & 0x03;

  /*!< Byte 1 */
  tmp = (unsigned char)((CSD_Tab[0] & 0x00FF0000) >> 16);
  cardinfo->SD_csd.TAAC = tmp;

  /*!< Byte 2 */
  tmp = (unsigned char)((CSD_Tab[0] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.NSAC = tmp;

  /*!< Byte 3 */
  tmp = (unsigned char)(CSD_Tab[0] & 0x000000FF);
  cardinfo->SD_csd.MaxBusClkFrec = tmp;

  /*!< Byte 4 */
  tmp = (unsigned char)((CSD_Tab[1] & 0xFF000000) >> 24);
  cardinfo->SD_csd.CardComdClasses = tmp << 4;

  /*!< Byte 5 */
  tmp = (unsigned char)((CSD_Tab[1] & 0x00FF0000) >> 16);
  cardinfo->SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;
  cardinfo->SD_csd.RdBlockLen = tmp & 0x0F;

  /*!< Byte 6 */
  tmp = (unsigned char)((CSD_Tab[1] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.DSRImpl = (tmp & 0x10) >> 4;
  cardinfo->SD_csd.Reserved2 = 0; /*!< Reserved */

  if ((CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0)) {
    cardinfo->SD_csd.DeviceSize = (tmp & 0x03) << 10;

    /*!< Byte 7 */
    tmp = (unsigned char)(CSD_Tab[1] & 0x000000FF);
    cardinfo->SD_csd.DeviceSize |= (tmp) << 2;

    /*!< Byte 8 */
    tmp = (unsigned char)((CSD_Tab[2] & 0xFF000000) >> 24);
    cardinfo->SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;

    cardinfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
    cardinfo->SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);

    /*!< Byte 9 */
    tmp = (unsigned char)((CSD_Tab[2] & 0x00FF0000) >> 16);
    cardinfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
    cardinfo->SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
    cardinfo->SD_csd.DeviceSizeMul = (tmp & 0x03) << 1;
    /*!< Byte 10 */
    tmp = (unsigned char)((CSD_Tab[2] & 0x0000FF00) >> 8);
    cardinfo->SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;

    cardinfo->CardCapacity = (cardinfo->SD_csd.DeviceSize + 1) ;
    cardinfo->CardCapacity *= (1 << (cardinfo->SD_csd.DeviceSizeMul + 2));
    cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);
    cardinfo->CardCapacity *= cardinfo->CardBlockSize;
  } else if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
    /*!< Byte 7 */
    tmp = (unsigned char)(CSD_Tab[1] & 0x000000FF);
    cardinfo->SD_csd.DeviceSize = (tmp & 0x3F) << 16;

    /*!< Byte 8 */
    tmp = (unsigned char)((CSD_Tab[2] & 0xFF000000) >> 24);

    cardinfo->SD_csd.DeviceSize |= (tmp << 8);

    /*!< Byte 9 */
    tmp = (unsigned char)((CSD_Tab[2] & 0x00FF0000) >> 16);

    cardinfo->SD_csd.DeviceSize |= (tmp);

    /*!< Byte 10 */
    tmp = (unsigned char)((CSD_Tab[2] & 0x0000FF00) >> 8);

    cardinfo->CardCapacity = (unsigned long long)(cardinfo->SD_csd.DeviceSize + 1) * (unsigned long long)(512 * 1024);
    cardinfo->CardBlockSize = 512;
  }

  cardinfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.EraseGrMul = (tmp & 0x3F) << 1;

  /*!< Byte 11 */
  tmp = (unsigned char)(CSD_Tab[2] & 0x000000FF);
  cardinfo->SD_csd.EraseGrMul |= (tmp & 0x80) >> 7;
  cardinfo->SD_csd.WrProtectGrSize = (tmp & 0x7F);

  /*!< Byte 12 */
  tmp = (unsigned char)((CSD_Tab[3] & 0xFF000000) >> 24);
  cardinfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.ManDeflECC = (tmp & 0x60) >> 5;
  cardinfo->SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
  cardinfo->SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;

  /*!< Byte 13 */
  tmp = (unsigned char)((CSD_Tab[3] & 0x00FF0000) >> 16);
  cardinfo->SD_csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
  cardinfo->SD_csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.Reserved3 = 0;
  cardinfo->SD_csd.ContentProtectAppli = (tmp & 0x01);

  /*!< Byte 14 */
  tmp = (unsigned char)((CSD_Tab[3] & 0x0000FF00) >> 8);
  cardinfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
  cardinfo->SD_csd.CopyFlag = (tmp & 0x40) >> 6;
  cardinfo->SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
  cardinfo->SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
  cardinfo->SD_csd.FileFormat = (tmp & 0x0C) >> 2;
  cardinfo->SD_csd.ECC = (tmp & 0x03);

  /*!< Byte 15 */
  tmp = (unsigned char)(CSD_Tab[3] & 0x000000FF);
  cardinfo->SD_csd.CSD_CRC = (tmp & 0xFE) >> 1;
  cardinfo->SD_csd.Reserved4 = 1;

  /*!< Byte 0 */
  tmp = (unsigned char)((CID_Tab[0] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ManufacturerID = tmp;

  /*!< Byte 1 */
  tmp = (unsigned char)((CID_Tab[0] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.OEM_AppliID = tmp << 8;

  /*!< Byte 2 */
  tmp = (unsigned char)((CID_Tab[0] & 0x000000FF00) >> 8);
  cardinfo->SD_cid.OEM_AppliID |= tmp;

  /*!< Byte 3 */
  tmp = (unsigned char)(CID_Tab[0] & 0x000000FF);
  cardinfo->SD_cid.ProdName1 = tmp << 24;

  /*!< Byte 4 */
  tmp = (unsigned char)((CID_Tab[1] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdName1 |= tmp << 16;

  /*!< Byte 5 */
  tmp = (unsigned char)((CID_Tab[1] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.ProdName1 |= tmp << 8;

  /*!< Byte 6 */
  tmp = (unsigned char)((CID_Tab[1] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ProdName1 |= tmp;

  /*!< Byte 7 */
  tmp = (unsigned char)(CID_Tab[1] & 0x000000FF);
  cardinfo->SD_cid.ProdName2 = tmp;

  /*!< Byte 8 */
  tmp = (unsigned char)((CID_Tab[2] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdRev = tmp;

  /*!< Byte 9 */
  tmp = (unsigned char)((CID_Tab[2] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.ProdSN = tmp << 24;

  /*!< Byte 10 */
  tmp = (unsigned char)((CID_Tab[2] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ProdSN |= tmp << 16;

  /*!< Byte 11 */
  tmp = (unsigned char)(CID_Tab[2] & 0x000000FF);
  cardinfo->SD_cid.ProdSN |= tmp << 8;

  /*!< Byte 12 */
  tmp = (unsigned char)((CID_Tab[3] & 0xFF000000) >> 24);
  cardinfo->SD_cid.ProdSN |= tmp;

  /*!< Byte 13 */
  tmp = (unsigned char)((CID_Tab[3] & 0x00FF0000) >> 16);
  cardinfo->SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
  cardinfo->SD_cid.ManufactDate = (tmp & 0x0F) << 8;

  /*!< Byte 14 */
  tmp = (unsigned char)((CID_Tab[3] & 0x0000FF00) >> 8);
  cardinfo->SD_cid.ManufactDate |= tmp;

  /*!< Byte 15 */
  tmp = (unsigned char)(CID_Tab[3] & 0x000000FF);
  cardinfo->SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
  cardinfo->SD_cid.Reserved2 = 1;

  return(errorstatus);
}

SD_Error SD_GetCardStatus(SD_CardStatus *cardstatus) {
  SD_Error errorstatus = SD_OK;
  unsigned char tmp = 0;

  errorstatus = SD_SendSDStatus((unsigned int *)SDSTATUS_Tab);

  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  /*!< Byte 0 */
  tmp = (unsigned char)((SDSTATUS_Tab[0] & 0xC0) >> 6);
  cardstatus->DAT_BUS_WIDTH = tmp;

  /*!< Byte 0 */
  tmp = (unsigned char)((SDSTATUS_Tab[0] & 0x20) >> 5);
  cardstatus->SECURED_MODE = tmp;

  /*!< Byte 2 */
  tmp = (unsigned char)((SDSTATUS_Tab[2] & 0xFF));
  cardstatus->SD_CARD_TYPE = tmp << 8;

  /*!< Byte 3 */
  tmp = (unsigned char)((SDSTATUS_Tab[3] & 0xFF));
  cardstatus->SD_CARD_TYPE |= tmp;

  /*!< Byte 4 */
  tmp = (unsigned char)(SDSTATUS_Tab[4] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA = tmp << 24;

  /*!< Byte 5 */
  tmp = (unsigned char)(SDSTATUS_Tab[5] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 16;

  /*!< Byte 6 */
  tmp = (unsigned char)(SDSTATUS_Tab[6] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 8;

  /*!< Byte 7 */
  tmp = (unsigned char)(SDSTATUS_Tab[7] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp;

  /*!< Byte 8 */
  tmp = (unsigned char)((SDSTATUS_Tab[8] & 0xFF));
  cardstatus->SPEED_CLASS = tmp;

  /*!< Byte 9 */
  tmp = (unsigned char)((SDSTATUS_Tab[9] & 0xFF));
  cardstatus->PERFORMANCE_MOVE = tmp;

  /*!< Byte 10 */
  tmp = (unsigned char)((SDSTATUS_Tab[10] & 0xF0) >> 4);
  cardstatus->AU_SIZE = tmp;

  /*!< Byte 11 */
  tmp = (unsigned char)(SDSTATUS_Tab[11] & 0xFF);
  cardstatus->ERASE_SIZE = tmp << 8;

  /*!< Byte 12 */
  tmp = (unsigned char)(SDSTATUS_Tab[12] & 0xFF);
  cardstatus->ERASE_SIZE |= tmp;

  /*!< Byte 13 */
  tmp = (unsigned char)((SDSTATUS_Tab[13] & 0xFC) >> 2);
  cardstatus->ERASE_TIMEOUT = tmp;

  /*!< Byte 13 */
  tmp = (unsigned char)((SDSTATUS_Tab[13] & 0x3));
  cardstatus->ERASE_OFFSET = tmp;

  return(errorstatus);
}

/**
  * @brief  Enables wide bus opeartion for the requeseted card if supported by
  *         card.
  * @param  WideMode: Specifies the SD card wide bus mode.
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
  *     @arg SDIO_BusWide_4b: 4-bit data transfer
  *     @arg SDIO_BusWide_1b: 1-bit data transfer
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_EnableWideBusOperation(unsigned int WideMode) {
  SD_Error errorstatus = SD_OK;

  /*!< MMC Card doesn't support this feature */
  if (SDIO_MULTIMEDIA_CARD == CardType) {
    errorstatus = SD_UNSUPPORTED_FEATURE;
    return(errorstatus);
  } else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
    if (SDIO_BusWidth_8b == WideMode) {
      errorstatus = SD_UNSUPPORTED_FEATURE;
      return(errorstatus);
    } else if (SDIO_BusWidth_4b == WideMode) {
      errorstatus = SDEnWideBus(ENABLE);

      if (SD_OK == errorstatus) {
        /*!< Configure the SDIO peripheral */
        SDIO_InitStructure.ClockDiv = SDIO_TRANSFER_CLK_DIV;
        SDIO_InitStructure.ClockEdge = SDIO_ClockEdge_Rising;
        SDIO_InitStructure.ClockBypass = SDIO_ClockBypass_Disable;
        SDIO_InitStructure.ClockPowerSave = SDIO_ClockPowerSave_Disable;
        SDIO_InitStructure.BusWidth = SDIO_BusWidth_4b;
        SDIO_InitStructure.HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
        SDIO_Init(&SDIO_InitStructure);
      }
    } else {
      errorstatus = SDEnWideBus(DISABLE);

      if (SD_OK == errorstatus) {
        /*!< Configure the SDIO peripheral */
        SDIO_InitStructure.ClockDiv = SDIO_TRANSFER_CLK_DIV;
        SDIO_InitStructure.ClockEdge = SDIO_ClockEdge_Rising;
        SDIO_InitStructure.ClockBypass = SDIO_ClockBypass_Disable;
        SDIO_InitStructure.ClockPowerSave = SDIO_ClockPowerSave_Disable;
        SDIO_InitStructure.BusWidth = SDIO_BusWidth_1b;
        SDIO_InitStructure.HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
        SDIO_Init(&SDIO_InitStructure);
      }
    }
  }

  return(errorstatus);
}

/**
  * @brief  Selects od Deselects the corresponding card.
  * @param  addr: Address of the Card to be selected.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SelectDeselect(unsigned int addr) {
  SD_Error errorstatus = SD_OK;

  /*!< Send CMD7 SDIO_SEL_DESEL_CARD */
  SDIO_CmdInitStructure.Argument =  addr;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SEL_DESEL_CARD;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SEL_DESEL_CARD);

  return(errorstatus);
}

/**
  * @brief  Allows to read blocks from a specified address  in a card.  The Data
  *         transfer can be managed by DMA mode or Polling mode.
  * @note   This operation should be followed by two functions to check if the
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the
  *            data transfer and it is ready for data.
  * @param  readbuff: pointer to the buffer that will contain the received data.
  * @param  ReadAddr: Address from where data are to be read.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be read.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ReadMultiBlocks(unsigned char *readbuff, unsigned int ReadAddr, unsigned int BlockSize, unsigned int NumberOfBlocks) {
  SD_Error errorstatus = SD_OK;
  TransferError = SD_OK;
  TransferEnd = 0;
  StopCondition = 1;

  SDIO->DCTRL = 0x0;
  set_read_led;

  if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
    BlockSize = 512;
  else
    ReadAddr *= BlockSize; // Convert to Bytes for NON SDHC

  /*!< Set Block Size for Card */
  SDIO_CmdInitStructure.Argument = (unsigned int) BlockSize;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

  if (SD_OK != errorstatus) {
    return(errorstatus);
  }

  SDIO_DataInitStructure.DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.DataLength = NumberOfBlocks * BlockSize;
  SDIO_DataInitStructure.DataBlockSize = (unsigned int) 9 << 4;
  SDIO_DataInitStructure.TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);

  /*!< Send CMD18 READ_MULT_BLOCK with argument data address */
  SDIO_CmdInitStructure.Argument = (unsigned int)ReadAddr;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_READ_MULT_BLOCK;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_READ_MULT_BLOCK);

  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);
  SDIO_DMACmd(ENABLE);
  SD_LowLevel_DMA_RxConfig((unsigned int *)readbuff, (NumberOfBlocks * BlockSize));

  return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished.
  *         This function should be called after SDIO_ReadMultiBlocks() function
  *         to insure that all data sent by the card are already transferred by
  *         the DMA controller.
  * @param  None.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WaitReadOperation(void) {
  SD_Error errorstatus = SD_OK;
  unsigned int timeout;

  timeout = SD_DATATIMEOUT;

  while ((DMAEndOfTransfer == 0x00) && (TransferEnd == 0) && (TransferError == SD_OK) && (timeout > 0)) {
    timeout--;
  }

  DMAEndOfTransfer = 0x00;

  timeout = SD_DATATIMEOUT;

  while(((SDIO->STA & SDIO_FLAG_RXACT)) && (timeout > 0)) {
    timeout--;
  }

  if (StopCondition == 1) {
    errorstatus = SD_StopTransfer();
  }

  if ((timeout == 0) && (errorstatus == SD_OK)) {
    errorstatus = SD_DATA_TIMEOUT;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  if (TransferError != SD_OK) {
    return(TransferError);
  } else {
    return(errorstatus);
  }
}

/**
  * @brief  Allows to write blocks starting from a specified address in a card.
  *         The Data transfer can be managed by DMA mode only.
  * @note   This operation should be followed by two functions to check if the
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the
  *            data transfer and it is ready for data.
  * @param  WriteAddr: Address from where data are to be read.
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be written.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WriteMultiBlocks(unsigned char *writebuff, unsigned int WriteAddr, unsigned int BlockSize, unsigned int NumberOfBlocks) {
  SD_Error errorstatus = SD_OK;

  TransferError = SD_OK;
  TransferEnd = 0;
  StopCondition = 1;

  SDIO->DCTRL = 0x0;

  set_write_led;

  if (CardType == SDIO_HIGH_CAPACITY_SD_CARD)
    BlockSize = 512;
  else
    WriteAddr *= BlockSize; // Convert to Bytes for NON SDHC

  /* Set Block Size for Card */
  SDIO_CmdInitStructure.Argument = (unsigned int) BlockSize;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

  if (SD_OK != errorstatus) return(errorstatus);

  /*!< To improve performance */
  SDIO_CmdInitStructure.Argument = (unsigned int) (RCA << 16);
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

  if (errorstatus != SD_OK) return(errorstatus);
  /*!< To improve performance */
  SDIO_CmdInitStructure.Argument = (unsigned int)NumberOfBlocks;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_BLOCK_COUNT;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);

  if (errorstatus != SD_OK) return(errorstatus);

 /*!< Send CMD25 WRITE_MULT_BLOCK with argument data address */
  SDIO_CmdInitStructure.Argument = (unsigned int)WriteAddr;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);

  if (SD_OK != errorstatus) return(errorstatus);

  SDIO_DataInitStructure.DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.DataLength = NumberOfBlocks * BlockSize;
  SDIO_DataInitStructure.DataBlockSize = (unsigned int) 9 << 4;
  SDIO_DataInitStructure.TransferDir = SDIO_TransferDir_ToCard;
  SDIO_DataInitStructure.TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);

  SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);
  SDIO_DMACmd(ENABLE);
  SD_LowLevel_DMA_TxConfig((unsigned int *)writebuff, (NumberOfBlocks * BlockSize));

  return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished.
  *         This function should be called after SDIO_WriteBlock() and
  *         SDIO_WriteMultiBlocks() function to insure that all data sent by the
  *         card are already transferred by the DMA controller.
  * @param  None.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WaitWriteOperation(void) {
  SD_Error errorstatus = SD_OK;
  unsigned int timeout;

  timeout = SD_DATATIMEOUT;

  while ((DMAEndOfTransfer == 0x00) && (TransferEnd == 0) && (TransferError == SD_OK) && (timeout > 0)) {
    timeout--;
  }

  DMAEndOfTransfer = 0x00;

  timeout = SD_DATATIMEOUT;

  while(((SDIO->STA & SDIO_FLAG_TXACT)) && (timeout > 0)) {
    timeout--;
  }

  if (StopCondition == 1) {
    errorstatus = SD_StopTransfer();
  }

  if ((timeout == 0) && (errorstatus == SD_OK)) {
    errorstatus = SD_DATA_TIMEOUT;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  if (TransferError != SD_OK)
    return(TransferError);
  return(errorstatus);
}

/**
  * @brief  Gets the cuurent data transfer state.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be:
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetTransferState(void) {
  if (SDIO->STA & (SDIO_FLAG_TXACT | SDIO_FLAG_RXACT))
    return(SD_TRANSFER_BUSY);
  return(SD_TRANSFER_OK);
}

/**
  * @brief  Aborts an ongoing data transfer.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_StopTransfer(void) {
  /*!< Send CMD12 STOP_TRANSMISSION  */
  SDIO_CmdInitStructure.Argument = 0x0;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_STOP_TRANSMISSION;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  return CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
}

/**
  * @brief  Allows to erase memory area specified for the given card.
  * @param  startaddr: the start address.
  * @param  endaddr: the end address.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Erase(unsigned int startaddr, unsigned int endaddr) {
  SD_Error errorstatus = SD_OK;
  unsigned int delay = 0;
  volatile unsigned int maxdelay = 0;
  unsigned char cardstate = 0;

  set_write_led;
  /*!< Check if the card coomnd class supports erase command */
  if (((CSD_Tab[1] >> 20) & SD_CCCC_ERASE) == 0) {
    return SD_REQUEST_NOT_APPLICABLE;
  }

  maxdelay = 120000 / ((SDIO->CLKCR & 0xFF) + 2);

  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED) {
    return SD_LOCK_UNLOCK_FAILED;
  }

  if (CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
    startaddr /= 512;
    endaddr /= 512;
  }

  /*!< According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and erase_group_end(CMD33) */
  if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == CardType) ||
      (SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) ||
      (SDIO_HIGH_CAPACITY_SD_CARD == CardType)) {
    /*!< Send CMD32 SD_ERASE_GRP_START with argument as addr  */
    SDIO_CmdInitStructure.Argument = startaddr;
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_SD_ERASE_GRP_START;
    SDIO_CmdInitStructure.Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_START);
    if (errorstatus != SD_OK) {
      return(errorstatus);
    }

    /*!< Send CMD33 SD_ERASE_GRP_END with argument as addr  */
    SDIO_CmdInitStructure.Argument = endaddr;
    SDIO_CmdInitStructure.CmdIndex = SD_CMD_SD_ERASE_GRP_END;
    SDIO_CmdInitStructure.Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);

    errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_END);
    if (errorstatus != SD_OK) {
      return(errorstatus);
    }
  }

  /*!< Send CMD38 ERASE */
  SDIO_CmdInitStructure.Argument = 0;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_ERASE;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_ERASE);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  for (delay = 0; delay < maxdelay; delay++);

  /*!< Wait till the card is in programming state */
  errorstatus = IsCardProgramming(&cardstate);
  delay = SD_DATATIMEOUT;
  while ((delay > 0) && (errorstatus == SD_OK) && ((SD_CARD_PROGRAMMING == cardstate) || (SD_CARD_RECEIVING == cardstate))) {
    errorstatus = IsCardProgramming(&cardstate);
    delay--;
  }

  reset_write_led;
  return(errorstatus);
}

/**
  * @brief  Returns the current card's status.
  * @param  pcardstatus: pointer to the buffer that will contain the SD card
  *         status (Card Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendStatus(unsigned int *pcardstatus) {
  SD_Error errorstatus = SD_OK;

  if (pcardstatus == NULL) {
    return SD_INVALID_PARAMETER;
  }

  SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SEND_STATUS;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  *pcardstatus = SDIO_GetResponse(SDIO_RESP1);

  return(errorstatus);
}

/**
  * @brief  Returns the current SD card's status.
  * @param  psdstatus: pointer to the buffer that will contain the SD card status
  *         (SD Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendSDStatus(unsigned int *psdstatus) {
  SD_Error errorstatus = SD_OK;
  unsigned int count = 0;

  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED) {
    return SD_LOCK_UNLOCK_FAILED;
  }

  /*!< Set block size for card if it is not equal to current block size for card. */
  SDIO_CmdInitStructure.Argument = 64;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  /*!< CMD55 */
  SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  SDIO_DataInitStructure.DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.DataLength = 64;
  SDIO_DataInitStructure.DataBlockSize = SDIO_DataBlockSize_64b;
  SDIO_DataInitStructure.TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);

  /*!< Send ACMD13 SD_APP_STAUS  with argument as card's RCA.*/
  SDIO_CmdInitStructure.Argument = 0;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SD_APP_STAUS;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_SD_APP_STAUS);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  while (!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR))) {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET) {
      for (count = 0; count < 8; count++) {
        *(psdstatus + count) = SDIO_ReadData();
      }
      psdstatus += 8;
    }
  }

  if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
    return SD_DATA_TIMEOUT;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
    return SD_DATA_CRC_FAIL;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
    return SD_RX_OVERRUN;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
    return SD_START_BIT_ERR;
  }

  while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET) {
    *psdstatus = SDIO_ReadData();
    psdstatus++;
  }
  /*!< Clear all the static status flags*/
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return(errorstatus);
}

/**
  * @brief  Allows to process all the interrupts that are high.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ProcessIRQSrc(void) {
  if (SDIO_GetITStatus(SDIO_IT_DATAEND) != RESET) {
    TransferError = SD_OK;
    SDIO_ClearITPendingBit(SDIO_IT_DATAEND);
    TransferEnd = 1;
  }
  else if (SDIO_GetITStatus(SDIO_IT_DCRCFAIL) != RESET) {
    SDIO_ClearITPendingBit(SDIO_IT_DCRCFAIL);
    TransferError = SD_DATA_CRC_FAIL;
  }
  else if (SDIO_GetITStatus(SDIO_IT_DTIMEOUT) != RESET) {
    SDIO_ClearITPendingBit(SDIO_IT_DTIMEOUT);
    TransferError = SD_DATA_TIMEOUT;
  }
  else if (SDIO_GetITStatus(SDIO_IT_RXOVERR) != RESET) {
    SDIO_ClearITPendingBit(SDIO_IT_RXOVERR);
    TransferError = SD_RX_OVERRUN;
  }
  else if (SDIO_GetITStatus(SDIO_IT_TXUNDERR) != RESET) {
    SDIO_ClearITPendingBit(SDIO_IT_TXUNDERR);
    TransferError = SD_TX_UNDERRUN;
  }
  else if (SDIO_GetITStatus(SDIO_IT_STBITERR) != RESET) {
    SDIO_ClearITPendingBit(SDIO_IT_STBITERR);
    TransferError = SD_START_BIT_ERR;
  }

  reset_write_led;
  reset_read_led;
  SDIO_ITConfig(SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND |
                SDIO_IT_TXFIFOHE | SDIO_IT_RXFIFOHF | SDIO_IT_TXUNDERR |
                SDIO_IT_RXOVERR | SDIO_IT_STBITERR, DISABLE);
  return(TransferError);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished.
  * @param  None.
  * @retval None.
  */
void SD_ProcessDMAIRQ(void) {
  //if(DMA2->LISR & SD_SDIO_DMA_FLAG_TCIF) {
#if SD_SDIO_DMA_STREAM < 4
  if(DMA_LISR(SD_SDIO_DMA_PORT) & SD_SDIO_DMA_FLAG_TCIF) {
#else
  if(DMA_HISR(SD_SDIO_DMA_PORT) & SD_SDIO_DMA_FLAG_TCIF) {
#endif
    DMAEndOfTransfer = 0x01;
    //DMA_ClearFlag(SD_SDIO_DMA_STREAM, SD_SDIO_DMA_FLAG_TCIF|SD_SDIO_DMA_FLAG_FEIF);
#if SD_SDIO_DMA_STREAM < 4
    DMA_LIFCR(SD_SDIO_DMA_PORT) = (SD_SDIO_DMA_FLAG_TCIF|SD_SDIO_DMA_FLAG_FEIF);
#else
    DMA_HIFCR(SD_SDIO_DMA_PORT) = (SD_SDIO_DMA_FLAG_TCIF|SD_SDIO_DMA_FLAG_FEIF);
#endif
  }
}

/**
  * @brief  Checks for error conditions for CMD0.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdError(void) {
  unsigned int timeout;

  timeout = SDIO_CMD0TIMEOUT; /*!< 10000 */

  while ((timeout > 0) && (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) == RESET)) {
    timeout--;
  }

  if (timeout == 0) {
    return SD_CMD_RSP_TIMEOUT;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return (SD_Error) SD_OK;
}

/**
  * @brief  Checks for error conditions for R7 response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp7Error(void) {
  unsigned int status;
  unsigned int timeout = SDIO_CMD0TIMEOUT;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)) && (timeout > 0)) {
    timeout--;
    status = SDIO->STA;
  }

  if ((timeout == 0) || (status & SDIO_FLAG_CTIMEOUT)) {
    /*!< Card is not V2.0 complient or card does not support the set voltage range */
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }

  if (status & SDIO_FLAG_CMDREND) {
    /*!< Card is SD V2.0 compliant */
    SDIO_ClearFlag(SDIO_FLAG_CMDREND);
    return SD_OK;
  }
  return (SD_Error) SD_OK;
}

/**
  * @brief  Checks for error conditions for R1 response.
  * @param  cmd: The sent command index.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp1Error(unsigned char cmd) {
  unsigned int status;
  unsigned int response_r1;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))) {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }
  else if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
  }

  /*!< Check response received is of desired command */
  if (SDIO_GetCommandResponse() != cmd) {
    return SD_ILLEGAL_CMD;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  /*!< We have received response, retrieve it for analysis  */
  response_r1 = SDIO_GetResponse(SDIO_RESP1);

  if ((response_r1 & SD_OCR_ERRORBITS) == SD_ALLZERO) {
    return(SD_OK);
  }

  if (response_r1 & SD_OCR_ADDR_OUT_OF_RANGE) {
    return(SD_ADDR_OUT_OF_RANGE);
  }

  if (response_r1 & SD_OCR_ADDR_MISALIGNED) {
    return(SD_ADDR_MISALIGNED);
  }

  if (response_r1 & SD_OCR_BLOCK_LEN_ERR) {
    return(SD_BLOCK_LEN_ERR);
  }

  if (response_r1 & SD_OCR_ERASE_SEQ_ERR) {
    return(SD_ERASE_SEQ_ERR);
  }

  if (response_r1 & SD_OCR_BAD_ERASE_PARAM) {
    return(SD_BAD_ERASE_PARAM);
  }

  if (response_r1 & SD_OCR_WRITE_PROT_VIOLATION) {
    return(SD_WRITE_PROT_VIOLATION);
  }

  if (response_r1 & SD_OCR_LOCK_UNLOCK_FAILED) {
    return(SD_LOCK_UNLOCK_FAILED);
  }

  if (response_r1 & SD_OCR_COM_CRC_FAILED) {
    return(SD_COM_CRC_FAILED);
  }

  if (response_r1 & SD_OCR_ILLEGAL_CMD) {
    return(SD_ILLEGAL_CMD);
  }

  if (response_r1 & SD_OCR_CARD_ECC_FAILED) {
    return(SD_CARD_ECC_FAILED);
  }

  if (response_r1 & SD_OCR_CC_ERROR) {
    return(SD_CC_ERROR);
  }

  if (response_r1 & SD_OCR_GENERAL_UNKNOWN_ERROR) {
    return(SD_GENERAL_UNKNOWN_ERROR);
  }

  if (response_r1 & SD_OCR_STREAM_READ_UNDERRUN) {
    return(SD_STREAM_READ_UNDERRUN);
  }

  if (response_r1 & SD_OCR_STREAM_WRITE_OVERRUN) {
    return(SD_STREAM_WRITE_OVERRUN);
  }

  if (response_r1 & SD_OCR_CID_CSD_OVERWRIETE) {
    return(SD_CID_CSD_OVERWRITE);
  }

  if (response_r1 & SD_OCR_WP_ERASE_SKIP) {
    return(SD_WP_ERASE_SKIP);
  }

  if (response_r1 & SD_OCR_CARD_ECC_DISABLED) {
    return(SD_CARD_ECC_DISABLED);
  }

  if (response_r1 & SD_OCR_ERASE_RESET) {
    return(SD_ERASE_RESET);
  }

  if (response_r1 & SD_OCR_AKE_SEQ_ERROR) {
    return(SD_AKE_SEQ_ERROR);
  }
  return(SD_OK);
}

/**
  * @brief  Checks for error conditions for R3 (OCR) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp3Error(void) {
  unsigned int status;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))) {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }
  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);
  return(SD_OK);
}

/**
  * @brief  Checks for error conditions for R2 (CID or CSD) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp2Error(void) {
  unsigned int status;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND))) {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }
  else if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return(SD_OK);
}

/**
  * @brief  Checks for error conditions for R6 (RCA) response.
  * @param  cmd: The sent command index.
  * @param  prca: pointer to the variable that will contain the SD card relative
  *         address RCA.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp6Error(unsigned char cmd, unsigned short *prca) {
  unsigned int status;
  unsigned int response_r1;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND))) {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }
  else if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
  }

  /*!< Check response received is of desired command */
  if (SDIO_GetCommandResponse() != cmd) {
    return SD_ILLEGAL_CMD;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  /*!< We have received response, retrieve it.  */
  response_r1 = SDIO_GetResponse(SDIO_RESP1);

  if (SD_ALLZERO == (response_r1 & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED))) {
    *prca = (unsigned short) (response_r1 >> 16);
    return(SD_OK);
  }

  if (response_r1 & SD_R6_GENERAL_UNKNOWN_ERROR) {
    return(SD_GENERAL_UNKNOWN_ERROR);
  }

  if (response_r1 & SD_R6_ILLEGAL_CMD) {
    return(SD_ILLEGAL_CMD);
  }

  if (response_r1 & SD_R6_COM_CRC_FAILED) {
    return(SD_COM_CRC_FAILED);
  }

  return(SD_OK);
}

/**
  * @brief  Enables or disables the SDIO wide bus mode.
  * @param  NewState: new state of the SDIO wide bus mode.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error SDEnWideBus(FunctionalState NewState) {
  SD_Error errorstatus = SD_OK;

  unsigned int scr[2] = {0, 0};

  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED) {
    return SD_LOCK_UNLOCK_FAILED;
  }

  /*!< Get SCR Register */
  errorstatus = FindSCR(RCA, scr);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  /*!< If wide bus operation to be enabled */
  if (NewState == ENABLE) {
    /*!< If requested card supports wide bus operation */
    if ((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO) {
      /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
      SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus != SD_OK) {
        return(errorstatus);
      }

      /*!< Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
      SDIO_CmdInitStructure.Argument = 0x2;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
      if (errorstatus != SD_OK) {
        return(errorstatus);
      }
      return(errorstatus);
    }
    return SD_REQUEST_NOT_APPLICABLE;
  }   /*!< If wide bus operation to be disabled */
  else {
    /*!< If requested card supports 1 bit mode operation */
    if ((scr[1] & SD_SINGLE_BUS_SUPPORT) != SD_ALLZERO) {
      /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
      SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus != SD_OK) {
        return(errorstatus);
      }

      /*!< Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
      SDIO_CmdInitStructure.Argument = 0x00;
      SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);

      errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
      if (errorstatus != SD_OK) {
        return(errorstatus);
      }

      return(errorstatus);
    }
    return SD_REQUEST_NOT_APPLICABLE;
  }
}

/**
  * @brief  Checks if the SD card is in programming state.
  * @param  pstatus: pointer to the variable that will contain the SD card state.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error IsCardProgramming(unsigned char *pstatus) {
  volatile unsigned int respR1 = 0, status = 0;

  SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SEND_STATUS;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))) {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
  }
  else if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
  }

  status = (unsigned int)SDIO_GetCommandResponse();

  /*!< Check response received is of desired command */
  if (status != SD_CMD_SEND_STATUS) {
    return SD_ILLEGAL_CMD;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

 /*!< We have received response, retrieve it for analysis  */
  respR1 = SDIO_GetResponse(SDIO_RESP1);

  /*!< Find out card status */
  *pstatus = (unsigned char) ((respR1 >> 9) & 0x0000000F);

  if ((respR1 & SD_OCR_ERRORBITS) == SD_ALLZERO) {
    return(SD_OK);
  }

  if (respR1 & SD_OCR_ADDR_OUT_OF_RANGE) {
    return(SD_ADDR_OUT_OF_RANGE);
  }

  if (respR1 & SD_OCR_ADDR_MISALIGNED) {
    return(SD_ADDR_MISALIGNED);
  }

  if (respR1 & SD_OCR_BLOCK_LEN_ERR) {
    return(SD_BLOCK_LEN_ERR);
  }

  if (respR1 & SD_OCR_ERASE_SEQ_ERR) {
    return(SD_ERASE_SEQ_ERR);
  }

  if (respR1 & SD_OCR_BAD_ERASE_PARAM) {
    return(SD_BAD_ERASE_PARAM);
  }

  if (respR1 & SD_OCR_WRITE_PROT_VIOLATION) {
    return(SD_WRITE_PROT_VIOLATION);
  }

  if (respR1 & SD_OCR_LOCK_UNLOCK_FAILED) {
    return(SD_LOCK_UNLOCK_FAILED);
  }

  if (respR1 & SD_OCR_COM_CRC_FAILED) {
    return(SD_COM_CRC_FAILED);
  }

  if (respR1 & SD_OCR_ILLEGAL_CMD) {
    return(SD_ILLEGAL_CMD);
  }

  if (respR1 & SD_OCR_CARD_ECC_FAILED) {
    return(SD_CARD_ECC_FAILED);
  }

  if (respR1 & SD_OCR_CC_ERROR) {
    return(SD_CC_ERROR);
  }

  if (respR1 & SD_OCR_GENERAL_UNKNOWN_ERROR) {
    return(SD_GENERAL_UNKNOWN_ERROR);
  }

  if (respR1 & SD_OCR_STREAM_READ_UNDERRUN) {
    return(SD_STREAM_READ_UNDERRUN);
  }

  if (respR1 & SD_OCR_STREAM_WRITE_OVERRUN) {
    return(SD_STREAM_WRITE_OVERRUN);
  }

  if (respR1 & SD_OCR_CID_CSD_OVERWRIETE) {
    return(SD_CID_CSD_OVERWRITE);
  }

  if (respR1 & SD_OCR_WP_ERASE_SKIP) {
    return(SD_WP_ERASE_SKIP);
  }

  if (respR1 & SD_OCR_CARD_ECC_DISABLED) {
    return(SD_CARD_ECC_DISABLED);
  }

  if (respR1 & SD_OCR_ERASE_RESET) {
    return(SD_ERASE_RESET);
  }

  if (respR1 & SD_OCR_AKE_SEQ_ERROR) {
    return(SD_AKE_SEQ_ERROR);
  }

  return(SD_OK);
}

/**
  * @brief  Find the SD card SCR register value.
  * @param  rca: selected card address.
  * @param  pscr: pointer to the buffer that will contain the SCR value.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error FindSCR(unsigned short rca, unsigned int *pscr) {
  unsigned int index = 0;
  SD_Error errorstatus = SD_OK;
  unsigned int tempscr[2] = {0, 0};

  /*!< Set Block Size To 8 Bytes */
  /*!< Send CMD55 APP_CMD with argument as card's RCA */
  SDIO_CmdInitStructure.Argument = (unsigned int)8;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  /*!< Send CMD55 APP_CMD with argument as card's RCA */
  SDIO_CmdInitStructure.Argument = (unsigned int) RCA << 16;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }
  SDIO_DataInitStructure.DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.DataLength = 8;
  SDIO_DataInitStructure.DataBlockSize = SDIO_DataBlockSize_8b;
  SDIO_DataInitStructure.TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);


  /*!< Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
  SDIO_CmdInitStructure.Argument = 0x0;
  SDIO_CmdInitStructure.CmdIndex = SD_CMD_SD_APP_SEND_SCR;
  SDIO_CmdInitStructure.Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
  if (errorstatus != SD_OK) {
    return(errorstatus);
  }

  while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR))) {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET) {
      *(tempscr + index) = SDIO_ReadData();
      index++;
    }
  }

  if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
    return SD_DATA_TIMEOUT;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
    return SD_DATA_CRC_FAIL;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
    return SD_RX_OVERRUN;
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) {
    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
    return SD_START_BIT_ERR;
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  *(pscr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) << 8) | ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);

  *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8) | ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);

  return(errorstatus);
}

/**
  * @brief  Converts the number of bytes in power of two and returns the power.
  * @param  NumberOfBytes: number of bytes.
  * @retval None
  */
unsigned char convert_from_bytes_to_power_of_two(unsigned short NumberOfBytes) {
  unsigned char count = 0;

  while (NumberOfBytes != 1) {
    NumberOfBytes >>= 1;
    count++;
  }
  return(count);
}
