#include <stdint.h>
#include <string.h>
#include "oled.h"
#include "irq.h"
#include "libopencm3/stm32/flash.h"

#define OTP_LOCK_ADDR	(0x1FFF7A00)
#define OTP_START_ADDR	(0x1FFF7800)
#define OTP_BYTES_IN_BLOCK	32
#define OTP_BLOCKS	16

// todo these should come from the otp bytes
const unsigned char pk[]= {0xe2,0x97,0x55,0x21,0x53,0x9f,0xb2,0xa3, 0xfd,0x49,0xef,0xba,0xc4,0xd9,0x2e,0x14,
                           0x4d,0x2f,0x18,0x47,0x8d,0x93,0x21,0x44, 0x9c,0xca,0x4d,0x75,0x27,0x18,0x12,0xd6 };

void store_key(void) {
  oled_clear();
  oled_print(0,0,"burning key", Font_8x8);
  int block=0; // only write in the 1st block
  int i;

  disable_irqs();

  flash_unlock();
  flash_clear_status_flags();
  // write pk into OTP block0
  for(i=0;i<sizeof(pk);i++) {
    flash_program_byte((OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + i), pk[i]);
  }
  // lock the 1st block
  flash_program_byte(OTP_LOCK_ADDR + block, 0x00);
  flash_lock();
  oled_print(0,9,"ok", Font_8x8);
  oled_print(0,18,"pls reboot", Font_8x8);
  while(1);
}
