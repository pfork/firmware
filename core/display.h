#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// device layer
#ifdef DISPLAY_NOKIA
  #include "lcd.h"
#else
  #ifdef DISPLAY_OLED
    #include "oled.h"
  #else
    #error "no display device specified"
  #endif // DISPLAY_OLED
#endif

// display layer
void disp_clear(void);
void disp_drawchar(uint8_t x, uint8_t y, uint8_t c, char inverted);
void disp_setpixel(uint8_t x, uint8_t y);
void disp_delpixel(uint8_t x, uint8_t y);
void disp_print(uint8_t x, uint8_t y, char* text);
void disp_print_inv(uint8_t x, uint8_t y, char* text);

#endif //DISPLAY_H
