/**
  ************************************************************************************
  * @file    uart.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides uart init and other functions.
  ************************************************************************************
  */

#include "stm32f.h"

int uart_init ( void ) {
  MMIO32(RCC_AHB1ENR) |= 1<<0; //enable port A
  MMIO32(RCC_APB1ENR) |= 1<<17;  //enable USART2
  //PA2 USART2_TX
  MMIO32(GPIOA_MODER) |= (2<<4);
  MMIO32(GPIOA_OTYPER) &=(1<<2);
  MMIO32(GPIOA_OTYPER) = 0;
  MMIO32(GPIOA_AFRL) |= (7<<8);

  // divisor 136 fractional divisor 11
  MMIO32(USART2_BRR) = (16<<4)|(4<<0);
  MMIO32(USART2_CR1) = (1<<13)|(1<<3);
  return(0);
}

void uart_putc(unsigned int x ) {
  while (( MMIO32(USART2_SR) & (1<<7)) == 0) continue;
  MMIO32(USART2_DR) = x;
}

void hexstring ( unsigned int d, unsigned int cr ) {
  unsigned int rb;
  unsigned int rc;

  rb=32;
  while(1) {
    rb-=4;
    rc=(d>>rb)&0xF;
    if(rc>9) rc+=0x37; else rc+=0x30;
    uart_putc(rc);
    if(rb==0) break;
  }
  if(cr) {
    uart_putc(0x0D);
    uart_putc(0x0A);
  } else {
    uart_putc(0x20);
  }
}

void uart_string ( const char *s ) {
    for(;*s;s++) {
        if(*s==0x0A) uart_putc(0x0D);
        uart_putc(*s);
    }
}
