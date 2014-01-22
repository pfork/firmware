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
    if(ra&1) break;
    if(ra&0x66) {
      while(1) {
      }
    }
  }
  if(ra&0x66) {
    while(1) {
    } // repeating sequence in rng
  }

  ra = RNG_DR;
  if(ra==rng_last) {
    while(1) {
    } // repeating sequence in rng
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
