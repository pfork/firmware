#ifndef XENTROPY_H
#define XENTROPY_H
#include "stddef.h"
#include "stdint.h"

void xesrc_init(void);
void get_entropy(uint8_t *buf, size_t buflen);
#endif // XENTROPY_H
