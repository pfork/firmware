#include "adc.h"
#include "rng.h"
#include "mixer.h"

static inline unsigned int rol32(unsigned int A, unsigned char n) {
  return ((A) << (n)) | ((A)>>(32-(n)));
}

static unsigned int const twist_table[8] = {
	0x00000000, 0x3b6e20c8, 0x76dc4190, 0x4db26158,
	0xedb88320, 0xd6d6a3e8, 0x9b64c2b0, 0xa00ae278 };

void mix_pool_bytes(struct entropy_store *r, const void *in,
                    int nbytes) {
	unsigned long i,tap1, tap2, tap3, tap4, tap5;
	int input_rotate;
	int wordmask = r->poolinfo->poolwords - 1;
	const char *bytes = in;
	unsigned int w;

	tap1 = r->poolinfo->tap1;
	tap2 = r->poolinfo->tap2;
	tap3 = r->poolinfo->tap3;
	tap4 = r->poolinfo->tap4;
	tap5 = r->poolinfo->tap5;

	input_rotate = ACCESS_ONCE(r->input_rotate);
	i = ACCESS_ONCE(r->add_ptr);

	/* mix one byte at a time to simplify size handling and churn faster */
	while (nbytes--) {
		w = rol32(*bytes++, input_rotate);
		i = (i - 1) & wordmask;

		/* XOR in the various taps */
		w ^= r->pool[i];
		w ^= r->pool[(i + tap1) & wordmask];
		w ^= r->pool[(i + tap2) & wordmask];
		w ^= r->pool[(i + tap3) & wordmask];
		w ^= r->pool[(i + tap4) & wordmask];
		w ^= r->pool[(i + tap5) & wordmask];

		/* Mix the result back in with a twist */
		r->pool[i] = (w >> 3) ^ twist_table[w & 7];

		/*
		 * Normally, we add 7 bits of rotation to the pool.
		 * At the beginning of the pool, add an extra 7 bits
		 * rotation, so that successive passes spread the
		 * input bits across the pool evenly.
		 */
		input_rotate = (input_rotate + (i ? 7 : 14)) & 31;
	}

	ACCESS_ONCE(r->input_rotate) = input_rotate;
	ACCESS_ONCE(r->add_ptr) = i;
}

unsigned int pool_data[INPUT_POOL_WORDS];

/* x^128 + x^104 + x^76 + x^51 +x^25 + x + 1 */
const struct poolinfo input_poolinfo = { 128,	104,	76,	51,	25,	1 };
struct entropy_store input_pool;

struct entropy_store* init_pool(void) {
  /* int i; */
  /* for(i=0;i<INPUT_POOL_WORDS;i++) */
  /*   pool_data[i]=0; */
  input_pool.poolinfo = &input_poolinfo;
  input_pool.pool = pool_data;
  input_pool.add_ptr = 0;
  input_pool.input_rotate = 0;

  return &input_pool;
}

#define TEMP_COLLECT_ITER (8/6)*INPUT_POOL_WORDS*32 << 1 // measured entropy / byte = 5.7
#define VREF_COLLECT_ITER (8/3)*INPUT_POOL_WORDS*32 << 1 // measured entropy / byte = 2.8
#define RNG_COLLECT_ITER INPUT_POOL_WORDS << 1 // advertised 32 bit rng

//#define TEMP_COLLECT_ITER 1
//#define VREF_COLLECT_ITER 1
//#define RNG_COLLECT_ITER 1

void seed_pool(void) {
  unsigned int i;
  unsigned short val16;
  unsigned int val32;
  for (i = 0; i < TEMP_COLLECT_ITER; ++i) {
    val16=read_temp();
    mix_pool_bytes(&input_pool, (const void *) &val16, 2);
  }
  for (i = 0; i < VREF_COLLECT_ITER; ++i) {
    val16=read_vref();
    mix_pool_bytes(&input_pool, (const void *) &val16, 2);
  }
  for (i = 0; i < RNG_COLLECT_ITER; ++i) {
    val32=next_rand();
    mix_pool_bytes(&input_pool, (const void *) &val32, 4);
  }
}

unsigned int get_entropy( const size_t size, unsigned char* buf) {
  //unsigned int ra;
  unsigned int cnt = 0;
  /* if (size - cnt >= 4) { */
  /*   PUT32((unsigned int) buf+cnt, ra); */
  /*   cnt+=4; */
  /*   continue; */
  /* } */
  /* unsigned int s=0; */
  /* while(cnt<size) { */
  /*   *(buf+cnt++)=*(((unsigned char*) &ra)+s++); */
  /* } */
  return cnt;
}
//-------------------------------------------------------------------
