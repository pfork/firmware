#include "itoa.h"
#include "storage.h"
#include "pitchfork.h"
#include "master.h"
#include "widgets.h"
#include <string.h>
#include <crypto_generichash.h>

extern MenuCtx appctx;

int flashdump(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t **items = (uint8_t**) bufs[0].start;
  size_t item_len=0;
  unsigned int ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  UserRecord *userdata = get_userrec();

  // todo/bug userdata==NULL not handled!!!!

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    items[item_len]=outptr;
    item_len++;
    switch(*((unsigned char*) ptr)) {
    case SEED: {
      *(outptr++) ='s';
      *(outptr++) =' ';
      nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
      if(nlen==0 || nlen >= PEER_NAME_MAX) {
        // couldn't map peerid to name
        *(outptr-2)='u';
        *(outptr++) =0;
        break;
      }
      outptr[nlen] = 0; //terminate it
      outptr+=nlen+1;

      // pad ciphertext with extra 16 bytes
      memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
             ((SeedRecord*) ptr)->mac,
             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
      // why >>2 ?
      memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
      // nonce
      crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                         (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                         (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
      // decrypt
      // todo/bug erase plain after usage, leaks keymaterial
      if(crypto_secretbox_open(plain,                 // ciphertext output
                               cipher,                // plaintext input
                               crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                               nonce,                 // nonce
                               get_master_key("flashdump"))      // key
         == -1) {
        *(outptr-2-(nlen+1)) = 'c';
      }
      break;
    }

    case PEERMAP: {
      *(outptr++) ='p';
      *(outptr++) =' ';
      nlen = get_peer(outptr, (unsigned char*) ((MapRecord*) ptr)->peerid);
      if(nlen==0 || nlen >= PEER_NAME_MAX) {
        // couldn't map peerid to name
        memcpy(outptr, "unknown", 7);
        outptr+=7;
        *(outptr++) =0;
        break;
      }
      outptr[nlen] = 0; //terminate it
      outptr+=nlen+1;
      break;
    }

    case USERDATA: {
      *(outptr++) ='u';
      *(outptr++) =' ';
      memcpy(outptr, ((UserRecord*) ptr)->name, ((UserRecord*) ptr)->len- USERDATA_HEADER_LEN);
      outptr+=((UserRecord*) ptr)->len- USERDATA_HEADER_LEN;
      *(outptr++)=0;
      break;
    }

    default: {
      item_len--;
      break;
    }
    }

    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }
  if(outptr>outbuf) {
    return menu(&appctx, (const uint8_t**) items,item_len,NULL);
  }
  return 1;
}

int flashstats(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t **items = (uint8_t**) bufs[0].start;
  size_t item_len=5;
  unsigned int ptr = FLASH_BASE, tmp;
  unsigned char *outptr = outbuf, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned short seeds=0, deleted = 0, corrupt = 0, unknown = 0;
  UserRecord *userdata = get_userrec();

  // todo/bug userdata==NULL not handled!!!!

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    if(*((unsigned char*)ptr) != SEED) { // only seeds
        if((*((unsigned char*)ptr) & 0xf0) == 0) {
           deleted++;
        }
        goto endloop; 
    }

    // try to unmask name
    nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
    if(nlen==0 || nlen >= PEER_NAME_MAX) {
      // couldn't map peerid to name
      // what to do? write "unresolvable name"
      unknown++;
    }
    // test if seed can be decrypted
    // pad ciphertext with extra 16 bytes
    memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
           ((SeedRecord*) ptr)->mac,
           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
    // why >>2 ?
    memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
    // nonce
    crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                       (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                       (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
    // decrypt
    // todo/bug erase plain after usage, leaks keymaterial
    if(crypto_secretbox_open(plain,                 // ciphertext output
                             cipher,                // plaintext input
                             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                             nonce,                 // nonce
                             get_master_key("flashstats"))      // key
       == -1) {
      // rewind name of corrupt seed
      corrupt++;
      goto endloop;
    }

    seeds++;

  endloop:
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }
  if(ptr>FLASH_BASE) {
    items[0]=outptr;
    memcpy(outptr, (void*) "seeds: ", 7);
    outptr+=7;
    outptr+=itos((char*) outptr, seeds);
    // write out stats
    items[1]=outptr;
    memcpy(outptr, (void*) "del: ", 5);
    outptr+=5;
    outptr+=itos((char*) outptr, deleted);
    //deleted;
    items[2]=outptr;
    memcpy(outptr, (void*) "cor: ", 5);
    outptr+=5;
    outptr+=itos((char*) outptr, corrupt);
    //corrupt;
    items[3]=outptr;
    memcpy(outptr, (void*) "unk: ", 5);
    outptr+=5;
    outptr+=itos((char*) outptr, unknown);
    //unknown;
    tmp=ptr - FLASH_BASE;
    items[4]=outptr;
    memcpy(outptr, (void*) "bytes: ", 7);
    outptr+=7;
    outptr+=itos((char*) outptr, tmp);

    return menu(&appctx, (const uint8_t**) items,item_len,NULL);
  }
  return 1;
}

// todo/fixme crashes if there's no seeds in the storage
int listseeds(void) {
  extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
  uint8_t **items = (uint8_t**) bufs[0].start;
  size_t item_len=0;
  unsigned int ptr = FLASH_BASE;
  unsigned char *outptr = outbuf, nlen;
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  unsigned char cipher[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  unsigned char plain[crypto_secretbox_KEYBYTES+crypto_secretbox_ZEROBYTES];
  UserRecord *userdata = get_userrec();

  // todo/bug userdata==NULL not handled!!!!

  while(ptr < FLASH_BASE + FLASH_SECTOR_SIZE && *((unsigned char*)ptr) != EMPTY ) {
    if(*((unsigned char*)ptr) != SEED) { // only seeds
        goto endloop;
    }

    // try to unmask name
    nlen = get_peer(outptr, (unsigned char*) ((SeedRecord*) ptr)->peerid);
    if(nlen==0 || nlen >= PEER_NAME_MAX) {
      // couldn't map peerid to name
      // what to do? write "unresolvable name"
      memcpy(outptr, "<corrupt>", 9);
      nlen=9;
    }
    outptr[nlen++] = 0; //terminate it
    // test if seed can be decrypted

    // pad ciphertext with extra 16 bytes
    memcpy(cipher+crypto_secretbox_BOXZEROBYTES,
           ((SeedRecord*) ptr)->mac,
           crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES);
    // why >>2 ?
    memset(cipher, 0, crypto_secretbox_BOXZEROBYTES>>2);
    // nonce
    crypto_generichash(nonce, crypto_secretbox_NONCEBYTES,                  // output
                       (unsigned char*) ((SeedRecord*) ptr)->peerid, STORAGE_ID_LEN<<1, // input
                       (unsigned char*) userdata->salt, USER_SALT_LEN);      // key
    // decrypt
    // todo/bug erase plain after usage, leaks keymaterial
    if(crypto_secretbox_open(plain,                 // ciphertext output
                             cipher,                // plaintext input
                             crypto_scalarmult_curve25519_BYTES+crypto_secretbox_ZEROBYTES, // plain length
                             nonce,                 // nonce
                             get_master_key("listseeds"))      // key
       == -1) {
      // rewind name of corrupt seed
      goto endloop;
    }
    items[item_len]=outptr;
    item_len++;
    outptr+=nlen;

  endloop:
    if( (ptr = next_rec(ptr)) == -1) {
      // invalid record type found, corrupt storage?
      //return -2;
      break;
    }
  }
  if(outptr>outbuf) {
    return menu(&appctx, (const uint8_t**) items,item_len,NULL);
  }
  return 1;
}
