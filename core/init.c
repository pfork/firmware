#include "cdcacm.h"
#include "stm32f.h"
#include "clock.h"
#include "uart.h"
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
  uart_init();
  rnd_init();
  adc_init();
  sysctr = 0;
  systick_init();
  irq_init();
  SD_Init();
  usb_init();
  keys_init();
}
