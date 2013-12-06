#include "cdcacm.h"
#include "stm32f.h"
#include "clock.h"
#include "uart.h"
#include "rng.h"
#include "adc.h"
#include "systimer.h"

void init(void) {
  clock_init();
  uart_init();
  rnd_init();
  adc_init();
  sysctr = 0;
  systick_init();
  usb_init();
}
