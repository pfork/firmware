#include <stdint.h>
#include <string.h>
#include "dual.h"
#include "nrf.h"
#include "display.h"
#include "buttons.h"
#include "delay.h"
#include "dma.h"
#include "user.h"
#include "widgets.h"
#include "master.h"
#include "itoa.h"

void getstr(char *prompt, uint8_t *name, int *len) {
  uint8_t buttons, i;
  short cur=32;
  *len=0;

  while(1) {
    disp_clear();
    disp_print(0,0, prompt);
#ifdef DEVICE_3310
    disp_print(0,(FONT_HEIGHT+1)*1, "[menu] save");
    disp_print(0,(FONT_HEIGHT+1)*2, "[5] select");
    disp_print(0,(FONT_HEIGHT+1)*3, "[c] delete");
    disp_print(0,(FONT_HEIGHT+1)*4, "4/6 <-/->");
    disp_print(0,(FONT_HEIGHT+1)*5, "2/8 up/down");
    disp_print(0,(FONT_HEIGHT+1)*6, "press any key");
    disp_print(0,(FONT_HEIGHT+1)*7, "to continue..");
#else
    disp_print(0,FONT_HEIGHT, "enter select");
    disp_print(0,(FONT_HEIGHT)*2, "enter+< delete");
    disp_print(0,(FONT_HEIGHT)*3, "enter+> submit");
    disp_print(0,(FONT_HEIGHT)*4, "press any key");
    disp_print(0,(FONT_HEIGHT)*5, "to continue..");
#endif // DEVICE_3310
    while((buttons=button_handler())==0);
    while(1) {
#ifdef DEVICE_GH
      mDelay(100);
#endif // DEVICE_GH
      disp_clear();

      // write name
      for(i=0;i<=32;i++) {
        if(i<*len) {
          disp_drawchar((i%16)*FONT_WIDTH,(i/16)*FONT_HEIGHT, name[i], 0);
        } else if(i==*len) {
          disp_drawchar((i%16)*FONT_WIDTH,(i/16)*FONT_HEIGHT, '_', 1 );
        } else {
          disp_drawchar((i%16)*FONT_WIDTH,(i/16)*FONT_HEIGHT, '_', 0);
        }
      }

      // write char table
      for(i=32;i<=128;i++) {
// todo is this ugly this macro magic below?
#if (FONT_HEIGHT==5)
#undef FONT_HEIGHT
#define FONT_HEIGHT 6
#endif
        disp_drawchar((i%16)*FONT_WIDTH,(i/16)*FONT_HEIGHT, i, i==cur);
      }
      disp_refresh();

#ifdef DEVICE_GH
      while(buttons_pressed()==0);
      mDelay(10);
      buttons = buttons_pressed();

      if(buttons == (BUTTON_ENTER | BUTTON_RIGHT)) { // submit
        break;
      } else if(buttons ==  (BUTTON_ENTER)) { // enter
        if(*len<32) {
          name[*len]=cur;
          (*len)++;
        }
      } else if (buttons == (BUTTON_ENTER | BUTTON_LEFT)) { // delete
        if(*len>0) {
          (*len)--;
        }
      } else if(buttons == BUTTON_LEFT) {
        cur--;
      } else if(buttons == BUTTON_RIGHT) {
        cur++;
      } else if(buttons == BUTTON_UP) {
        cur-=16;
      } else if(buttons == BUTTON_DOWN) {
        cur+=16;
      }
#else // we assume DEVICE_3310
      while((buttons=button_handler())==0);

      if(buttons == ('m')) { // submit
        break;
      } else if(buttons ==  '5') { // enter
        if(*len<32) {
          name[*len]=cur;
          (*len)++;
        }
      } else if (buttons == 'c') { // delete
        if(*len>0) {
          (*len)--;
        }
      } else if(buttons == '4') {
        cur--;
      } else if(buttons == '6') {
        cur++;
      } else if(buttons == '2') {
        cur-=16;
      } else if(buttons == '8') {
        cur+=16;
      }
#endif // DEVICE_GH
      if(cur<32) {
        cur=128+cur-32;
      } else if(cur>128) {
        cur=cur-128+32;
      }
    }
    disp_clear();
    name[*len]=0;
    disp_print(0,0,(char*) name);
#ifdef DEVICE_GH
    disp_print(0,FONT_HEIGHT, "< cancel");
    disp_print(0,FONT_HEIGHT*2, "> ok");
    disp_print(0,FONT_HEIGHT*3, "j/k edit");
    mDelay(500);
    while((buttons=button_handler())==0);
    if(buttons & BUTTON_LEFT) {
      dmaset32(name,0, 8);
      len=0;
      return;
    } else if(buttons & BUTTON_RIGHT) {
      return;
    }
#else // assume DEVICE_3310
    disp_print(0,8, "[c] cancel");
    disp_print(0,16, "[menu] ok");
    disp_print(0,24, "other keys: edit");
    mDelay(500);
    while((buttons=button_handler())==0);
    if(buttons == 'c') {
      dmaset32(name,0, 8);
      len=0;
      return;
    } else if(buttons == 'm') {
      return;
    }
#endif // DEVICE_GH
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
      disp_print(0,DISPLAY_HEIGHT-FONT_HEIGHT, "R");
    } else {
      disp_print(0,DISPLAY_HEIGHT-FONT_HEIGHT, "r");
    }
    if(chrg) {
      disp_print(FONT_WIDTH,DISPLAY_HEIGHT-FONT_HEIGHT, "d");
    } else {
      disp_print(FONT_WIDTH,DISPLAY_HEIGHT-FONT_HEIGHT, "c");
    }
    if(sdcd) {
      disp_print(FONT_WIDTH*2,DISPLAY_HEIGHT-FONT_HEIGHT, "s");
    } else {
      disp_print(FONT_WIDTH*2,DISPLAY_HEIGHT-FONT_HEIGHT, "S");
    }
    if(mode==CRYPTO) {
      disp_print(FONT_WIDTH*3,DISPLAY_HEIGHT-FONT_HEIGHT, "C");
    } else {
      disp_print(FONT_WIDTH*3,DISPLAY_HEIGHT-FONT_HEIGHT, "D");
    }
    if(pitchfork_hot==0) {
      disp_print_inv(0,0,  " PITCHFORK!!5!  ");
    }
    if(have_user) {
      disp_print_inv(DISPLAY_WIDTH-(userrec->len)*FONT_WIDTH, FONT_HEIGHT, (char*) &userrec->name);
    }
  }
}

// returns 0 if <- has been pressed, else 1
int menu(MenuCtx *ctx, const uint8_t *menuitems[], const size_t menulen, void fn(char)) {
  uint8_t buttons;
  int i;

  // menu
  buttons = button_handler();
#ifdef DEVICE_GH
  if((buttons & BUTTON_UP) && (ctx->idx>0)) {
#else // assume DEVICE_3310
  if((buttons == BUTTON_UP) && (ctx->idx>0)) {
#endif
    ctx->idx--;
    if(ctx->idx<ctx->top) ctx->top--;
    gui_refresh=1;
  }
#ifdef DEVICE_GH
  if((buttons & BUTTON_DOWN) && (ctx->idx<menulen-1)) {
#else // assume DEVICE_3310
  if((buttons == BUTTON_DOWN) && (ctx->idx<menulen-1)) {
#endif
    ctx->idx++;
    if(ctx->idx>=ctx->top+4) ctx->top++;
    gui_refresh=1;
  }

#ifdef DEVICE_GH
  if(buttons & BUTTON_RIGHT) {
#else // assume DEVICE_3310
  if(buttons == BUTTON_RIGHT) {
#endif
    if(fn!=NULL) fn(ctx->idx);
    return 1;
  }
#ifdef DEVICE_GH
  if(buttons & BUTTON_LEFT) {
#else // assume DEVICE_3310
  if(buttons == BUTTON_LEFT) {
#endif
    disp_clear();
    gui_refresh=1;
    return 0;
  }

  if(gui_refresh) {
    disp_clear();
    statusline();
    for(i=ctx->top;i<ctx->top+4 && i<menulen;i++) {
      disp_print(FONT_WIDTH*1.5,FONT_HEIGHT*(i-ctx->top)+FONT_HEIGHT*2, (char*) menuitems[i]);
      if(i==ctx->idx) {
        disp_print_inv(0,FONT_HEIGHT*(i-ctx->top)+FONT_HEIGHT*2,  ">");
      } else {
        disp_print(0,FONT_HEIGHT*(i-ctx->top)+FONT_HEIGHT*2,  " ");
      }
    }
    gui_refresh=0;
  }
  return 1;
}

#ifdef DEVICE_GH
// reads in max (size*2) hex-digits as chords composing a size byte long string.
// reads the next digit on pressing the enter button
// premature exiting can be achieved by pressing enter button longer for the last digit
int get_passcode(uint8_t *dst, unsigned int size) {
  uint8_t buttons, lastbyte=0;
  int n=0, i=0;
  char prev=0;
  while(i<size*2) {
    buttons = buttons_pressed();
    if(buttons & BUTTON_ENTER) {
      n++;
      if(n>400) {
        if(i>0)
            dst[(i-1)/2]=lastbyte; // undo the last enter-ed digit
        return i/2;
      }
    } else {
      n=0;
    }

    if(buttons & BUTTON_ENTER && prev!=buttons) {
      lastbyte=dst[i/2]; // in case this is the enter for finishing the chords
      // accept entry
      if(i%2==0) {
        dst[i/2] = (dst[i/2] & 0xf) | ((buttons&0xf) << 4);
      } else {
        dst[i/2] = (dst[i/2] & 0xf0) | (buttons&0xf);
      }
      i++;
      char tmp[16];
      itos(tmp,i);
      disp_print(7*8,DISPLAY_HEIGHT/2-4,tmp);
      mDelay(200);
    }
    prev=buttons;
  }
  return i/2;
}
#else // assume DEVICE_3310
#include "sms_passphrase.c"
#endif
