#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define PCD8544_MAX_BANKS 6
#define PCD8544_MAX_COLS 84

#define DISPLAY_WIDTH PCD8544_MAX_COLS
#define DISPLAY_HEIGHT (PCD8544_MAX_BANKS*8)

#define FONT_START 0x20
#define FONT_END  0x7e
#define FONT_WIDTH  5
#define FONT_HEIGHT 5

extern uint8_t frame_buffer[48 * 84 / 8];

void disp_init(void);
void disp_refresh(void);
void disp_show_logo(void);
void disp_invert(void);
void disp_normal(void);

#endif // LCD_H
