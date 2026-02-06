/**
 * Encrypt JWK with user password: PBKDF2-SHA256 + AES-256-GCM.
 * Outputs salt_hex, iv_hex, ciphertext_base64 for storage in jwk.txt.
 */

#ifndef WALLET_ENCRYPT_H
#define WALLET_ENCRYPT_H

#include <stddef.h>

#define WALLET_ENCRYPT_SALT_HEX_LEN   32   /* 16 bytes -> 32 hex chars */
#define WALLET_ENCRYPT_IV_HEX_LEN    24   /* 12 bytes -> 24 hex chars */
#define WALLET_ENCRYPT_PBKDF2_ITER   100000

/**
 * Encrypt jwk_json with password.
 * Out buffers: salt_hex (WALLET_ENCRYPT_SALT_HEX_LEN+1), iv_hex (WALLET_ENCRYPT_IV_HEX_LEN+1),
 * ct_b64 (size at least (jwk_len + 16 + 2)/3*4 + 1 for GCM ciphertext+tag in base64).
 * Returns 0 on success, negative on error.
 */
int wallet_encrypt_jwk(const char *password, const char *jwk_json,
                       char *salt_hex, char *iv_hex, char *ct_b64, size_t ct_b64_size);

/**
 * Decrypt stored JWK with password.
 * salt_hex, iv_hex, ct_b64 are as read from jwk.txt (v1 format).
 * jwk_buf receives the decrypted JWK string; jwk_buf_size must be at least WALLET_GEN_JWK_MAX.
 * Returns 0 on success, negative on error (wrong password -> auth failure).
 */
int wallet_decrypt_jwk(const char *password, const char *salt_hex, const char *iv_hex,
                       const char *ct_b64, char *jwk_buf, size_t jwk_buf_size);

#endif
