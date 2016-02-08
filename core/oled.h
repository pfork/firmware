#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include "smallfonts.h"

extern uint8_t frame_buffer[];

void oled_init(void);
void oled_clear(void);
void oled_refresh(void);
void oled_setpixel(uint8_t x, uint8_t y);
void oled_delpixel(uint8_t x, uint8_t y);
void oled_print(uint8_t x, uint8_t y, char* text, struct FONT_DEF font);
void oled_cmd(uint8_t reg);

#endif //OLED_H
