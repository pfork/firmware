/**
  ************************************************************************************
  * @file    mixer.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef mixer_h
#define mixer_h

#define INPUT_POOL_SHIFT	12
#define INPUT_POOL_WORDS	(1 << (INPUT_POOL_SHIFT-5))
#define ENTROPY_SHIFT 3

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

struct entropy_store {
	/* read-only data: */
	const struct poolinfo *poolinfo;
	unsigned int *pool;

	/* read-write data: */
	unsigned short add_ptr;
	unsigned short input_rotate;
};

struct poolinfo {
	int poolwords;
	int tap1, tap2, tap3, tap4, tap5;
};

void mix_pool_bytes(struct entropy_store *r, const void *in, int nbytes);
void seed_pool(void);
struct entropy_store* init_pool(void);

#endif
