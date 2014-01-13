#include "storage.h"
#include <string.h> //memcpy
#include "libopencm3/stm32/flash.h"
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include "crypto_secretbox.h"
#include <crypto_generichash.h>
#include "storage.h"
#include "irq.h"
#include "master.h"
#include <utils.h>

void topeerid(unsigned char* peer,
              unsigned char peer_len,
              unsigned char* peerid) {
  UserRecord *userdata;
  userdata = get_userrec();
  crypto_generichash(peerid, STORAGE_ID_LEN,              // output
                     peer, peer_len,                      // input
                     (userdata->salt), USER_SALT_LEN);    // key
}

SeedRecord* get_seedrec(unsigned char type, unsigned char* peerid, unsigned char* keyid) {
  SeedRecord *seedrec, *curseedrec, *seed = 0;

  curseedrec = (SeedRecord*) find(0,type);
  while((unsigned int) curseedrec >= FLASH_BASE && (unsigned int) curseedrec < FLASH_BASE+FLASH_SECTOR_SIZE) {
    seedrec = curseedrec;
    if((peerid != 0 && sodium_memcmp(peerid,seedrec->peerid, STORAGE_ID_LEN) == 0) ||
       (keyid != 0 && sodium_memcmp(keyid,seedrec->keyid, STORAGE_ID_LEN) == 0)) seed = seedrec;
    curseedrec = (SeedRecord*) find((unsigned int) seedrec, type);
  }
  return seed;
}

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

unsigned int combine(unsigned char* a, unsigned char alen, unsigned char* b, unsigned char blen, unsigned char* dst, unsigned char dst_len) {
  if(alen+blen+2>dst_len)
    return 0;
  memcpy(dst, a, alen);
  dst[alen]=0;
  memcpy(dst+alen+1, b, blen);
  dst[alen+blen+1]=0;
  return alen+blen+2;
}

unsigned int store_seed(unsigned char *seed, unsigned char* peer, unsigned char len) {
  unsigned char intmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char outtmp[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char peers[2+PEER_NAME_MAX*2];
  SeedRecord rec, *ptr;
  unsigned int i;
  UserRecord *userdata;

  userdata = get_userrec();

  rec.type = SEED;
  // set peerid
  topeerid(peer, len, (unsigned char*) rec.peerid);
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
  if( (ptr = get_seedrec(SEED, 0, (unsigned char*) rec.keyid)) != 0 )
    return (unsigned int) ptr;
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
                   get_master_key());     // key
  memcpy(rec.mac,
         outtmp+crypto_secretbox_BOXZEROBYTES,
         crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);

  // todo clear plaintext seed in RAM
  store_map(peer, len, (unsigned char*) rec.peerid);
  return data_store((unsigned char*) &rec, sizeof(SeedRecord));
}

unsigned int del_seed(unsigned char* peerid, unsigned char* keyid) {
  DeletedSeed rec;
  unsigned int ptr;
  if((ptr = (unsigned int) get_seedrec(SEED|0x80, peerid, keyid))!=0)
    return ptr;
  rec.type = SEED | 0x80;
  memcpy(rec.peerid, peerid, STORAGE_ID_LEN);
  memcpy(rec.keyid, keyid, STORAGE_ID_LEN);
  return data_store((unsigned char*) &rec, sizeof(DeletedSeed));
}

unsigned int next_rec(unsigned int ptr) {
  switch(*((unsigned char*) ptr)) {
  case SEED:
    return ptr + sizeof(SeedRecord);
  case (SEED | 0x80):
    return ptr + sizeof(DeletedSeed);
  case PEERMAP:
    return ptr + ((MapRecord*) ptr)->len;
  case USERDATA:
    return ptr + ((UserRecord*) ptr)->len;
  default:
    // unknown record type found
    return -1;
  }
}

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

UserRecord* get_userrec(void) {
  UserRecord *userrec = (UserRecord*) find_last(0,USERDATA);
  if((unsigned int)userrec >= FLASH_BASE &&
     (unsigned int)userrec < FLASH_BASE + FLASH_SECTOR_SIZE) {
    // already existing user record returning it
    return userrec;
  }
  return 0;
}

UserRecord* init_user(unsigned char* name, unsigned char name_len) {
  // todo test this!!!
  unsigned char rec[255];
  UserRecord *userrec = get_userrec();
  if((unsigned int)userrec != 0 &&
     sodium_memcmp(userrec->name, name, name_len) == 0) {
    // already existing user record returning it
    return userrec;
  }
  ((UserRecord*) &rec)->type = USERDATA;
  ((UserRecord*) &rec)->len = USERDATA_HEADER_LEN+name_len;
  // initialize salt
  randombytes_salsa20_random_buf((void *) ((UserRecord*) &rec)->salt, (size_t) USER_SALT_LEN);
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

// invoke with clear_flash(FLASH_SECTOR_ID)
void clear_flash(unsigned int sector_id) {
  disable_irqs();
  flash_unlock();
  /* Erasing page*/
  flash_erase_sector(sector_id, FLASH_CR_PROGRAM_X64);
  flash_lock();
  enable_irqs();
}

int get_seed(unsigned char* seed, unsigned char* peerid, unsigned char* keyid) {
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned int i;
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];

  SeedRecord* seedrec = get_seedrec(SEED, peerid, keyid);
  UserRecord *userdata = get_userrec();

  if(seedrec == 0) return 0; // seed not found
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
                           get_master_key())      // key
     == -1)
    return -1;
  memcpy(seed, plain+crypto_secretbox_ZEROBYTES,crypto_scalarmult_curve25519_BYTES);
  return (unsigned int) seedrec;
}

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

int get_peer_seed(unsigned char *seed, unsigned char* peer, unsigned char len) {
  unsigned char peerid[STORAGE_ID_LEN];
  topeerid(peer, len, peerid);
  return get_seed(seed, peerid, 0);
}

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
                   get_master_key());     // key
  memcpy(map.mac,
         outtmp+crypto_secretbox_BOXZEROBYTES,
         len+crypto_secretbox_MACBYTES);
  return (MapRecord*) data_store((unsigned char*) &map, PEERMAP_HEADER_LEN+len);
}

int get_peer(unsigned char* map, unsigned char* peerid) {
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned int i;
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];

  MapRecord* maprec = get_maprec(peerid);
  UserRecord *userdata = get_userrec();

  if(maprec == 0) return 0; // seed not found
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
                           get_master_key())      // key
     == -1)
    return -1;
  memcpy(map, plain+crypto_secretbox_ZEROBYTES,(maprec->len - PEERMAP_HEADER_LEN));
  return (maprec->len - PEERMAP_HEADER_LEN);
}
