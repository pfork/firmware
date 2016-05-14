#include <string.h>
#include <stdint.h>

char itos(char *d, uint32_t x) {
  uint8_t t[11], *p=t;
  p += 11;
  *--p = 0;
  do {
    *--p = '0' + x % 10;
    x /= 10;
  } while (x);
  memcpy(d,p,11-(p-t));
  return 11-(p-t);
}
