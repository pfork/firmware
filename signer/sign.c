
#include <string.h>

#include "crypto_generichash.h"
#include "crypto_sign_ed25519.h"
#include "utils.h"
#include "private/curve25519_ref10.h"


int
crypto_sign_ed25519_detached(unsigned char *sig, unsigned long long *siglen_p,
                             const unsigned char *m, unsigned long long mlen,
                             const unsigned char *sk)
{
    crypto_generichash_state hs;
    unsigned char az[64];
    unsigned char nonce[64];
    unsigned char hram[64];
    ge_p3 R;

    crypto_generichash(az, 64, sk, 32, NULL, 0);
    az[0] &= 248;
    az[31] &= 63;
    az[31] |= 64;

    crypto_generichash_init(&hs, NULL, 0, 64);
    crypto_generichash_update(&hs, az + 32, 32);
    crypto_generichash_update(&hs, m, mlen);
    crypto_generichash_final(&hs, nonce, 64);

    memmove(sig + 32, sk + 32, 32);

    sc_reduce(nonce);
    ge_scalarmult_base(&R, nonce);
    ge_p3_tobytes(sig, &R);

    crypto_generichash_init(&hs, NULL, 0, 64);
    crypto_generichash_update(&hs, sig, 64);
    crypto_generichash_update(&hs, m, mlen);
    crypto_generichash_final(&hs, hram, 64);

    sc_reduce(hram);
    sc_muladd(sig + 32, hram, az, nonce);

    sodium_memzero(az, sizeof az);

    if (siglen_p != NULL) {
        *siglen_p = 64U;
    }
    return 0;
}

int
crypto_sign_ed25519(unsigned char *sm, unsigned long long *smlen_p,
                    const unsigned char *m, unsigned long long mlen,
                    const unsigned char *sk)
{
    unsigned long long siglen;

    memmove(sm + crypto_sign_ed25519_BYTES, m, mlen);
/* LCOV_EXCL_START */
    if (crypto_sign_ed25519_detached(sm, &siglen,
                                     sm + crypto_sign_ed25519_BYTES,
                                     mlen, sk) != 0 ||
        siglen != crypto_sign_ed25519_BYTES) {
        if (smlen_p != NULL) {
            *smlen_p = 0;
        }
        memset(sm, 0, mlen + crypto_sign_ed25519_BYTES);
        return -1;
    }
/* LCOV_EXCL_STOP */

    if (smlen_p != NULL) {
        *smlen_p = mlen + siglen;
    }
    return 0;
}
