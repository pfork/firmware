#include "stm32f.h"

/**
* @brief  clock_init
*         main clock 120MHz
*         PPRE1 divide by 4 = 30MHz
*         30000000/115200 = 260.4166
*           260/16 = 16.25
*          .25*16 = 4
*          divisor 16 fractional divisor 4
*          21000000/260 = 115384
* @param  None
* @retval None
*/
void clock_init ( void ) {
    //enable HSE
    MMIO32(RCC_CR) &= ~(0xF<<16);
    MMIO32(RCC_CR) |= 1<<16;
    while(1) {
      if(MMIO32(RCC_CR)&(1<<17)) break;
    }
    PUT32(RCC_CFGR,0x00009401); //PPRE2 /2 PPRE1 /4 sw=hse
    // enable caches/prefetch, set waitstate to 3
    PUT32(FLASH_ACR,0x00000703);
    //Q 5 P 2 N 240 M 25  vcoin 1 pllvco 336 pllgen 120 pllusb 48
    MMIO32(RCC_PLLCFGR) = (5<<24)|(1<<22)|(((2>>1)-1)<<16)|(240<<6)|(25<<0);
    // enable pll
    MMIO32(RCC_CR) |= (1<<24);
    //wait for pll lock
    while(1) {
      if(GET32(RCC_CR)&(1<<25)) break;
    }
    //select pll
    PUT32(RCC_CFGR,0x00009402); //PPRE2 /2 PPRE1 /8 sw=pllclk
    //wait for it to use the pll
    while(1) {
        if((GET32(RCC_CFGR)&0xC)==0x8) break;
    }
 }
