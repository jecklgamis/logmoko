#ifndef LOGMOKO_ENCRYPTION_H
#define LOGMOKO_ENCRYPTION_H

#include <stddef.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#define CRYPTO_KEY_LEN  32
#define CRYPTO_IV_LEN   12
#define CRYPTO_TAG_LEN  16
#define CRYPTO_OVERHEAD (CRYPTO_IV_LEN + CRYPTO_TAG_LEN)

/* Derive a 32-byte AES key from a preshared key string via SHA-256. */
void derive_key(const char *psk, unsigned char *key);

/*
 * Encrypt plain[0..plain_len) with AES-256-GCM using a fresh random IV.
 * Wire format written to out: [12-byte IV][ciphertext][16-byte GCM tag]
 * *out_len is set to plain_len + CRYPTO_OVERHEAD on success.
 * Returns 0 on success, -1 on failure.
 */
int encrypt_packet(EVP_CIPHER_CTX *ctx, const unsigned char *key,
                   const unsigned char *plain, int plain_len,
                   unsigned char *out, int *out_len);

/*
 * Decrypt an AES-256-GCM packet produced by encrypt_packet.
 * Expects in[0..in_len): [12-byte IV][ciphertext][16-byte GCM tag]
 * *out_len is set to in_len - CRYPTO_OVERHEAD on success.
 * Returns 0 on success, -1 on failure (including authentication failure).
 */
int decrypt_packet(EVP_CIPHER_CTX *ctx, const unsigned char *key,
                   const unsigned char *in, int in_len,
                   unsigned char *out, int *out_len);

#endif /* LOGMOKO_ENCRYPTION_H */
