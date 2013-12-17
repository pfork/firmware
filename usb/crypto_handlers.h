#ifndef crypto_handlers_h
#define crypto_handlers_h

enum CRYPTO_CMD {
  USB_CRYPTO_CMD_ENCRYPT = 0,
  USB_CRYPTO_CMD_DECRYPT,
  USB_CRYPTO_CMD_SIGN,
  USB_CRYPTO_CMD_VERIFY,
  USB_CRYPTO_CMD_RNG,
  USB_CRYPTO_CMD_ABORT,
  USB_CRYPTO_CMD_STORAGE,
  USB_CRYPTO_CMD_STOP,
};

void handle_ctl(void);
void handle_data(void);

#endif //crypto_handlers_h
