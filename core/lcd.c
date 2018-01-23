#include <string.h>
#include "stm32f.h"
#include "delay.h"
#include "display.h"
#include "font5x5.h"
#include "utils/lzg/lzg.h"

#define GPIO_BASE GPIOB_BASE
#define DCPIN 4
#define RSTPIN 5
#define CSPIN 6
#define SDCLKPIN 13
#define SDINPIN 15
#define GPIO_AF_SPI GPIO_AF_SPI2
#define SPI_BASE SPI2_BASE

#define SPIx ((SPI_Regs*) SPI_BASE)

#define DC(x)					x ? (gpio_set(GPIO_BASE, 1 << DCPIN)) : (gpio_reset(GPIO_BASE, 1 << DCPIN));
#define CS(x)					x ? (gpio_set(GPIO_BASE, 1 << CSPIN)) : (gpio_reset(GPIO_BASE, 1 << CSPIN));
#define RST(x)					x ? (gpio_set(GPIO_BASE, 1 << RSTPIN)) : (gpio_reset(GPIO_BASE, 1 << RSTPIN));

#define LCD_RESET_TIME 1

#define PCD8544_SET_Y_ADDRESS 0x40
#define PCD8544_SET_X_ADDRESS 0x80

#define PCD8544_EXTENDED_INSTRUCTION (1<<0)
#define PCD8544_DISPLAY_NORMAL       (1<<2)
#define PCD8544_DISPLAY_CONTROL      (1<<3)
#define PCD8544_SET_BIAS             (1<<4)
#define PCD8544_FUNCTION_SET         (1<<5)
#define PCD8544_SET_VOP              (1<<7)
#define PCD8544_DISPLAY_INVERTED     (1<<2|1<<0)

uint8_t frame_buffer[48 * 84 / 8];

static void send(const unsigned char data) {
  while(spi_status_txe(SPIx) == 0);
  spi_send(SPIx, data);
  uDelay(25);
}

/**
 * @brief  Writes command to the LCD
 * @param LCD_Reg: address of the selected register.
 * @arg LCD_RegValue: value to write to the selected register.
 * @retval : None
 */
static void lcd_cmd(uint8_t reg) {
  CS(1);
  DC(0);
  CS(0);
  send(reg);
  CS(1);
}

//static void lcd_data(uint8_t data) {
//  CS(1);
//  DC(1);
//  CS(0);
//  send(data);
//  CS(1);
//}

static void lcd_reset(void) {
  /* Toggle RST low to reset. Minimum pulse 100ns on datasheet. */
  CS(0);
  RST(0);
  mDelay(LCD_RESET_TIME);
  RST(1);
  CS(1);
}

static void lcd_PCD8544_init(void) {
	lcd_reset();
	/* Get into the EXTENDED mode! */
	lcd_cmd(PCD8544_FUNCTION_SET | PCD8544_EXTENDED_INSTRUCTION);
	/* LCD bias select (4 is optimal?) */
	lcd_cmd(PCD8544_SET_BIAS | 0x0);
	/* Set VOP (affects contrast) */
	lcd_cmd(PCD8544_SET_VOP | 80); /* 0x0-0x7f, but doesn't seem to change anything */
	/* switch back to standard instructions */
	lcd_cmd(PCD8544_FUNCTION_SET);
	/* Normal mode */
	lcd_cmd(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_NORMAL);
}

void disp_init(void) {
  GPIO_Regs *greg;
  SPI_Regs *sreg;
  // enable gpiob, gpioa(spi)
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOB;
  // enable spi clock
  MMIO32(RCC_APB1ENR) &= ~RCC_APB1Periph_SPI2;
  MMIO32(RCC_APB1ENR) |= RCC_APB1Periph_SPI2;

  greg = (GPIO_Regs *) GPIO_BASE;
  greg->MODER  &= (unsigned int) ~((3 << (DCPIN << 1))     |
                                   (3 << (RSTPIN << 1))    |
                                   (3 << (CSPIN << 1))     |
                                   (3 << (SDCLKPIN << 1))  |
                                   (3 << (SDINPIN << 1)));
  greg->PUPDR  &= (unsigned int) ~((3 << (DCPIN    << 1))  |
                                   (3 << (RSTPIN   << 1))  |
                                   (3 << (CSPIN    << 1))  |
                                   (3 << (SDCLKPIN << 1))  |
                                   (3 << (SDINPIN  << 1)));
  greg->AFR[0] &= (unsigned int) ~((15 << (DCPIN << 2))    |
                                   (15 << (RSTPIN << 2))   |
                                   (15 << (CSPIN << 2)));
  greg->AFR[1] &= (unsigned int) ~((15 << ((SDCLKPIN-8) << 2)) |
                                   (15 << ((SDINPIN-8) << 2)));

  greg->AFR[1] |=  ((GPIO_AF_SPI << ((SDCLKPIN-8) << 2))    |
                    (GPIO_AF_SPI << ((SDINPIN-8) << 2)));
  greg->MODER |=   ((GPIO_Mode_OUT << (DCPIN    << 1))  |
                    (GPIO_Mode_OUT << (RSTPIN   << 1))  |
                    (GPIO_Mode_OUT << (CSPIN    << 1))  |
                    (GPIO_Mode_AF << (SDCLKPIN << 1))   |
                    (GPIO_Mode_AF << (SDINPIN  << 1))
                    );
  greg->PUPDR |=   ((GPIO_PuPd_UP << (DCPIN    << 1))   |
                    (GPIO_PuPd_UP << (RSTPIN   << 1))   |
                    (GPIO_PuPd_UP << (CSPIN    << 1)) |
                    (GPIO_PuPd_DOWN << (SDCLKPIN << 1)) |
                    (GPIO_PuPd_DOWN << (SDINPIN  << 1))
                    );
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (DCPIN    << 1))  |
                    (GPIO_Speed_100MHz << (RSTPIN   << 1))  |
                    (GPIO_Speed_100MHz << (CSPIN    << 1))  |
                    (GPIO_Speed_100MHz << (SDCLKPIN << 1))    |
                    (GPIO_Speed_100MHz << (SDINPIN  << 1))
                    );
  CS(1);
  RST(1);

  sreg = (SPI_Regs *) SPI_BASE;

  sreg->CR1 = 0;
  sreg->CR1 |= (unsigned short)(SPI_Direction_1Line_Tx | SPI_Mode_Master |
                                SPI_DataSize_8b | SPI_CPOL_Low  |
                                SPI_CPHA_1Edge | SPI_NSS_Soft |
                                SPI_BaudRatePrescaler_64 | SPI_FirstBit_MSB);

  /* Activate the SPI mode (Reset I2SMOD bit in I2SCFGR register) */
  sreg->I2SCFGR &= (unsigned short)~((unsigned short)SPI_I2SCFGR_I2SMOD);

  /* Write to SPIx CRCPOLY */
  sreg->CRCPR = 7;

  sreg->CR1 |= SPI_CR1_SPE;

  lcd_PCD8544_init();
  disp_clear();
}

void disp_refresh(void) {
	uint8_t bank;
   uint8_t column;

   CS(0);
   for (bank = 0; bank < PCD8544_MAX_BANKS; bank++) {
		/* Each bank is a single row 8 bits tall */

      //lcd_cmd(PCD8544_SET_Y_ADDRESS | bank);
      //lcd_cmd(PCD8544_SET_X_ADDRESS | 0);
      DC(0);
      while(spi_status_txe(SPIx) == 0);
      spi_send(SPIx,PCD8544_SET_Y_ADDRESS | bank);
      while(spi_status_txe(SPIx) == 0);
      uDelay(5);
      spi_send(SPIx,PCD8544_SET_X_ADDRESS | 0);
      while(spi_status_txe(SPIx) == 0);
      uDelay(5);
      DC(1);

		for (column = 0; column <= PCD8544_MAX_COLS; column++) {
        //lcd_data(frame_buffer[PCD8544_MAX_COLS * bank + column]);
        while(spi_status_txe(SPIx) == 0);
        spi_send(SPIx, frame_buffer[PCD8544_MAX_COLS * bank + column]);
        uDelay(5);
		}
	}
   CS(1);
}

static const unsigned char logo_lzg[]={
  0x4c, 0x5a, 0x47, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x00, 0xfa, 0xa4,
  0xf3, 0x6c, 0x4f, 0x01, 0x02, 0x05, 0x06, 0x08, 0xff, 0xff, 0x08, 0x0e,
  0xdf, 0x3f, 0x05, 0x08, 0x04, 0x7f, 0x3f, 0x9f, 0xdf, 0xdf, 0xcf, 0xdf,
  0xdf, 0x9f, 0x05, 0x09, 0x0c, 0x08, 0x16, 0x0f, 0x1f, 0x05, 0x11, 0x0d,
  0x8f, 0x1f, 0x7f, 0x7f, 0x06, 0x3d, 0x87, 0xf0, 0xfc, 0x06, 0xc8, 0xff,
  0x03, 0x01, 0xf8, 0x05, 0x06, 0x03, 0xff, 0xe0, 0x01, 0x0f, 0x06, 0x5b,
  0x03, 0x7f, 0x05, 0x0a, 0x2a, 0xf9, 0x63, 0x67, 0xcf, 0x9f, 0x08, 0x01,
  0x83, 0x07, 0x0f, 0xc7, 0xe3, 0x05, 0x08, 0x24, 0xff, 0xff, 0xf8, 0xf9,
  0xfb, 0xf3, 0xf7, 0xf7, 0xf7, 0xf3, 0xfb, 0xfb, 0xf9, 0xf8, 0xf8, 0xe2,
  0xc7, 0x8f, 0x1f, 0x05, 0x07, 0x2f, 0xf0, 0xc0, 0x87, 0x1f, 0x7f, 0x7f,
  0x01, 0x01, 0x7f, 0x7f, 0x7f, 0x1f, 0xc0, 0x05, 0x05, 0x62, 0xfe, 0xfc,
  0x79, 0x09, 0x83, 0x06, 0x27, 0x08, 0x02, 0xf3, 0x05, 0x08, 0x3f, 0xf8,
  0xe0, 0x03, 0x05, 0x12, 0x9f, 0x08, 0x07, 0xf8, 0xf8, 0xe3, 0xc7, 0x05,
  0x07, 0x51, 0xfe, 0x04, 0x00, 0xc0, 0xfe, 0xfe, 0x05, 0x07, 0x14, 0x3f,
  0x0f, 0xc1, 0xf0, 0x05, 0x08, 0x06, 0x08, 0x0b, 0xf8, 0xe0, 0x87, 0x05,
  0x05, 0x4f, 0xcf, 0x08, 0x15, 0xce, 0xcc, 0xc1, 0xc7, 0x06, 0x81, 0xc0,
  0xc0, 0x05, 0x08, 0x0d, 0xc3, 0xc0, 0xcc, 0x05, 0x16, 0x28, 0x08, 0x03,
  0xcc, 0xc0, 0xc3, 0x08, 0xc2, 0x05, 0x05, 0x5d, 0x80, 0xf6, 0x08, 0x01,
  0xf9, 0xff, 0x80, 0xff, 0xfe, 0xfe, 0x80, 0xfe, 0xfe, 0xff, 0xc1, 0xbe,
  0x08, 0x01, 0xdd, 0xff, 0x80, 0x05, 0x02, 0xda, 0x80, 0x06, 0x95, 0x05,
  0x05, 0x0c, 0xc1, 0x06, 0x45, 0xe6, 0xd6, 0xb9, 0x06, 0x53, 0xeb, 0xdd,
  0xbe, 0xff, 0xa0, 0x08, 0x21, 0xb0, 0xb6, 0x08, 0x01, 0xce, 0x06, 0x01,
  0x08, 0x06
};
#define LOGO_LZG_LEN 266

void disp_show_logo(void) {
  LZG_Decode(logo_lzg, LOGO_LZG_LEN, frame_buffer, sizeof(frame_buffer));
  disp_refresh();
}

void disp_invert(void) {
  lcd_cmd(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_INVERTED);
}

void disp_normal(void) {
  lcd_cmd(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_NORMAL);
}
