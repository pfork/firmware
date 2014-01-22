/**
  ************************************************************************************
  * @file    memcmp.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides a naive memcmp implementation.
  ************************************************************************************
  */

#include <stddef.h>

int memcmp(const void* s1, const void* s2, size_t len) {
  unsigned int i;
  for(i=0;i<len; i++) {
    if(((unsigned char*) s1)[i] != ((unsigned char*) s2)[i]) {
      break;
    }
  }
  if(i>=len) {
    return 0;
  }
  return (((unsigned char*) s1)[i] > ((unsigned char*) s2)[i]) ? 1 : -1;
}

