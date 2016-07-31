#include "oled.h"
#include "itoa.h"
#include <string.h>

void bsod(char* file, int line) {
   oled_clear();
   oled_print(0,0,"something died",Font_8x8);

   int len=strlen(file);
   if(len>16) file+=(len-16);
   oled_print(0,9,file,Font_8x8);

   char tmp[16];
   itos(tmp,line);
   oled_print(0,18,"line: ",Font_8x8);
   oled_print(6*8, 18, tmp, Font_8x8);

   oled_print_inv(18,45,"  HALT :/  ",Font_8x8);
   while(1);
}
