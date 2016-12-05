#ifndef PITCHFORK_H
#define PITCHFORK_H
#include <libusb-1.0/libusb.h>

#define EKID_LEN 16
#define EKID_NONCE_LEN 15
#define EKID_SIZE (EKID_LEN+EKID_NONCE_LEN)

#define USB_CRYPTO_EP_CTRL_IN 0x01
#define USB_CRYPTO_EP_DATA_IN 0x02
#define USB_CRYPTO_EP_CTRL_OUT 0x81
#define USB_CRYPTO_EP_DATA_OUT 0x82

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

int open_pitchfork(libusb_context **ctx, libusb_device_handle **dev_handle);
void pf_flush(libusb_device_handle *dev_handle, int endpoint);
int pf_close(libusb_context *ctx, libusb_device_handle *dev_handle);
void pf_reset(libusb_device_handle *dev_handle);
int pf_stop(libusb_device_handle *dev_handle);
int pf_rng(libusb_device_handle *dev_handle, int size);
int pf_list(libusb_device_handle *dev_handle, uint8_t type, uint8_t *peer);
int pf_encrypt(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_decrypt(libusb_device_handle *dev_handle);
int pf_ax_send(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_ax_recv(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_kex_start(libusb_device_handle *dev_handle);
int pf_kex_respond(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_kex_end(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_pqsign(libusb_device_handle *dev_handle);
int pf_pqverify(void);
int pf_sign(libusb_device_handle *dev_handle);
int pf_verify(libusb_device_handle *dev_handle, uint8_t *peer);
int pf_encrypt_anon(void);
int pf_decrypt_anon(libusb_device_handle *dev_handle);
int pf_get_pub(libusb_device_handle *dev_handle, int type);

#endif // PITCHFORK_H
