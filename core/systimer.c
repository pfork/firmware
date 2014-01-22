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
