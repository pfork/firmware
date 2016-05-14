#include <stdint.h>
#include <string.h>
#include "dual.h"
#include "nrf.h"
#include "oled.h"
#include "keys.h"
#include "delay.h"
#include "dma.h"
#include "storage.h"
#include "widgets.h"

void getstr(char *prompt, uint8_t *name, int *len) {
  uint8_t keys, i;
  short cur=32;

  while(1) {
    oled_clear();
    oled_print(0,0,(char*) prompt,Font_8x8);
    oled_print(0,8,(char*) "<+> select",Font_8x8);
    oled_print(0,16,(char*) "j+k delete",Font_8x8);
    oled_print(0,24,(char*) "<+j+k+> submit",Font_8x8);
    oled_print(0,40,(char*) "press any key",Font_8x8);
    oled_print(0,48,(char*) "to continue..",Font_8x8);
    while((keys=keys_pressed())==0);
    while(1) {
      mDelay(100);
      oled_clear();

      // write name
      for(i=0;i<=32;i++) {
        if(i<*len) {
          oled_drawchar((i%16)*8,(i/16)*8, name[i], Font_8x8, 0);
        } else if(i==*len) {
          oled_drawchar((i%16)*8,(i/16)*8, '_', Font_8x8, 1 );
        } else {
          oled_drawchar((i%16)*8,(i/16)*8, '_', Font_8x8, 0);
        }
      }

      // write char table
      for(i=32;i<=128;i++) {
        oled_drawchar((i%16)*8,(i/16)*8, i, Font_8x8, i==cur);
      }
      oled_refresh();

      while(keys_pressed()==0);
      mDelay(10);
      keys = keys_pressed();

      if(keys == (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)) { // submit
        break;
      } else if(keys ==  (BUTTON_LEFT | BUTTON_RIGHT)) { // enter
        if(*len<32) {
          name[*len]=cur;
          (*len)++;
        }
      } else if (keys == (BUTTON_DOWN| BUTTON_UP)) { // delete
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
        cur=cur+32;
      } else if(cur>128) {
        cur=cur-128+32;
      }
    }
    oled_clear();
    name[*len]=0;
    oled_print(0,0,(char*) name,Font_8x8);
    oled_print(0,8,(char*) "< cancel",Font_8x8);
    oled_print(0,16,(char*) "> ok",Font_8x8);
    oled_print(0,24,(char*) "j/k edit",Font_8x8);
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
uint8_t rf=0, mode=CRYPTO, chrg=0, sdcd=0;
UserRecord* userec = NULL;

void statusline(void) {
  uint8_t refresh=gui_refresh;
  uint8_t tmp;
  if(userec==NULL) userec=get_userrec();

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

  // todo this chrg_stat info is unreliable/inconclusive?
  // e.g. does show c even if not on usb
  tmp=gpio_get(GPIOA_BASE, 2);
  if(tmp && chrg==0) {
    refresh=1;
    chrg=1;
  } else if(!tmp && chrg==1) {
    refresh=1;
    chrg=0;
  }

  tmp=gpio_get(GPIOC_BASE, 13);
  if(tmp && sdcd==0) {
    refresh=1;
    sdcd=1;
  } else if(!tmp && sdcd==1) {
    refresh=1;
    sdcd=0;
  }
  if(refresh) {
    if(rf) {
      oled_print(0,56, (char*) "R", Font_8x8);
    } else {
      oled_print(0,56, (char*) "r", Font_8x8);
    }
    if(chrg) {
      oled_print(8,56, (char*) "c", Font_8x8);
    } else {
      oled_print(8,56, (char*) "d", Font_8x8);
    }
    if(sdcd) {
      oled_print(16,56, (char*) "s", Font_8x8);
    } else {
      oled_print(16,56, (char*) "S", Font_8x8);
    }
    if(mode==CRYPTO) {
      oled_print_inv(32,56, (char*) " Crypto mode", Font_8x8);
    } else {
      oled_print_inv(32,56, (char*) "   Disk mode", Font_8x8);
    }
    oled_print_inv(0,0, (char*) " PITCHFORK!!5!  ", Font_8x8);
    if(userec) {
      char name[33];
      memcpy(name, userec->name, userec->len - USERDATA_HEADER_LEN);
      name[userec->len - USERDATA_HEADER_LEN]=0;
      oled_print_inv(128-(userec->len - USERDATA_HEADER_LEN)*8,8, name, Font_8x8);
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
    oled_clear();
    gui_refresh=1;
    return 0;
  }

  if(gui_refresh) {
    oled_clear();
    statusline();
    for(i=ctx->top;i<ctx->top+4 && i<menulen;i++) {
      oled_print(12,8*(i-ctx->top)+16, (char*) menuitems[i], Font_8x8);
      if(i==ctx->idx) {
        oled_print_inv(0,8*(i-ctx->top)+16, (char*) ">", Font_8x8);
      } else {
        oled_print(0,8*(i-ctx->top)+16, (char*) " ", Font_8x8);
      }
    }
    gui_refresh=0;
  }
  return 1;
}
