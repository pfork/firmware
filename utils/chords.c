#include <stdint.h>
#include "pitchfork.h"
#include "keys.h"
#include "oled.h"
#include "ntohex.h"

extern void (*app)(void);

static uint8_t prev=0;
static int n=0, i=0;

void chord_reset(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t *outptr = outbuf, *outptr2 = outbuf+1024; // no bounds check careful!
  n=0; i=0;
  outptr[0]=0;
  outptr2[0]=0;
  oled_print(0,0,"train chords",Font_8x8);
  oled_print(0,16,"737466",Font_8x8);
  oled_print(0,48,"press any key",Font_8x8);
  oled_print(0,56,"long to exit",Font_8x8);
}

int chord_train(void) {
  uint8_t keys, str[2]=" ";
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t *outptr = outbuf, *outptr2 = outbuf+1024; // no bounds check careful!
  str[1]=0;
  keys = keys_pressed();
  if(prev!=keys) {
    ntohex(str, keys);
    oled_print(60,24,(char*) str,Font_8x8);
    n=0;
    prev=keys;
    return 1;
  }
  n++;
  if(keys!=0) {
    if(n==10000) {
      // accept entry
      outptr2[i/2] |= keys << (((i+1)%2)*4);
      outptr2[(i/2)+1] =0;
      outptr[i++] = str[0];
      outptr[i] =0;
      oled_print(00,32,(char*) outptr,Font_8x8);
      oled_print(00,40,(char*) outptr2,Font_8x8);
    }
    if(n>40000) {
      oled_clear();
      return 0;
    }
  }
  return 1;
}
