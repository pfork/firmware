#include <stdint.h>
#include <stddef.h>

void ntohex(uint8_t* o, uint8_t d) {
  unsigned int rb;
  unsigned int rc;
  if(o==NULL) return;

  rb=4;
  while(1) {
    rb-=4;
    rc=(d>>rb)&0xF;
    if(rc>9) rc+=0x37; else rc+=0x30;
    *(o++)=rc;
    if(rb==0) break;
  }
}
