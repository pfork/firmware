#include "crypto_secretbox.h"

unsigned char* get_master_key(void) {
  // todo implement button input, key stretching, proper KDF
  static unsigned char mk[crypto_secretbox_KEYBYTES];
  unsigned int i;
  for(i=0;i<(crypto_secretbox_KEYBYTES>>2);i++)
    ((unsigned int* )mk)[i] = 0;
  return mk;
}

