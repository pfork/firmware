#include <stdint.h>
#include <unistd.h>
#include <crypto_generichash.h>
#include "oled.h"
#include "xeddsa.h"
#include "randombytes_pitchfork.h"
#include "stm32f.h"

#define pitchfork_fwsig_BYTES 64

extern const uint8_t *_firmware_sig;

// fw based last resort key. in case the immutable keys are dead
const uint8_t pk[]={0x79, 0xf9, 0xbe, 0x84, 0xa4, 0xe0, 0x1c, 0x32,
                    0x47, 0xbf, 0xf3, 0xbb, 0xc6, 0xb1, 0x5a, 0x3e,
                    0x44, 0xae, 0xac, 0xfd, 0x9c, 0x79, 0x75, 0x4f,
                    0x3d, 0x6b, 0x34, 0x01, 0x0b, 0x5c, 0xeb, 0x75};

int verify_fwsig(void) {
  uint8_t digest[pitchfork_fwsig_BYTES];

  oled_clear();
  oled_print(0,0,"verifying..", Font_8x8);
  oled_print(0,9,"...firmware", Font_8x8);
  oled_print(0,18,"hashing...", Font_8x8);
  crypto_generichash(digest, 64, (uint8_t *)0x08000000, 0x40000-64, NULL, 0);

  oled_print(0,18,"hashing: done", Font_8x8);

  oled_print(0,27,"signature...", Font_8x8);
  // check master pubkey
  if (xed25519_verify((void*) &_firmware_sig, (uint8_t*) (OTP_START_ADDR), digest, sizeof digest) != 0) {
    // check user pubkey
    if (xed25519_verify((void*) &_firmware_sig, (uint8_t*) (OTP_START_ADDR + 1 * OTP_BYTES_IN_BLOCK), digest, sizeof digest) != 0) {
      // check soft pubkey
      if (xed25519_verify((void*) &_firmware_sig, pk, digest, sizeof digest) != 0) {
        oled_print_inv(0,36,"invalid", Font_8x8);
        while(1);
      } else {
        oled_print(0,36,"valid", Font_8x8);
        oled_print(0,45,"(soft)", Font_8x8);
      }
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
