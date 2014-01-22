/**
  ************************************************************************************
  * @file    ecdho.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides the 3 functions for performing an
  *          ecdh exchange using the internal storage
  ************************************************************************************
  */

#include <string.h> //memcpy
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include <utils.h>
#include "storage.h"

/**
  * @brief  start_ecdh: calculate initial ecdh params
  * @param  peer: pointer to peers name
  * @param  peer_len: length of peers name
  * @param  pub: pointer to buffer for output public exponent
  * @param  keyid: pointer to buffer for output keyid
  * @retval 0 on success
  */
int start_ecdh(unsigned char* peer,
               unsigned char peer_len,
               unsigned char* pub,      // output
               unsigned char* keyid)    // output
{
  unsigned char e[crypto_scalarmult_curve25519_BYTES];
  unsigned int ptr;

  randombytes_salsa20_random_buf((void *) e, (size_t) crypto_scalarmult_curve25519_BYTES);

  ptr = store_seed(e, peer, peer_len);
  if(ptr == -1 || ptr == -2)
    return -1;
  // copy ? todo why not return the pointer itself?
  memcpy(keyid, &(((SeedRecord*) ptr)->keyid), STORAGE_ID_LEN);
  // calculate pub
  crypto_scalarmult_curve25519_base(pub, e);
  return 0;
}

/**
  * @brief  respond_ecdh: calculate response ecdh params
  * @param  peer: pointer to peers name
  * @param  peer_len: length of peers name
  * @param  pub: pointer to buffer for output public exponent
  * @param  keyid: pointer to buffer for output keyid
  * @retval 0 on success
  */
int respond_ecdh(unsigned char* peer,
                 unsigned char peer_len,
                 unsigned char* pub,      // input/output
                 unsigned char* keyid)    // output
{
  unsigned char e[crypto_scalarmult_curve25519_BYTES];
  unsigned char s[crypto_scalarmult_curve25519_BYTES];
  unsigned int ptr;

  // calculate secret exp
  randombytes_salsa20_random_buf((void *) e, (size_t) crypto_scalarmult_curve25519_BYTES);

  // calculate shared secret
  crypto_scalarmult_curve25519(s, e, pub);

  ptr = store_seed(s, peer, peer_len);
  // copy keyid to output
  if(ptr == -1 || ptr == -2)
    return -1;
  // copy ? todo why not return the pointer itself?
  memcpy(keyid, &(((SeedRecord*) ptr)->keyid), STORAGE_ID_LEN);

  // calculate public
  crypto_scalarmult_curve25519_base(pub, e);
  return 0;
}

/**
  * @brief  finish_ecdh: calculate final ecdh params
  * @param  peer: pointer to peers name
  * @param  peer_len: length of peers name
  * @param  keyid: pointer to buffer for keyid from start_ecdh
  * @param  pub: pointer to buffer for public exponent from peer
  * @param  seedid: pointer to shared secret seedid
  * @retval 0 on success
  */
int finish_ecdh(unsigned char* peer,
                unsigned char peer_len,
                unsigned char* keyid,
                unsigned char* pub,
                unsigned char* seedid) // output
{
  unsigned char e[crypto_scalarmult_curve25519_BYTES];
  unsigned char s[crypto_scalarmult_curve25519_BYTES];
  unsigned char peerid[STORAGE_ID_LEN];
  unsigned int ptr;

  if(get_seed(e, 0, keyid) != 1) {
    // not found or integrity fault
    return -2;
  }

  // calculate shared secret
  crypto_scalarmult_curve25519(s, e, pub);

  // todo replace peer with peerid!
  ptr = store_seed(s, peer, peer_len);
  // copy keyid to output
  if(ptr == -1 || ptr == -2)
    return -1;

  // del seed containing e
  topeerid(peer, peer_len, peerid);
  del_seed(peerid, keyid);

  // copy ? todo why not return the pointer itself?
  memcpy(seedid, &(((SeedRecord*) ptr)->keyid), STORAGE_ID_LEN);

  return 0;
}
