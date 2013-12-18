#ifndef crypto_handlers_h
#define crypto_handlers_h

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

void handle_ctl(void);
void handle_data(void);
void handle_buf(void);

#endif //crypto_handlers_h
