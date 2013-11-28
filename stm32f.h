#ifndef stm32f_h
#define stm32f_h

//-------------------------------------------------------------------
void PUT32 ( unsigned int, unsigned int );
void PUT16 ( unsigned int, unsigned int );
void PUT8 ( unsigned int, unsigned int );
unsigned int GET32 ( unsigned int );
unsigned int GET16 ( unsigned int );

#define gpio_set(port, pin) ((GPIO_Regs*) port)->BSRRL = pin
#define gpio_reset(port, pin) ((GPIO_Regs*) port)->BSRRH = pin
#define gpio_toggle(port, pin) ((GPIO_Regs*) port)->ODR ^= pin
#define gpio_get(port, pin) (((GPIO_Regs*) port)->ODR & pin)

#define spi_status(spi) spi->SR
#define spi_status_txe(spi) (spi_status(spi) & SPI_I2S_FLAG_TXE)
#define spi_status_rxne(spi) (spi_status(spi) & SPI_I2S_FLAG_RXNE)
#define spi_read(spi) (spi)->DR
#define spi_send(spi,val) (spi)->DR = val

#define nrf24l0_SPIx ((SPI_Regs*) SPI1_BASE)

# ifndef MMIO32
#  define MMIO32(addr)		(*(volatile unsigned int *)(addr))
# endif

//-------------------------------------------------------------------
#define PERIPH_BASE			0x40000000
#define PERIPH_BASE_APB1		(PERIPH_BASE + 0x00000)
#define PERIPH_BASE_APB2		(PERIPH_BASE + 0x10000)
#define PERIPH_BASE_AHB1		(PERIPH_BASE + 0x20000)

//#define PERIPH_BASE_AHB2		0x50000000

#define RCCBASE   0x40023800
#define RCC_CR    (RCCBASE+0x00)
#define RCC_PLLCFGR (RCCBASE+0x04)
#define RCC_CFGR  (RCCBASE+0x08)
#define RCC_AHB1ENR (RCCBASE+0x30)
#define RCC_AHB2ENR (RCCBASE+0x34)
#define RCC_APB1ENR (RCCBASE+0x40)
#define RCC_APB2ENR (RCCBASE+0x44)

#define RCC_AHB1Periph_GPIOA             ((unsigned int)0x00000001)
#define RCC_AHB1Periph_GPIOB             ((unsigned int)0x00000002)
#define RCC_AHB1Periph_GPIOC             ((unsigned int)0x00000004)
#define RCC_AHB1Periph_GPIOD             ((unsigned int)0x00000008)
#define RCC_AHB1Periph_GPIOE             ((unsigned int)0x00000010)
#define RCC_AHB1Periph_GPIOF             ((unsigned int)0x00000020)
#define RCC_AHB1Periph_GPIOG             ((unsigned int)0x00000040)
#define RCC_AHB1Periph_GPIOH             ((unsigned int)0x00000080)
#define RCC_AHB1Periph_GPIOI             ((unsigned int)0x00000100)
#define RCC_APB2Periph_SPI1              ((unsigned int)0x00001000)
#define RCC_APB1Periph_SPI2              ((unsigned int)0x00004000)
#define RCC_APB1Periph_SPI3              ((unsigned int)0x00008000)
#define RCC_AHB2ENR_OTGFSEN              ((unsigned int)(1<<7))

//-------------------------------------------------------------------
#define TIM5BASE  0x40000C00
#define FLASH_ACR  0x40023C00

//-------------------------------------------------------------------

#define GPIOA_BASE (0x40020000)
#define GPIOB_BASE (0x40020400)
#define GPIOC_BASE (0x40020800)
#define GPIOD_BASE (0x40020C00)
#define GPIOE_BASE (0x40021000)
#define GPIOF_BASE (0x40021400)
#define GPIOG_BASE (0x40021800)
#define GPIOH_BASE (0x40021C00)
#define GPIOI_BASE (0x40022000)

// needed for uart.c
#define GPIOA_MODER (GPIOA_BASE+0x00)
#define GPIOA_OTYPER (GPIOA_BASE+0x04)
#define GPIOA_AFRL (GPIOA_BASE+0x20)

typedef struct
{
  volatile unsigned int MODER;    /* GPIO port mode register,               Address offset: 0x00      */
  volatile unsigned int OTYPER;   /* GPIO port output type register,        Address offset: 0x04      */
  volatile unsigned int OSPEEDR;  /* GPIO port output speed register,       Address offset: 0x08      */
  volatile unsigned int PUPDR;    /* GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
  volatile unsigned int IDR;      /* GPIO port input data register,         Address offset: 0x10      */
  volatile unsigned int ODR;      /* GPIO port output data register,        Address offset: 0x14      */
  volatile unsigned short BSRRL;  /* GPIO port bit set/reset low register,  Address offset: 0x18      */
  volatile unsigned short BSRRH;  /* GPIO port bit set/reset high register, Address offset: 0x1A      */
  volatile unsigned int LCKR;     /* GPIO port configuration lock register, Address offset: 0x1C      */
  volatile unsigned int AFR[2];   /* GPIO alternate function registers,     Address offset: 0x24-0x28 */
} GPIO_Regs;

#define GPIO_Mode_IN    0x00  /* GPIO Input Mode */
#define GPIO_Mode_OUT   0x01  /* GPIO Output Mode */
#define GPIO_Mode_AF    0x02  /* GPIO Alternate function Mode */
#define GPIO_Mode_AN    0x03  /* GPIO Analog Mode */

#define GPIO_OType_PP  0x00
#define GPIO_OType_OD  0x01

#define GPIO_Speed_2MHz    0x00  /* Low speed */
#define GPIO_Speed_25MHz   0x01  /* Medium speed */
#define GPIO_Speed_50MHz   0x02  /* Fast speed */
#define GPIO_Speed_100MHz  0x03  /* High speed on 30 pF (80 MHz Output max speed on 15 pF) */

#define GPIO_PuPd_NOPULL  0x00
#define GPIO_PuPd_UP      0x01
#define GPIO_PuPd_DOWN    0x02

#define Bit_RESET 0
#define Bit_SET 1

#define GPIO_Pin_0     ((unsigned short)0x0001)  /* Pin 0 selected */
#define GPIO_Pin_1     ((unsigned short)0x0002)  /* Pin 1 selected */
#define GPIO_Pin_2     ((unsigned short)0x0004)  /* Pin 2 selected */
#define GPIO_Pin_3     ((unsigned short)0x0008)  /* Pin 3 selected */
#define GPIO_Pin_4     ((unsigned short)0x0010)  /* Pin 4 selected */
#define GPIO_Pin_5     ((unsigned short)0x0020)  /* Pin 5 selected */
#define GPIO_Pin_6     ((unsigned short)0x0040)  /* Pin 6 selected */
#define GPIO_Pin_7     ((unsigned short)0x0080)  /* Pin 7 selected */
#define GPIO_Pin_8     ((unsigned short)0x0100)  /* Pin 8 selected */
#define GPIO_Pin_9     ((unsigned short)0x0200)  /* Pin 9 selected */
#define GPIO_Pin_10    ((unsigned short)0x0400)  /* Pin 10 selected */
#define GPIO_Pin_11    ((unsigned short)0x0800)  /* Pin 11 selected */
#define GPIO_Pin_12    ((unsigned short)0x1000)  /* Pin 12 selected */
#define GPIO_Pin_13    ((unsigned short)0x2000)  /* Pin 13 selected */
#define GPIO_Pin_14    ((unsigned short)0x4000)  /* Pin 14 selected */
#define GPIO_Pin_15    ((unsigned short)0x8000)  /* Pin 15 selected */
#define GPIO_Pin_All   ((unsigned short)0xFFFF)  /* All pins selected */

#define GPIO_PinSource0   ((unsigned char)0x00)
#define GPIO_PinSource1   ((unsigned char)0x01)
#define GPIO_PinSource2   ((unsigned char)0x02)
#define GPIO_PinSource3   ((unsigned char)0x03)
#define GPIO_PinSource4   ((unsigned char)0x04)
#define GPIO_PinSource5   ((unsigned char)0x05)
#define GPIO_PinSource6   ((unsigned char)0x06)
#define GPIO_PinSource7   ((unsigned char)0x07)
#define GPIO_PinSource8   ((unsigned char)0x08)
#define GPIO_PinSource9   ((unsigned char)0x09)
#define GPIO_PinSource10  ((unsigned char)0x0A)
#define GPIO_PinSource11  ((unsigned char)0x0B)
#define GPIO_PinSource12  ((unsigned char)0x0C)
#define GPIO_PinSource13  ((unsigned char)0x0D)
#define GPIO_PinSource14  ((unsigned char)0x0E)
#define GPIO_PinSource15  ((unsigned char)0x0F)

#define GPIO_AF_RTC_50Hz      ((unsigned char)0x00)  /* RTC_50Hz Alternate Function mapping */
#define GPIO_AF_MCO           ((unsigned char)0x00)  /* MCO (MCO1 and MCO2) Alternate Function mapping */
#define GPIO_AF_TAMPER        ((unsigned char)0x00)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define GPIO_AF_SWJ           ((unsigned char)0x00)  /* SWJ (SWD and JTAG) Alternate Function mapping */
#define GPIO_AF_TRACE         ((unsigned char)0x00)  /* TRACE Alternate Function mapping */

#define GPIO_AF_TIM1          ((unsigned char)0x01)  /* TIM1 Alternate Function mapping */
#define GPIO_AF_TIM2          ((unsigned char)0x01)  /* TIM2 Alternate Function mapping */

#define GPIO_AF_TIM3          ((unsigned char)0x02)  /* TIM3 Alternate Function mapping */
#define GPIO_AF_TIM4          ((unsigned char)0x02)  /* TIM4 Alternate Function mapping */
#define GPIO_AF_TIM5          ((unsigned char)0x02)  /* TIM5 Alternate Function mapping */

#define GPIO_AF_TIM8          ((unsigned char)0x03)  /* TIM8 Alternate Function mapping */
#define GPIO_AF_TIM9          ((unsigned char)0x03)  /* TIM9 Alternate Function mapping */
#define GPIO_AF_TIM10         ((unsigned char)0x03)  /* TIM10 Alternate Function mapping */
#define GPIO_AF_TIM11         ((unsigned char)0x03)  /* TIM11 Alternate Function mapping */

#define GPIO_AF_I2C1          ((unsigned char)0x04)  /* I2C1 Alternate Function mapping */
#define GPIO_AF_I2C2          ((unsigned char)0x04)  /* I2C2 Alternate Function mapping */
#define GPIO_AF_I2C3          ((unsigned char)0x04)  /* I2C3 Alternate Function mapping */

#define GPIO_AF_SPI1          ((unsigned char)0x05)  /* SPI1 Alternate Function mapping */
#define GPIO_AF_SPI2          ((unsigned char)0x05)  /* SPI2/I2S2 Alternate Function mapping */

#define GPIO_AF_SPI3          ((unsigned char)0x06)  /* SPI3/I2S3 Alternate Function mapping */

#define GPIO_AF_USART1        ((unsigned char)0x07)  /* USART1 Alternate Function mapping */
#define GPIO_AF_USART2        ((unsigned char)0x07)  /* USART2 Alternate Function mapping */
#define GPIO_AF_USART3        ((unsigned char)0x07)  /* USART3 Alternate Function mapping */

#define GPIO_AF_UART4         ((unsigned char)0x08)  /* UART4 Alternate Function mapping */
#define GPIO_AF_UART5         ((unsigned char)0x08)  /* UART5 Alternate Function mapping */
#define GPIO_AF_USART6        ((unsigned char)0x08)  /* USART6 Alternate Function mapping */

#define GPIO_AF_CAN1          ((unsigned char)0x09)  /* CAN1 Alternate Function mapping */
#define GPIO_AF_CAN2          ((unsigned char)0x09)  /* CAN2 Alternate Function mapping */
#define GPIO_AF_TIM12         ((unsigned char)0x09)  /* TIM12 Alternate Function mapping */
#define GPIO_AF_TIM13         ((unsigned char)0x09)  /* TIM13 Alternate Function mapping */
#define GPIO_AF_TIM14         ((unsigned char)0x09)  /* TIM14 Alternate Function mapping */

#define GPIO_AF_OTG_FS         ((unsigned char)0xA)  /* OTG_FS Alternate Function mapping */
#define GPIO_AF_OTG_HS         ((unsigned char)0xA)  /* OTG_HS Alternate Function mapping */

#define GPIO_AF_ETH             ((unsigned char)0x0B)  /* ETHERNET Alternate Function mapping */

#define GPIO_AF_FSMC            ((unsigned char)0xC)  /* FSMC Alternate Function mapping */
#define GPIO_AF_OTG_HS_FS       ((unsigned char)0xC)  /* OTG HS configured in FS, Alternate Function mapping */
#define GPIO_AF_SDIO            ((unsigned char)0xC)  /* SDIO Alternate Function mapping */

#define GPIO_AF_DCMI          ((unsigned char)0x0D)  /* DCMI Alternate Function mapping */

#define GPIO_AF_EVENTOUT      ((unsigned char)0x0F)  /* EVENTOUT Alternate Function mapping */

//-------------------------------------------------------------------
#define USART2_BASE 0x40004400
#define USART2_SR  (USART2_BASE+0x00)
#define USART2_DR  (USART2_BASE+0x04)
#define USART2_BRR (USART2_BASE+0x08)
#define USART2_CR1 (USART2_BASE+0x0C)
#define USART2_CR2 (USART2_BASE+0x10)
#define USART2_CR3 (USART2_BASE+0x14)
#define USART2_GTPR (USART2_BASE+0x18)

//-------------------------------------------------------------------
#define RNG_BASE 0x50060800
#define RNG_CR MMIO32(RNG_BASE+0x00)
#define RNG_SR MMIO32(RNG_BASE+0x04)
#define RNG_DR MMIO32(RNG_BASE+0x08)

//-------------------------------------------------------------------
#define SYSCLCK 120000000
//-------------------------------------------------------------------

#define ADC1_BASE			(PERIPH_BASE_APB2 + 0x2000)
#define ADC1				ADC1_BASE
/* ADC control register 1 (ADC_CR1) */
#define ADC_CR1(block)			MMIO32(block + 0x04)
#define ADC1_CR1			ADC_CR1(ADC1)
/* ADC control register 2 (ADC_CR2) */
#define ADC_CR2(block)			MMIO32(block + 0x08)
#define ADC1_CR2			ADC_CR2(ADC1)
/* ADC sample time register 1 (ADC_SMPR1) */
#define ADC_SMPR1(block)		MMIO32(block + 0x0c)
#define ADC1_SMPR1			ADC_SMPR1(ADC1)
/* ADC sample time register 2 (ADC_SMPR2) */
#define ADC_SMPR2(block)		MMIO32(block + 0x10)
#define ADC1_SMPR2			ADC_SMPR2(ADC1)
#define ADC_CBASE			(ADC1 + 0x300)
#define ADC_CCR			MMIO32(ADC_CBASE + 0x4)

/* ALIGN: Data alignment. */
#define ADC_CR2_ALIGN_RIGHT             (0 << 11)
#define ADC_CR2_ALIGN_LEFT              (1 << 11)
#define ADC_CR2_ALIGN			(1 << 11)
/* DMA: Direct memory access mode. (ADC1 and ADC3 only!) */
#define ADC_CR2_DMA			(1 << 8)
/* DMA: DDS: DMA disable selection */
#define ADC_CR2_DDS			(1 << 9)
/* Note: Bits [7:4] are reserved and must be kept at reset value. */
/* RSTCAL: Reset calibration. */
#define ADC_CR2_RSTCAL			(1 << 3)
/* CAL: A/D Calibration. */
#define ADC_CR2_CAL			(1 << 2)
/* CONT: Continous conversion. */
#define ADC_CR2_CONT			(1 << 1)
/* must be set individually - does not start conversion automatically else */
#define ADC_CR2_ADON			(1 << 0)
/* to enable temp sensor */
#define ADC_CCR_TSVREFE			(1 << 23)
/* SWSTART: */ /** Start conversion of regular channels. */
#define ADC_CR2_SWSTART			(1 << 30)

#define ADC_CR1_SCAN			(1 << 8)
#define ADC_CR2_EXTTRIG			(1 << 28)

#define ADC_SR_EOC			(1 << 1)

#define ADC_SQR1_L_LSB			20

#define ADC_SMPR_SMP_1DOT5CYC		0x0
#define ADC_SMPR_SMP_7DOT5CYC		0x1
#define ADC_SMPR_SMP_13DOT5CYC		0x2
#define ADC_SMPR_SMP_28DOT5CYC		0x3
#define ADC_SMPR_SMP_41DOT5CYC		0x4
#define ADC_SMPR_SMP_55DOT5CYC		0x5
#define ADC_SMPR_SMP_71DOT5CYC		0x6
#define ADC_SMPR_SMP_239DOT5CYC		0x7

#define ADC_CHANNEL16		0x10
#define ADC_CHANNEL17		0x11

/* ADC regular sequence register 1 (ADC_SQR1) */
#define ADC_SQR1(block)			MMIO32(block + 0x2c)
#define ADC1_SQR1			ADC_SQR1(ADC1)
/* ADC regular sequence register 2 (ADC_SQR2) */
#define ADC_SQR2(block)			MMIO32(block + 0x30)
#define ADC1_SQR2			ADC_SQR2(ADC1)
/* ADC regular sequence register 3 (ADC_SQR3) */
#define ADC_SQR3(block)			MMIO32(block + 0x34)
#define ADC1_SQR3			ADC_SQR3(ADC1)

/* ADC status register (ADC_SR) */
#define ADC_SR(block)			MMIO32(block + 0x00)
#define ADC1_SR				ADC_SR(ADC1)

/* ADC regular data register (ADC_DR) */
#define ADC_DR(block)			MMIO32(block + 0x4c)
#define ADC1_DR				ADC_DR(ADC1)

//-------------------------------------------------------------------
#define DESIG_UNIQUE_ID_BASE		(0x1FFF7A10)
#define DESIG_UNIQUE_ID0		MMIO32(DESIG_UNIQUE_ID_BASE)
#define DESIG_UNIQUE_ID1		MMIO32(DESIG_UNIQUE_ID_BASE + 4)
#define DESIG_UNIQUE_ID2		MMIO32(DESIG_UNIQUE_ID_BASE + 8)

//-------------------------------------------------------------------
#define SYSTICK_BASE 0xe000e010
#define SYSTICK_CTRL MMIO32(SYSTICK_BASE)
#define SYSTICK_LOAD MMIO32(SYSTICK_BASE + 0x4)
#define SYSTICK_VAL MMIO32(SYSTICK_BASE + 0x8)

//-------------------------------------------------------------------

#define DMA1_BASE			(PERIPH_BASE_AHB1 + 0x6000)
#define DMA2_BASE			(PERIPH_BASE_AHB1 + 0x6400)

#define DMA1				DMA1_BASE
#define DMA2				DMA2_BASE

#define DMA_STREAM0			0
#define DMA_STREAM1			1
#define DMA_STREAM2			2
#define DMA_STREAM3			3
#define DMA_STREAM4			4
#define DMA_STREAM5			5
#define DMA_STREAM6			6
#define DMA_STREAM7			7

#define DMA_STREAM(port, n)		((port) + 0x10 + (24 * (n)))
#define DMA1_STREAM(n)			DMA_STREAM(DMA1, n)
#define DMA2_STREAM(n)			DMA_STREAM(DMA2, n)

#define DMA1_STREAM0			DMA1_STREAM(0)
#define DMA1_STREAM1			DMA1_STREAM(1)
#define DMA1_STREAM2			DMA1_STREAM(2)
#define DMA1_STREAM3			DMA1_STREAM(3)
#define DMA1_STREAM4			DMA1_STREAM(4)
#define DMA1_STREAM5			DMA1_STREAM(5)
#define DMA1_STREAM6			DMA1_STREAM(6)
#define DMA1_STREAM7			DMA1_STREAM(7)

#define DMA2_STREAM0			DMA2_STREAM(0)
#define DMA2_STREAM1			DMA2_STREAM(1)
#define DMA2_STREAM2			DMA2_STREAM(2)
#define DMA2_STREAM3			DMA2_STREAM(3)
#define DMA2_STREAM4			DMA2_STREAM(4)
#define DMA2_STREAM5			DMA2_STREAM(5)
#define DMA2_STREAM6			DMA2_STREAM(6)
#define DMA2_STREAM7			DMA2_STREAM(7)

/* --- DMA controller registers -------------------------------------------- */

/* DMA low interrupt status register (DMAx_LISR) */
#define DMA_LISR(port)			MMIO32(port + 0x00)
#define DMA1_LISR			DMA_LISR(DMA1)
#define DMA2_LISR			DMA_LISR(DMA2)

/* DMA high interrupt status register (DMAx_HISR) */
#define DMA_HISR(port)			MMIO32(port + 0x04)
#define DMA1_HISR			DMA_HISR(DMA1)
#define DMA2_HISR			DMA_HISR(DMA2)

/* DMA low interrupt flag clear register (DMAx_LIFCR) */
#define DMA_LIFCR(port)			MMIO32(port + 0x08)
#define DMA1_LIFCR			DMA_LIFCR(DMA1)
#define DMA2_LIFCR			DMA_LIFCR(DMA2)

/* DMA high interrupt flag clear register (DMAx_HIFCR) */
#define DMA_HIFCR(port)			MMIO32(port + 0x0C)
#define DMA1_HIFCR			DMA_HIFCR(DMA1)
#define DMA2_HIFCR			DMA_HIFCR(DMA2)

/* --- DMA stream registers ------------------------------------------------ */

/* DMA Stream x configuration register (DMA_SxCR) */
#define DMA_SCR(port, n)		MMIO32(DMA_STREAM(port, n) + 0x00)
#define DMA1_SCR(n)			DMA_SCR(DMA1, n)
#define DMA2_SCR(n)			DMA_SCR(DMA2, n)

#define DMA1_S0CR			DMA1_SCR(0)
#define DMA1_S1CR			DMA1_SCR(1)
#define DMA1_S2CR			DMA1_SCR(2)
#define DMA1_S3CR			DMA1_SCR(3)
#define DMA1_S4CR			DMA1_SCR(4)
#define DMA1_S5CR			DMA1_SCR(5)
#define DMA1_S6CR			DMA1_SCR(6)
#define DMA1_S7CR			DMA1_SCR(7)

#define DMA2_S0CR			DMA2_SCR(0)
#define DMA2_S1CR			DMA2_SCR(1)
#define DMA2_S2CR			DMA2_SCR(2)
#define DMA2_S3CR			DMA2_SCR(3)
#define DMA2_S4CR			DMA2_SCR(4)
#define DMA2_S5CR			DMA2_SCR(5)
#define DMA2_S6CR			DMA2_SCR(6)
#define DMA2_S7CR			DMA2_SCR(7)

/* DMA Stream x number of data register (DMA_SxNDTR) */
#define DMA_SNDTR(port, n)		MMIO32(DMA_STREAM(port, n) + 0x04)
#define DMA1_SNDTR(n)			DMA_SNDTR(DMA1, n)
#define DMA2_SNDTR(n)			DMA_SNDTR(DMA2, n)

#define DMA1_S0NDTR			DMA1_SNDTR(0)
#define DMA1_S1NDTR			DMA1_SNDTR(1)
#define DMA1_S2NDTR			DMA1_SNDTR(2)
#define DMA1_S3NDTR			DMA1_SNDTR(3)
#define DMA1_S4NDTR			DMA1_SNDTR(4)
#define DMA1_S5NDTR			DMA1_SNDTR(5)
#define DMA1_S6NDTR			DMA1_SNDTR(6)
#define DMA1_S7NDTR			DMA1_SNDTR(7)

#define DMA2_S0NDTR			DMA2_SNDTR(0)
#define DMA2_S1NDTR			DMA2_SNDTR(1)
#define DMA2_S2NDTR			DMA2_SNDTR(2)
#define DMA2_S3NDTR			DMA2_SNDTR(3)
#define DMA2_S4NDTR			DMA2_SNDTR(4)
#define DMA2_S5NDTR			DMA2_SNDTR(5)
#define DMA2_S6NDTR			DMA2_SNDTR(6)
#define DMA2_S7NDTR			DMA2_SNDTR(7)

/* DMA Stream x peripheral address register (DMA_SxPAR) */
#define DMA_SPAR(port, n)		(*(volatile void **)\
					 (DMA_STREAM(port, n) + 0x08))
#define DMA1_SPAR(n)			DMA_SPAR(DMA1, n)
#define DMA2_SPAR(n)			DMA_SPAR(DMA2, n)

#define DMA1_S0PAR			DMA1_SPAR(0)
#define DMA1_S1PAR			DMA1_SPAR(1)
#define DMA1_S2PAR			DMA1_SPAR(2)
#define DMA1_S3PAR			DMA1_SPAR(3)
#define DMA1_S4PAR			DMA1_SPAR(4)
#define DMA1_S5PAR			DMA1_SPAR(5)
#define DMA1_S6PAR			DMA1_SPAR(6)
#define DMA1_S7PAR			DMA1_SPAR(7)

#define DMA2_S0PAR			DMA2_SPAR(0)
#define DMA2_S1PAR			DMA2_SPAR(1)
#define DMA2_S2PAR			DMA2_SPAR(2)
#define DMA2_S3PAR			DMA2_SPAR(3)
#define DMA2_S4PAR			DMA2_SPAR(4)
#define DMA2_S5PAR			DMA2_SPAR(5)
#define DMA2_S6PAR			DMA2_SPAR(6)
#define DMA2_S7PAR			DMA2_SPAR(7)

/* DMA Stream x memory address 0 register (DMA_SxM0AR) */
#define DMA_SM0AR(port, n)		(*(volatile void **) \
					 (DMA_STREAM(port, n) + 0x0c))
#define DMA1_SM0AR(n)			DMA_SM0AR(DMA1, n)
#define DMA2_SM0AR(n)			DMA_SM0AR(DMA2, n)

#define DMA1_S0M0AR			DMA1_SM0AR(0)
#define DMA1_S1M0AR			DMA1_SM0AR(1)
#define DMA1_S2M0AR			DMA1_SM0AR(2)
#define DMA1_S3M0AR			DMA1_SM0AR(3)
#define DMA1_S4M0AR			DMA1_SM0AR(4)
#define DMA1_S5M0AR			DMA1_SM0AR(5)
#define DMA1_S6M0AR			DMA1_SM0AR(6)
#define DMA1_S7M0AR			DMA1_SM0AR(7)

#define DMA2_S0M0AR			DMA2_SM0AR(0)
#define DMA2_S1M0AR			DMA2_SM0AR(1)
#define DMA2_S2M0AR			DMA2_SM0AR(2)
#define DMA2_S3M0AR			DMA2_SM0AR(3)
#define DMA2_S4M0AR			DMA2_SM0AR(4)
#define DMA2_S5M0AR			DMA2_SM0AR(5)
#define DMA2_S6M0AR			DMA2_SM0AR(6)
#define DMA2_S7M0AR			DMA2_SM0AR(7)

/* DMA Stream x memory address 1 register (DMA_SxM1AR) */
#define DMA_SM1AR(port, n)		(*(volatile void **)\
					 (DMA_STREAM(port, n) + 0x10))
#define DMA1_SM1AR(n)			DMA_SM1AR(DMA1, n)
#define DMA2_SM1AR(n)			DMA_SM1AR(DMA2, n)

#define DMA1_S0M1AR			DMA1_SM1AR(0)
#define DMA1_S1M1AR			DMA1_SM1AR(1)
#define DMA1_S2M1AR			DMA1_SM1AR(2)
#define DMA1_S3M1AR			DMA1_SM1AR(3)
#define DMA1_S4M1AR			DMA1_SM1AR(4)
#define DMA1_S5M1AR			DMA1_SM1AR(5)
#define DMA1_S6M1AR			DMA1_SM1AR(6)
#define DMA1_S7M1AR			DMA1_SM1AR(7)

#define DMA2_S0M1AR			DMA2_SM1AR(0)
#define DMA2_S1M1AR			DMA2_SM1AR(1)
#define DMA2_S2M1AR			DMA2_SM1AR(2)
#define DMA2_S3M1AR			DMA2_SM1AR(3)
#define DMA2_S4M1AR			DMA2_SM1AR(4)
#define DMA2_S5M1AR			DMA2_SM1AR(5)
#define DMA2_S6M1AR			DMA2_SM1AR(6)
#define DMA2_S7M1AR			DMA2_SM1AR(7)

/* DMA Stream x FIFO control register (DMA_SxFCR) */
#define DMA_SFCR(port, n)		MMIO32(DMA_STREAM(port, n) + 0x14)
#define DMA1_SFCR(n)			DMA_SFCR(DMA1, n)
#define DMA2_SFCR(n)			DMA_SFCR(DMA2, n)

#define DMA1_S0FCR			DMA1_SFCR(0)
#define DMA1_S1FCR			DMA1_SFCR(1)
#define DMA1_S2FCR			DMA1_SFCR(2)
#define DMA1_S3FCR			DMA1_SFCR(3)
#define DMA1_S4FCR			DMA1_SFCR(4)
#define DMA1_S5FCR			DMA1_SFCR(5)
#define DMA1_S6FCR			DMA1_SFCR(6)
#define DMA1_S7FCR			DMA1_SFCR(7)

#define DMA2_S0FCR			DMA2_SFCR(0)
#define DMA2_S1FCR			DMA2_SFCR(1)
#define DMA2_S2FCR			DMA2_SFCR(2)
#define DMA2_S3FCR			DMA2_SFCR(3)
#define DMA2_S4FCR			DMA2_SFCR(4)
#define DMA2_S5FCR			DMA2_SFCR(5)
#define DMA2_S6FCR			DMA2_SFCR(6)
#define DMA2_S7FCR			DMA2_SFCR(7)

/* --- DMA Interrupt Flag offset values ------------------------------------- */

/* For API parameters. These are based on every interrupt flag and flag clear
being at the same relative location */
/** @defgroup dma_if_offset DMA Interrupt Flag Offsets within stream flag group.
@ingroup dma_defines

@{*/
/** Transfer Complete Interrupt Flag */
#define DMA_TCIF		    (1 << 5)
/** Half Transfer Interrupt Flag */
#define DMA_HTIF		    (1 << 4)
/** Transfer Error Interrupt Flag */
#define DMA_TEIF		    (1 << 3)
/** Direct Mode Error Interrupt Flag */
#define DMA_DMEIF		    (1 << 2)
/** FIFO Error Interrupt Flag */
#define DMA_FEIF		    (1 << 0)
/**@}*/

/* Offset within interrupt status register to start of stream interrupt flag
 * field
 */
#define DMA_ISR_OFFSET(stream)	(6*(stream & 0x01)+16*((stream & 0x02) >> 1))
#define DMA_ISR_FLAGS		(DMA_TCIF | DMA_HTIF | DMA_TEIF | DMA_DMEIF | \
				 DMA_FEIF)
#define DMA_ISR_MASK(stream)	(DMA_ISR_FLAGS << DMA_ISR_OFFSET(stream))

/* --- DMA_LISR values ----------------------------------------------------- */

#define DMA_LISR_FEIF0			(1 << 0)
#define DMA_LISR_DMEIF0			(1 << 2)
#define DMA_LISR_TEIF0			(1 << 3)
#define DMA_LISR_HTIF0			(1 << 4)
#define DMA_LISR_TCIF0			(1 << 5)

#define DMA_LISR_FEIF1			(1 << 6)
#define DMA_LISR_DMEIF1			(1 << 8)
#define DMA_LISR_TEIF1			(1 << 9)
#define DMA_LISR_HTIF1			(1 << 10)
#define DMA_LISR_TCIF1			(1 << 11)

#define DMA_LISR_FEIF2			(1 << 16)
#define DMA_LISR_DMEIF2			(1 << 18)
#define DMA_LISR_TEIF2			(1 << 19)
#define DMA_LISR_HTIF2			(1 << 20)
#define DMA_LISR_TCIF2			(1 << 21)

#define DMA_LISR_FEIF3			(1 << 22)
#define DMA_LISR_DMEIF3			(1 << 24)
#define DMA_LISR_TEIF3			(1 << 25)
#define DMA_LISR_HTIF3			(1 << 26)
#define DMA_LISR_TCIF3			(1 << 27)

/* --- DMA_HISR values ----------------------------------------------------- */

#define DMA_HISR_FEIF4			(1 << 0)
#define DMA_HISR_DMEIF4			(1 << 2)
#define DMA_HISR_TEIF4			(1 << 3)
#define DMA_HISR_HTIF4			(1 << 4)
#define DMA_HISR_TCIF4			(1 << 5)

#define DMA_HISR_FEIF5			(1 << 6)
#define DMA_HISR_DMEIF5			(1 << 8)
#define DMA_HISR_TEIF5			(1 << 9)
#define DMA_HISR_HTIF5			(1 << 10)
#define DMA_HISR_TCIF5			(1 << 11)

#define DMA_HISR_FEIF6			(1 << 16)
#define DMA_HISR_DMEIF6			(1 << 18)
#define DMA_HISR_TEIF6			(1 << 19)
#define DMA_HISR_HTIF6			(1 << 20)
#define DMA_HISR_TCIF6			(1 << 21)

#define DMA_HISR_FEIF7			(1 << 22)
#define DMA_HISR_DMEIF7			(1 << 24)
#define DMA_HISR_TEIF7			(1 << 25)
#define DMA_HISR_HTIF7			(1 << 26)
#define DMA_HISR_TCIF7			(1 << 27)

/* --- DMA_LIFCR values ----------------------------------------------------- */

#define DMA_LIFCR_CFEIF0		(1 << 0)
#define DMA_LIFCR_CDMEIF0		(1 << 2)
#define DMA_LIFCR_CTEIF0		(1 << 3)
#define DMA_LIFCR_CHTIF0		(1 << 4)
#define DMA_LIFCR_CTCIF0		(1 << 5)

#define DMA_LIFCR_CFEIF1		(1 << 6)
#define DMA_LIFCR_CDMEIF1		(1 << 8)
#define DMA_LIFCR_CTEIF1		(1 << 9)
#define DMA_LIFCR_CHTIF1		(1 << 10)
#define DMA_LIFCR_CTCIF1		(1 << 11)

#define DMA_LIFCR_CFEIF2		(1 << 16)
#define DMA_LIFCR_CDMEIF2		(1 << 18)
#define DMA_LIFCR_CTEIF2		(1 << 19)
#define DMA_LIFCR_CHTIF2		(1 << 20)
#define DMA_LIFCR_CTCIF2		(1 << 21)

#define DMA_LIFCR_CFEIF3		(1 << 22)
#define DMA_LIFCR_CDMEIF3		(1 << 24)
#define DMA_LIFCR_CTEIF3		(1 << 25)
#define DMA_LIFCR_CHTIF3		(1 << 26)
#define DMA_LIFCR_CTCIF3		(1 << 27)

/* --- DMA_HIFCR values ----------------------------------------------------- */

#define DMA_HIFCR_CFEIF4		(1 << 0)
#define DMA_HIFCR_CDMEIF4		(1 << 2)
#define DMA_HIFCR_CTEIF4		(1 << 3)
#define DMA_HIFCR_CHTIF4		(1 << 4)
#define DMA_HIFCR_CTCIF4		(1 << 5)

#define DMA_HIFCR_CFEIF5		(1 << 6)
#define DMA_HIFCR_CDMEIF5		(1 << 8)
#define DMA_HIFCR_CTEIF5		(1 << 9)
#define DMA_HIFCR_CHTIF5		(1 << 10)
#define DMA_HIFCR_CTCIF5		(1 << 11)

#define DMA_HIFCR_CFEIF6		(1 << 16)
#define DMA_HIFCR_CDMEIF6		(1 << 18)
#define DMA_HIFCR_CTEIF6		(1 << 19)
#define DMA_HIFCR_CHTIF6		(1 << 20)
#define DMA_HIFCR_CTCIF6		(1 << 21)

#define DMA_HIFCR_CFEIF7		(1 << 22)
#define DMA_HIFCR_CDMEIF7		(1 << 24)
#define DMA_HIFCR_CTEIF7		(1 << 25)
#define DMA_HIFCR_CHTIF7		(1 << 26)
#define DMA_HIFCR_CTCIF7		(1 << 27)

/* --- DMA_SxCR values ----------------------------------------------------- */

/* EN: Stream enable */
#define DMA_SxCR_EN			(1 << 0)
/* DMEIE: Direct Mode error interrupt enable */
#define DMA_SxCR_DMEIE			(1 << 1)
/* TEIE: Transfer error interrupt enable */
#define DMA_SxCR_TEIE			(1 << 2)
/* HTIE: Half transfer interrupt enable */
#define DMA_SxCR_HTIE			(1 << 3)
/* TCIE: Transfer complete interrupt enable */
#define DMA_SxCR_TCIE			(1 << 4)
/* PFCTRL: Peripheral Flow Controller */
#define DMA_SxCR_PFCTRL			(1 << 5)

/* DIR[7:6]: Data transfer direction */
/** @defgroup dma_st_dir DMA Stream Data transfer direction
@ingroup dma_defines

@{*/
#define DMA_SxCR_DIR_PERIPHERAL_TO_MEM	(0 << 6)
#define DMA_SxCR_DIR_MEM_TO_PERIPHERAL	(1 << 6)
#define DMA_SxCR_DIR_MEM_TO_MEM		(2 << 6)
/**@}*/
#define DMA_SxCR_DIR_SHIFT		6
#define DMA_SxCR_DIR_MASK		(3 << 6)

/* CIRC: Circular mode */
#define DMA_SxCR_CIRC			(1 << 8)
/* PINC: Peripheral increment mode */
#define DMA_SxCR_PINC			(1 << 9)
/* MINC: Memory increment mode */
#define DMA_SxCR_MINC			(1 << 10)

/* PSIZE[12:11]: Peripheral size */
/** @defgroup dma_st_perwidth DMA Stream Peripheral Word Width
@ingroup STM32F4xx_dma_defines

@{*/
#define DMA_SxCR_PSIZE_8BIT		(0 << 11)
#define DMA_SxCR_PSIZE_16BIT		(1 << 11)
#define DMA_SxCR_PSIZE_32BIT		(2 << 11)
/**@}*/
#define DMA_SxCR_PSIZE_SHIFT		11
#define DMA_SxCR_PSIZE_MASK		(3 << 11)

/* MSIZE[14:13]: Memory size */
/** @defgroup dma_st_memwidth DMA Stream Memory Word Width
@ingroup STM32F4xx_dma_defines

@{*/
#define DMA_SxCR_MSIZE_8BIT		(0 << 13)
#define DMA_SxCR_MSIZE_16BIT		(1 << 13)
#define DMA_SxCR_MSIZE_32BIT		(2 << 13)
/**@}*/
#define DMA_SxCR_MSIZE_SHIFT		13
#define DMA_SxCR_MSIZE_MASK		(3 << 13)

/* PINCOS: Peripheral increment offset size */
#define DMA_SxCR_PINCOS			(1 << 15)

/* PL[17:16]: Stream priority level */
/** @defgroup dma_st_pri DMA Stream Priority Levels
@ingroup dma_defines

@{*/
#define DMA_SxCR_PL_LOW			(0 << 16)
#define DMA_SxCR_PL_MEDIUM		(1 << 16)
#define DMA_SxCR_PL_HIGH		(2 << 16)
#define DMA_SxCR_PL_VERY_HIGH		(3 << 16)
/**@}*/
#define DMA_SxCR_PL_SHIFT		16
#define DMA_SxCR_PL_MASK		(3 << 16)

/* DBM: Double buffered mode */
#define DMA_SxCR_DBM			(1 << 18)
/* CT: Current target (in double buffered mode) */
#define DMA_SxCR_CT			(1 << 19)

/* Bit 20 reserved */

/* PBURST[13:12]: Peripheral Burst Configuration */
/** @defgroup dma_pburst DMA Peripheral Burst Length
@ingroup dma_defines

@{*/
#define DMA_SxCR_PBURST_SINGLE		(0 << 21)
#define DMA_SxCR_PBURST_INCR4		(1 << 21)
#define DMA_SxCR_PBURST_INCR8		(2 << 21)
#define DMA_SxCR_PBURST_INCR16		(3 << 21)
/**@}*/
#define DMA_SxCR_PBURST_SHIFT		21
#define DMA_SxCR_PBURST_MASK		(3 << 21)

/* MBURST[13:12]: Memory Burst Configuration */
/** @defgroup dma_mburst DMA Memory Burst Length
@ingroup STM32F4xx_dma_defines

@{*/
#define DMA_SxCR_MBURST_SINGLE		(0 << 23)
#define DMA_SxCR_MBURST_INCR4		(1 << 23)
#define DMA_SxCR_MBURST_INCR8		(2 << 23)
#define DMA_SxCR_MBURST_INCR16		(3 << 23)
/**@}*/
#define DMA_SxCR_MBURST_SHIFT		23
#define DMA_SxCR_MBURST_MASK		(3 << 23)

/* CHSEL[25:27]: Channel Select */
/** @defgroup dma_ch_sel DMA Channel Select
@ingroup dma_defines

@{*/
#define DMA_SxCR_CHSEL_0		(0 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_1		(1 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_2		(2 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_3		(3 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_4		(4 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_5		(5 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_6		(6 << DMA_SxCR_CHSEL_SHIFT)
#define DMA_SxCR_CHSEL_7		(7 << DMA_SxCR_CHSEL_SHIFT)
/**@}*/
#define DMA_SxCR_CHSEL_SHIFT		25
#define DMA_SxCR_CHSEL_MASK		(7 << 25)
#define DMA_SxCR_CHSEL(n)		(n << DMA_SxCR_CHSEL_SHIFT)

/* Reserved [31:28] */

/* --- DMA_SxNDTR values --------------------------------------------------- */

/* DMA_SxNDTR[15:0]: Number of data register. */

/* --- DMA_SxPAR values ---------------------------------------------------- */

/* DMA_SxPAR[31:0]: Peripheral address register. */

/* --- DMA_SxM0AR values --------------------------------------------------- */

/* DMA_SxM0AR[31:0]: Memory 0 address register. */

/* --- DMA_SxM1AR values --------------------------------------------------- */

/* DMA_SxM1AR[31:0]: Memory 1 address register. */

/* --- DMA_SxFCR values ---------------------------------------------------- */

/* FTH[1:0]: FIFO Threshold selection */
#define DMA_SxFCR_FTH_1_4_FULL		(0 << 0)
#define DMA_SxFCR_FTH_2_4_FULL		(1 << 0)
#define DMA_SxFCR_FTH_3_4_FULL		(2 << 0)
#define DMA_SxFCR_FTH_4_4_FULL		(3 << 0)
#define DMA_SxFCR_FTH_SHIFT		0
#define DMA_SxFCR_FTH_MASK		(3 << 0)

/* DMDIS: Direct Mode disable */
#define DMA_SxFCR_DMDIS			(1 << 2)

/* FS[5:3]: FIFO Status */
#define DMA_SxFCR_FS_LT_1_4_FULL	(0 << 0)
#define DMA_SxFCR_FS_LT_2_4_FULL	(1 << 0)
#define DMA_SxFCR_FS_LT_3_4_FULL	(2 << 0)
#define DMA_SxFCR_FS_LT_4_4_FULL	(3 << 0)
#define DMA_SxFCR_FS_FULL		(4 << 3)
#define DMA_SxFCR_FS_EMPTY		(5 << 3)
#define DMA_SxFCR_FS_SHIFT		3
#define DMA_SxFCR_FS_MASK		(7 << 3)

/* [6]: reserved */

/* FEIE[7]: FIFO error interrupt enable */
#define DMA_SxFCR_FEIE			(1 << 7)

/* [31:8]: Reserved */

#define DMA_Stream0_IT_MASK     (unsigned int)(DMA_LISR_FEIF0 | DMA_LISR_DMEIF0 | \
                                           DMA_LISR_TEIF0 | DMA_LISR_HTIF0 | \
                                           DMA_LISR_TCIF0)

/* ---------------------------------------------------- */
#define DWT_BASE        0xE0001000
#define DWT_CONTROL		MMIO32(DWT_BASE)
#define DWT_CYCCNT		MMIO32(DWT_BASE + 0x04)
#define SCB_DEMCR			MMIO32(0xE000EDFC)

/* ---------------------------------------------------- */

#define SPI1_BASE             (PERIPH_BASE_APB2 + 0x3000)
#define SPI2_BASE             (PERIPH_BASE_APB1 + 0x3800)

typedef struct
{
  volatile unsigned int CR1;
  volatile unsigned int CR2;
  volatile unsigned int SR;
  volatile unsigned int DR;
  volatile unsigned int CRCPR;
  volatile unsigned int RXCRCR;
  volatile unsigned int TXCRCR;
  volatile unsigned int I2SCFGR;
  volatile unsigned int I2SPR;
} SPI_Regs;

#define SPI_Direction_2Lines_FullDuplex ((unsigned short)0x0000)
#define SPI_Direction_2Lines_RxOnly     ((unsigned short)0x0400)
#define SPI_Direction_1Line_Rx          ((unsigned short)0x8000)
#define SPI_Direction_1Line_Tx          ((unsigned short)0xC000)

#define SPI_Mode_Master                 ((unsigned short)0x0104)
#define SPI_Mode_Slave                  ((unsigned short)0x0000)

#define SPI_DataSize_16b                ((unsigned short)0x0800)
#define SPI_DataSize_8b                 ((unsigned short)0x0000)

#define SPI_CPOL_Low                    ((unsigned short)0x0000)
#define SPI_CPOL_High                   ((unsigned short)0x0002)

#define SPI_CPHA_1Edge                  ((unsigned short)0x0000)
#define SPI_CPHA_2Edge                  ((unsigned short)0x0001)

#define SPI_NSS_Soft                    ((unsigned short)0x0200)
#define SPI_NSS_Hard                    ((unsigned short)0x0000)

#define SPI_BaudRatePrescaler_2         ((unsigned short)0x0000)
#define SPI_BaudRatePrescaler_4         ((unsigned short)0x0008)
#define SPI_BaudRatePrescaler_8         ((unsigned short)0x0010)
#define SPI_BaudRatePrescaler_16        ((unsigned short)0x0018)
#define SPI_BaudRatePrescaler_32        ((unsigned short)0x0020)
#define SPI_BaudRatePrescaler_64        ((unsigned short)0x0028)
#define SPI_BaudRatePrescaler_128       ((unsigned short)0x0030)
#define SPI_BaudRatePrescaler_256       ((unsigned short)0x0038)

#define SPI_FirstBit_MSB                ((unsigned short)0x0000)
#define SPI_FirstBit_LSB                ((unsigned short)0x0080)

#define SPI_I2S_DMAReq_Tx               ((unsigned short)0x0002)
#define SPI_I2S_DMAReq_Rx               ((unsigned short)0x0001)

#define SPI_NSSInternalSoft_Set         ((unsigned short)0x0100)
#define SPI_NSSInternalSoft_Reset       ((unsigned short)0xFEFF)

#define SPI_CRC_Tx                      ((unsigned char)0x00)
#define SPI_CRC_Rx                      ((unsigned char)0x01)

#define SPI_CR1_SPE                     ((unsigned short)0x0040)            /*!<SPI Enable */

#define SPI_Direction_Rx                ((unsigned short)0xBFFF)
#define SPI_Direction_Tx                ((unsigned short)0x4000)

#define SPI_I2S_IT_TXE                  ((unsigned char)0x71)
#define SPI_I2S_IT_RXNE                 ((unsigned char)0x60)
#define SPI_I2S_IT_ERR                  ((unsigned char)0x50)
#define I2S_IT_UDR                      ((unsigned char)0x53)
#define SPI_I2S_IT_TIFRFE               ((unsigned char)0x58)

#define SPI_I2S_IT_OVR                  ((unsigned char)0x56)
#define SPI_IT_MODF                     ((unsigned char)0x55)
#define SPI_IT_CRCERR                   ((unsigned char)0x54)

#define SPI_I2S_FLAG_RXNE               ((unsigned short)0x0001)
#define SPI_I2S_FLAG_TXE                ((unsigned short)0x0002)
#define I2S_FLAG_CHSIDE                 ((unsigned short)0x0004)
#define I2S_FLAG_UDR                    ((unsigned short)0x0008)
#define SPI_FLAG_CRCERR                 ((unsigned short)0x0010)
#define SPI_FLAG_MODF                   ((unsigned short)0x0020)
#define SPI_I2S_FLAG_OVR                ((unsigned short)0x0040)
#define SPI_I2S_FLAG_BSY                ((unsigned short)0x0080)
#define SPI_I2S_FLAG_TIFRFE             ((unsigned short)0x0100)

#define  SPI_I2SCFGR_I2SMOD             ((unsigned short)0x0800)            /*!<I2S mode selection */

//-------------------------------------------------------------------

#define NVIC_OTG_FS_IRQ 67

#define PPBI_BASE                       0xE0000000
#define SCS_BASE                        (PPBI_BASE + 0xE000)
#define NVIC_BASE                       (SCS_BASE + 0x0100)

/* ISER: Interrupt Set Enable Registers */
/* Note: 8 32bit Registers */
/* Note: Single register on CM0 */
#define NVIC_ISER(iser_id)		MMIO32(NVIC_BASE + 0x00 + \
						(iser_id * 4))

/* NVIC_BASE + 0x020 (0xE000 E120 - 0xE000 E17F): Reserved */

/* ICER: Interrupt Clear Enable Registers */
/* Note: 8 32bit Registers */
/* Note: Single register on CM0 */
#define NVIC_ICER(icer_id)		MMIO32(NVIC_BASE + 0x80 + \
						(icer_id * 4))

/* NVIC_BASE + 0x0A0 (0xE000 E1A0 - 0xE000 E1FF): Reserved */

/* ISPR: Interrupt Set Pending Registers */
/* Note: 8 32bit Registers */
/* Note: Single register on CM0 */
#define NVIC_ISPR(ispr_id)		MMIO32(NVIC_BASE + 0x100 + \
						(ispr_id * 4))

/* NVIC_BASE + 0x120 (0xE000 E220 - 0xE000 E27F): Reserved */

/* ICPR: Interrupt Clear Pending Registers */
/* Note: 8 32bit Registers */
/* Note: Single register on CM0 */
#define NVIC_ICPR(icpr_id)		MMIO32(NVIC_BASE + 0x180 + \
						(icpr_id * 4))

/* NVIC_BASE + 0x1A0 (0xE000 E2A0 - 0xE00 E2FF): Reserved */

/* Those defined only on ARMv7 and above */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
/* IABR: Interrupt Active Bit Register */
/* Note: 8 32bit Registers */
#define NVIC_IABR(iabr_id)		MMIO32(NVIC_BASE + 0x200 + \
						(iabr_id * 4))
#endif

/* NVIC_BASE + 0x220 (0xE000 E320 - 0xE000 E3FF): Reserved */

/* IPR: Interrupt Priority Registers */
/* Note: 240 8bit Registers */
/* Note: 32 8bit Registers on CM0 */
#define NVIC_IPR(ipr_id)		MMIO8(NVIC_BASE + 0x300 + \
						ipr_id)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
/* STIR: Software Trigger Interrupt Register */
#define NVIC_STIR			MMIO32(STIR_BASE)
#endif

//-------------------------------------------------------------------

#endif
