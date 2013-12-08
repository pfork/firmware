#include "stm32f.h"

void systick_handler(void);

volatile unsigned long long sysctr;
volatile unsigned int TimingDelay ;

void sys_tick_handler(void) {
  sysctr++;
  if ( TimingDelay != 0x00)
    TimingDelay --;
}

void systick_init(void) {
  SYSTICK_LOAD = SYSCLCK / 1000; /* Frequency of 1 kHz (ms)*/
  SYSTICK_CTRL |= 4; /* use sysclck */
  SYSTICK_CTRL |= 1; /* Enable Counter */
  SYSTICK_CTRL |= 2; /* Enable interrupts */
}

void Delay(unsigned int nTime ){
  TimingDelay = nTime ;
  while ( TimingDelay != 0);
}
