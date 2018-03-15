#ifndef pitchfork_h
#define pitchfork_h

#include <crypto_secretbox.h>
#include <libopencm3/usb/usbd.h>

#define BUF_SIZE 32768

/**
  * @brief  CRYPTO_CMD: enum for all PITCHFORK USB cmd byte
  */
typedef enum {
  PITCHFORK_CMD_STOP = 0,

  // list all keys
  PITCHFORK_CMD_LIST_KEYS,

  // dump rng
  PITCHFORK_CMD_RNG,

  // kex functions
  PITCHFORK_CMD_KEX_START,

  // dumping pubkeys
  PITCHFORK_CMD_DUMP_PUB,

  // SPHINX ops
  PITCHFORK_CMD_SPHINX_CREATE,
  PITCHFORK_CMD_SPHINX_GET,
  PITCHFORK_CMD_SPHINX_CHANGE,
  PITCHFORK_CMD_SPHINX_COMMIT,
  PITCHFORK_CMD_SPHINX_DELETE,

  // ops needing double input buffers, starting at 0x10
  // so we can test for them like (modus & PITCHFORK_CMD_BUFFERED)
  PITCHFORK_CMD_BUFFERED = 16,

  PITCHFORK_CMD_SIGN,      // xedsa
  PITCHFORK_CMD_PQSIGN,    // sphincs

  // for encrypting with a shared secret
  PITCHFORK_CMD_ENCRYPT,
  // for encrypting in an axolotl session
  PITCHFORK_CMD_AX_SEND,

  // for decrypting with a shared secret
  PITCHFORK_CMD_DECRYPT,
  // decrypt with our lt pubkey and an ephemeral key? like:
  // epk,esk=genkey() send( epk|nonce|encrypt(esk+pub,plain) )
  PITCHFORK_CMD_DECRYPT_ANON,
  // for decrypting in an axolotl session
  PITCHFORK_CMD_AX_RECEIVE,

  PITCHFORK_CMD_VERIFY,
  // msg+sig+pubkey - as pubkeys are not stored for sphincs
  // we don't do these, because this doesn't need
  // secret key material and can be done on a host
  //PITCHFORK_CMD_PQVERIFY,

  // kex fns which have prekeys as input
  PITCHFORK_CMD_KEX_RESPOND,
  PITCHFORK_CMD_KEX_END,
} CRYPTO_CMD;

/**
  * @brief  Buffer_State: USB buffer state
  */
typedef enum {
  INPUT,      // buffer is ready to accept
  OUTPUT,     // buffer is awaiting output processing
  CLOSED      // buffer is the final one.
} Buffer_State;

typedef enum {
  // /lt /ax /sph /keys /pub /prekeys
  PF_KEY_LONGTERM,
  PF_KEY_AXOLOTL,
  PF_KEY_SPHINCS,
  PF_KEY_SHARED,
  PF_KEY_PUBCURVE,
  PF_KEY_PREKEY,
} PF_KeyType;

/**
  * @brief  Buffer: USB read buffer for data double buffering
  */
typedef struct {
  Buffer_State state;                                        /* buffer state (i/o/c) */
  int size;                                                  /* size of buffer */
  unsigned char* start;                                      /* ptr to start of unused space in buffer */
  unsigned char buf[BUF_SIZE+crypto_secretbox_ZEROBYTES+64]; /* extra crypto_secretbox_ZEROBYTES (32) for encryption
                                                              * and decryption needs 16 (mac)
                                                              */
} Buffer;

extern Buffer bufs[2];
extern unsigned char outbuf[crypto_secretbox_ZEROBYTES+BUF_SIZE];

extern unsigned char blocked;
extern char cmd_blocked;
extern CRYPTO_CMD modus;

void handle_ctl(usbd_device *usbd_dev, uint8_t ep);
void handle_data(usbd_device *usbd_dev, uint8_t ep);
void pitchfork_main(void);

#endif // pitchfork_h
