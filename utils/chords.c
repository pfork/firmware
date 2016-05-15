#include <stdint.h>
#include "pitchfork.h"
#include "keys.h"
#include "oled.h"
#include "delay.h"
#include "ntohex.h"

static int n=0, i=0;
static char prev;

void chord_reset(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t *outptr = outbuf, *outptr2 = outbuf+1024; // no bounds check careful!
  n=0; i=0; prev=0;
  outptr[0]=0;
  outptr2[0]=0;
  oled_print(0,0,"train chords",Font_8x8);
  oled_print(0,16,"5049544348464f524b",Font_8x8);
  oled_print(0,48,"press any key",Font_8x8);
  oled_print(0,56,"long + to exit",Font_8x8);
}

int chord_train(void) {
  uint8_t keys, str[2]=" ";
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t *outptr = outbuf, *outptr2 = outbuf+1024; // no bounds check careful!
  str[1]=0;
  keys = keys_pressed();
  if((keys & BUTTON_ENTER)!=0) {
    n++;
    if(n>400) {
      i=0;
      return 0;
    }
  } else {
    n=0;
  }

  ntohex(str, keys&0xf);
  oled_print(60,24,(char*) str,Font_8x8);

  if(i%2==0) {
    outptr2[i/2] = (outptr2[i/2] & 0xf) | ((keys&0xf) << 4);
  } else {
    outptr2[i/2] = (outptr2[i/2] & 0xf0) | (keys&0xf);
  }
  outptr2[(i/2)+1] =0;
  outptr[i] = str[0];
  outptr[i+1] =0;
  oled_print(00,32,(char*) outptr+((i>=16)*(i-15)),Font_8x8);
  oled_print(00,40,(char*) outptr2+((i>=32)*(i/2-15)),Font_8x8);

  if(keys & BUTTON_ENTER && prev!=keys) {
    // accept entry
    i++;
    mDelay(200);
  }
  prev=keys;
  return 1;
}
