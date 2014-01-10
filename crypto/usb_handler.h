#ifndef crypto_handlers_h
#define crypto_handlers_h

#include <crypto_secretbox.h>

#define BUF_SIZE 32768

typedef enum {
  USB_CRYPTO_CMD_ENCRYPT = 0,
  USB_CRYPTO_CMD_DECRYPT,
  USB_CRYPTO_CMD_SIGN,
  USB_CRYPTO_CMD_VERIFY,
  USB_CRYPTO_CMD_RNG,
  USB_CRYPTO_CMD_STOP,
  USB_CRYPTO_CMD_STORAGE,
} CRYPTO_CMD;

typedef enum {
  INPUT,
  OUTPUT,
  CLOSED
} Crypto_State;

typedef struct {
  Crypto_State state;
  int size;
  unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES+64]; // extra crypto_secretbox_ZEROBYTES (32) for encryption
                                                             // but decryption needs 40 (nonce+mac) - since this is read
                                                             // it is padded to maxpacketsize so we can handle meh.
  unsigned char* start;
} Buffer;

void handle_ctl(void);
void handle_data(void);
void handle_buf(void);

extern Buffer bufs[2];

#endif //crypto_handlers_h
