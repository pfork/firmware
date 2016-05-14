#include "chacha20/e/ref/e/ecrypt-sync.h"

#ifndef CRYPTO_STREAM_CHACHA20
#define CRYPTO_STREAM_CHACHA20

#define crypto_stream_chacha20_KEYBYTES 32
#define crypto_stream_chacha20_NONCEBYTES 8

int crypto_stream_chacha20(unsigned char *out, unsigned long long outlen, const unsigned char *n, const unsigned char *k);
int crypto_stream_chacha20_setup(ECRYPT_ctx *x, const unsigned char *n, const unsigned char *k);
int crypto_stream_chacha20_64b(ECRYPT_ctx *x, unsigned char *out);

#endif
