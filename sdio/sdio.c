/**
  ******************************************************************************
  * @file    sdio.c
  * @author  stf
  * @version V0.0.1
  * @date    01-December-2013
  * @brief   This file provides firmware functions to manage the following
  *          functionalities of the Secure digital input/output interface (SDIO)
  *          peripheral:
  *           - Initialization and Configuration
  *           - Command path state machine (CPSM) management
  *           - Data path state machine (DPSM) management
  *           - SDIO IO Cards mode management
  *           - CE-ATA mode management
  *           - DMA transfers management
  *           - Interrupts and flags management
  *
  *  @verbatim
  *
  *
  *          ===================================================================
  *                                 How to use this driver
  *          ===================================================================
  *          1. The SDIO clock (SDIOCLK = 48 MHz) is coming from a specific output
  *             of PLL (PLL48CLK). Before to start working with SDIO peripheral
  *             make sure that the PLL is well configured.
  *          The SDIO peripheral uses two clock signals:
  *              - SDIO adapter clock (SDIOCLK = 48 MHz)
  *              - APB2 bus clock (PCLK2)
  *          PCLK2 and SDIO_CK clock frequencies must respect the following condition:
  *                   Frequenc(PCLK2) >= (3 / 8 x Frequency(SDIO_CK))
  *
  *          2. Enable peripheral clock RCC_APB2PeriphClock_ENR |= RCC_APB2Periph_SDIO
  *
  *          3.  According to the SDIO mode, enable the GPIO clocks using
  *              RCC_AHB1PeriphClockCmd() function.
  *              The I/O can be one of the following configurations:
  *                 - 1-bit data length: SDIO_CMD, SDIO_CK and D0.
  *                 - 4-bit data length: SDIO_CMD, SDIO_CK and D[3:0].
  *                 - 8-bit data length: SDIO_CMD, SDIO_CK and D[7:0].
  *
  *          4. Peripheral's alternate function:
  *                 - Connect the pin to the desired peripherals' Alternate
  *                   Function (AF) using GPIO_PinAFConfig() function
  *                 - Configure the desired pin in alternate function by:
  *                   args->GPIO_Mode = GPIO_Mode_AF
  *                 - Select the type, pull-up/pull-down and output speed via
  *                   GPIO_PuPd, GPIO_OType and GPIO_Speed members
  *                 - Call GPIO_Init() function
  *
  *          5. Program the Clock Edge, Clock Bypass, Clock Power Save, Bus Width,
  *             hardware, flow control and the Clock Divider using the SDIO_Init()
  *             function.
  *
  *          6. Enable the Power ON State using the SDIO_SetPowerState(SDIO_PowerState_ON)
  *             function.
  *
  *          7. Enable the clock using the SDIO_ClockCmd() function.
  *
  *          8. Enable the NVIC and the corresponding interrupt using the function
  *             SDIO_ITConfig() if you need to use interrupt mode.
  *
  *          9. When using the DMA mode
  *                   - Configure the DMA using DMA_Init() function
  *                   - Active the needed channel Request using SDIO_DMACmd() function
  *
  *          10. Enable the DMA using the DMA_Cmd() function, when using DMA mode.
  *
  *          11. To control the CPSM (Command Path State Machine) and send
  *              commands to the card use the SDIO_SendCommand(),
  *              SDIO_GetCommandResponse() and SDIO_GetResponse() functions.
  *              First, user has to fill the command structure (pointer to
  *              SDIO_CmdInitArgs) according to the selected command to be sent.
  *                 The parameters that should be filled are:
  *                   - Command Argument
  *                   - Command Index
  *                   - Command Response type
  *                   - Command Wait
  *                   - CPSM Status (Enable or Disable)
  *
  *              To check if the command is well received, read the SDIO_CMDRESP
  *              register using the SDIO_GetCommandResponse().
  *              The SDIO responses registers (SDIO_RESP1 to SDIO_RESP2), use the
  *              SDIO_GetResponse() function.
  *
  *          12. To control the DPSM (Data Path State Machine) and send/receive
  *              data to/from the card use the SDIO_DataConfig(), SDIO_GetDataCounter(),
  *              SDIO_ReadData(), SDIO_WriteData() and SDIO_GetFIFOCount() functions.
  *
  *              Read Operations
  *              ---------------
  *              a) First, user has to fill the data structure (pointer to
  *                 SDIO_DataInitArgs) according to the selected data type to
  *                 be received.
  *                 The parameters that should be filled are:
  *                   - Data TimeOut
  *                   - Data Length
  *                   - Data Block size
  *                   - Data Transfer direction: should be from card (To SDIO)
  *                   - Data Transfer mode
  *                   - DPSM Status (Enable or Disable)
  *
  *              b) Configure the SDIO resources to receive the data from the card
  *                 according to selected transfer mode (Refer to Step 8, 9 and 10).
  *
  *              c) Send the selected Read command (refer to step 11).
  *
  *              d) Use the SDIO flags/interrupts to check the transfer status.
  *
  *              Write Operations
  *              ---------------
  *              a) First, user has to fill the data structure (pointer to
  *                 SDIO_DataInitArgs) according to the selected data type to
  *                 be received.
  *                 The parameters that should be filled are:
  *                   - Data TimeOut
  *                   - Data Length
  *                   - Data Block size
  *                   - Data Transfer direction:  should be to card (To CARD)
  *                   - Data Transfer mode
  *                   - DPSM Status (Enable or Disable)
  *
  *              b) Configure the SDIO resources to send the data to the card
  *                 according to selected transfer mode (Refer to Step 8, 9 and 10).
  *
  *              c) Send the selected Write command (refer to step 11).
  *
  *              d) Use the SDIO flags/interrupts to check the transfer status.
  *
  *
  *  @endverbatim
  */

#include "stm32f.h"
#include "sdio.h"
#include "dma.h"

/* ------------ SDIO registers bit address in the alias region ----------- */
#define SDIO_OFFSET                (SDIO_BASE - PERIPH_BASE)

/* --- CLKCR Register ---*/
/* Alias word address of CLKEN bit */
#define CLKCR_OFFSET              (SDIO_OFFSET + 0x04)
#define CLKEN_BitNumber           0x08
#define CLKCR_CLKEN_BB            (PERIPH_BB_BASE + (CLKCR_OFFSET * 32) + (CLKEN_BitNumber * 4))

/* --- CMD Register ---*/
/* Alias word address of SDIOSUSPEND bit */
#define CMD_OFFSET                (SDIO_OFFSET + 0x0C)
#define SDIOSUSPEND_BitNumber     0x0B
#define CMD_SDIOSUSPEND_BB        (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (SDIOSUSPEND_BitNumber * 4))

/* Alias word address of ENCMDCOMPL bit */
#define ENCMDCOMPL_BitNumber      0x0C
#define CMD_ENCMDCOMPL_BB         (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (ENCMDCOMPL_BitNumber * 4))

/* Alias word address of NIEN bit */
#define NIEN_BitNumber            0x0D
#define CMD_NIEN_BB               (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (NIEN_BitNumber * 4))

/* Alias word address of ATACMD bit */
#define ATACMD_BitNumber          0x0E
#define CMD_ATACMD_BB             (PERIPH_BB_BASE + (CMD_OFFSET * 32) + (ATACMD_BitNumber * 4))

/* --- DCTRL Register ---*/
/* Alias word address of DMAEN bit */
#define DCTRL_OFFSET              (SDIO_OFFSET + 0x2C)
#define DMAEN_BitNumber           0x03
#define DCTRL_DMAEN_BB            (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (DMAEN_BitNumber * 4))

/* Alias word address of RWSTART bit */
#define RWSTART_BitNumber         0x08
#define DCTRL_RWSTART_BB          (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWSTART_BitNumber * 4))

/* Alias word address of RWSTOP bit */
#define RWSTOP_BitNumber          0x09
#define DCTRL_RWSTOP_BB           (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWSTOP_BitNumber * 4))

/* Alias word address of RWMOD bit */
#define RWMOD_BitNumber           0x0A
#define DCTRL_RWMOD_BB            (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (RWMOD_BitNumber * 4))

/* Alias word address of SDIOEN bit */
#define SDIOEN_BitNumber          0x0B
#define DCTRL_SDIOEN_BB           (PERIPH_BB_BASE + (DCTRL_OFFSET * 32) + (SDIOEN_BitNumber * 4))

/* ---------------------- SDIO registers bit mask ------------------------ */
/* --- CLKCR Register ---*/
/* CLKCR register clear mask */
#define CLKCR_CLEAR_MASK         ((unsigned int)0xFFFF8100)

/* --- PWRCTRL Register ---*/
/* SDIO PWRCTRL Mask */
#define PWR_PWRCTRL_MASK         ((unsigned int)0xFFFFFFFC)

/* --- DCTRL Register ---*/
/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK         ((unsigned int)0xFFFFFF08)

/* --- CMD Register ---*/
/* CMD Register clear mask */
#define CMD_CLEAR_MASK           ((unsigned int)0xFFFFF800)

/* SDIO RESP Registers Address */
#define SDIO_RESP_ADDR           ((unsigned int)(SDIO_BASE + 0x14))

/** @defgroup SDIO_Group1 Initialization and Configuration functions
 *  @brief   Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
                 Initialization and Configuration functions
 ===============================================================================

@endverbatim
  * @{
  */

/**
  * @brief  Deinitializes the SDIO peripheral registers to their default reset values.
  * @param  None
  * @retval None
  */
void SDIO_DeInit(void) {
  RCC->APB2RSTR |= RCC_APB2Periph_SDIO;  // disable sdio clock
  RCC->APB2RSTR &= ~((unsigned int) RCC_APB2Periph_SDIO); // re-enable sdio clock
}

/**
  * @brief  Initializes the SDIO peripheral according to the specified
  *         parameters in the args.
  * @param  args : pointer to a SDIO_InitArgs structure
  *         that contains the configuration information for the SDIO peripheral.
  * @retval None
  */
void SDIO_Init(SDIO_InitArgs* args) {
  unsigned int tmpreg = 0;
/*---------------------------- SDIO CLKCR Configuration ------------------------*/
  /* Get the SDIO CLKCR value */
  tmpreg = SDIO->CLKCR;
  tmpreg &= CLKCR_CLEAR_MASK;
  tmpreg |= (args->ClockDiv  | args->ClockPowerSave |
             args->ClockBypass | args->BusWidth |
             args->ClockEdge | args->HardwareFlowControl);
  SDIO->CLKCR = tmpreg;
}

/**
  * @brief  Fills each args member with its default value.
  * @param  args: pointer to an SDIO_InitArgs structure which
  *         will be initialized.
  * @retval None
  */
void SDIO_StructInit(SDIO_InitArgs* args) {
  /* args members default value */
  args->ClockDiv = 0x00;
  args->ClockEdge = SDIO_ClockEdge_Rising;
  args->ClockBypass = SDIO_ClockBypass_Disable;
  args->ClockPowerSave = SDIO_ClockPowerSave_Disable;
  args->BusWidth = SDIO_BusWidth_1b;
  args->HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
}

/**
  * @brief  Enables or disables the SDIO Clock.
  * @param  state: new state of the SDIO Clock.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_ClockCmd(FunctionalState state) {
  MMIO32(CLKCR_CLKEN_BB) = (unsigned int)state;
}

/**
  * @brief  Sets the power status of the controller.
  * @param  SDIO_PowerState: new state of the Power state.
  *          This parameter can be one of the following values:
  *            @arg SDIO_PowerState_OFF: SDIO Power OFF
  *            @arg SDIO_PowerState_ON: SDIO Power ON
  * @retval None
  */
void SDIO_SetPowerState(unsigned int PowerState) {
  SDIO->POWER &= PWR_PWRCTRL_MASK;
  SDIO->POWER |= PowerState;
}

/**
  * @brief  Gets the power status of the controller.
  * @param  None
  * @retval Power status of the controller. The returned value can be one of the
  *         following values:
  *            - 0x00: Power OFF
  *            - 0x02: Power UP
  *            - 0x03: Power ON
  */
unsigned int SDIO_GetPowerState(void) {
  return (SDIO->POWER & (~PWR_PWRCTRL_MASK));
}

/** @defgroup SDIO_Group2 Command path state machine (CPSM) management functions
 *  @brief   Command path state machine (CPSM) management functions
 *
@verbatim
 ===============================================================================
              Command path state machine (CPSM) management functions
 ===============================================================================

  This section provide functions allowing to program and read the Command path
  state machine (CPSM).

@endverbatim
  */

/**
  * @brief  Initializes the SDIO Command according to the specified
  *         parameters in the args and send the command.
  * @param  args : pointer to a SDIO_CmdInitArgs
  *         structure that contains the configuration information for the SDIO
  *         command.
  * @retval None
  */
void SDIO_SendCommand(SDIO_CmdInitArgs *args) {
  unsigned int tmpreg = 0;
  /* Set the SDIO Argument value */
  SDIO->ARG = args->Argument;
  /* Get the SDIO CMD value */
  tmpreg = SDIO->CMD;
  /* Clear CMDINDEX, WAITRESP, WAITINT, WAITPEND, CPSMEN bits */
  tmpreg &= CMD_CLEAR_MASK;
  tmpreg |= (unsigned int)args->CmdIndex | args->Response
           | args->Wait | args->CPSM;
  /* Write to SDIO CMD */
  SDIO->CMD = tmpreg;
}

/**
  * @brief  Fills each args member with its default value.
  * @param  args: pointer to an SDIO_CmdInitArgs
  *         structure which will be initialized.
  * @retval None
  */
void SDIO_CmdStructInit(SDIO_CmdInitArgs* args) {
  /* args members default value */
  args->Argument = 0x00;
  args->CmdIndex = 0x00;
  args->Response = SDIO_Response_No;
  args->Wait = SDIO_Wait_No;
  args->CPSM = SDIO_CPSM_Disable;
}

/**
  * @brief  Returns command index of last command for which response received.
  * @param  None
  * @retval Returns the command index of the last command response received.
  */
unsigned char SDIO_GetCommandResponse(void) {
  return (unsigned char)(SDIO->RESPCMD);
}

/**
  * @brief  Returns response received from the card for the last command.
  * @param  SDIO_RESP: Specifies the SDIO response register.
  *          This parameter can be one of the following values:
  *            @arg SDIO_RESP1: Response Register 1
  *            @arg SDIO_RESP2: Response Register 2
  *            @arg SDIO_RESP3: Response Register 3
  *            @arg SDIO_RESP4: Response Register 4
  * @retval The Corresponding response register value.
  */
unsigned int SDIO_GetResponse(unsigned int SDIO_RESP) {
  return MMIO32(SDIO_RESP_ADDR + SDIO_RESP);
}

/** @defgroup SDIO_Group3 Data path state machine (DPSM) management functions
 *  @brief   Data path state machine (DPSM) management functions
 *
 ===============================================================================
              Data path state machine (DPSM) management functions
 ===============================================================================

  This section provide functions allowing to program and read the Data path
  state machine (DPSM).
  */

/**
  * @brief  Initializes the SDIO data path according to the specified
  *         parameters in the args.
  * @param  args : pointer to a SDIO_DataInitArgs structure
  *         that contains the configuration information for the SDIO command.
  * @retval None
  */
void SDIO_DataConfig(SDIO_DataInitArgs* args) {
  unsigned int tmpreg = 0;
  /* Set the SDIO Data TimeOut value */
  SDIO->DTIMER = args->DataTimeOut;
  /* Set the SDIO DataLength value */
  SDIO->DLEN = args->DataLength;
  /* Get the SDIO DCTRL value */
  tmpreg = SDIO->DCTRL;
  /* Clear DEN, DTMODE, DTDIR and DBCKSIZE bits */
  tmpreg &= DCTRL_CLEAR_MASK;
  tmpreg |= (unsigned int)args->DataBlockSize | args->TransferDir
           | args->TransferMode | args->DPSM;
  /* Write to SDIO DCTRL */
  SDIO->DCTRL = tmpreg;
}

/**
  * @brief  Fills each args member with its default value.
  * @param  args: pointer to an SDIO_DataInitArgs structure
  *         which will be initialized.
  * @retval None
  */
void SDIO_DataStructInit(SDIO_DataInitArgs* args) {
  /* args members default value */
  args->DataTimeOut = 0xFFFFFFFF;
  args->DataLength = 0x00;
  args->DataBlockSize = SDIO_DataBlockSize_1b;
  args->TransferDir = SDIO_TransferDir_ToCard;
  args->TransferMode = SDIO_TransferMode_Block;
  args->DPSM = SDIO_DPSM_Disable;
}

/**
  * @brief  Returns number of remaining data bytes to be transferred.
  * @param  None
  * @retval Number of remaining data bytes to be transferred
  */
unsigned int SDIO_GetDataCounter(void) {
  return SDIO->DCOUNT;
}

/**
  * @brief  Read one data word from Rx FIFO.
  * @param  None
  * @retval Data received
  */
unsigned int SDIO_ReadData(void) {
  return SDIO->FIFO;
}

/**
  * @brief  Write one data word to Tx FIFO.
  * @param  Data: 32-bit data word to write.
  * @retval None
  */
void SDIO_WriteData(unsigned int Data) {
  SDIO->FIFO = Data;
}

/**
  * @brief  Returns the number of words left to be written to or read from FIFO.
  * @param  None
  * @retval Remaining number of words.
  */
unsigned int SDIO_GetFIFOCount(void) {
  return SDIO->FIFOCNT;
}

/** @defgroup SDIO_Group4 SDIO IO Cards mode management functions
 *  @brief   SDIO IO Cards mode management functions
 *
 ===============================================================================
              SDIO IO Cards mode management functions
 ===============================================================================

  This section provide functions allowing to program and read the SDIO IO Cards.
  */

/**
  * @brief  Starts the SD I/O Read Wait operation.
  * @param  state: new state of the Start SDIO Read Wait operation.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_StartSDIOReadWait(FunctionalState state) {
  MMIO32(DCTRL_RWSTART_BB) = (unsigned int) state;
}

/**
  * @brief  Stops the SD I/O Read Wait operation.
  * @param  state: new state of the Stop SDIO Read Wait operation.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_StopSDIOReadWait(FunctionalState state) {
  MMIO32(DCTRL_RWSTOP_BB) = (unsigned int) state;
}

/**
  * @brief  Sets one of the two options of inserting read wait interval.
  * @param  SDIO_ReadWaitMode: SD I/O Read Wait operation mode.
  *          This parameter can be:
  *            @arg SDIO_ReadWaitMode_CLK: Read Wait control by stopping SDIOCLK
  *            @arg SDIO_ReadWaitMode_DATA2: Read Wait control using SDIO_DATA2
  * @retval None
  */
void SDIO_SetSDIOReadWaitMode(unsigned int SDIO_ReadWaitMode) {
  MMIO32(DCTRL_RWMOD_BB) = SDIO_ReadWaitMode;
}

/**
  * @brief  Enables or disables the SD I/O Mode Operation.
  * @param  state: new state of SDIO specific operation.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_SetSDIOOperation(FunctionalState state) {
  MMIO32(DCTRL_SDIOEN_BB) = (unsigned int)state;
}

/**
  * @brief  Enables or disables the SD I/O Mode suspend command sending.
  * @param  state: new state of the SD I/O Mode suspend command.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_SendSDIOSuspendCmd(FunctionalState state) {
  MMIO32(CMD_SDIOSUSPEND_BB) = (unsigned int)state;
}

/** @defgroup SDIO_Group5 CE-ATA mode management functions
 *  @brief   CE-ATA mode management functions
 *
 ===============================================================================
              CE-ATA mode management functions
 ===============================================================================

  This section provide functions allowing to program and read the CE-ATA card.

  */

/**
  * @brief  Enables or disables the command completion signal.
  * @param  state: new state of command completion signal.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_CommandCompletionCmd(FunctionalState state) {
  MMIO32(CMD_ENCMDCOMPL_BB) = (unsigned int)state;
}

/**
  * @brief  Enables or disables the CE-ATA interrupt.
  * @param  state: new state of CE-ATA interrupt.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_CEATAITCmd(FunctionalState state) {
  MMIO32(CMD_NIEN_BB) = (unsigned int)((~((unsigned int)state)) & ((unsigned int)0x1));
}

/**
  * @brief  Sends CE-ATA command (CMD61).
  * @param  state: new state of CE-ATA command.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_SendCEATACmd(FunctionalState state) {
  MMIO32(CMD_ATACMD_BB) = (unsigned int)state;
}

/** @defgroup SDIO_Group6 DMA transfers management functions
 *  @brief   DMA transfers management functions
 *
 ===============================================================================
              DMA transfers management functions
 ===============================================================================

  This section provide functions allowing to program SDIO DMA transfer.

  */

/**
  * @brief  Enables or disables the SDIO DMA request.
  * @param  state: new state of the selected SDIO DMA request.
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_DMACmd(FunctionalState state) {
  MMIO32(DCTRL_DMAEN_BB) = (unsigned int)state;
}

/** @defgroup SDIO_Group7 Interrupts and flags management functions
 *  @brief   Interrupts and flags management functions
 *
 ===============================================================================
                       Interrupts and flags management functions
 ===============================================================================

  */

/**
  * @brief  Enables or disables the SDIO interrupts.
  * @param  SDIO_IT: specifies the SDIO interrupt sources to be enabled or disabled.
  *          This parameter can be one or a combination of the following values:
  *            @arg SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
  *            @arg SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
  *            @arg SDIO_IT_CTIMEOUT: Command response timeout interrupt
  *            @arg SDIO_IT_DTIMEOUT: Data timeout interrupt
  *            @arg SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
  *            @arg SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
  *            @arg SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
  *            @arg SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
  *            @arg SDIO_IT_DATAEND:  Data end (data counter, SDIDCOUNT, is zero) interrupt
  *            @arg SDIO_IT_STBITERR: Start bit not detected on all data signals in width
  *                                   bus mode interrupt
  *            @arg SDIO_IT_DBCKEND:  Data block sent/received (CRC check passed) interrupt
  *            @arg SDIO_IT_CMDACT:   Command transfer in progress interrupt
  *            @arg SDIO_IT_TXACT:    Data transmit in progress interrupt
  *            @arg SDIO_IT_RXACT:    Data receive in progress interrupt
  *            @arg SDIO_IT_TXFIFOHE: Transmit FIFO Half Empty interrupt
  *            @arg SDIO_IT_RXFIFOHF: Receive FIFO Half Full interrupt
  *            @arg SDIO_IT_TXFIFOF:  Transmit FIFO full interrupt
  *            @arg SDIO_IT_RXFIFOF:  Receive FIFO full interrupt
  *            @arg SDIO_IT_TXFIFOE:  Transmit FIFO empty interrupt
  *            @arg SDIO_IT_RXFIFOE:  Receive FIFO empty interrupt
  *            @arg SDIO_IT_TXDAVL:   Data available in transmit FIFO interrupt
  *            @arg SDIO_IT_RXDAVL:   Data available in receive FIFO interrupt
  *            @arg SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
  *            @arg SDIO_IT_CEATAEND: CE-ATA command completion signal received for CMD61 interrupt
  * @param  state: new state of the specified SDIO interrupts.
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SDIO_ITConfig(unsigned int SDIO_IT, FunctionalState state) {
  if (state != DISABLE) {
    /* Enable the SDIO interrupts */
    SDIO->MASK |= SDIO_IT;
  } else {
    /* Disable the SDIO interrupts */
    SDIO->MASK &= ~SDIO_IT;
  }
}

/**
  * @brief  Checks whether the specified SDIO flag is set or not.
  * @param  SDIO_FLAG: specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg SDIO_FLAG_CCRCFAIL: Command response received (CRC check failed)
  *            @arg SDIO_FLAG_DCRCFAIL: Data block sent/received (CRC check failed)
  *            @arg SDIO_FLAG_CTIMEOUT: Command response timeout
  *            @arg SDIO_FLAG_DTIMEOUT: Data timeout
  *            @arg SDIO_FLAG_TXUNDERR: Transmit FIFO underrun error
  *            @arg SDIO_FLAG_RXOVERR:  Received FIFO overrun error
  *            @arg SDIO_FLAG_CMDREND:  Command response received (CRC check passed)
  *            @arg SDIO_FLAG_CMDSENT:  Command sent (no response required)
  *            @arg SDIO_FLAG_DATAEND:  Data end (data counter, SDIDCOUNT, is zero)
  *            @arg SDIO_FLAG_STBITERR: Start bit not detected on all data signals in width bus mode.
  *            @arg SDIO_FLAG_DBCKEND:  Data block sent/received (CRC check passed)
  *            @arg SDIO_FLAG_CMDACT:   Command transfer in progress
  *            @arg SDIO_FLAG_TXACT:    Data transmit in progress
  *            @arg SDIO_FLAG_RXACT:    Data receive in progress
  *            @arg SDIO_FLAG_TXFIFOHE: Transmit FIFO Half Empty
  *            @arg SDIO_FLAG_RXFIFOHF: Receive FIFO Half Full
  *            @arg SDIO_FLAG_TXFIFOF:  Transmit FIFO full
  *            @arg SDIO_FLAG_RXFIFOF:  Receive FIFO full
  *            @arg SDIO_FLAG_TXFIFOE:  Transmit FIFO empty
  *            @arg SDIO_FLAG_RXFIFOE:  Receive FIFO empty
  *            @arg SDIO_FLAG_TXDAVL:   Data available in transmit FIFO
  *            @arg SDIO_FLAG_RXDAVL:   Data available in receive FIFO
  *            @arg SDIO_FLAG_SDIOIT:   SD I/O interrupt received
  *            @arg SDIO_FLAG_CEATAEND: CE-ATA command completion signal received for CMD61
  * @retval The new state of SDIO_FLAG (SET or RESET).
  */
FlagStatus SDIO_GetFlagStatus(unsigned int SDIO_FLAG) {
  return ((SDIO->STA & SDIO_FLAG) != (unsigned int)RESET);
}

/**
  * @brief  Clears the SDIO's pending flags.
  * @param  SDIO_FLAG: specifies the flag to clear.
  *          This parameter can be one or a combination of the following values:
  *            @arg SDIO_FLAG_CCRCFAIL: Command response received (CRC check failed)
  *            @arg SDIO_FLAG_DCRCFAIL: Data block sent/received (CRC check failed)
  *            @arg SDIO_FLAG_CTIMEOUT: Command response timeout
  *            @arg SDIO_FLAG_DTIMEOUT: Data timeout
  *            @arg SDIO_FLAG_TXUNDERR: Transmit FIFO underrun error
  *            @arg SDIO_FLAG_RXOVERR:  Received FIFO overrun error
  *            @arg SDIO_FLAG_CMDREND:  Command response received (CRC check passed)
  *            @arg SDIO_FLAG_CMDSENT:  Command sent (no response required)
  *            @arg SDIO_FLAG_DATAEND:  Data end (data counter, SDIDCOUNT, is zero)
  *            @arg SDIO_FLAG_STBITERR: Start bit not detected on all data signals in width bus mode
  *            @arg SDIO_FLAG_DBCKEND:  Data block sent/received (CRC check passed)
  *            @arg SDIO_FLAG_SDIOIT:   SD I/O interrupt received
  *            @arg SDIO_FLAG_CEATAEND: CE-ATA command completion signal received for CMD61
  * @retval None
  */
void SDIO_ClearFlag(unsigned int SDIO_FLAG) {
  SDIO->ICR = SDIO_FLAG;
}

/**
  * @brief  Checks whether the specified SDIO interrupt has occurred or not.
  * @param  SDIO_IT: specifies the SDIO interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
  *            @arg SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
  *            @arg SDIO_IT_CTIMEOUT: Command response timeout interrupt
  *            @arg SDIO_IT_DTIMEOUT: Data timeout interrupt
  *            @arg SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
  *            @arg SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
  *            @arg SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
  *            @arg SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
  *            @arg SDIO_IT_DATAEND:  Data end (data counter, SDIDCOUNT, is zero) interrupt
  *            @arg SDIO_IT_STBITERR: Start bit not detected on all data signals in width
  *                                   bus mode interrupt
  *            @arg SDIO_IT_DBCKEND:  Data block sent/received (CRC check passed) interrupt
  *            @arg SDIO_IT_CMDACT:   Command transfer in progress interrupt
  *            @arg SDIO_IT_TXACT:    Data transmit in progress interrupt
  *            @arg SDIO_IT_RXACT:    Data receive in progress interrupt
  *            @arg SDIO_IT_TXFIFOHE: Transmit FIFO Half Empty interrupt
  *            @arg SDIO_IT_RXFIFOHF: Receive FIFO Half Full interrupt
  *            @arg SDIO_IT_TXFIFOF:  Transmit FIFO full interrupt
  *            @arg SDIO_IT_RXFIFOF:  Receive FIFO full interrupt
  *            @arg SDIO_IT_TXFIFOE:  Transmit FIFO empty interrupt
  *            @arg SDIO_IT_RXFIFOE:  Receive FIFO empty interrupt
  *            @arg SDIO_IT_TXDAVL:   Data available in transmit FIFO interrupt
  *            @arg SDIO_IT_RXDAVL:   Data available in receive FIFO interrupt
  *            @arg SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
  *            @arg SDIO_IT_CEATAEND: CE-ATA command completion signal received for CMD61 interrupt
  * @retval The new state of SDIO_IT (SET or RESET).
  */
ITStatus SDIO_GetITStatus(unsigned int SDIO_IT) {
  return ((SDIO->STA & SDIO_IT) != (unsigned int)RESET);
}

/**
  * @brief  Clears the SDIO's interrupt pending bits.
  * @param  SDIO_IT: specifies the interrupt pending bit to clear.
  *          This parameter can be one or a combination of the following values:
  *            @arg SDIO_IT_CCRCFAIL: Command response received (CRC check failed) interrupt
  *            @arg SDIO_IT_DCRCFAIL: Data block sent/received (CRC check failed) interrupt
  *            @arg SDIO_IT_CTIMEOUT: Command response timeout interrupt
  *            @arg SDIO_IT_DTIMEOUT: Data timeout interrupt
  *            @arg SDIO_IT_TXUNDERR: Transmit FIFO underrun error interrupt
  *            @arg SDIO_IT_RXOVERR:  Received FIFO overrun error interrupt
  *            @arg SDIO_IT_CMDREND:  Command response received (CRC check passed) interrupt
  *            @arg SDIO_IT_CMDSENT:  Command sent (no response required) interrupt
  *            @arg SDIO_IT_DATAEND:  Data end (data counter, SDIO_DCOUNT, is zero) interrupt
  *            @arg SDIO_IT_STBITERR: Start bit not detected on all data signals in width
  *                                   bus mode interrupt
  *            @arg SDIO_IT_SDIOIT:   SD I/O interrupt received interrupt
  *            @arg SDIO_IT_CEATAEND: CE-ATA command completion signal received for CMD61
  * @retval None
  */
void SDIO_ClearITPendingBit(unsigned int SDIO_IT) {
  SDIO->ICR = SDIO_IT;
}

/**
  * @brief  DeInitializes the SDIO interface.
  * @param  None
  * @retval None
  */
void SD_LowLevel_DeInit(void) {
  GPIO_Regs *greg;

  /*!< Disable SDIO Clock */
  SDIO_ClockCmd(DISABLE);

  /*!< Set Power State to OFF */
  SDIO_SetPowerState(SDIO_PowerState_OFF);

  /*!< DeInitializes the SDIO peripheral */
  SDIO_DeInit();

  /* disable sdio clock */
  RCC->APB2ENR &= ~RCC_APB2Periph_SDIO;

  /* Configure PC.08, PC.09, PC.10, PC.11 pins: D0, D1, D2, D3 pins */
  greg = (GPIO_Regs *) GPIOC_BASE;
  greg->AFR[1] &= ~((unsigned int)((7 << (0 << 2)) |
                                   (7 << (1 << 2)) |
                                   (7 << (2 << 2)) |
                                   (7 << (3 << 2)) |
                                   (7 << (4 << 2))));
  greg->MODER &= ~((unsigned int )((3 << (8 << 1)) |
                                   (3 << (9 << 1)) |
                                   (3 << (10 << 1)) |
                                   (3 << (11 << 1))));
  greg->PUPDR &= ~((unsigned int )((3 << (8 << 1)) |
                                   (3 << (9 << 1)) |
                                   (3 << (10 << 1)) |
                                   (3 << (11 << 1))));

  /* Configure PD.02 CMD line */
  greg = (GPIO_Regs *) GPIOD_BASE;
  greg->AFR[0] &= ~((unsigned int )((7 << (2 << 2))));
  greg->MODER &= ~((unsigned int )((3 << (2 << 1))));
  greg->PUPDR &= ~((unsigned int )((3 << (2 << 1))));

  /* Configure PC.12 pin: CLK pin */
  greg = (GPIO_Regs *) GPIOC_BASE;
  greg->MODER &= ~((unsigned int )((3 << (12 << 1))));
  greg->PUPDR &= ~((unsigned int )((3 << (12 << 1))));
}

/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for
  *         data transfer).
  * @param  None
  * @retval None
  */
void SD_LowLevel_Init(void) {
  GPIO_Regs *greg;

  /* GPIOC and GPIOD Periph clock enable */
  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | SD_DETECT_GPIO_CLK;

  greg = (GPIO_Regs *) GPIOC_BASE;
  greg->AFR[1] |= (GPIO_AF_SDIO << (0 << 2)) |
                   (GPIO_AF_SDIO << (1 << 2)) |
                   (GPIO_AF_SDIO << (2 << 2)) |
                   (GPIO_AF_SDIO << (3 << 2)) |
                   (GPIO_AF_SDIO << (4 << 2));

  /* Configure PC.08, PC.09, PC.10, PC.11 pins: D0, D1, D2, D3 pins */
  greg->MODER |= (GPIO_Mode_AF << (8 << 1)) |
                 (GPIO_Mode_AF << (9 << 1)) |
                 (GPIO_Mode_AF << (10 << 1)) |
                 (GPIO_Mode_AF << (11 << 1));
  // todo
  greg->OTYPER |= (GPIO_OType_PP << 8) |
                  (GPIO_OType_PP << 9) |
                  (GPIO_OType_PP << 10) |
                  (GPIO_OType_PP << 11);
  greg->PUPDR |= (GPIO_PuPd_UP << (8 << 1)) |
                 (GPIO_PuPd_UP << (9 << 1)) |
                 (GPIO_PuPd_UP << (10 << 1)) |
                 (GPIO_PuPd_UP << (11 << 1)) |
                 (GPIO_PuPd_UP << (12 << 1));
  // todo
  greg->OSPEEDR |= (GPIO_Speed_25MHz << (8 << 1)) |
                   (GPIO_Speed_25MHz << (9 << 1)) |
                   (GPIO_Speed_25MHz << (10 << 1)) |
                   (GPIO_Speed_25MHz << (11 << 1));

  /* Configure PD.02 CMD line */
  greg = (GPIO_Regs *) GPIOD_BASE;
  greg->AFR[0] |= (GPIO_AF_SDIO << (2 << 2));
  greg->MODER |= (GPIO_Mode_AF << (2 << 1));
  //todo
  greg->OTYPER |= (GPIO_OType_PP << 2);
  greg->PUPDR |= (GPIO_PuPd_UP << (2 << 1));
  //todo
  greg->OSPEEDR |= (GPIO_Speed_25MHz << (2 << 1));

  /* Configure PC.12 pin: CLK pin */
  greg = (GPIO_Regs *) GPIOC_BASE;
  greg->MODER |= (GPIO_Mode_AF << (12 << 1));
  //todo
  greg->OTYPER |= (GPIO_OType_PP << 12);
  greg->PUPDR |= (GPIO_PuPd_NOPULL << (12 << 1));
  //todo
  greg->OSPEEDR |= (GPIO_Speed_25MHz << (12 << 1));

  /*!< Configure SD_SPI_DETECT_PIN pin: SD Card detect pin */
  greg = (GPIO_Regs *) SD_DETECT_GPIO_PORT;
  greg->MODER |= (GPIO_Mode_IN << (SD_DETECT_PIN_SOURCE << 1));
  greg->PUPDR |= (GPIO_PuPd_UP << (SD_DETECT_PIN_SOURCE << 1));

  /* Enable the SDIO APB2 Clock */
  RCC->APB2ENR |= RCC_APB2Periph_SDIO;

  /* Enable the DMA2 Clock */
  RCC->AHB1ENR |= SD_SDIO_DMA_CLK;
}

/**
  * @brief  Configures the DMA2 Channel4 for SDIO Tx request.
  * @param  BufferSRC: pointer to the source buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
void SD_LowLevel_DMA_TxConfig(unsigned int *BufferSRC, unsigned int BufferSize) {
#if SD_SDIO_DMA_STREAM < 4
   DMA_LIFCR(SD_SDIO_DMA_PORT) = DMA_ISR_MASK(SD_SDIO_DMA_STREAM);
#else
   DMA_HIFCR(SD_SDIO_DMA_PORT) = DMA_ISR_MASK(SD_SDIO_DMA_STREAM);
#endif

  /* DMA2 Stream3  or Stream6 disable */
  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN);

  /* DMA2 Stream3  or Stream6 deConfig */
  DMA_DeInit(SD_SDIO_DMA_STREAM_REGS);

  // setup DMA for SDIO
  DMA_SPAR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) =  (void*) SDIO_FIFO_ADDRESS; // peripheral (source) address
  DMA_SM0AR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) = (void*) BufferSRC;         // memory (desination) address

  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= (SD_SDIO_DMA_CHANNEL | DMA_SxCR_DIR_MEM_TO_PERIPHERAL |
                                                    DMA_SxCR_MINC |
                                                    DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                                                    DMA_SxCR_PBURST_INCR4 | DMA_SxCR_MBURST_INCR4 |
                                                    DMA_SxCR_PL_VERY_HIGH | DMA_SxCR_PFCTRL);
  DMA_SFCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) &= ~((unsigned int) DMA_SxFCR_DMDIS |DMA_SxFCR_FTH_MASK);
  DMA_SFCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= (DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_4_4_FULL);

  DMA_SNDTR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) = 0;

  /* DMA2 Stream3  or Stream6 enable */
  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= DMA_SxCR_EN;
}

/**
  * @brief  Configures the DMA2 Channel4 for SDIO Rx request.
  * @param  BufferDST: pointer to the destination buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
void SD_LowLevel_DMA_RxConfig(unsigned int *BufferDST, unsigned int BufferSize) {
#if SD_SDIO_DMA_STREAM < 4
   DMA_LIFCR(SD_SDIO_DMA_PORT) = DMA_ISR_MASK(SD_SDIO_DMA_STREAM);
#else
   DMA_HIFCR(SD_SDIO_DMA_PORT) = DMA_ISR_MASK(SD_SDIO_DMA_STREAM);
#endif

  /* DMA2 Stream3  or Stream6 disable */
  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) &= ~((unsigned int) DMA_SxCR_EN);

  /* DMA2 Stream3 or Stream6 Config */
  DMA_DeInit(SD_SDIO_DMA_STREAM_REGS);

  // setup DMA for SDIO
  DMA_SPAR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) =  (void*) SDIO_FIFO_ADDRESS; // peripheral (source) address
  DMA_SM0AR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) = (void*) BufferDST;         // memory (desination) address

  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= (SD_SDIO_DMA_CHANNEL | DMA_SxCR_DIR_PERIPHERAL_TO_MEM |
                                                    DMA_SxCR_MINC |
                                                    DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                                                    DMA_SxCR_PBURST_INCR4 | DMA_SxCR_MBURST_INCR4 |
                                                    DMA_SxCR_PL_VERY_HIGH | DMA_SxCR_PFCTRL);
  DMA_SFCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) &= ~((unsigned int) DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_MASK);
  DMA_SFCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= (DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_4_4_FULL);

  DMA_SNDTR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) = 0;

  /* DMA2 Stream3 or Stream6 enable */
  DMA_SCR(SD_SDIO_DMA_PORT, SD_SDIO_DMA_STREAM) |= DMA_SxCR_EN;
}

/**
  * @brief  Returns the DMA End Of Transfer Status.
  * @param  None
  * @retval DMA SDIO Stream Status.
  */
unsigned int SD_DMAEndOfTransferStatus(void) {
  return (unsigned int)DMA_GetFlagStatus(SD_SDIO_DMA_STREAM_REGS, SD_SDIO_DMA_FLAG_TCIF);
}
