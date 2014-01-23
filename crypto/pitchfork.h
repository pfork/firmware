/**
  ************************************************************************************
  * @file    pitchfork.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */
#ifndef crypto_handlers_h
#define crypto_handlers_h

#include <crypto_secretbox.h>
#include <crypto_generichash.h>
#include "crypto_scalarmult_curve25519.h"
#include "storage.h"

#define BUF_SIZE 32768
#define outstart (outbuf+crypto_secretbox_BOXZEROBYTES)
#define outstart32 (outbuf+crypto_secretbox_ZEROBYTES)

/**
  * @brief  CRYPTO_CMD: enum for all PITCHFORK USB cmd byte
  */
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

/**
  * @brief  Buffer_State: USB buffer state
  */
typedef enum {
  INPUT,      // buffer is ready to accept
  OUTPUT,     // buffer is awaiting output processing
  CLOSED      // buffer is the final one.
} Buffer_State;

/**
  * @brief  Buffer: USB read buffer for double buffering
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

/**
  * @brief ECDH_Start_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char len;
  unsigned char name[32];
} __attribute((packed)) ECDH_Start_Params;

/**
  * @brief ECDH_Response_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char len;
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
  unsigned char name[32];
} __attribute((packed)) ECDH_Response_Params;

/**
  * @brief ECDH_End_Params: struct for accessing params passed from
  *        irq to handler in global params buffer
  */
typedef struct {
  unsigned char keyid[STORAGE_ID_LEN];
  unsigned char pub[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) ECDH_End_Params;

void handle_ctl(usbd_device *usbd_dev, uint8_t ep);
void handle_data(usbd_device *usbd_dev, uint8_t ep);
void handle_buf(void);
void reset(void);

extern Buffer bufs[2];
extern CRYPTO_CMD modus;
extern unsigned char params[128];
extern crypto_generichash_state hash_state;
extern unsigned char blocked;
extern unsigned char outbuf[crypto_secretbox_NONCEBYTES+crypto_secretbox_ZEROBYTES+BUF_SIZE];
extern unsigned char active_buf;

#endif //crypto_handlers_h
