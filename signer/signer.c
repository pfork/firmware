#include <stdint.h>
#include "blake512.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <crypto_sign.h>
#include <string.h>

unsigned char sk[crypto_sign_SECRETKEYBYTES];

int main(int argc, char *argv[]) {
  int i;
  if(argc<3) {
    fprintf(stderr, "%s master.key image.bin\n",argv[0]);
    exit(1);
  }

  // read secret key
  // todo check argv for malicious paths
  FILE *f = fopen(argv[1], "r");
  size_t size;
  if((size = fread(sk,sizeof(sk),1,f)) !=1) {
    fprintf(stderr, "error reading file: %s\n", argv[1]);
    exit(1);
  };
  fclose(f);

  // todo check argv for malicious paths
  f = fopen(argv[2], "r");
  uint8_t buf[256*1024];
  memset(buf,0xff,sizeof(buf));
  if((size = fread(buf,1,sizeof(buf)-crypto_sign_BYTES,f)) <= 0) {
    fprintf(stderr, "error reading file: %s\n", argv[2]);
    exit(1);
  };
  fclose(f);
  fprintf(stderr, "[i] read %d bytes\n", size);

  fprintf(stderr, "[+] hashing %d bytes... ", sizeof(buf)-crypto_sign_BYTES);
  uint8_t digest[BLAKE512_BYTES];
  crypto_hash_blake512(digest, buf, sizeof(buf)-crypto_sign_BYTES);

  fprintf(stderr, "done\n");

  fprintf(stderr, "[+] signing... ");
  uint8_t sig[crypto_sign_BYTES];
  crypto_sign_detached(sig, NULL, digest, sizeof(digest), sk);
  fprintf(stderr, "done\n");

  for(i=0;i<sizeof(sig);i++) {
    printf("%c", sig[i]);
  }

  return 0;
}