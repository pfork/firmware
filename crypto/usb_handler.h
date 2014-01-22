#ifndef crypto_handlers_h
#define crypto_handlers_h

#include <crypto_secretbox.h>
#include <crypto_generichash.h>
#include "storage.h"

#define BUF_SIZE 32768

typedef enum {
  USB_CRYPTO_CMD_ENCRYPT = 0,
  USB_CRYPTO_CMD_DECRYPT,
  USB_CRYPTO_CMD_SIGN,
  USB_CRYPTO_CMD_VERIFY,
  USB_CRYPTO_CMD_ECDH_START,
  USB_CRYPTO_CMD_ECDH_RESPOND,
  USB_CRYPTO_CMD_ECDH_END,
  USB_CRYPTO_CMD_LIST_KEYS,
  USB_CRYPTO_CMD_RNG,
  USB_CRYPTO_CMD_STOP,
  USB_CRYPTO_CMD_STORAGE,
} CRYPTO_CMD;

typedef enum {
  INPUT,
  OUTPUT,
  CLOSED
} Buffer_State;

typedef struct {
  Buffer_State state;
  int size;
  unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES+64]; // extra crypto_secretbox_ZEROBYTES (32) for encryption
                                                             // but decryption needs 40 (nonce+mac) - since this is read
                                                             // it is padded to maxpacketsize so we can handle meh.
  unsigned char* start;
} Buffer;

typedef struct {
  unsigned char len;
  unsigned char name[32];
} __attribute((packed)) ECDH_Start_Params;

typedef struct {
  unsigned char len;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char name[32];
} __attribute((packed)) ECDH_Response_Params;

typedef struct {
  unsigned char keyid[STORAGE_ID_LEN];
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) ECDH_End_Params;

void handle_ctl(void);
void handle_data(void);
void handle_buf(void);
void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep);

extern Buffer bufs[2];

#endif //crypto_handlers_h