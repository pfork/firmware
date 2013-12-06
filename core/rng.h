#ifndef rng_h
#define rng_h
#include <sys/types.h>

void rnd_init ( void );
unsigned int next_rand ( void );
unsigned int rand_read ( const size_t size, unsigned char* buf);
unsigned int get_entropy ( const size_t size, unsigned char* buf);
#endif
