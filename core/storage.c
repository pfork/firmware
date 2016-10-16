/**
  ************************************************************************************
  * @file    storage.c
  * @author  stf
  * @version V0.2
  * @date    05-December-2013, 06-June-2016
  * @brief   This file provides all functions to handle the internal
  *          flash ram as storage for keys and other data.
  ************************************************************************************
  */

#include "storage.h"
#include <string.h> //memcpy
#include "libopencm3/stm32/flash.h"
#include "randombytes_pitchfork.h"
#include "crypto_scalarmult_curve25519.h"
#include "crypto_secretbox.h"
#include <crypto_generichash.h>
#include "irq.h"
#include "master.h"
#include <utils.h>

/**
  * @brief  topeerid: calculates peerid from peers name
  *         a peerid can be used to get a key from the storage
  * @param  peer: pointer to peers name
  * @param  peer_len: length of peers name
  * @param  peerid: pointer to buffer receiving peerid
  * @retval None
  */
void topeerid(unsigned char* peer,
              unsigned char peer_len,
              unsigned char* peerid) {
  UserRecord *userdata;
  userdata = get_userrec();
  crypto_generichash(peerid, STORAGE_ID_LEN,              // output
                     peer, peer_len,                      // input
                     (userdata!=0)?(userdata->salt):0,    // salt
                     (userdata!=0)?USER_SALT_LEN:0);      // salt len
}

/**
  * @brief  get_ekid: calculates temp key id
  * @param  keyid: pointer to keyid name
  * @param  nonce: pointer to nonce
  * @param  ekid: pointer to buffer receiving ekid
  * @retval None
  */
void get_ekid(unsigned char* keyid,
              unsigned char* nonce,
              unsigned char* ekid) {
  randombytes_buf((void *) nonce, (size_t) EKID_NONCE_LEN);
  crypto_generichash(ekid, EKID_LEN,           // output
                     nonce, EKID_NONCE_LEN,    // output
                     keyid, STORAGE_ID_LEN);   // salt
}

/**
  * @brief  get_seedrec: returns a seedrecord matching the search parameters.
  * @param  peerid: pointer to peerid to search for or 0 (keyid overrides if specified)
  * @param  keyid: pointer to keyid to search for or 0
  * @param  ptr: pointer to starting record in storage or 0 if searching from start
  * @retval pointer to found seed or 0
  */
SeedRecord* get_seedrec(unsigned char* peerid, unsigned char* keyid, unsigned int ptr, unsigned char is_ephemeral) {
  SeedRecord *seedrec, *curseedrec, *seed = 0;
  unsigned char ekid[EKID_LEN+EKID_NONCE_LEN];

  curseedrec = (SeedRecord*) find(ptr,SEED);
  while((unsigned int) curseedrec >= FLASH_BASE && (unsigned int) curseedrec < FLASH_BASE+FLASH_SECTOR_SIZE) {
    seedrec = curseedrec; // TODO refactor curseedrec is redundant
    if(is_ephemeral != 0) {
      // calculate ephemeral keyid
      crypto_generichash(ekid, EKID_LEN,                                     // output
                         keyid+EKID_LEN, EKID_NONCE_LEN,                     // nonce
                         (unsigned char*) seedrec->keyid, STORAGE_ID_LEN);   // key
      if(sodium_memcmp(ekid,keyid,EKID_LEN) == 0)
        return seedrec;
    } else if((peerid != 0 && sodium_memcmp(peerid,seedrec->peerid, STORAGE_ID_LEN) == 0) ||
              (keyid != 0 && sodium_memcmp(keyid,seedrec->keyid, STORAGE_ID_LEN) == 0))
      seed = seedrec;
    curseedrec = (SeedRecord*) find((unsigned int) seedrec, SEED);
  }
  return seed;
}

/**
  * @brief  data_store: appends the record in the internal flash storage
  * @param  data: pointer to buffer to store
  * @param  len: length of record
  * @param  ptr: pointer to starting record in storage or 0 if searching from start
  * @retval pointer to stored record or -1|-2
  */
unsigned int data_store(unsigned char *data, unsigned int len) {
  unsigned int dst;

  dst = find(0, EMPTY);
  if(dst == -2) {
    // corrupt field found.
    // todo handle it
    return -1;
  }
  if(dst == -1 ||
     dst+len >= (FLASH_BASE + FLASH_SECTOR_SIZE) ) {
    // sector is full
    // todo gc
    return -2;
  }

  disable_irqs();
  flash_unlock();
  flash_program(dst, data, len);
  flash_lock();
  enable_irqs();
  return dst;
}

/**
  * @brief  combine: concats two strings
  * @param  a: pointer to 1st buffer
  * @param  alen: length of 1st buffer
  * @param  b: pointer to 2nd buffer
  * @param  blen: length of 2nd buffer
  * @param  dst: pointer to output buffer
  * @param  dst_len: length of output buffer
  * @retval length of result
  */
unsigned int combine(unsigned char* a, unsigned char alen, unsigned char* b, unsigned char blen, unsigned char* dst, unsigned char dst_len) {
  if(alen+blen+2>dst_len)
    return 0;
  memcpy(dst, a, alen);
  dst[alen]=0;
  memcpy(dst+alen+1, b, blen);
  dst[alen+blen+1]=0;
  return alen+blen+2;
}

/**
  * @brief  store_seed: stores a seed related to a peer
  * @param  seed: pointer to seed
  * @param  peer: pointer to peers name
  * @param  len: length of peers name
  * @retval pointer to stored seed or 0
  */
SeedRecord* store_seed(unsigned char *seed, unsigned char* peer, unsigned char len) {
  unsigned char intmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char outtmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char peers[2+PEER_NAME_MAX*2];
  SeedRecord rec, *ptr;
  unsigned int i;
  UserRecord *userdata;

  userdata = get_userrec();
  if(userdata == 0) return 0; // userdata not found - todo test this, this fn had no 0 return before

  rec.type = SEED;
  // set peerid
  topeerid(peer, len, (unsigned char*) rec.peerid);
  // check if there is an existing peermap already
  // and if it decrypts with the current masterkey
  uint8_t junk[33];
  int retries=3;
  while(get_peer(junk, (uint8_t*) rec.peerid)==-1 && retries-->0) {
    erase_master_key();
    get_master_key("bad key");
  }
  if(retries==0) return NULL; // or fail

  // set keyid
  switch(memcmp(peer, userdata->name, MIN(len,userdata->len - USERDATA_HEADER_LEN))) {
  case -1:
    combine(peer, len, userdata->name, userdata->len - USERDATA_HEADER_LEN, peers, 2+PEER_NAME_MAX*2);
    break;
  case 1:
    combine(userdata->name, userdata->len - USERDATA_HEADER_LEN, peer, len, peers, 2+PEER_NAME_MAX*2);
    break;
  case 0:
    if(len<userdata->len - USERDATA_HEADER_LEN)
      combine(peer, len, userdata->name, userdata->len - USERDATA_HEADER_LEN, peers, 2+PEER_NAME_MAX*2);
    else
      combine(userdata->name, userdata->len - USERDATA_HEADER_LEN, peer, len, peers, 2+PEER_NAME_MAX*2);
    break;
  }
  crypto_generichash((unsigned char*) rec.keyid, STORAGE_ID_LEN,         // output
                     peers, len+userdata->len - USERDATA_HEADER_LEN+2,   // input
                     seed, crypto_secretbox_KEYBYTES);                   // key

  // skip storing if already existing record
  if( (ptr = get_seedrec(0, (unsigned char*) rec.keyid, 0, 0)) != 0 )
    return ptr;
  // calculate nonce
  crypto_generichash(nonce, sizeof(nonce),                               // output
                     (unsigned char*) &rec.peerid, STORAGE_ID_LEN<<1,    // input
                     (unsigned char*) userdata->salt, USER_SALT_LEN);    // key

  // encrypt the value
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*)intmp)[i]=0;
  memcpy(intmp+crypto_secretbox_ZEROBYTES, seed, crypto_scalarmult_curve25519_BYTES);
  crypto_secretbox(outtmp,                // ciphertext output
                   intmp,                 // plaintext input
                   crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                   nonce,                 // nonce
                   get_master_key("store key"));     // key

  // clear plaintext seed in RAM
  memset(intmp,0,sizeof(intmp));
  memset(seed,0,crypto_scalarmult_curve25519_BYTES);

  memcpy(rec.mac,
         outtmp+crypto_secretbox_BOXZEROBYTES,
         crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);

  if(store_map(peer, len, (unsigned char*) rec.peerid) == 0) {
    //handle error
    return 0;
  }
  return (SeedRecord*) data_store((unsigned char*) &rec, sizeof(SeedRecord));
}

/**
  * @brief  del_seed: deletes a seed by peerid or keyid
  * @param  peerid: pointer to peerid
  * @param  keyid: pointer to keyid
  * @retval 0 error, success 1
  */
int del_seed(unsigned char* peerid, unsigned char* keyid) {
  unsigned int ptr;
  if((ptr = (unsigned int) get_seedrec(peerid, keyid, 0, 0))==0)
    return 0;

  SeedRecord deleted_seed;
  memset(&deleted_seed, 0, sizeof(deleted_seed));
  deleted_seed.type=DELETED_SEED;

  disable_irqs();
  flash_unlock();
  flash_program(ptr, (unsigned char*) &deleted_seed, sizeof(deleted_seed));
  flash_lock();
  enable_irqs();

  return 1;
}

/**
  * @brief  next_rec: advances the record ptr base on the size of the current record
  * @param  ptr: pointer to current record
  * @retval pointer to next record or -1
  */
unsigned int next_rec(unsigned int ptr) {
  switch(*((unsigned char*) ptr)) {
  case SEED:
    return ptr + sizeof(SeedRecord);
  case DELETED_SEED:
    return ptr + sizeof(SeedRecord);
  case PEERMAP:
    // todo add length check
    return ptr + ((MapRecord*) ptr)->len;
  case USERDATA:
    // todo add length check
    return ptr + ((UserRecord*) ptr)->len;
  default:
    // unhandled/unknown record type found
    return -1;
  }
}

/**
  * @brief  find_last: finds the last record of a given type
  * @param  ptr: pointer to current record
  * @param  type: record type to find.
  * @retval pointer to last record or 0
  */
unsigned int find_last(unsigned int ptr, unsigned char type) {
  unsigned int last = 0;
  unsigned int cur = 0;

  last = find(ptr, type);
  if(last < FLASH_BASE || last >= (FLASH_BASE + FLASH_SECTOR_SIZE) )
    return -1; // not even a first found :/

  while(1) {
    cur = find(last, type);
    if(cur < FLASH_BASE || cur >= (FLASH_BASE + FLASH_SECTOR_SIZE) )
      break;
    last = cur;
  }
  return last;
}

/**
  * @brief  find: finds the next record matching type
  * @param  ptr: pointer to current record
  * @param  type: record type to find.
  * @retval pointer to last record or 0, -1 or -2
  */
unsigned int find(unsigned int ptr, unsigned char type) {
  if(ptr == 0) {
    ptr = FLASH_BASE;
  } else if(ptr < FLASH_BASE || ptr >= FLASH_BASE + FLASH_SECTOR_SIZE) {
    // out of bounds error
    return -2;
  } else {
    // skip to next record
    ptr = next_rec(ptr);
  }
  while( (*((unsigned char*) ptr) != type) &&
         (ptr < (FLASH_BASE + FLASH_SECTOR_SIZE))) {
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      return -2;
    }
    if( type != EMPTY && *((unsigned char*) ptr) == EMPTY)
      // current record is empty -> start of free storage
      // that means, no sense in searching further :/
      return 0;
  }
  if(ptr >= FLASH_BASE + FLASH_SECTOR_SIZE) {
    // sector is full
    // todo gc
    return -1;
  }
  return ptr;
}

/**
  * @brief  get_userrec: finds the last userrec
  * @param  None
  * @retval pointer to last userrec or 0
  */
UserRecord* get_userrec(void) {
  UserRecord *userrec = (UserRecord*) find_last(0,USERDATA);
  if((unsigned int)userrec >= FLASH_BASE &&
     (unsigned int)userrec < FLASH_BASE + FLASH_SECTOR_SIZE) {
    // already existing user record returning it
    return userrec;
  }
  return 0;
}

/**
  * @brief  init_user: creates a new user record with a fresh salt.
  * @param  name: pointer to users name (max 32 char)
  * @param  name_len: length of users name.
  * @retval pointer to new userrec or 0
  */
UserRecord* init_user(unsigned char* name, unsigned char name_len) {
  // todo test this!!!
  // todo check for max size of name
  unsigned char rec[255];
  UserRecord *userrec = get_userrec();
  if(userrec != 0) { // userdata uninitialized
    if((uint32_t)userrec < FLASH_BASE || (uint32_t)userrec >= FLASH_BASE + FLASH_SECTOR_SIZE) {
      // userrec pointing outside of flash sector
      return 0;
    }
    if(memcmp(userrec->name, name, name_len) == 0) {
      // already existing user record returning it
      return userrec;
    }
  }
  ((UserRecord*) &rec)->type = USERDATA;
  ((UserRecord*) &rec)->len = USERDATA_HEADER_LEN+name_len;
  // initialize salt
  randombytes_buf((void *) ((UserRecord*) &rec)->salt, (size_t) USER_SALT_LEN);
  // set name
  memcpy(&(((UserRecord*) &rec)->name), name, name_len);
  // store the record
  userrec = (UserRecord*) data_store(rec, USERDATA_HEADER_LEN+name_len);
  if((int) userrec < 0 ){
    // todo handle error
    return 0;
  }
  return userrec;
}

/**
  * @brief  clear_flash: erases given sector
  * @param  sector_id: # of sector to delete
  * @retval None
  */
void clear_flash(unsigned int sector_id) {
  disable_irqs();
  flash_unlock();
  /* Erasing page*/
  flash_erase_sector(sector_id, FLASH_CR_PROGRAM_X64);
  flash_lock();
  enable_irqs();
}

/**
  * @brief  get_seed: returns pointer to seed for peerid or keyid
  * @param  seed: ptr to buffer receiving the seed
  * @param  peerid: pointer to buffer containing peerid
  * @param  keyid: pointer to buffer containing keyid
  * @retval pointer to seedrecord for found seed or 0.
  */
SeedRecord* get_seed(unsigned char* seed, unsigned char* peerid, unsigned char* keyid, unsigned char is_ephemeral) {
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned int i;
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];

  SeedRecord* seedrec = get_seedrec(peerid, keyid, 0, is_ephemeral);
  if(seedrec == 0) return 0; // seed not found

  UserRecord *userdata = get_userrec();
  if(userdata == 0) return 0; // userdata not found

  // pad ciphertext with extra 16 bytes
  memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
         seedrec->mac,
         crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
  for(i=0;i<(crypto_secretbox_BOXZEROBYTES>>2);i++) ((unsigned int*)cipher)[i]=0;
  // nonce
  crypto_generichash(nonce, sizeof(nonce),                                 // output
                     (unsigned char*) &seedrec->peerid, STORAGE_ID_LEN<<1, // input
                     (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
  // decrypt
  if(crypto_secretbox_open(plain,                 // ciphertext output
                           cipher,                // plaintext input
                           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                           nonce,                 // nonce
                           get_master_key("get key"))      // key
     == -1)
    return 0;
  memcpy(seed, plain+crypto_secretbox_ZEROBYTES,crypto_scalarmult_curve25519_BYTES);
  return seedrec;
}

/**
  * @brief  get_maprec: returns pointer to peerid to name mapping record
  * @param  peerid: pointer to buffer containing peerid
  * @retval pointer to maprecord for found peerid or 0.
  */
MapRecord* get_maprec(unsigned char* peerid) {
  MapRecord *maprec, *curmaprec, *map = 0;

  curmaprec = (MapRecord*) find(0,PEERMAP);
  while((unsigned int) curmaprec >= FLASH_BASE && (unsigned int) curmaprec < FLASH_BASE+FLASH_SECTOR_SIZE) {
    maprec = curmaprec;
    if(memcmp(peerid,maprec->peerid, STORAGE_ID_LEN) == 0)
      map = maprec;
    curmaprec = (MapRecord*) find((unsigned int) maprec,PEERMAP);
  }
  return map;
}

/**
  * @brief  get_peer_seed: returns seed for peer given by name.
  * @param  seed: pointer to storage receiving found seed
  * @param  peer: pointer to name of peer
  * @param  len: length of peer name.
  * @retval pointer to seedrecord for found seed or 0.
  */
SeedRecord* get_peer_seed(unsigned char *seed, unsigned char* peer, unsigned char len) {
  unsigned char peerid[STORAGE_ID_LEN];
  topeerid(peer, len, peerid);
  return get_seed(seed, peerid, 0, 0);
}

/**
  * @brief  store_map: stores a peerid to c(name) mapping
  * @param  name: pointer to name of peer
  * @param  len: length of peer name.
  * @param  peerid: pointer to peerid
  * @retval pointer to maprecord or 0
  */
MapRecord* store_map(unsigned char* name, unsigned char len, unsigned char* peerid) {
  MapRecord map, *maprec;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char intmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char outtmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  UserRecord *userdata;
  unsigned int i;

  if( (maprec = get_maprec(peerid)) != 0)
    return maprec;

  userdata = get_userrec();
  if(userdata == 0) return 0; // userdata not found

  map.type = PEERMAP;
  map.len = PEERMAP_HEADER_LEN + len;
  memcpy(map.peerid, peerid, STORAGE_ID_LEN);
  // encrypt username

  // calculate nonce
  crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                 // output
                     peerid, STORAGE_ID_LEN,                             // input
                     (unsigned char*) userdata->salt, USER_SALT_LEN);    // key

  // encrypt the value
  for(i=0;i<(crypto_secretbox_ZEROBYTES>>2);i++) ((unsigned int*)intmp)[i]=0;
  memcpy(intmp+crypto_secretbox_ZEROBYTES, name, len);
  crypto_secretbox(outtmp,                // ciphertext output
                   intmp,                 // plaintext input
                   len+crypto_secretbox_ZEROBYTES, // plain length
                   nonce,                 // nonce
                   get_master_key("store user"));     // key
  memcpy(map.mac,
         outtmp+crypto_secretbox_BOXZEROBYTES,
         len+crypto_secretbox_MACBYTES);
  return (MapRecord*) data_store((unsigned char*) &map, PEERMAP_HEADER_LEN+len);
}

/**
  * @brief  get_peer: reverses peerid into name
  * @param  map: pointer to receiving 32B buf for name of peer
  * @param  peerid: pointer to peerid
  * @retval length of peers name
  */
int get_peer(unsigned char* map, unsigned char* peerid) {
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned int i;
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];

  MapRecord* maprec = get_maprec(peerid);
  UserRecord *userdata = get_userrec();

  if(maprec == 0) return 0; // seed not found
  if(userdata == 0) return 0; // userdata not found
  // pad ciphertext with extra 16 bytes
  memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
         maprec->mac,
         maprec->len - PEERMAP_MAC_PREFIX_LEN);
  for(i=0;i<(crypto_secretbox_BOXZEROBYTES>>2);i++) ((unsigned int*)cipher)[i]=0;
  // nonce
  crypto_generichash(nonce, sizeof(nonce),                                 // output
                     (unsigned char*) &maprec->peerid, STORAGE_ID_LEN,     // input
                     (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
  // decrypt
  if(crypto_secretbox_open(plain,                 // ciphertext output
                           cipher,                // plaintext input
                           (maprec->len - PEERMAP_HEADER_LEN)+crypto_secretbox_ZEROBYTES, // plain length
                           nonce,                 // nonce
                           get_master_key("peerid 2 peer"))      // key
     == -1)
    return -1;
  if(map!=NULL) memcpy(map, plain+crypto_secretbox_ZEROBYTES,(maprec->len - PEERMAP_HEADER_LEN));
  // clear plaintext
  memset(plain+crypto_secretbox_ZEROBYTES,0,(maprec->len - PEERMAP_HEADER_LEN));
  return (maprec->len - PEERMAP_HEADER_LEN);
}
