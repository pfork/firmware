#ifndef DEVICE_3310
#err "this file makes only sense on 3310 style PITCHFORKS"
#endif // DEVICE_3310

#include <string.h>
#include "stm32f.h"
#include "display.h"
#include "buttons.h"
#include "systimer.h"
#include "widgets.h"
#include "itoa.h"

/* button to character mappings
 * 1: .,?!1@'"-()/:_;
 * 2: abc2
 * 3: def3
 * 4: ghi4
 * 5: jkl5
 * 6: mno6
 * 7: pqrs7
 * 8: tuv8
 * 9: wxyz9
 * #: +&%*=<>$[]\~^#|
 * 0:  0
 * *: Upper/lower-case, numbers, symbol
 *
 * in symbol mode:
 *
 * 0: " "
 * 1: ".,?"
 * 2: "!@'"
 * 3: "\"-("
 * 4: ")/:"
 * 5: "_;+"
 * 6: "&%*"
 * 7: "=<>"
 * 8: "$[]"
 * 9: "\\~^"
 * #: #
 *
 * total available chars 62 (Ab9) + 30
 */

const static struct {
  uint32_t elems;
  char *chars;
} main_keymap[] = {
  /* 0 */ {2, " 0"},
  /* 1 */ {15, ".,?!1@'\"-()/:_;"},
  /* 2 */ {4, "abc2"},
  /* 3 */ {4, "def3"},
  /* 4 */ {4, "ghi4"},
  /* 5 */ {4, "jkl5"},
  /* 6 */ {4, "mno6"},
  /* 7 */ {5, "pqrs7"},
  /* 8 */ {4, "tuv8"},
  /* 9 */ {5, "wxyz9"},
  /* # */ {15, "+&%*=<>$[]\\~^#|"}
}, symbol_keymap[] = {
  /* 0 */ {2, " |"},
  /* 1 */ {3, ".,?"},
  /* 2 */ {3, "!@'"},
  /* 3 */ {3, "\"-("},
  /* 4 */ {3, ")/:"},
  /* 5 */ {3, "_;+"},
  /* 6 */ {3, "&%*"},
  /* 7 */ {3, "=<>"},
  /* 8 */ {3, "$[]"},
  /* 9 */ {3, "\\~^"},
  /* # */ {1, "#"}
};

typedef enum {Lower = 0, Upper, Digits, Symbols} KeyMap;

#define KEY_TIMEOUT 1000 // milliseconds = 1s

typedef struct {
  uint8_t *passphrase;
  int pmax;
  unsigned long long tsp;
  int cur_char;
  KeyMap mode;
  int plen;
  char prev_butt;
} Sms_Input_State;

static int get_sms_pwd(Sms_Input_State *state) {
  char button;
  char *mode_indicator=0;
  int bindex;

  if(state->tsp && sysctr - state->tsp>KEY_TIMEOUT) { // start a new char
    state->cur_char=0;
    if(state->plen<state->pmax) {
      state->plen++;
      state->passphrase[state->plen]=0;
      gui_refresh=1;
    }
    state->prev_butt=0;
    state->tsp=0;
  }

  // handle display refresh
  if(gui_refresh) {
    switch(state->mode) {
    case Lower: {mode_indicator="abc"; break;; }
    case Upper: {mode_indicator="ABC"; break;; }
    case Digits: {mode_indicator="123"; break;; }
    case Symbols: {mode_indicator="!@?"; break;; }
    }
    disp_clear();
    disp_print(0,DISPLAY_HEIGHT-FONT_HEIGHT,mode_indicator);

    char slen[16];
    itos(slen,state->plen);
    if(slen[1]==0) { // padding
      slen[1]=slen[0];
      slen[0]=0x20;
      slen[2]=0;
    }
    disp_drawchar(DISPLAY_WIDTH/2-FONT_WIDTH/2, FONT_HEIGHT+1, state->passphrase[state->plen], 0);
    disp_print(7*FONT_WIDTH,0,slen);

    if(state->mode==Lower || state->mode==Upper) {
      disp_print(DISPLAY_WIDTH/2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*5,"1");
      disp_print(0,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*4,main_keymap[1].chars);
      if(state->prev_butt=='1') {
        // need to adjust for '@'
        int adjust=(state->cur_char>6?1:(state->cur_char==0?1:0));
        // calculate `(state->cur_char-1) % main_keymap[1].elems` correctly for negative values
        int cc=(((state->cur_char-1)%(int)main_keymap[1].elems) + ((int) main_keymap[1].elems)) % ((int) main_keymap[1].elems);
        disp_drawchar(FONT_WIDTH*cc+adjust,
                      DISPLAY_HEIGHT-(FONT_HEIGHT+1)*4,
                      main_keymap[1].chars[cc],
                      1);
        disp_refresh();
      }
      disp_print(DISPLAY_WIDTH/2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*3,"#");
      disp_print(0,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,main_keymap[10].chars);
      if(state->prev_butt=='#') {
        // adjust for '&'=2, '%'=3, '$'=8, '#'=14
        int adjust=(state->cur_char>14?4:(state->cur_char>8?3:(state->cur_char>3?2:(state->cur_char>2?1:(state->cur_char==0?4:0)))));
        // calculate `(state->cur_char-1) % main_keymap[10].elems` correctly for negative values
        int cc=(((state->cur_char-1)%(int)main_keymap[10].elems) + ((int) main_keymap[10].elems)) % ((int) main_keymap[10].elems);
        disp_drawchar(FONT_WIDTH*cc+adjust,
                      DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,
                      main_keymap[10].chars[cc],
                      1);
        disp_refresh();
      }
    } else if(state->mode==Symbols) {
      int i;
      for(i=0;i<11;i++) {
        if(i==10) {
          disp_drawchar(5*FONT_WIDTH*2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,'#',0);
          int j;
          for(j=0;j<symbol_keymap[i].elems;j++) {
            disp_drawchar(5*FONT_WIDTH*2+FONT_WIDTH*(j+1)+FONT_WIDTH/2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,symbol_keymap[i].chars[j],0);
          }
        } else if(i==0) {
          disp_drawchar(0,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,'0',0);
          int j;
          for(j=0;j<symbol_keymap[i].elems;j++) {
            disp_drawchar(FONT_WIDTH*(j+1)+FONT_WIDTH/2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*2,symbol_keymap[i].chars[j],0);
          }
        } else {
          disp_drawchar(5*FONT_WIDTH*((i-1)%3),DISPLAY_HEIGHT-(FONT_HEIGHT+1)*(5-((i-1)/3)),'0'+i,0);
          int j;
          for(j=0;j<symbol_keymap[i].elems;j++) {
            disp_drawchar(5*FONT_WIDTH*((i-1)%3)+FONT_WIDTH*(j+1)+FONT_WIDTH/2,DISPLAY_HEIGHT-(FONT_HEIGHT+1)*(5-((i-1)/3)),symbol_keymap[i].chars[j],0);
          }
        }
      }
      disp_refresh();
    }
    gui_refresh=0;
  }

  // check button pressed
  button = button_handler();
  if(button==0) {
    return 0;
  }

  if(button=='u' || button=='d') return 0; // these buttons are unused

  // some button of interest was released. let's handle it.

  if(button=='m') { // submit entered text, or cancel if len==0
    if((state->prev_butt || state->tsp) && state->plen<state->pmax) {
      state->plen++;
      state->passphrase[state->plen]=0;
    }
    if(state->plen>0) return state->plen; // submit input
    else return -1;
  }

  if(button=='c') { // delete last char
    state->passphrase[state->plen]=0;
    if(state->plen>0) {
      state->plen--;
    }
    state->cur_char = 0;
    state->tsp = 0;
    state->prev_butt=0;
    gui_refresh=1;
    return 0;
  }

  if(button=='*') { // change keymap
    state->cur_char=0;
    state->mode=(state->mode+1)%4;
    gui_refresh=1;
    state->prev_butt=0;
    if(state->tsp!=0) {
      if(state->plen<state->pmax) {
        state->plen++;
        state->passphrase[state->plen]=0;
      }
      state->tsp=0;
    }
    return 0;
  }

  if(button>='0' && button<='9') {
    bindex=button-'0';
  } else if(button=='#') {
    bindex=10;
    state->prev_butt=0;
  } else {
    return 0;
  }

  // button is 0-9# and bindex set accordingly
  gui_refresh = 1;

  if(state->mode == Digits) {
    state->passphrase[state->plen]=button;
    if(state->plen<state->pmax) {
      state->plen++;
      state->passphrase[state->plen]=0;
    }
    state->prev_butt=0;
    return 0;
  }

  if(state->prev_butt && button != state->prev_butt) { // start a new char
    state->cur_char=0;
    if(state->plen<state->pmax) {
      state->plen++;
      state->passphrase[state->plen]=0;
    }
  }
  state->prev_butt = button;

  state->tsp=sysctr; // update last used timestamp

  switch(state->mode) {
  case Lower: {
    state->passphrase[state->plen]=main_keymap[bindex].chars[state->cur_char];
    state->cur_char=(state->cur_char + 1) % main_keymap[bindex].elems;
    break;
  }
  case Upper: {
    state->passphrase[state->plen]=main_keymap[bindex].chars[state->cur_char];
    if(state->passphrase[state->plen]>='a' && state->passphrase[state->plen]<='z') {
      // uppercase
      state->passphrase[state->plen]^=0x20;
    }
    state->cur_char=(state->cur_char + 1) % main_keymap[bindex].elems;
    break;
  }
  case Symbols: {
    state->passphrase[state->plen]=symbol_keymap[bindex].chars[state->cur_char];
    state->cur_char=(state->cur_char + 1) % symbol_keymap[bindex].elems;
    break;
  }
  case Digits: { break; } // cannot happen
  }

  return 0;
}

static void get_sms_init(Sms_Input_State *state, uint8_t* passphrase, int pmax) {
  memset(passphrase,0,pmax);
  state->passphrase=passphrase;
  state->pmax=pmax;
  state->tsp=0;
  state->cur_char=0;
  state->mode=Lower;
  state->plen=0;
  state->prev_butt=0;
  gui_refresh=1;
}

/* @brief reads in max size byte string as a passphrase.
 *
 * Is a wrapper around get_sms_init and get_sms_pwd.
 */
int get_passcode(uint8_t *dst, unsigned int size) {
  int n;
  Sms_Input_State state;

  get_sms_init(&state, dst, size-1);

  while((n=get_sms_pwd(&state))==0);

  disp_clear();
  
  if(n<state.pmax && n>=0) return n;

  return 0;
}
