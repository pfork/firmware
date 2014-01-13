#ifndef storage_h
#define storage_h
#include "crypto_scalarmult_curve25519.h"
#include "crypto_secretbox.h"

#define MIN(a, b)      (((a) < (b)) ? (a) : (b))
#define crypto_secretbox_MACBYTES (crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES)

typedef enum {
  USERDATA,
  SEED,
  PEERMAP,
  EMPTY = 0xff
} StorageType;

#define DeletedEntry(ptr) (ptr->type & 0x80)

#define STORAGE_ID_LEN 16
#define USER_SALT_LEN 32
#define PEER_NAME_MAX 32
#define PEERMAP_MAC_PREFIX_LEN (PEERMAP_HEADER_LEN - crypto_secretbox_MACBYTES)
#define USERDATA_HEADER_LEN 35
#define PEERMAP_HEADER_LEN 35

#define FLASH_SR_ALL_FLAGS (FLASH_SR_PGSERR | FLASH_SR_PGPERR | \
                            FLASH_SR_PGAERR | FLASH_SR_WRPERR | \
                            FLASH_SR_OPERR | FLASH_SR_EOP | FLASH_SR_BSY)

#define FLASH_SECTOR00 0x08000000 // sector 0 (16KB)
#define FLASH_SECTOR00_SIZE (16 << 10)
#define FLASH_SECTOR01 0x08004000 // sector 1 (16KB)
#define FLASH_SECTOR01_SIZE (16 << 10)
#define FLASH_SECTOR02 0x08008000 // sector 2 (16KB)
#define FLASH_SECTOR02_SIZE (16 << 10)
#define FLASH_SECTOR03 0x0800c000 // sector 3 (16KB)
#define FLASH_SECTOR03_SIZE (16 << 10)
#define FLASH_SECTOR04 0x08010000 // sector 4 (64KB)
#define FLASH_SECTOR04_SIZE (64 << 10)
#define FLASH_SECTOR05 0x08020000 // sector 5 (128KB)
#define FLASH_SECTOR05_SIZE (128 << 10)

#define FLASH_BASE FLASH_SECTOR04
#define FLASH_SECTOR_SIZE FLASH_SECTOR04_SIZE
#define FLASH_SECTOR_ID 4

#define FLASH_CR_PROGRAM_X8      (0x00 << 8)
#define FLASH_CR_PROGRAM_X16     (0x01 << 8)
#define FLASH_CR_PROGRAM_X32     (0x02 << 8)
#define FLASH_CR_PROGRAM_X64     (0x03 << 8)

// user data
typedef struct {
  unsigned char type;
  unsigned short len;
  unsigned char salt[32];
  unsigned char name[4]; // dummy length defined by len
} __attribute((packed)) UserRecord;

// seed
typedef struct {
  unsigned char type;
  unsigned int peerid[STORAGE_ID_LEN>>2];
  unsigned int keyid[STORAGE_ID_LEN>>2];
  unsigned char mac[crypto_secretbox_MACBYTES];
  unsigned char value[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) SeedRecord;

// deleted seed
typedef struct {
  unsigned char type;
  unsigned int peerid[STORAGE_ID_LEN>>2];
  unsigned int keyid[STORAGE_ID_LEN>>2];
} __attribute((packed)) DeletedSeed;

// name mapping - caution variable length record!
typedef struct {
  unsigned char type;
  unsigned short len;
  unsigned int peerid[STORAGE_ID_LEN>>2];
  unsigned char mac[crypto_secretbox_MACBYTES];
  unsigned char peer_name; // dummy holder, \0 separated
} __attribute((packed)) MapRecord;

void clear_flash(unsigned int sector_id);
unsigned int data_store(unsigned char *data, unsigned int len);
unsigned int next_rec(unsigned int ptr);
unsigned int find(unsigned int ptr, unsigned char type);
unsigned int find_last(unsigned int ptr, unsigned char type);

unsigned int store_seed(unsigned char *seed, unsigned char* peer, unsigned char len);
int get_seed(unsigned char* seed, unsigned char* peerid, unsigned char* keyid);
int get_peer_seed(unsigned char *seed, unsigned char* peer, unsigned char len);
SeedRecord* get_seedrec(unsigned char type, unsigned char* peerid, unsigned char* keyid);
unsigned int del_seed(unsigned char* peerid, unsigned char* keyid);

// reverse mapping of peerids
MapRecord* store_map(unsigned char* name, unsigned char len, unsigned char* peerid);
int get_peer(unsigned char* map, unsigned char* peerid);

UserRecord* init_user(unsigned char* name, unsigned char name_len);
UserRecord* get_userrec(void);

void topeerid(unsigned char* peer, unsigned char peer_len, unsigned char* peerid);

#endif // storage_h
