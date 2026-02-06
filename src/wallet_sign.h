/**
 * Sign Arweave data-item hash (48-byte SHA-384) with JWK -> 512-byte signature (RSA-PSS).
 * Hash and signature are base64url-encoded strings.
 */

#ifndef WALLET_SIGN_H
#define WALLET_SIGN_H

#include <stddef.h>

/**
 * Sign hash_b64url (base64url of 48-byte hash) with jwk_json (RSA JWK).
 * Writes base64url of 512-byte signature into sig_b64url_out (at least 700 bytes).
 * Returns 0 on success, negative on error.
 */
int wallet_sign_hash_from_jwk(const char *jwk_json, const char *hash_b64url,
                               char *sig_b64url_out, size_t sig_b64url_size);

#endif
