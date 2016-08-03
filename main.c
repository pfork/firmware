/**
  ************************************************************************************
  * @file    main.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file contains the main init and then runs the mainloop
  ************************************************************************************
  */

#ifdef HAVE_MSC
#  include "dual.h"
#else
#  include "usb.h"
#endif // HAVE_MSC
#include "init.h"
#include "led.h"
#include "keys.h"
#include "randombytes_pitchfork.h"
#include "mixer.h"
#include "pitchfork.h"
#include "systimer.h"
#include "oled.h"
#include <string.h>

#include "delay.h"
#include "nrf.h"
#include "dma.h"
#include "utils/lzg/lzg.h"
#include "irq.h"
#include "widgets.h"
#include "flashdbg.h"
#include "chords.h"
#include "kex.h"
#include "storage.h"
#include "main.h"
#include "fwsig.h"

void randombytes_pitchfork_init(struct entropy_store* pool);
struct entropy_store* pool;

void fancycls(void) {
  // vertical line
  uint8_t i,j,w;
  for(j=0;j<0x80-8;j++) {
    for(i=0;i<64;i++) {
      for(w=0;w<8;w++) {
        //oled_delpixel((j-1+w)&0x7f,16+i);
        //oled_setpixel((j+w)&0x7f,16+i);
        oled_delpixel((j-1+w)&0x7f,i);
        oled_setpixel((j+w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    oled_refresh();
  }
  for(j=0x80;j>8;j--) {
    for(i=0;i<64;i++) {
      for(w=0;w<8;w++) {
        //oled_delpixel((j-1+w)&0x7f,16+i);
        //oled_setpixel((j+w)&0x7f,16+i);
        oled_setpixel((j-1-w)&0x7f,i);
        oled_delpixel((j-w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    oled_refresh();
  }
  oled_clear();
}

void init_chrgstat(void) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) GPIOA_BASE;
  greg->MODER |= (GPIO_Mode_IN << (2 << 1));
  greg->PUPDR |= (GPIO_PuPd_UP << (2 << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (2 << 1));
}

void init_sdcd(void) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) GPIOC_BASE;
  greg->MODER |= (GPIO_Mode_IN << (13 << 1));
  greg->PUPDR |= (GPIO_PuPd_UP << (13 << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (13 << 1));
}

static const unsigned char bitmap_lzg[] = {
  0x4c, 0x5a, 0x47, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x70, 0x37,
  0x35, 0x6b, 0xbb, 0x01, 0x05, 0x06, 0x08, 0x09, 0x00, 0x00, 0x09, 0x1b,
  0x80, 0xc0, 0xc0, 0x80, 0x06, 0x0f, 0x0a, 0xe0, 0x09, 0x01, 0x06, 0x0f,
  0x10, 0x09, 0x1b, 0xe0, 0xe0, 0x06, 0x1c, 0x1d, 0x09, 0x09, 0x80, 0x80,
  0xc0, 0x06, 0x05, 0x01, 0xf3, 0xff, 0x7f, 0x06, 0x08, 0x0f, 0xf0, 0xfc,
  0x7e, 0x1f, 0x07, 0x03, 0x01, 0x08, 0x43, 0x01, 0x03, 0x07, 0x1f, 0xfe,
  0xf8, 0x06, 0x04, 0x4f, 0xf8, 0xf8, 0x18, 0x06, 0x0c, 0x37, 0x3c, 0x7c,
  0xf0, 0xe0, 0x06, 0x04, 0x98, 0xf0, 0xf0, 0xe0, 0x80, 0xc0, 0xe0, 0xf8,
  0x3f, 0x1f, 0x06, 0x1c, 0x6d, 0x00, 0x70, 0x78, 0x08, 0x2f, 0x09, 0x04,
  0xc3, 0xe7, 0xff, 0x7e, 0x78, 0xf8, 0xfc, 0x9f, 0x0f, 0x06, 0x04, 0x66,
  0x09, 0x04, 0x3f, 0xff, 0xff, 0x06, 0x03, 0x4d, 0xe0, 0x06, 0x04, 0xc7,
  0x80, 0xff, 0x06, 0x05, 0x94, 0x07, 0x1f, 0x3e, 0x78, 0x70, 0x08, 0x32,
  0x80, 0x09, 0x04, 0xc0, 0xc0, 0xe0, 0xf8, 0x7c, 0x19, 0x01, 0x03, 0x09,
  0x03, 0x07, 0x1f, 0xff, 0xf9, 0x06, 0x1c, 0xf5, 0x09, 0x06, 0x01, 0x09,
  0x06, 0x08, 0x85, 0x03, 0x0f, 0x1f, 0x3c, 0xf8, 0xf0, 0x06, 0x28, 0x62,
  0x08, 0x08, 0x06, 0x02, 0x62, 0xff, 0xff, 0xe1, 0xe0, 0x70, 0x78, 0x3c,
  0x1f, 0x0f, 0x03, 0x06, 0x27, 0x28, 0xe0, 0xf8, 0xff, 0x1f, 0x07, 0x06,
  0x04, 0x6b, 0x06, 0x05, 0x35, 0x09, 0x08, 0x07, 0x1f, 0xff, 0xfc, 0x06,
  0x1c, 0x7b, 0x09, 0x16, 0x01, 0x07, 0x0f, 0x1e, 0x7c, 0xf8, 0xe0, 0x06,
  0x25, 0x89, 0x09, 0x01, 0xff, 0xff, 0xff, 0x06, 0x2c, 0xa5, 0xf0, 0xfc,
  0x7f, 0x1f, 0x06, 0x07, 0x83, 0x09, 0x13, 0x03, 0x06, 0x02, 0x7b, 0x06,
  0x53, 0x29, 0x60, 0x09, 0x1c, 0x09, 0x02, 0x63, 0x67, 0x7f, 0x7e, 0x7c,
  0x70, 0x08, 0x83, 0x7f, 0x7f, 0x63, 0x06, 0x09, 0x11, 0x70, 0x7e, 0x7f,
  0x6f, 0x61, 0x06, 0x1c, 0x39, 0x63, 0x7f, 0x7f, 0x7c, 0x06, 0x11, 0x0f,
  0x06, 0x15, 0xa9, 0xf0, 0x90, 0x09, 0x01, 0x08, 0x15, 0xf0, 0x00, 0x00,
  0x10, 0x10, 0xf0, 0x10, 0x10, 0x00, 0xe0, 0x10, 0x09, 0x01, 0x20, 0x08,
  0x09, 0x06, 0x22, 0xda, 0x08, 0x0e, 0x08, 0x59, 0x06, 0x05, 0x0e, 0xe0,
  0x06, 0x09, 0x27, 0x80, 0x80, 0x40, 0x20, 0x10, 0x08, 0xaf, 0x09, 0x42,
  0xd0, 0x09, 0x01, 0x90, 0x08, 0x83, 0x09, 0x1d, 0x07, 0x06, 0x25, 0xfc,
  0x07, 0x08, 0x85, 0x09, 0x61, 0x03, 0x04, 0x09, 0x01, 0x02, 0x06, 0x05,
  0x11, 0x08, 0x09, 0x09, 0xe2, 0x06, 0x05, 0x0e, 0x03, 0x08, 0xc6, 0x02,
  0x04, 0x09, 0xe8, 0x09, 0x45, 0x06, 0x05, 0x16, 0x08, 0x03, 0x09, 0x17
};
#define BITMAP_LZG_LEN 384

#define MENU_SWITCH_MODE 0
#define MENU_KEX 1
#define MENU_TRAIN_CHORDS 2
#define MENU_FLASH_INFO 3
#define MENU_FLASH_DUMP 4
#define MENU_LIST_SEEDS 5
#define MENU_DEL_RAM 6
#define MENU_DEL_KEYS 7
#define MENU_UPDATE 8
#define MENU_ABOUT 9
#define MENU_LEN 10
static const char *menuitems[]={"Switch Mode", "Key Exchange", "Train Chords", "Flash info", "Flash dump", "List Seeds", "Erase RAM", "Erase Keystore", "FW Update", "About"};
typedef enum {None=0, Flashstats, Flashdump, Listseeds, Chord_train, KEXMenu} AppModes;
AppModes appmode=None;

MenuCtx menuctx={0,0};
MenuCtx appctx={0,0};

static UserRecord* init_pf_user(void) {
  uint8_t keys;
  oled_clear();
  oled_print_inv(0,0, (char*) " PITCHFORK!!5!  ", Font_8x8);
  oled_print(0,8, (char*) "This is an" , Font_8x8);
  oled_print(0,16, (char*) "uninitialized" , Font_8x8);
  oled_print(0,24, (char*) "pitchfork" , Font_8x8);
  oled_print(0,40, (char*) "initialize?" , Font_8x8);
  oled_print(0,48, (char*) "yes >" , Font_8x8);
  while((keys = key_handler())==0);
  if(!(keys & BUTTON_RIGHT)) {
    oled_clear();
    return (UserRecord*) 0;
  }
  uint8_t name[33];
  int nlen=0;
  getstr("enter your name", name,&nlen);
  if(nlen>0 && nlen<=32) {
    return init_user(name, nlen);
  }
  return (UserRecord*) 0;
}

void softreset() {
  disable_irqs();
  // stop clocks
  RCC->AHB1ENR = 0;
  RCC->AHB2ENR = 0;
  RCC->AHB3ENR = 0;
  RCC->APB1ENR = 0;
  RCC->APB2ENR = 0;

  // clear RAM
  memset((void*) 0x20000000,0, 0x20000);

  SCB->AIRCR  = (NVIC_AIRCR_VECTKEY | (SCB->AIRCR & (0x700)) | (1<<NVIC_SYSRESETREQ)); /* Keep priority group unchanged */
  asm("dsb"); /* Ensure completion of memory access */
  while(1);
}

static void erase_keystore(void) {
  clear_flash(FLASH_SECTOR_ID);
}

static void about(void) {
  oled_clear();
  oled_print(0,16, (char*) "Pitchfork v"VERSION , Font_8x8);
  oled_print(0,32, (char*) "2013-2016" , Font_8x8);
  oled_print(0,40, (char*) "the pitchfork" , Font_8x8);
  oled_print(0,48, (char*) "     team" , Font_8x8);
}

extern int firmware_updater;
extern int _binary_fwupdater_bin_size;
extern int _binary_fwupdater_bin_lzg_size;
extern unsigned char* _binary_fwupdater_bin_lzg_start;
extern unsigned char* _load_addr;

void fwupdate_trampoline(void) {
  LZG_Decode((uint8_t*) &_binary_fwupdater_bin_lzg_start,
             (int) &_binary_fwupdater_bin_lzg_size,
             (void*) &_load_addr,
             (int) &_binary_fwupdater_bin_size);
  //fw_updater(usbd_dev);
  asm volatile("mov r0, %[value]" ::[value] "r" (usbd_dev));
  asm volatile("blx %[addr]" ::[addr] "r" (&firmware_updater));
}

static void menu_cb(char menuidx) {
  gui_refresh=1;
  switch(menuidx) {
  case MENU_SWITCH_MODE: {
    switch(dual_usb_mode) {
    case CRYPTO: { storage_mode(); break; }
    case DISK: { crypto_mode(); break; }
    }
    break;
  }
  case MENU_KEX: {
    if(kex_menu_init()==1) {
      oled_clear();
      appmode=KEXMenu;
      appctx.idx=0;
      appctx.top=0;
    }
    break;
  }
  case MENU_FLASH_INFO: { oled_clear(); appmode=Flashstats; appctx.idx=0; appctx.top=0; break; }
  case MENU_FLASH_DUMP: { oled_clear(); appmode=Flashdump; appctx.idx=0; appctx.top=0; break; }
  case MENU_LIST_SEEDS: { oled_clear(); appmode=Listseeds; appctx.idx=0; appctx.top=0; break; }
  case MENU_TRAIN_CHORDS: { oled_clear(); chord_reset(); appmode=Chord_train; gui_refresh=0; break; }
  case MENU_DEL_RAM: { softreset(); break; }
  case MENU_DEL_KEYS: { erase_keystore(); softreset(); break; }
  case MENU_UPDATE: { fwupdate_trampoline(); gui_refresh=0; break; }
  case MENU_ABOUT: { about(); gui_refresh=0; break; }
  }
}

static void app(void) {
  switch(appmode) {
  case None: {menu(&menuctx, (const uint8_t **) menuitems,MENU_LEN,menu_cb); break;}
  case Flashstats: { if(flashstats()==0) appmode=None; break; }
  case Flashdump: { if(flashdump()==0) appmode=None; break; }
  case Listseeds: { if(listseeds()==0) appmode=None; break; }
  case Chord_train: { if(chord_train()==0) {
        appmode=None;
        gui_refresh=1;
        break;
      }
  }
  case KEXMenu: { if(kex_menu()==0) appmode=None; break; }
  }
}

int main(void) {
  init();
  verify_fwsig();
  mDelay(500);
  pool = init_pool();
  randombytes_pitchfork_init(pool);

  bufs[0].start =  bufs[0].buf + crypto_secretbox_ZEROBYTES;
  bufs[1].start =  bufs[1].buf + crypto_secretbox_ZEROBYTES;

  // logo + clearscreen "anim"
  LZG_Decode(bitmap_lzg, BITMAP_LZG_LEN, frame_buffer, 1024);
  oled_refresh();

  nrf24_init(); // todo add to main init
  init_chrgstat(); // todo add to main init
  init_sdcd(); // todo add to main init

  mDelay(100);
  oled_cmd(0xa7);//--set inverted display
  oled_refresh();
  mDelay(100);
  oled_cmd(0xa6);//--set normal display
  fancycls();

  // check if user is initialized, if not attempt so
  // user is needed for crypto ops, without an initialized user
  // pitchfork cannot execute crypto ops, only rng
  if(get_userrec()==NULL) init_pf_user();

  while(1){

#ifdef HAVE_MSC
    // todo implement ifndef have_msc part ;)
    if(dual_usb_mode == CRYPTO) pitchfork_main();
#endif // HAVE_MSC

    if(!(sysctr & 16383)) { // reseed about every 16s
      randombytes_pitchfork_stir();
    }
    if(gui_refresh==0) statusline();
    app();
  }

  return 0;
}
