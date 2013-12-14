#include "usb_crypto.h"
#include "stm32f.h"
#include "clock.h"
#ifdef USE_UART2
#include "uart.h"
#endif
#include "rng.h"
#include "adc.h"
#include "systimer.h"
#include "irq.h"
#include "sd.h"
#include "led.h"
#include "keys.h"

void init(void) {
  clock_init();
  led_init();
#ifdef USE_UART2
  uart_init();
#endif
  rnd_init();
  adc_init();
  sysctr = 0;
  systick_init();
  irq_init();
  SD_Init();
  usb_init();
  keys_init();
}
