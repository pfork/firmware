#include <string.h> //memcpy
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include <utils.h>
#include "storage.h"

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

  if(get_seed(e, 0, keyid) == -1) {
    // integrity fault
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

