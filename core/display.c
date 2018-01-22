#include <stdint.h>
#include <string.h>
#include "display.h"

#ifdef DEVICE_3310
  #include "lcd.c"
#else // defined(DEVICE_GH)
  #include "oled.c"
#endif

void disp_clear(void) {
  memset(frame_buffer, 0, sizeof(frame_buffer));
  disp_refresh();
}

void disp_setpixel(uint8_t x, uint8_t y) {
  if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT))
    return;
  frame_buffer[x+(y/8)*DISPLAY_WIDTH] |= 1 << (y%8);
}

void disp_delpixel(uint8_t x, uint8_t y) {
  if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT))
    return;

  frame_buffer[x+ (y/8)*DISPLAY_WIDTH] &= ~(1 << y%8);
}

void disp_drawchar(uint8_t x, uint8_t y, uint8_t c, char inverted) {
  uint8_t col, column[FONT_WIDTH];

  // Check if the requested character is available
  if ((c >= FONT_START) && (c <= FONT_END)) {
    // Retrieve appropriate columns from font data
    for (col = 0; col < FONT_WIDTH; col++) {
      column[col] = Font[((c - FONT_START) * FONT_WIDTH) + col]; // Get first column of appropriate character
    }
  } else {
    // Requested character is not available in this font ... send a space instead
    for (col = 0; col < FONT_WIDTH; col++) {
      column[col] = 0xFF; // Send solid space
    }
  }

  // Render each column
  uint8_t xoffset, yoffset;
  for (xoffset = 0; xoffset < FONT_WIDTH; xoffset++) {
    for (yoffset = 0; yoffset < (FONT_HEIGHT + 1); yoffset++) {
      if ((column[xoffset] >> yoffset) & 1) {
        if(inverted)
          disp_delpixel(x + xoffset, y + yoffset);
        else
          disp_setpixel(x + xoffset, y + yoffset);
      } else {
        if(inverted)
          disp_setpixel(x + xoffset, y + yoffset);
        else
          disp_delpixel(x + xoffset, y + yoffset);
      }
    }
  }
}

static void _disp_print(uint8_t x, uint8_t y, char* text, char inv) {
  int l;
  const int text_len=strlen(text);
  for (l = 0; l < text_len && x+(l*(FONT_WIDTH))<DISPLAY_WIDTH; l++) {
#if(defined(DEVICE_3310) && FONT_WIDTH==5)
    // do some semi-proportional rendering of certain characters
    switch(text[l]) {
    case '#':
    case '$':
    case '%':
    case '&':
    case '0':
    case '@':
    case 'M':
    case 'V':
    case 'W':
    case 'm':
    case 'w':
      if((x+(l*(FONT_WIDTH))) % FONT_WIDTH < DISPLAY_WIDTH % FONT_WIDTH &&
         (l > 1)) {
        x++;
      }
    default: break;
    }
#endif // FONT_WIDTH==5
    disp_drawchar(x + (l * (FONT_WIDTH)), y, text[l], inv);
  }
  disp_refresh();
}

void disp_print_inv(uint8_t x, uint8_t y, char* text) {
  _disp_print(x, y, text, 1);
}

void disp_print(uint8_t x, uint8_t y, char* text) {
  _disp_print(x, y, text, 0);
}
