#include <stdlib.h>

void * memmove(void *dest, const void *src, size_t n) {
  signed char operation;
  size_t end;
  size_t current;

  if(dest != src) {
    if(dest < src) {
      operation = 1;
      current = 0;
      end = n;
    } else {
      operation = -1;
      current = n - 1;
      end = -1;
    }

    for( ; current != end; current += operation) {
      *(((unsigned char*)dest) + current) = *(((unsigned char*)src) + current);
    }
  }
  return dest;
}

