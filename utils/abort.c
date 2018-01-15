#include "display.h"
#include "itoa.h"
#include <string.h>

void bsod(char* file, int line) {
   disp_clear();
   disp_print(0,0,"something died");

   int len=strlen(file);
   if(len>16) file+=(len-16);
   disp_print(0,9,file);

   char tmp[16];
   itos(tmp,line);
   disp_print(0,18,"line: ");
   disp_print(6*8, 18, tmp);

   disp_print_inv(18,DISPLAY_HEIGHT-8,"  HALT :/  ");
   while(1);
}
