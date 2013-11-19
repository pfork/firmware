#include <sys/types.h>
#include "stm32f.h"
#include "uart.h"
#include "systimer.h"
#include "haveged.h"
#include "adc.h"

#define rotate32(num,bits) ((num >> bits) | (num << (32 -bits)))
unsigned int rng_last;

//-------------------------------------------------------------------
void rnd_init ( void )
{
    unsigned int ra;

    ra=GET32(RCC_AHB2ENR);
    ra|=1<<6;
    PUT32(RCC_AHB2ENR,ra);
    //RCC_AHB2ENR|=1<<6;
    RNG_CR|=4;
}
//-------------------------------------------------------------------
unsigned int next_rand ( void )
{
    unsigned int ra;

    while(1)
    {
        ra=RNG_SR;
        if(ra&1) break;
        if(ra&0x66) {
            while(1) {
              hexstring(0x00BADBAD,0); hexstring(ra,1);
            }
        }
    }
    if(ra&0x66) {
      while(1) { hexstring(0x01BADBAD,0); hexstring(ra,1); }
    }

    ra = RNG_DR;
    if(ra==rng_last) {
      while(1) hexstring(0x02BADBAD,1);
    }
    rng_last=ra;
    return(ra);
}

//-------------------------------------------------------------------
unsigned int rand_read ( const size_t size, unsigned char* buf)
{
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
unsigned int get_rand ( const size_t size, unsigned char* buf)
{
  unsigned int ra;
  unsigned int cnt = 0;
  unsigned char extrabits;
  extrabits = ((sysctr & 1) << 3) | ((read_temp() & 3) <<1) | (read_volt() & 1);
  while(cnt<size) {
    ra = next_rand() ^ Entropy[cnt/4];
    ra = rotate32(ra ,extrabits);
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
