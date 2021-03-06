#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sodium/crypto_generichash.h>
#include <sodium/randombytes.h>
#include <sodium/utils.h>

#include "xeddsa.h"
#include "xeddsa_keygen.h"

unsigned char sk[crypto_xeddsa_SECRETKEYBYTES];

int main(int argc, char *argv[]) {
  int i;
  if(argc<3) {
    fprintf(stderr, "%s master.key image.bin\n",argv[0]);
    exit(1);
  }

  // read secret key
  // todo check argv for malicious paths
  FILE *f = fopen(argv[1], "r");
  if(f==NULL) {
    fprintf(stderr, "error reading file: %s\n", argv[1]);
    exit(1);
  }
  size_t size;
  if((size = fread(sk,sizeof(sk),1,f)) !=1) {
    fprintf(stderr, "error reading file: %s\n", argv[1]);
    exit(1);
  };
  fclose(f);

  // load image
  // todo check argv for malicious paths
  f = fopen(argv[2], "r");
  uint8_t buf[256*1024];
  memset(buf,0xff,sizeof(buf));
  if((size = fread(buf,1,sizeof(buf)-crypto_xeddsa_BYTES,f)) <= 0) {
    fprintf(stderr, "error reading file: %s\n", argv[2]);
    exit(1);
  };
  fclose(f);
  fprintf(stderr, "[i] read %ld bytes (%ldKB)\n", size, size/1024-256);

  fprintf(stderr, "[+] hashing %ld bytes... ", sizeof(buf)-crypto_xeddsa_BYTES);
  uint8_t digest[64];
  crypto_generichash(digest, 64, buf, sizeof(buf)-crypto_xeddsa_BYTES, NULL, 0);
  char hex[1024];
  sodium_bin2hex(hex,1024,digest,64);
  fprintf(stderr, "done\n[h] %s\n", hex);

  fprintf(stderr, "[+] signing... ");
  uint8_t sig[crypto_xeddsa_BYTES], random[64];
  randombytes_buf(random,sizeof(random));
  sc_clamp(sk);
  xed25519_sign(sig, sk, digest, sizeof digest, random);
  sodium_bin2hex(hex,1024,sig,64);
  fprintf(stderr, "done\n[s] %s\n", hex);

  for(i=0;i<sizeof(sig);i++) {
    printf("%c", sig[i]);
  }

  return 0;
}
