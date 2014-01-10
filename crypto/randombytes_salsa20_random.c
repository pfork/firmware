#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <crypto_core_salsa20.h>
#include <crypto_auth_hmacsha512256.h>
#include <crypto_stream_salsa20.h>
#include <randombytes.h>
#include <randombytes_salsa20_random.h>
#include <crypto_generichash.h>
#include <utils.h>

#include "mixer.h"
#include "systimer.h"
#include "stm32f.h"

#define STIRPERIOD 128
#define SALSA20_RANDOM_BLOCK_SIZE crypto_core_salsa20_OUTPUTBYTES
#define SHA512_BLOCK_SIZE 128U
#define SHA512_MIN_PAD_SIZE (1U + 16U)
#define COMPILER_ASSERT(X) (void) sizeof(char[(X) ? 1 : -1])

typedef struct Salsa20Random_ {
  unsigned char key[crypto_stream_salsa20_KEYBYTES];
  unsigned char rnd32[SALSA20_RANDOM_BLOCK_SIZE];
  unsigned char s[crypto_generichash_KEYBYTES];
  unsigned int  fresh;
  uint64_t      nonce;
  size_t        rnd32_outleft;
  int           initialized;
  struct entropy_store* pool;
} Salsa20Random;

Salsa20Random stream = {
    .rnd32_outleft = (size_t) 0U,
    .initialized = 0,
    .fresh = 0
};

void randombytes_salsa20_random_init(struct entropy_store* pool) {
    unsigned int dev_uuid[4];
    stream.nonce = sysctr;
    stream.pool = pool;

    dev_uuid[0]=DESIG_UNIQUE_ID0;
    dev_uuid[1]=DESIG_UNIQUE_ID1;
    dev_uuid[2]=DESIG_UNIQUE_ID2;
    dev_uuid[3]=0;
    crypto_generichash(stream.s, crypto_generichash_KEYBYTES, (unsigned char *) dev_uuid, (uint64_t) 16, NULL, 0);
    stream.fresh = 0;
    stream.initialized = 1;
    //assert(stream.nonce != (uint64_t) 0U);
}

#define HASHSIZE (crypto_stream_salsa20_KEYBYTES >> 1)
void randombytes_salsa20_random_stir(void) {
  	int i;
   unsigned int w[HASHSIZE];

   // gather some entropy
   seed_pool();

   // zero out random pool
   memset(stream.rnd32, 0, sizeof stream.rnd32);
   stream.rnd32_outleft = (size_t) 0U;

	/* Generate a hash across the pool */
   crypto_generichash((unsigned char *) w,
                      HASHSIZE,
                      (unsigned char *) stream.pool,
                      (uint64_t) INPUT_POOL_WORDS,
                      (unsigned char *) stream.s,
                      (uint64_t) crypto_generichash_KEYBYTES);

	/*
	 * We mix the hash back into the pool to prevent backtracking
	 * attacks (where the attacker knows the state of the pool
	 * plus the current outputs, and attempts to find previous
	 * ouputs), unless the hash function can be inverted. By
	 * mixing at least a SHA1 worth of hash data back, we make
	 * brute-forcing the feedback as hard as brute-forcing the
	 * hash.
	 */
   mix_pool_bytes(stream.pool, w, HASHSIZE);

	/*
	 * To avoid duplicates, we extract a portion of the pool while
	 * mixing, and hash one final time.
	 */
   crypto_generichash((unsigned char *) w,
                      HASHSIZE,
                      (unsigned char *) stream.pool,
                      (uint64_t) INPUT_POOL_WORDS,
                      (unsigned char *) stream.s,
                      (uint64_t) crypto_generichash_KEYBYTES);

	/*
	 * In case the hash function has some recognizable output
	 * pattern, we fold it in half. Thus, we always feed back
	 * twice as much data as we output.
	 */
   for(i=0;i<HASHSIZE/2;i++)
     stream.key[i] ^= w[i] ^ w[HASHSIZE-(i+1)];

	sodium_memzero(w, sizeof(w));
}

static void
randombytes_salsa20_random_stir_if_needed(void)
{
    if (stream.fresh == 0) {
      randombytes_salsa20_random_stir();
      stream.fresh = STIRPERIOD;
    } else {
      stream.fresh--;
    }
}

static uint32_t
randombytes_salsa20_random_getword(void)
{
    uint32_t val;

    COMPILER_ASSERT(sizeof stream.rnd32 >= sizeof val);
    COMPILER_ASSERT(sizeof stream.rnd32 % sizeof val == (size_t) 0U);
    if (stream.rnd32_outleft <= (size_t) 0U) {
        randombytes_salsa20_random_stir_if_needed();
        COMPILER_ASSERT(sizeof stream.nonce == crypto_stream_salsa20_NONCEBYTES);
        crypto_stream_salsa20((unsigned char *) stream.rnd32,
                              (unsigned long long) sizeof stream.rnd32,
                              (unsigned char *) &stream.nonce,
                              stream.key);
        stream.nonce++;
        stream.rnd32_outleft = sizeof stream.rnd32;
    }
    stream.rnd32_outleft -= sizeof val;
    memcpy(&val, &stream.rnd32[stream.rnd32_outleft], sizeof val);

    return val;
}

int
randombytes_salsa20_random_close(void)
{
    int ret = -1;

    stream.initialized = 0;
    ret = 0;

    return ret;
}

uint32_t
randombytes_salsa20_random(void)
{
    return randombytes_salsa20_random_getword();
}

void
randombytes_salsa20_random_buf(void * const buf, const size_t size)
{
  //int ret;

    randombytes_salsa20_random_stir_if_needed();
    COMPILER_ASSERT(sizeof stream.nonce == crypto_stream_salsa20_NONCEBYTES);
#ifdef ULONG_LONG_MAX
    //assert(size <= ULONG_LONG_MAX);
#endif
    /* ret = */crypto_stream_salsa20((unsigned char *) buf, (unsigned long long) size,
                                (unsigned char *) &stream.nonce,
                                stream.key);
    //assert(ret == 0);
    stream.nonce++;
}

/*
 * randombytes_salsa20_random_uniform() derives from OpenBSD's arc4random_uniform()
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 */

uint32_t
randombytes_salsa20_random_uniform(const uint32_t upper_bound)
{
    uint32_t min;
    uint32_t r;

    if (upper_bound < 2) {
        return 0;
    }
    min = (uint32_t) (-upper_bound % upper_bound);
    for (;;) {
        r = randombytes_salsa20_random();
        if (r >= min) {
            break;
        }
    }
    return r % upper_bound;
}

const char *
randombytes_salsa20_implementation_name(void)
{
    return "salsa20";
}

struct randombytes_implementation randombytes_salsa20_implementation = {
    _SODIUM_C99(.implementation_name =) randombytes_salsa20_implementation_name,
    _SODIUM_C99(.random =) randombytes_salsa20_random,
    _SODIUM_C99(.stir =) randombytes_salsa20_random_stir,
    _SODIUM_C99(.uniform =) randombytes_salsa20_random_uniform,
    _SODIUM_C99(.buf =) randombytes_salsa20_random_buf,
    _SODIUM_C99(.close =) randombytes_salsa20_random_close
};
