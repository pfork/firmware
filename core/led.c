#include "stm32f.h"
#include "led.h"

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

void handle_led(const unsigned int port, const unsigned int pin, volatile unsigned int* ctr, const unsigned int period) {
  if(period>0) {
    if(*ctr>0) {
      *ctr=(*ctr)-1;
    } else {
      gpio_toggle(port, pin);
      *ctr=period;
    }
  }
}


volatile unsigned int ledperiod[2];
volatile unsigned int ledcounter[2];
void led_handler(void) {
  handle_led(LED_BASE, GPIO_Pin_13, &ledcounter[0], ledperiod[0]); // status 1
  handle_led(LED_BASE, GPIO_Pin_12, &ledcounter[1], ledperiod[1]); // status 2
}
