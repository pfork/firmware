#include <sys/types.h>
#include "stm32f.h"

unsigned int rng_last;

//-------------------------------------------------------------------
void rnd_init ( void ) {
    MMIO32(RCC_AHB2ENR) |=1<<6;
    RNG_CR|=4;
}
//-------------------------------------------------------------------
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
      }
    }

    ra = RNG_DR;
    if(ra==rng_last) {
      while(1) {
      }
    }
    rng_last=ra;
    return(ra);
}

//-------------------------------------------------------------------
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
//-------------------------------------------------------------------
