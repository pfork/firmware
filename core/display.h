#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// device layer
#ifdef DEVICE_3310
  #include "lcd.h"
#else
  #ifdef DEVICE_GH
    #include "oled.h"
  #else
    #error "no device specified"
  #endif // DEVICE_GH
#endif

// display layer
void disp_clear(void);
void disp_drawchar(uint8_t x, uint8_t y, uint8_t c, char inverted);
void disp_setpixel(uint8_t x, uint8_t y);
void disp_delpixel(uint8_t x, uint8_t y);
void disp_print(uint8_t x, uint8_t y, char* text);
void disp_print_inv(uint8_t x, uint8_t y, char* text);

#endif //DISPLAY_H
