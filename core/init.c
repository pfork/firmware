/**
  ************************************************************************************
  * @file    init.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides the main initialization function.
  ************************************************************************************
  */

#include "usb.h"
#include "stm32f.h"
#include "clock.h"
#include "rng.h"
#include "adc.h"
#include "systimer.h"
#include "irq.h"
#ifdef HAVE_MSC
#include "sd.h"
#endif // HAVE_MSC
#include "led.h"
#include "keys.h"
#include "display.h"
#include "mpu.h"

/**
  * @brief  main initialization routine
  * @param  None
  * @retval None
  */
void init(void) {
  mpu_init();
  clock_init();
  //led_init();
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
  disp_init();
}
