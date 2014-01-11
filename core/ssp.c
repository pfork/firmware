#include "randombytes_salsa20_random.h"

void * __stack_chk_guard = NULL;

void __stack_chk_guard_setup() {
    unsigned char * p;
    p = (unsigned char *) &__stack_chk_guard;

    randombytes_salsa20_random_buf((void *) p, 4);
}

void __attribute__((noreturn)) __stack_chk_fail() {
  for(;;);
}
