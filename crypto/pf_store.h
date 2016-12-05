#ifndef PF_STORE_H
#define PF_STORE_H

#include "stfs.h"
#include "user.h"
#include "axolotl.h"

#define STORAGE_ID_LEN 16
#define USER_SALT_LEN 32

#define EKID_LEN 16
#define EKID_NONCE_LEN 15
#define EKID_SIZE (EKID_LEN+EKID_NONCE_LEN)

#ifndef MIN
#define MIN(a, b)      (((a) < (b)) ? (a) : (b))
#endif

int cwrite(int fd, uint8_t *plain, uint32_t len, uint8_t clear);
int cread(uint8_t *fname, uint8_t *buf, uint32_t len);
int save_seed(unsigned char *seed, unsigned char* peer, unsigned char len);
int load_key(uint8_t *path, int sep, uint8_t *buf, int buflen);
int peerbykeyid(uint8_t *keyid, uint8_t *path, uint8_t *peer);
unsigned char peer2seed(unsigned char* key, unsigned char* peer, const unsigned char len);
int ekid2key(uint8_t* key, uint8_t *ekid );
int topeerid(uint8_t *peerid, const uint8_t *peer, const int len);
int load_ltkeypair(Axolotl_KeyPair *kp);
int get_owner(uint8_t *name);
int store_key(const uint8_t* key, const int keylen, const char *type, const uint8_t *keyid, uint8_t* peer, const uint8_t peer_len);
int save_ax(Axolotl_ctx *ctx, uint8_t *peerpub, uint8_t *peer, uint8_t peer_len);
void calc_verifier(uint8_t *out, int outlen, uint8_t *k1, uint8_t *k2);
int write_enc(uint8_t *path, const uint8_t *key, const int keylen);
int pf_store_init(void);
int peer2pub(uint8_t *pub, uint8_t *peer, int peerlen);

int unhex(uint8_t *out, const uint8_t *hex, const int hexlen);
void stohex(uint8_t* d, const uint8_t *s, const uint32_t len);
//unsigned int combine(unsigned char* a, unsigned char alen, unsigned char* b, unsigned char blen, unsigned char* dst, unsigned char dst_len);

#endif // PF_STORE_H
