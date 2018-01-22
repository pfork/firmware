#include <stdint.h>
#include "stm32f.h"
#include "delay.h"
#include "display.h"
#include "utils/lzg/lzg.h"
#include "font8x8.h"

#include <string.h>

/*
   dc   pb4
   res  pb5
   cs   pb6
   //sclk pb13 (clk)
   //sdin pb15 (mosi)
   sclk pb15 (clk)
   sdin pb13 (mosi)
 */

#define DCPIN 4
#define RSTPIN 5
#define CSPIN 6
#ifdef HWrev1
#define SDCLKPIN 15
#define SDINPIN 13
#else
#define SDCLKPIN 13
#define SDINPIN 15
#endif // HWrev1

#define DC(x)					x ? (gpio_set(GPIOB_BASE, 1 << DCPIN)) : (gpio_reset(GPIOB_BASE, 1 << DCPIN));
#define CS(x)					x ? (gpio_set(GPIOB_BASE, 1 << CSPIN)) : (gpio_reset(GPIOB_BASE, 1 << CSPIN));
#define SCLK(x)				x ? (gpio_set(GPIOB_BASE, 1 << SDCLKPIN)) : (gpio_reset(GPIOB_BASE, 1 << SDCLKPIN));
#define SDIN(x)				x ? (gpio_set(GPIOB_BASE, 1 << SDINPIN)) : (gpio_reset(GPIOB_BASE, 1 << SDINPIN));
#define RST(x)					x ? (gpio_set(GPIOB_BASE, 1 << RSTPIN)) : (gpio_reset(GPIOB_BASE, 1 << RSTPIN));

uint8_t frame_buffer[128 * 64 / 8];

static void send(uint8_t byte) {
  int8_t i;
  SCLK(1);
  for (i=7; i>=0; i--) {
    SCLK(0);
    SDIN(byte & (1 << i));
    SCLK(1);
  }
}

/**
 * @brief  Writes command to the LCD
 * @param LCD_Reg: address of the selected register.
 * @arg LCD_RegValue: value to write to the selected register.
 * @retval : None
 */
static void oled_cmd(uint8_t reg) {
  CS(1);
  DC(0);
  CS(0);
  send(reg);
  CS(1);
}

static void oled_data(uint8_t data) {
  CS(1);
  DC(1);
  CS(0);
  send(data);
  CS(1);
}

void disp_refresh(void) {
  oled_cmd(0x00);//---set low column address
  oled_cmd(0x10);//---set high column address
  oled_cmd(0x40);//--set start line address

  oled_cmd(0x22);//---set address
  oled_cmd(0x00);//---set start page address
  oled_cmd(0x07);//--set end page address

  uint16_t i;
  for (i=0; i<1024; i++) {
    oled_data(frame_buffer[i]);
  }
}

void disp_init(void) {      //Generated by Internal DC/DC Circuit // VCC externally supplied
  GPIO_Regs *greg;

  /* Enable GPIOD clock */
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOB;

  greg = (GPIO_Regs *) GPIOB_BASE;
  greg->MODER  &= (unsigned int) ~((3 << (4 << 1))  |
                                   (3 << (5 << 1))  |
                                   (3 << (6 << 1))  |
                                   (3 << (13 << 1))  |
                                   (3 << (15 << 1))
                                   );
  greg->PUPDR  &= (unsigned int) ~((3 << (4 << 1))  |
                                   (3 << (5 << 1))  |
                                   (3 << (6 << 1))  |
                                   (3 << (13 << 1))  |
                                   (3 << (15 << 1))
                                   );
  greg->MODER |=   ((GPIO_Mode_OUT << (4 << 1))  |
                    (GPIO_Mode_OUT << (5 << 1))  |
                    (GPIO_Mode_OUT << (6 << 1))  |
                    (GPIO_Mode_OUT << (13 << 1))  |
                    (GPIO_Mode_OUT << (15 << 1))
                    );
  greg->PUPDR |= ((GPIO_PuPd_UP << (4 << 1)) |
                  (GPIO_PuPd_UP << (5 << 1)) |
                  (GPIO_PuPd_UP << (6 << 1)) |
                  (GPIO_PuPd_UP << (13 << 1)) |
                  (GPIO_PuPd_UP << (15 << 1))
                  );
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (4 << 1))  |
                    (GPIO_Speed_100MHz << (5 << 1))  |
                    (GPIO_Speed_100MHz << (6 << 1))  |
                    (GPIO_Speed_100MHz << (13 << 1))  |
                    (GPIO_Speed_100MHz << (15 << 1))
                    );

  RST(1);
  uDelay(2);
  RST(0);
  mDelay(10);
  RST(1);
  uDelay(2);

  oled_cmd(0xae);//--turn off oled panel
  oled_cmd(0x20);//---set page addressing mode
  oled_cmd(0x00);// 0x02
  // if address mode 02
  //oled_cmd(0x00);//---set low column address
  //oled_cmd(0x10);//---set high column address
  //oled_cmd(0x40);//--set start line address
  // if address mode 00
  oled_cmd(0x21);//---set column address
  oled_cmd(0x00);//---set start column address
  oled_cmd(0x7f);//--set end column address
  oled_cmd(0x22);//---set address
  oled_cmd(0x00);//---set start page address
  oled_cmd(0x07);//--set end page address
  oled_cmd(0x81);//--set contrast control register
  oled_cmd(0xcf);
  //oled_cmd(0xa0);//--set segment re-map 95 to 0
  // rotate 180
  oled_cmd(0xa1);
  oled_cmd(0xc8); // COMSCANDEC - 0xc0 is comscaninc, but ususally in reset anyway. so unset

  oled_cmd(0xa6);//--set normal display
  oled_cmd(0xa4);//--set display all on resume
  oled_cmd(0xa8);//--set multiplex ratio(1 to 64)
  oled_cmd(0x3f);//--1/64 duty
  oled_cmd(0xd3);//--set display offset
  oled_cmd(0x00);//--not offset
  oled_cmd(0xd5);//--set display clock divide ratio/oscillator frequency
  oled_cmd(0x80);//--set divide ratio
  oled_cmd(0xd9);//--set pre-charge period
  oled_cmd(0xf1);
  oled_cmd(0xda);//--set com pins hardware configuration
  oled_cmd(0x12);
  oled_cmd(0xdb);//--set vcomh
  oled_cmd(0x40);
  oled_cmd(0x8d);//--set Charge Pump enable/disable
  oled_cmd(0x14);//--set(0x14) enable
  oled_cmd(0xaf);//--turn on oled panel

  uDelay(100);
  disp_clear();
}

void disp_invert(void) {
  oled_cmd(0xa7);//--set inverted display
}

void disp_normal(void) {
  oled_cmd(0xa6);//--set normal display
}

static const unsigned char bitmap_lzg[] = {
  0x4c, 0x5a, 0x47, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x70, 0x37,
  0x35, 0x6b, 0xbb, 0x01, 0x05, 0x06, 0x08, 0x09, 0x00, 0x00, 0x09, 0x1b,
  0x80, 0xc0, 0xc0, 0x80, 0x06, 0x0f, 0x0a, 0xe0, 0x09, 0x01, 0x06, 0x0f,
  0x10, 0x09, 0x1b, 0xe0, 0xe0, 0x06, 0x1c, 0x1d, 0x09, 0x09, 0x80, 0x80,
  0xc0, 0x06, 0x05, 0x01, 0xf3, 0xff, 0x7f, 0x06, 0x08, 0x0f, 0xf0, 0xfc,
  0x7e, 0x1f, 0x07, 0x03, 0x01, 0x08, 0x43, 0x01, 0x03, 0x07, 0x1f, 0xfe,
  0xf8, 0x06, 0x04, 0x4f, 0xf8, 0xf8, 0x18, 0x06, 0x0c, 0x37, 0x3c, 0x7c,
  0xf0, 0xe0, 0x06, 0x04, 0x98, 0xf0, 0xf0, 0xe0, 0x80, 0xc0, 0xe0, 0xf8,
  0x3f, 0x1f, 0x06, 0x1c, 0x6d, 0x00, 0x70, 0x78, 0x08, 0x2f, 0x09, 0x04,
  0xc3, 0xe7, 0xff, 0x7e, 0x78, 0xf8, 0xfc, 0x9f, 0x0f, 0x06, 0x04, 0x66,
  0x09, 0x04, 0x3f, 0xff, 0xff, 0x06, 0x03, 0x4d, 0xe0, 0x06, 0x04, 0xc7,
  0x80, 0xff, 0x06, 0x05, 0x94, 0x07, 0x1f, 0x3e, 0x78, 0x70, 0x08, 0x32,
  0x80, 0x09, 0x04, 0xc0, 0xc0, 0xe0, 0xf8, 0x7c, 0x19, 0x01, 0x03, 0x09,
  0x03, 0x07, 0x1f, 0xff, 0xf9, 0x06, 0x1c, 0xf5, 0x09, 0x06, 0x01, 0x09,
  0x06, 0x08, 0x85, 0x03, 0x0f, 0x1f, 0x3c, 0xf8, 0xf0, 0x06, 0x28, 0x62,
  0x08, 0x08, 0x06, 0x02, 0x62, 0xff, 0xff, 0xe1, 0xe0, 0x70, 0x78, 0x3c,
  0x1f, 0x0f, 0x03, 0x06, 0x27, 0x28, 0xe0, 0xf8, 0xff, 0x1f, 0x07, 0x06,
  0x04, 0x6b, 0x06, 0x05, 0x35, 0x09, 0x08, 0x07, 0x1f, 0xff, 0xfc, 0x06,
  0x1c, 0x7b, 0x09, 0x16, 0x01, 0x07, 0x0f, 0x1e, 0x7c, 0xf8, 0xe0, 0x06,
  0x25, 0x89, 0x09, 0x01, 0xff, 0xff, 0xff, 0x06, 0x2c, 0xa5, 0xf0, 0xfc,
  0x7f, 0x1f, 0x06, 0x07, 0x83, 0x09, 0x13, 0x03, 0x06, 0x02, 0x7b, 0x06,
  0x53, 0x29, 0x60, 0x09, 0x1c, 0x09, 0x02, 0x63, 0x67, 0x7f, 0x7e, 0x7c,
  0x70, 0x08, 0x83, 0x7f, 0x7f, 0x63, 0x06, 0x09, 0x11, 0x70, 0x7e, 0x7f,
  0x6f, 0x61, 0x06, 0x1c, 0x39, 0x63, 0x7f, 0x7f, 0x7c, 0x06, 0x11, 0x0f,
  0x06, 0x15, 0xa9, 0xf0, 0x90, 0x09, 0x01, 0x08, 0x15, 0xf0, 0x00, 0x00,
  0x10, 0x10, 0xf0, 0x10, 0x10, 0x00, 0xe0, 0x10, 0x09, 0x01, 0x20, 0x08,
  0x09, 0x06, 0x22, 0xda, 0x08, 0x0e, 0x08, 0x59, 0x06, 0x05, 0x0e, 0xe0,
  0x06, 0x09, 0x27, 0x80, 0x80, 0x40, 0x20, 0x10, 0x08, 0xaf, 0x09, 0x42,
  0xd0, 0x09, 0x01, 0x90, 0x08, 0x83, 0x09, 0x1d, 0x07, 0x06, 0x25, 0xfc,
  0x07, 0x08, 0x85, 0x09, 0x61, 0x03, 0x04, 0x09, 0x01, 0x02, 0x06, 0x05,
  0x11, 0x08, 0x09, 0x09, 0xe2, 0x06, 0x05, 0x0e, 0x03, 0x08, 0xc6, 0x02,
  0x04, 0x09, 0xe8, 0x09, 0x45, 0x06, 0x05, 0x16, 0x08, 0x03, 0x09, 0x17
};
#define BITMAP_LZG_LEN 384

void disp_show_logo(void) {
  // logo + clearscreen "anim"
  LZG_Decode(bitmap_lzg, BITMAP_LZG_LEN, frame_buffer, 1024);
  disp_refresh();
}
