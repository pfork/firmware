#include <stdint.h>
#include <string.h>
#include "crypto_generichash.h"

#define CEIL(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

void pbkdf2_generichash(uint8_t* mk, const uint8_t *password, const size_t pwlen, const uint8_t *salt) {
  // according to SP 800-132 Recommendation for Password-Based Key Derivation December 2010
  // since most values are fixed, some simplifications were possible.
  const size_t c=5000; // Iteration count
  //const int hlen=32*8; // Digest size of the hash function
  //const int klen=32*8; // Length of MK in bits; at most ((2^32)-1)*hLen
  //if(klen>(2^32-1)*hlen) {
  //  return 0;
  //}
  //const int len=1; // CEIL((float) klen / (float) hlen); == ceil(256/256) == 1
  //const int r=512; // r = klen - (len - 1) * hlen; == 512 - (1 - 1)*512 == 512
  int j, k; //, i;
  //for(i=1;i<=len;i++) { // since len==1 we only do this once
    uint8_t uj[32+4]; // ,ti[32]; // ti == mk at the end anyway.
    memset(mk,0,32);  // ti(==mk)=0
    memcpy(uj,salt,32);  // uj = salt || int(i)
    *((int*) (uj+32))=1; // concat salt with int(i) in uj
    int *mk4 = (int*) mk, *uj4 = (int*) uj;
    crypto_generichash(uj, 32,  // uj = hash(password,uj)
                       password, pwlen,
                       uj, 36);
    for(k=0;k<8;k++)  // xor uj into ti
      mk4[k] ^= uj4[k];
    for(j=2;j<=c;j++) {
      crypto_generichash(uj, 32,
                         password, pwlen,
                         uj, 32);
      for(k=0;k<8;k++)  // xor uj into ti
        mk4[k] ^= uj4[k];
    }
  //}
}
