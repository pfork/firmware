#ifndef pbkdf2_generichash_h
#define pbkdf2_generichash_h

#include <stdint.h>

void pbkdf2_generichash(uint8_t* mk, const uint8_t *password, const size_t pwlen, const uint8_t *salt);

#endif // pbkdf2_generichash_h
