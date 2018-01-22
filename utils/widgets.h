#ifndef WIDGETS_H
#define WIDGETS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  unsigned char idx;
  unsigned short top;
} __attribute((packed)) MenuCtx;

extern uint8_t gui_refresh;

void getstr(char *prompt, uint8_t *name, int *len);
int menu(MenuCtx *ctx, const uint8_t *menuitems[], const size_t menulen, void fn(char));
int get_passcode(uint8_t *dst, unsigned int size);
void statusline(void);

#endif // WIDGETS_H
