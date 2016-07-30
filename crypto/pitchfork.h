#ifndef pitchfork_h
#define pitchfork_h

#include <crypto_secretbox.h>
#include <libopencm3/usb/usbd.h>

#define BUF_SIZE 32768

/**
  * @brief  Buffer_State: USB buffer state
  */
typedef enum {
  INPUT,      // buffer is ready to accept
  OUTPUT,     // buffer is awaiting output processing
  CLOSED      // buffer is the final one.
} Buffer_State;

/**
  * @brief  Buffer: USB read buffer for data double buffering
  */
typedef struct {
  Buffer_State state;                                        /* buffer state (i/o/c) */
  int size;                                                  /* size of buffer */
  unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES+64]; /* extra crypto_secretbox_ZEROBYTES (32) for encryption
                                                              * but decryption needs 40 (nonce+mac) - since this is read
                                                              * it is padded to maxpacketsize so we can handle meh.
                                                              */
  unsigned char* start;                                      /* ptr to start of unused space in buffer */
} Buffer;

extern Buffer bufs[2];
extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];

void handle_ctl(usbd_device *usbd_dev, uint8_t ep);
void handle_data(usbd_device *usbd_dev, uint8_t ep);
void pitchfork_main(void);

#endif // pitchfork_h
