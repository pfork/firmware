/**
  ************************************************************************************
  * @file    ssp.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides stack protector callbacks
  ************************************************************************************
  */

#include "randombytes_pitchfork.h"

void * __stack_chk_guard = NULL;

void __stack_chk_guard_setup() {
    unsigned char * p;
    p = (unsigned char *) &__stack_chk_guard;

    randombytes_buf((void *) p, 4);
}

void __attribute__((noreturn)) __stack_chk_fail() {
  for(;;);
}
