/**
  ************************************************************************************
  * @file    rng.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides initializers and supplementary
  *          functions for the HW RNG
  ************************************************************************************
  */

#include <sys/types.h>
#include "stm32f.h"
#include <display.h>

unsigned int rng_last;

/**
  * @brief  initializes HW rng
  * @param  None
  * @retval None
  */
void rnd_init ( void ) {
  MMIO32(RCC_AHB2ENR) |= 1<<6;
  RNG_CR|=4;
}

/**
  * @brief  reads HW rng
  * @param  None
  * @retval random 4 byte
  */
unsigned int next_rand ( void ) {
  unsigned int ra;

  while(1) {
    ra=RNG_SR;
    // see chapter 20.3.2 Error management in stm32f2xx ref manual
    if(ra&0x44) {
      // 0x40 not enough entropy, clear this bit, reset&set the rnggen in rng_cr to restart
      RNG_SR&=~0x40; // clear SEIS bit
      RNG_CR&=~0x04; // clear RNGGEN bit
      RNG_CR|=0x04; // set RNGGEN bit again.
      continue;
    }
    if(ra&0x22) {
      // 0x20 == clock error, no more rng from here
      disp_clear();
      disp_print(0, DISPLAY_HEIGHT/2 - FONT_HEIGHT, "RNG Clock Error");
      disp_print(0, DISPLAY_HEIGHT/2 + FONT_HEIGHT, "please reboot");
      while(1);
    }
    if(ra&1) break; // we got enough entropy
  }

  ra = RNG_DR;
  if(ra==rng_last) { // repeating sequence in rng
    disp_clear();
    disp_print(0, DISPLAY_HEIGHT/2 - FONT_HEIGHT, "RNG Repeat Error");
    disp_print(0, DISPLAY_HEIGHT/2 + FONT_HEIGHT, "please reboot");
    while(1) {
    }
  }

  rng_last=ra;
  return(ra);
}

/**
  * @brief  reads HW rng into buffer
  * @param  size of result buffer
  * @param  ptr to buffer
  * @retval number of bytes stored
  */
unsigned int rand_read ( const size_t size, unsigned char* buf) {
  unsigned int ra;
  unsigned int cnt = 0;

  while(cnt<size) {
    ra = next_rand();
    if (size - cnt >= 4) {
      PUT32((unsigned int) buf+cnt, ra);
      cnt+=4;
      continue;
    }
    unsigned int s=0;
    while(cnt<size) {
      *(buf+cnt++)=*(((unsigned char*) &ra)+s++);
    }
  }
  return cnt;
}
