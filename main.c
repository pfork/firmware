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
#include "buttons.h"
#include "randombytes_pitchfork.h"
#include "mixer.h"
#include "pitchfork.h"
#include "systimer.h"
#include "display.h"
#include <string.h>

#include "delay.h"
#include "nrf.h"
#include "dma.h"
#include "utils/lzg/lzg.h"
#include "irq.h"
#include "widgets.h"
#include "kex.h"
#include "user.h"
#include "main.h"
#include "fwsig.h"
#include "iap.h"
#include "master.h"
#include "stfs.h"
#include "pf_store.h"
#include "browser.h"

void randombytes_pitchfork_init(struct entropy_store* pool);
struct entropy_store* pool;

static void fancycls(void) {
  // vertical line
  uint8_t i,j,w;
  for(j=0;j<DISPLAY_WIDTH-8;j++) {
    for(i=0;i<DISPLAY_HEIGHT;i++) {
      for(w=0;w<8;w++) {
        //disp_delpixel((j-1+w)&0x7f,16+i);
        //disp_setpixel((j+w)&0x7f,16+i);
        disp_delpixel((j-1+w)&0x7f,i);
        disp_setpixel((j+w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    disp_refresh();
  }
  for(j=DISPLAY_WIDTH;j>8;j--) {
    for(i=0;i<DISPLAY_HEIGHT;i++) {
      for(w=0;w<8;w++) {
        //disp_delpixel((j-1+w)&0x7f,16+i);
        //disp_setpixel((j+w)&0x7f,16+i);
        disp_setpixel((j-1-w)&0x7f,i);
        disp_delpixel((j-w)&0x7f,i);
      }
    }
    //j=(j+1) & 0x7f;
    disp_refresh();
  }
  disp_clear();
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

#define MENU_UNLOCK 0
#define MENU_SWITCH_MODE 1
#define MENU_KEX 2
#define MENU_KEYS 3
#define MENU_DEL_RAM 4
#define MENU_DEL_KEYS 5
#define MENU_UPDATE 6
#define MENU_ABOUT 7
#define MENU_LEN 8
static const char *menuitems[]={"(un)lock","Switch Mode", "Key Exchange", "Keys", "Erase RAM", "Erase Keystore", "FW Update", "About"};
typedef enum {None=0, KeysMenu, KEXMenu} AppModes;
AppModes appmode=None;

MenuCtx menuctx={0,0};
MenuCtx appctx={0,0};

static void init_pf_user(void) {
  uint8_t buttons;
  disp_clear();
  disp_print_inv(0,0, " PITCHFORK!!5!  ");
  disp_print(0,(FONT_HEIGHT+1), "This is an" );
  disp_print(0,(FONT_HEIGHT+1)*2, "uninitialized");
  disp_print(0,(FONT_HEIGHT+1)*3, "pitchfork");
  disp_print(0,(FONT_HEIGHT+1)*4, "initialize?");
  disp_print(0,(FONT_HEIGHT+1)*5, "yes >");

  while((buttons = button_handler())==0);
#ifdef DEVICE_GH
  if(!(buttons & BUTTON_RIGHT)) {
#else // assume DEVICE_3310
  if(!(buttons == BUTTON_RIGHT)) {
#endif // DEVICE_GH
    disp_clear();
    return;
  }
  if(0!=pf_store_init()) {
    disp_clear();
    disp_print(0,0,"fail store init");
    disp_print(0,DISPLAY_HEIGHT/2,":/");
    mDelay(1000);
    return;
  }
  uint8_t name[33];
  int nlen=0;
  getstr("enter your name", name,&nlen);
  if(nlen>0 && nlen<=32) {
    uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
    UserRecord *userrec=(UserRecord*) userbuf;
    // todo set unique device random in OTP[2] if not locked yet
    // todo set panic key
    new_user(userrec, name, nlen);
  }
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

static void about(void) {
  disp_clear();
  disp_print(0,16, (char*) "PITCHFORK v"VERSION);
  disp_print(0,24, (char*) "2013-2018");
  disp_print(0,32, (char*) "the pitchfork");
  disp_print(0,40, (char*) "     team");
}

void fwupdate_trampoline(void) {
  LZG_Decode((uint8_t*) &_binary_fwupdater_bin_lzg_start,
             (int) &_binary_fwupdater_bin_lzg_size,
             (void*) &_load_addr,
             (int) &_binary_fwupdater_bin_size);
  //fw_updater(usbd_dev);
  asm volatile("mov r0, %[value]" ::[value] "r" (usbd_dev));
  asm volatile("blx %[addr]" ::[addr] "r" (&firmware_updater));
}

void toggle_lock(void) {
  if(pitchfork_hot!=0) {
    erase_master_key();
  } else {
    get_master_key("unlock from gui");
  }
  gui_refresh=1;
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
      disp_clear();
      appmode=KEXMenu;
      appctx.idx=0;
      appctx.top=0;
    }
    break;
  }
  case MENU_KEYS: {
    if(browser_init()==0) {
      disp_clear();
      appmode=KeysMenu;
      appctx.idx=0;
      appctx.top=0;
    }
    break;
  }
  case MENU_DEL_RAM: { softreset(); break; }
  case MENU_DEL_KEYS: {
#ifdef DEVICE_GH
    disp_print_inv(40,DISPLAY_WIDTH - 11*FONT_WIDTH, "     format");
#else // assume DEVICE_3310 
    // add one more pixel to account for the proportional 'm'
    disp_print_inv(40,DISPLAY_WIDTH - 11*FONT_WIDTH+1, "     format");
#endif // DEVICE_GH
    stfs_format();
    softreset();
    break;
  }
  case MENU_UPDATE: { fwupdate_trampoline(); gui_refresh=0; break; }
  case MENU_UNLOCK: { toggle_lock(); break; }
  case MENU_ABOUT: { about(); gui_refresh=0; break; }
  }
}

static void app(void) {
  switch(appmode) {
  case None: {menu(&menuctx, (const uint8_t **) menuitems,MENU_LEN,menu_cb); break;}
  case KeysMenu: { if(browser()==0) appmode=None; break; }
  case KEXMenu: { if(kex_menu()==0) appmode=None; break; }
  }
}

int main(void) {
  init();
  verify_fwsig();
  mDelay(500);
  pool = init_pool();
  randombytes_pitchfork_init(pool);
  stfs_init();

  bufs[0].start =  bufs[0].buf + crypto_secretbox_ZEROBYTES;
  bufs[1].start =  bufs[1].buf + crypto_secretbox_ZEROBYTES;

  disp_show_logo();

  nrf24_init(); // todo add to main init
  init_chrgstat(); // todo add to main init
  init_sdcd(); // todo add to main init

  mDelay(100);
  disp_invert();
  disp_refresh();
  mDelay(100);
  disp_normal();
  fancycls();

  // check if user is initialized, if not attempt so
  // user is needed for crypto ops, without an initialized user
  // pitchfork cannot execute crypto ops, only rng
  {
    uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
    UserRecord *userrec=(UserRecord*) userbuf;
    if(get_user(userrec)==-1) init_pf_user();
  }

  while(1){

#ifdef HAVE_MSC
    // todo implement ifndef have_msc part ;)
    if(dual_usb_mode == CRYPTO) pitchfork_main();
#endif // HAVE_MSC

    if(!(sysctr & 16383)) { // reseed about every 16s
      randombytes_pitchfork_stir();
    }
    expire_master_key();
    //if(gui_refresh==0) statusline();
    statusline();
    app();
  }

  return 0;
}
