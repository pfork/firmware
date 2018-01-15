#include <stdint.h>
#include <unistd.h>
#include <crypto_generichash.h>
#include "display.h"
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

  disp_clear();
  disp_print(0,0,"verifying..");
  disp_print(0,9,"...firmware");
  disp_print(0,18,"hashing...");
  crypto_generichash(digest, 64, (uint8_t *)0x08000000, 0x40000-64, NULL, 0);

  disp_print(0,18,"hashing: done");

  disp_print(0,27,"signature...");
  // check master pubkey
  if (xed25519_verify((void*) &_firmware_sig, (uint8_t*) (OTP_START_ADDR), digest, sizeof digest) != 0) {
    // check user pubkey
    if (xed25519_verify((void*) &_firmware_sig, (uint8_t*) (OTP_START_ADDR + 1 * OTP_BYTES_IN_BLOCK), digest, sizeof digest) != 0) {
      // check soft pubkey
      if (xed25519_verify((void*) &_firmware_sig, pk, digest, sizeof digest) != 0) {
        disp_print_inv(0,36,"invalid");
        while(1);
      } else {
        disp_print(0,36,"valid (soft)");
      }
    } else {
      disp_print(0,36,"valid (private)");
    }
  } else {
    disp_print(0,36,"valid (original)");
  }
  return 1;
}
