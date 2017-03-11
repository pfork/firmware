#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils/lzg/lzg.h"

#define PGPWORDS_SIZE 5456
const extern int  _binary_utils_pgpwords_data_lzg_size;
const extern uint8_t *_binary_utils_pgpwords_data_lzg_start;

#define PGPWORDS_LZG_LEN ((int) &_binary_utils_pgpwords_data_lzg_size)
#define pgpwords_lzg ((uint8_t*) &_binary_utils_pgpwords_data_lzg_start)

static const uint8_t* pgpword(uint16_t *wordlist, const uint8_t n, const size_t idx) {
  return((const uint8_t*) wordlist+wordlist[(idx%2)*256 + n]);
}

// wordlist is menuitems for menu()
// words is a buffer for retaining the zero-terminated words (not deduplicated), max wordsize is 11+1
// buf contains the bytes to convert and buflen its size
int to_pgpwords(uint8_t **wordlist, uint8_t *words, const unsigned char* buf, const size_t buflen) {
  uint8_t pgpwords[PGPWORDS_SIZE];
  int i;
  uint8_t *ptr = words;
  LZG_Decode(pgpwords_lzg, PGPWORDS_LZG_LEN, pgpwords, PGPWORDS_SIZE);
  for(i=0;i<buflen;i++) {
    const uint8_t *word=pgpword((uint16_t *) pgpwords, buf[i], i);
    // todo deduplicate already used words
    const int wlen=strlen((char*) word)+1;
    memcpy(ptr, word, wlen);
    wordlist[i]=ptr;
    ptr+=wlen;
  }
  return buflen;
}
