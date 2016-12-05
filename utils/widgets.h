#ifndef WIDGETS_H
#define WIDGETS_H

typedef struct {
  unsigned char idx;
  unsigned short top;
} __attribute((packed)) MenuCtx;

extern uint8_t gui_refresh;

void getstr(char *prompt, uint8_t *name, int *len);
int menu(MenuCtx *ctx, const uint8_t *menuitems[], const size_t menulen, void fn(char));
void statusline(void);

#endif // WIDGETS_H
