#include <string.h>
#include "user.h"
#include "stfs.h"
#include "randombytes_pitchfork.h"
#include <crypto_generichash.h>
#include "crypto_scalarmult_curve25519.h"
#include "xeddsa_keygen.h"

#include "pf_store.h"

#include "pqcrypto_sign.h"

/**
  * @brief  get_user: returns userrec
  * @param  None
  * @retval pointer to last userrec or 0
  */
int get_user(UserRecord* userrec) {
  uint8_t fname[]="/cfg/user";
  int fd;
  if((fd=stfs_open(fname, 0))==-1) {
    return -1;
  }
  int len;
  if((len=stfs_size(fd))>USER_META_SIZE+32) {
    // too big
    return -1;
  }
  if(stfs_read(fd, (void*) userrec, len)!=len) {
    // short read
    return -1;
  }
  return stfs_close(fd);
}

static int default_sphincs_key() {
  uint8_t pk[PQCRYPTO_PUBLICKEYBYTES],
    _sk[crypto_secretbox_ZEROBYTES+PQCRYPTO_SECRETKEYBYTES],
    *sk=_sk+crypto_secretbox_ZEROBYTES,
    keyid[STORAGE_ID_LEN];
  randombytes_buf((void *) sk, PQCRYPTO_SECRETKEYBYTES);
  pqcrypto_sign_public_key(pk, sk);
  crypto_generichash(keyid, sizeof(keyid), pk, sizeof(pk), NULL, 0);

  uint8_t sphpath[]="/sph/                                ";
  // just in case mkdir /sph
  sphpath[4]=0;
  stfs_mkdir(sphpath); // just in case
  sphpath[4]='/';

  stohex(sphpath+5,keyid, sizeof(keyid));
  // write out sk
  int fd=stfs_open(sphpath,64);
  if(fd<0) {
    return -1;
  } else {
    if(cwrite(fd, _sk, PQCRYPTO_SECRETKEYBYTES, 1)==-1) return -1;
  }
  return 0;
}

static int default_lt_key() {
  uint8_t pk[crypto_scalarmult_curve25519_BYTES],
    _sk[crypto_secretbox_ZEROBYTES+crypto_scalarmult_curve25519_BYTES],
    *sk=_sk+crypto_secretbox_ZEROBYTES,
    keyid[STORAGE_ID_LEN];
  randombytes_buf((void *) sk, crypto_scalarmult_curve25519_BYTES);
  //crypto_scalarmult_curve25519_base(pk, sk);
  sc_clamp(sk);
  curve25519_keygen(pk,sk);
  crypto_generichash(keyid, sizeof(keyid), pk, sizeof(pk), NULL, 0);

  uint8_t lpath[]="/lt/                                ";
  // just in case mkdir /lt
  lpath[3]=0;
  stfs_mkdir(lpath);
  lpath[3]='/';

  stohex(lpath+4,keyid, sizeof(keyid));
  // write out sk
  int fd=stfs_open(lpath,64);
  if(fd<0) {
    return -1;
  } else {
    if(cwrite(fd, _sk, crypto_scalarmult_curve25519_BYTES,1)==-1) return -1;
  }
  return 0;
}

/**
  * @brief  new_user: creates a new user record with a fresh salt.
  * @param  name: pointer to users name (max 32 char)
  * @param  name_len: length of users name.
  * @param  userrec: pointer to new userrec
  * @retval 0 on success -1 otherwise
  */
int new_user(UserRecord* rec, unsigned char* name, unsigned char name_len) {
  if(name_len>PEER_NAME_MAX) {
    return -1;
  }
  rec->len=name_len;
  // initialize salt
  randombytes_buf((void *) rec->salt, (size_t) USER_SALT_LEN);
  // set name
  memcpy(rec->name, name, name_len);

  uint8_t cfgdir[]="/cfg";
  uint8_t fname[]="/cfg/user";
  stfs_mkdir(cfgdir); // just in case
  stfs_unlink(fname); // just in case
  int fd;
  if((fd=stfs_open(fname, 64))==-1) {
    return -1;
  }
  if(stfs_write(fd, (void*) rec, USER_META_SIZE+name_len)!=USER_META_SIZE+name_len) {
    // short write
    return -1;
  }
  if(stfs_close(fd)==-1) return -1;

  // add long-term key automatically
  if(0!=default_lt_key()) {
    stfs_unlink(fname);
    return -1;
  }

  // create sphincs signing key
  if(0!=default_sphincs_key()) {
    stfs_unlink(fname);
    return -1;
  }

  return 0;
}
