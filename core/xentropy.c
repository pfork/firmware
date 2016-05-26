#include "stm32f.h"
#include "stddef.h"
#include "stdint.h"

#define SWEN2 3 // PB3
#define SWEN1 7 // PB7
#define COMP2 8 // PB8
#define COMP1 9 // PB9

void xesrc_init(void) {
  GPIO_Regs *greg;
  // enable gpiob, gpioa(spi)
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOB;

  greg = (GPIO_Regs *) GPIOB_BASE;
  greg->MODER &= ~((3 << (SWEN2 << 1)) |
                   (3 << (SWEN1 << 1)) |
                   (3 << (COMP2 << 1)) |
                   (3 << (COMP1 << 1)));
  greg->MODER |= ((GPIO_Mode_OUT << (SWEN2 << 1)) |
                  (GPIO_Mode_OUT << (SWEN1 << 1)) |
                  (GPIO_Mode_IN << (COMP2 << 1)) |
                  (GPIO_Mode_IN << (COMP1 << 1)));
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (SWEN2 << 1)) |
                    (GPIO_Speed_100MHz << (SWEN1 << 1)) |
                    (GPIO_Speed_100MHz << (COMP2 << 1)) |
                    (GPIO_Speed_100MHz << (COMP1 << 1)));
  greg->PUPDR &= ~((3 << (SWEN2 << 1)) |
                   (3 << (SWEN1 << 1)) |
                   (3 << (COMP2 << 1)) |
                   (3 << (COMP1 << 1)));
  greg->PUPDR |= ((GPIO_PuPd_DOWN << (SWEN2 << 1)) |
                  (GPIO_PuPd_DOWN << (SWEN1 << 1))); // |
                  //(GPIO_PuPd_DOWN << (COMP2 << 1)) |
                  //(GPIO_PuPd_DOWN << (COMP1 << 1)));
}

void get_entropy(uint8_t *buf, size_t buflen) {
  size_t i;
  for(i=0;i<buflen*8;i++) {
    // alternate swen1/swen2
    if(i%2==0) {
      gpio_reset(GPIOB_BASE, 1 << SWEN1);
      gpio_set(GPIOB_BASE, 1 << SWEN2);
    } else {
      gpio_reset(GPIOB_BASE, 1 << SWEN2);
      gpio_set(GPIOB_BASE, 1 << SWEN1);
    }

    // read out comp1/comp2
    buf[i/8]= (buf[i/8] << 1) | ((i & 1) ? (!!gpio_get(GPIOB_BASE, 1<< COMP1)) : (!!gpio_get(GPIOB_BASE, 1<< COMP2)));
  }
}
