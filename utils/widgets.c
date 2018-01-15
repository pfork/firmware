#include <stdint.h>
#include <string.h>
#include "dual.h"
#include "nrf.h"
#include "display.h"
#include "keys.h"
#include "delay.h"
#include "dma.h"
#include "user.h"
#include "widgets.h"
#include "master.h"

void getstr(char *prompt, uint8_t *name, int *len) {
  uint8_t keys, i;
  short cur=32;

  while(1) {
    disp_clear();
    disp_print(0,0, prompt);
    disp_print(0,8, "enter select");
    disp_print(0,16, "enter+< delete");
    disp_print(0,24, "enter+> submit");
    disp_print(0,40, "press any key");
    disp_print(0,48, "to continue..");
    while((keys=keys_pressed())==0);
    while(1) {
      mDelay(100);
      disp_clear();

      // write name
      for(i=0;i<=32;i++) {
        if(i<*len) {
          disp_drawchar((i%16)*8,(i/16)*8, name[i], 0);
        } else if(i==*len) {
          disp_drawchar((i%16)*8,(i/16)*8, '_', 1 );
        } else {
          disp_drawchar((i%16)*8,(i/16)*8, '_', 0);
        }
      }

      // write char table
      for(i=32;i<=128;i++) {
        // todo does not work on NOKIA display, it has only 6 rows :/
        disp_drawchar((i%16)*8,(i/16)*8, i, i==cur);
      }
      disp_refresh();

      while(keys_pressed()==0);
      mDelay(10);
      keys = keys_pressed();

      if(keys == (BUTTON_ENTER | BUTTON_RIGHT)) { // submit
        break;
      } else if(keys ==  (BUTTON_ENTER)) { // enter
        if(*len<32) {
          name[*len]=cur;
          (*len)++;
        }
      } else if (keys == (BUTTON_ENTER | BUTTON_LEFT)) { // delete
        if(*len>0) {
          (*len)--;
        }
      } else if(keys == BUTTON_LEFT) {
        cur--;
      } else if(keys == BUTTON_RIGHT) {
        cur++;
      } else if(keys == BUTTON_UP) {
        cur-=16;
      } else if(keys == BUTTON_DOWN) {
        cur+=16;
      }
      if(cur<32) {
        cur=128+cur-32;
      } else if(cur>128) {
        cur=cur-128+32;
      }
    }
    disp_clear();
    name[*len]=0;
    disp_print(0,0,(char*) name);
    disp_print(0,8, "< cancel");
    disp_print(0,16, "> ok");
    disp_print(0,24, "j/k edit");
    mDelay(500);
    while((keys=keys_pressed())==0);
    if(keys & BUTTON_LEFT) {
      dmaset32(name,0, 8);
      len=0;
      return;
    } else if(keys & BUTTON_RIGHT) {
      return;
    }
  }
}

uint8_t gui_refresh=1;
static uint8_t rf=0, mode=CRYPTO, chrg=0, sdcd=0;
static uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX+1];
static UserRecord *userrec=(UserRecord*) userbuf;
static char have_user=0;
static uint32_t blinker;

void statusline(void) {
  uint8_t refresh=gui_refresh;
  uint8_t tmp;
  if(have_user==0) {
    if(get_user(userrec)==0 &&
       userrec->len>0 &&
       userrec->len<=32) {
      have_user=1;
      ((uint8_t*) &(userrec->name))[userrec->len]=0;
    }
  }

  tmp = nrf_read_reg(STATUS);
  if(tmp != 0 && rf==0) {
    refresh=1;
    rf=1;
  } else if(tmp==0 && rf==1) {
    refresh=1;
    rf=0;
  }

  if(dual_usb_mode == CRYPTO && mode==DISK) {
    refresh=1;
    mode=CRYPTO;
  } else if(dual_usb_mode == DISK && mode==CRYPTO) {
    refresh=1;
    mode=DISK;
  }

  tmp=gpio_get(GPIOA_BASE, 1 << 2);
  if(tmp && chrg==0) {
    refresh=1;
    chrg=1;
  } else if(!tmp && chrg==1) {
    refresh=1;
    chrg=0;
  }

  tmp=gpio_get(GPIOC_BASE, 1 << 13);
  if(tmp && sdcd==0) {
    refresh=1;
    sdcd=1;
  } else if(!tmp && sdcd==1) {
    refresh=1;
    sdcd=0;
  }

  if(pitchfork_hot!=0) {
    if(blinker++ & 0x80) {
      disp_print_inv(0,0,  " PITCHFORK!!5!  ");
    } else {
      disp_print(0,0,  " PITCHFORK!!5!  ");
    }
  }

  if(refresh) {
    if(rf) {
      disp_print(0,56,  "R");
    } else {
      disp_print(0,56,  "r");
    }
    if(chrg) {
      disp_print(8,56,  "d");
    } else {
      disp_print(8,56,  "c");
    }
    if(sdcd) {
      disp_print(16,56,  "s");
    } else {
      disp_print(16,56,  "S");
    }
    if(mode==CRYPTO) {
      disp_print(24,56,  "C");
    } else {
      disp_print(24,56,  "D");
    }
    if(pitchfork_hot==0) {
      disp_print_inv(0,0,  " PITCHFORK!!5!  ");
    }
    if(have_user) {
      disp_print_inv(128-(userrec->len)*8,8, (char*) &userrec->name);
    }
  }
}

// returns 0 if <- has been pressed, else 1
int menu(MenuCtx *ctx, const uint8_t *menuitems[], const size_t menulen, void fn(char)) {
  uint8_t keys;
  int i;

  // menu
  keys = key_handler();
  if((keys & BUTTON_UP) && (ctx->idx>0)) {
    ctx->idx--;
    if(ctx->idx<ctx->top) ctx->top--;
    gui_refresh=1;
  }
  if((keys & BUTTON_DOWN) && (ctx->idx<menulen-1)) {
    ctx->idx++;
    if(ctx->idx>=ctx->top+4) ctx->top++;
    gui_refresh=1;
  }

  if(keys & BUTTON_RIGHT) {
    if(fn!=NULL) fn(ctx->idx);
    return 1;
  }
  if(keys & BUTTON_LEFT) {
    disp_clear();
    gui_refresh=1;
    return 0;
  }

  if(gui_refresh) {
    disp_clear();
    statusline();
    // todo does not work on NOKIA display, it has only 6 rows :/
    for(i=ctx->top;i<ctx->top+4 && i<menulen;i++) {
      disp_print(12,8*(i-ctx->top)+16, (char*) menuitems[i]);
      if(i==ctx->idx) {
        disp_print_inv(0,8*(i-ctx->top)+16,  ">");
      } else {
        disp_print(0,8*(i-ctx->top)+16,  " ");
      }
    }
    gui_refresh=0;
  }
  return 1;
}
