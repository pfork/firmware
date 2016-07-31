/**
  ************************************************************************************
  * @file    randombytes_pitchfork.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef randombytes_pitchfork_H
#define randombytes_pitchfork_H

#include <stddef.h>
#include <stdint.h>

#include "export.h"
#include "mixer.h"

SODIUM_EXPORT
void        randombytes_pitchfork_init(struct entropy_store* pool);

SODIUM_EXPORT
void        randombytes_pitchfork_stir(void);

SODIUM_EXPORT
void        randombytes_buf(void * const buf, const size_t size);

#endif
