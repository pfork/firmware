#include <stdint.h>
#include <string.h>
#include "display.h"
#include "irq.h"
#include "libopencm3/stm32/flash.h"

#define READ_PROTECTION (0x1FFFC000)

#include <libopencm3/stm32/flash.h>

#define OPTKEY1 0x08192A3B
#define OPTKEY2 0x4C5D6E7F

#define FLASH_OPT_LOCK (1<<0)
#define FLASH_OPT_STRT (1<<1)
#define FLASH_BOR_LEV (1<<2)
#define FLASH_WDG_SW (1<<5)
#define FLASH_nRST_STP (1<<6)
#define FLASH_nRST_STBY (1<<7)
#define FLASH_RDP (1<<8)
#define FLASH_nWRP (1<<16)

void opt_unlock(void) {
   /* Clear the unlock sequence state. */
   FLASH_OPTCR |= FLASH_OPT_LOCK;

   FLASH_OPTKEYR = OPTKEY1;
   FLASH_OPTKEYR = OPTKEY2;
}

void opt_lock(void) {
   FLASH_OPTCR |= FLASH_OPT_LOCK;
}

void opt_write_rdp(unsigned char rdp) {
   opt_unlock();
   flash_wait_for_last_operation();
   FLASH_OPTCR |= (rdp <<8) | FLASH_OPT_STRT;
   flash_wait_for_last_operation();
   opt_lock();
}

void ramload(void) {
  disp_clear();
  disp_print(0,0,"locking flash");

  disable_irqs();
  opt_unlock();
  flash_clear_status_flags();
  opt_write_rdp(0x55); // farthest away (hamming) from level0
  opt_lock();
  disp_print(0,9,"ok");
  disp_print(0,18,"pls reboot");
  while(1);
}
