#include "stm32f.h"

void led_init(void) {
  GPIO_Regs * greg;

  /* GPIOF Periph clock enable */
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOD;

  greg = (GPIO_Regs *) GPIOD_BASE;
  /* Configure Pd15 Pd14 Pd13 Pd12 in output pushpull mode */
  greg->MODER |= ((GPIO_Mode_OUT << (15 << 1)) |
                  (GPIO_Mode_OUT << (14 << 1)) |
                  (GPIO_Mode_OUT << (13 << 1)) |
                  (GPIO_Mode_OUT << (12 << 1)));
  //greg->OTYPER |= ((GPIO_OType_PP << 15) |
  //                 (GPIO_OType_PP << 14) |
  //                 (GPIO_OType_PP << 13) |
  //                 (GPIO_OType_PP << 12));
  greg->PUPDR |= ((GPIO_PuPd_UP << (15 << 1)) |
                  (GPIO_PuPd_UP << (14 << 1)) |
                  (GPIO_PuPd_UP << (13 << 1)) |
                  (GPIO_PuPd_UP << (12 << 1)));
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (15 << 1)) |
                    (GPIO_Speed_100MHz << (14 << 1)) |
                    (GPIO_Speed_100MHz << (13 << 1)) |
                    (GPIO_Speed_100MHz << (12 << 1)));
}
