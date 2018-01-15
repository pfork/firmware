#ifndef OLED_H
#define OLED_H

#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define FONT_START  0x20
#define FONT_END    0x80
#define FONT_WIDTH  8
#define FONT_HEIGHT 8

extern uint8_t frame_buffer[128 * 64 / 8];

void disp_init(void);
void disp_refresh(void);
void disp_show_logo(void);
void disp_invert(void);
void disp_normal(void);

#endif //OLED_H
