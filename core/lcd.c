#include <string.h>
#include "stm32f.h"
#include "delay.h"
#include "display.h"
#include "font5x5.h"

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

static const char logo[]="\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xdf\x3f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x7f\x3f\x9f\xdf\xdf\xcf\xdf\xdf\x9f\x3f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x0f\x1f\xff\xff\xff\xff\xff\xff\xff\xff" \
  "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x8f\x1f\x7f\x7f\x7f\x3f\x9f\x87\xf0\xfc\xff\xff\xff\xff\xff\xff\xff\x03\x01\xf8\xfc\xff\xff\xff\xff\xff\xff\xff\xff\xe0\x01\x0f\xff\xff\xff\x8f\x03\x7f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xf9\x63\x67\xcf\x9f\x9f\x9f\x9f\x83\x07\x0f\xc7\xe3\xf8\xfc\xff\xff\xff\xff\xff\xff\xff\xff" \
  "\xff\xff\xf8\xf9\xfb\xf3\xf7\xf7\xf7\xf3\xfb\xfb\xf9\xf8\xf8\xe2\xc7\x8f\x1f\x7f\xff\xff\xff\xff\xff\xff\xff\xff\xf0\xc0\x87\x1f\x7f\x7f\x01\x01\x7f\x7f\x7f\x1f\xc0\xf0\xfc\xff\xff\xff\xff\xff\xfe\xfc\x79\x09\x83\xf7\xf7\xf7\xf7\xf7\xf7\xf7\xf3\xf8\xfc\xff\xff\xff\xff\xff\xff\xff\xff\xf8\xe0\x03\x1f\xff\xff\xff\xff\xff\xff\xff\xff\xff" \
  "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xf8\xf8\xe3\xc7\x1f\x7f\xff\xff\xff\xff\xff\xff\xff\xfe\x04\x00\xc0\xfe\xfe\xff\xff\xff\xff\xff\xff\xff\xff\xff\x3f\x0f\xc1\xf0\xfe\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xf8\xe0\x87\x1f\xff\xff\xff\xff\xff\xff" \
  "\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xce\xcc\xc1\xc7\xcf\xcf\xcf\xcf\xcf\xc0\xc0\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xc3\xc0\xcc\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcf\xcc\xc0\xc3\xcf\xcf\xcf\xcf" \
  "\xff\xff\xff\xff\xff\xff\xff\x80\xf6\xf6\xf6\xf6\xf9\xff\x80\xff\xfe\xfe\x80\xfe\xfe\xff\xc1\xbe\xbe\xbe\xbe\xdd\xff\x80\xf7\xf7\xf7\xf7\x80\xff\x80\xf6\xf6\xf6\xfe\xff\xc1\xbe\xbe\xbe\xbe\xc1\xff\x80\xf6\xf6\xe6\xd6\xb9\xff\x80\xf7\xf7\xeb\xdd\xbe\xff\xa0\xff\xa0\xff\xb0\xb6\xb6\xb6\xb6\xce\xff\xa0\xff\xff\xff\xff\xff\xff\xff\xff\xff";

void disp_show_logo(void) {
    memcpy(frame_buffer, logo, sizeof(logo));
    disp_refresh();
}

void disp_invert(void) {
  lcd_cmd(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_INVERTED);
}

void disp_normal(void) {
  lcd_cmd(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_NORMAL);
}
