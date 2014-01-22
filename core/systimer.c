/**
  ************************************************************************************
  * @file    systimer.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides an initializer for the system timer.
  ************************************************************************************
  */

#include "stm32f.h"

volatile unsigned long long sysctr;

/**
  * @brief  SysTick IRQ handler maintains syctr
  * @param  None
  * @retval None
  */
void systick_init(void) {
  SYSTICK_LOAD = SYSCLCK / 1000; /* Frequency of 1 kHz (ms)*/
  SYSTICK_CTRL |= 4; /* use sysclck */
  SYSTICK_CTRL |= 1; /* Enable Counter */
  SYSTICK_CTRL |= 2; /* Enable interrupts */
}
