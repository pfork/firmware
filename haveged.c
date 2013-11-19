#include "stm32f.h"
#include "haveged.h"

#define THRESHOLD 10
#define NMININT 64

unsigned int Entropy[SIZEENTROPY];
unsigned int NBINTERRUPT;
unsigned int last;

unsigned int hardtick( void ) {
  unsigned int ctr;
  ctr = DWT_CYCCNT;
  if(ctr - last > THRESHOLD) NBINTERRUPT++;
  last = ctr;
  return ctr;
}

void haveged_collect(void) {
  int A=0;
  int K=0;
  NBINTERRUPT=0;
  last = DWT_CYCCNT;
  while (NBINTERRUPT < NMININT) {
    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);

    if (A==0) A++; else A--;
    Entropy[K] = (Entropy[K] << 5) ^ (Entropy[K] >> 27) ^
      hardtick()^(Entropy[(K + 1) & (SIZEENTROPY - 1)] >> 31);
    K = (K + 1) & (SIZEENTROPY - 1);
    /**repeated XX times **/
  }
}

void haveged_init( void ) {
  SCB_DEMCR |= 0x01000000;
  DWT_CYCCNT = 0; // reset the counter
  DWT_CONTROL |= 1 ; // enable the counter
  // initialize haveged
  haveged_collect();
  haveged_collect();
}

