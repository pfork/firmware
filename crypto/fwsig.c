#include <stdint.h>
#include "blake512.h"
#include <unistd.h>
#include "oled.h"
#include <crypto_sign.h>
#include <string.h>

#define OTP_START_ADDR	(0x1FFF7800)
#define OTP_BYTES_IN_BLOCK	32
#define OTP_BLOCKS	16

extern const uint8_t *_firmware_sig;

//const unsigned char pk[2][crypto_sign_PUBLICKEYBYTES]=
//  {{0xe2,0x97,0x55,0x21,0x53,0x9f,0xb2,0xa3, 0xfd,0x49,0xef,0xba,0xc4,0xd9,0x2e,0x14,
//    0x4d,0x2f,0x18,0x47,0x8d,0x93,0x21,0x44, 0x9c,0xca,0x4d,0x75,0x27,0x18,0x12,0xd6 },
//   {0xaa,0x17,0x7a,0xc5,0x27,0x08,0x24,0x29, 0x6d,0x6b,0x84,0x50,0x15,0xa2,0x81,0x56,
//    0x73,0xac,0x14,0xe9,0xda,0x2a,0x85,0x06, 0xcb,0x9f,0x44,0x63,0x7a,0x02,0xa3,0x90 }
//  };

static char read_otp(uint32_t block, uint32_t byte) {
  return *(char*)(OTP_START_ADDR + block * OTP_BYTES_IN_BLOCK + byte);
}

int verify_fwsig(void) {
  uint8_t digest[BLAKE512_BYTES];

  oled_clear();
  oled_print(0,0,"verifying..", Font_8x8);
  oled_print(0,9,"...firmware", Font_8x8);

  int i;
  for(i=0;i<crypto_sign_PUBLICKEYBYTES && read_otp(0,i)==0xff;i++);
  if(i>=crypto_sign_PUBLICKEYBYTES) {
    oled_print(0,18,"no key found", Font_8x8);
    return 0;
  }

  oled_print(0,18,"hashing...", Font_8x8);
  crypto_hash_blake512(digest, (uint8_t *)0x08000000, 0x40000-64);

  oled_print(0,18,"hashing: done", Font_8x8);

  oled_print(0,27,"signature...", Font_8x8);
  if (crypto_sign_verify_detached((void*) &_firmware_sig, digest, 64, (uint8_t*) (OTP_START_ADDR)) != 0) {
    if (crypto_sign_verify_detached((void*) &_firmware_sig, digest, 64, (uint8_t*) (OTP_START_ADDR + 1 * OTP_BYTES_IN_BLOCK)) != 0) {
      oled_print_inv(0,36,"invalid", Font_8x8);
      while(1);
    } else {
      oled_print(0,36,"valid", Font_8x8);
      oled_print(0,45,"(private)", Font_8x8);
    }
  } else {
    oled_print(0,36,"valid", Font_8x8);
    oled_print(0,45,"(original)", Font_8x8);
  }
  return 1;
}
