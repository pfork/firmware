#include "usb.h"
#include "stm32f.h"
#include "clock.h"
#ifdef USE_UART2
#  include "uart.h"
#endif
#include "rng.h"
#include "adc.h"
#include "systimer.h"
#include "irq.h"
#ifdef HAVE_MSC
#include "sd.h"
#endif // HAVE_MSC
#include "led.h"
#include "keys.h"

/**
  * @brief  main initialization routine
  * @param  None
  * @retval None
  */
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
#ifdef HAVE_MSC
  SD_Init();
#endif // HAVE_MSC
  usb_init();
  keys_init();
}
