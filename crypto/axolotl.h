#ifndef AXOLOTL_H
#define AXOLOTL_H

#include "crypto_scalarmult.h"
#include "crypto_secretbox.h"
#include <stdint.h>
#include "newhope.h"
#include "poly.h"

#define axolotl_box_BYTES 128
#define PADDEDHCRYPTLEN (16+sizeof(long long)*2 + crypto_scalarmult_curve25519_BYTES+crypto_secretbox_MACBYTES)


#ifndef BagSize
#define BagSize 8
#endif // BagSize

#ifndef BagReuseDeleted
#define BagReuseDeleted 1
#endif // BagReuseDeleted

#ifndef AXOLOTL_DEBUG
#define AXOLOTL_DEBUG 0
#endif // AXOLOTL_DEBUG

typedef struct {
  uint8_t id;
  uint8_t mk[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) BagEntry;

typedef struct {
  uint8_t sk[crypto_scalarmult_curve25519_BYTES];
  uint8_t pk[crypto_scalarmult_curve25519_BYTES];
} __attribute((packed)) Axolotl_KeyPair;

typedef struct {
  uint8_t identitykey[crypto_scalarmult_curve25519_BYTES];
  uint8_t ephemeralkey[crypto_scalarmult_curve25519_BYTES];
  uint8_t DHRs[crypto_scalarmult_curve25519_BYTES];
  uint8_t newhope[POLY_BYTES];
  uint8_t sig[64]; // for xeddsa signature
} __attribute((packed)) Axolotl_PreKey;

typedef struct {
  uint8_t dhis[crypto_scalarmult_curve25519_BYTES];
  uint8_t eph[crypto_scalarmult_curve25519_BYTES];
  uint8_t dhrs[crypto_scalarmult_curve25519_BYTES];
  poly newhope;
} __attribute((packed)) Axolotl_prekey_private;

typedef struct {
  // 32-byte root key which gets updated by DH ratchet
  uint8_t rk[crypto_secretbox_KEYBYTES];

  // 32-byte header keys (send and recv versions)
  uint8_t hks[crypto_secretbox_KEYBYTES];
  uint8_t hkr[crypto_secretbox_KEYBYTES];

  // 32-byte next header keys (~)
  uint8_t nhks[crypto_secretbox_KEYBYTES];
  uint8_t nhkr[crypto_secretbox_KEYBYTES];

  // 32-byte chain keys (used for forward-secrecy updating)
  uint8_t cks[crypto_secretbox_KEYBYTES];
  uint8_t ckr[crypto_secretbox_KEYBYTES];

  // ECDH Ratchet keys
  Axolotl_KeyPair dhrs;
  uint8_t dhrr[crypto_scalarmult_curve25519_BYTES];

  // Message numbers (reset to 0 with each new ratchet)
  unsigned long long ns, nr;

  // Previous message numbers (# of msgs sent under prev ratchet)
  unsigned long long pns;

  // bobs 1st message?
  unsigned char bobs1stmsg :1;
  // is Alice
  unsigned char isAlice :1;

  // A array[STAGING_SIZE] of stored message keys and their associated header
  // keys for "skipped" messages, i.e. messages that have not been
  // received despite the reception of more recent messages.
  BagEntry skipped_HK_MK[BagSize];
} __attribute((packed)) Axolotl_ctx;

void axolotl_genid(Axolotl_KeyPair * keys);
//void axolotl_prekey(Axolotl_PreKey *prekey, Axolotl_ctx *ctx, const Axolotl_KeyPair *keypair);
//int axolotl_handshake(Axolotl_ctx* ctx, const Axolotl_PreKey *prekey);
void axolotl_prekey(Axolotl_PreKey *prekey, Axolotl_prekey_private *ctx, const Axolotl_KeyPair *keypair, uint8_t nh);
int axolotl_handshake(Axolotl_ctx* ctx, Axolotl_PreKey *resp, const Axolotl_PreKey *prekey, Axolotl_prekey_private *private);
void axolotl_box(Axolotl_ctx *ctx, uint8_t *out, uint32_t *out_len, const uint8_t *in, const uint32_t in_len);
int axolotl_box_open(Axolotl_ctx *ctx, uint8_t *out, uint32_t *out_len, const uint8_t *in, const uint32_t in_len);
int ax_recv(Axolotl_ctx *ctx, uint8_t *paddedout, uint32_t *out_len,
            const uint8_t *hnonce, const uint8_t *mnonce,
            const uint8_t *hcrypt, uint8_t *paddedmcrypt,
            const int mcryptlen, uint8_t *mk);

#if AXOLOTL_DEBUG
void print_ctx(Axolotl_ctx *ctx);
void print_key(const char* prefix, const uint8_t* key);
#endif // AXOLOTL_DEBUG

#endif // AXOLOTL_H
