/**
 * Arweave address from JWK: address = base64url(SHA256(n)).
 * WALLET_RECIPE.md section 3.
 */

#ifndef WALLET_ADDRESS_H
#define WALLET_ADDRESS_H

#include <stddef.h>

/** Arweave address is exactly 43 characters (base64url of 32-byte SHA256). */
#define WALLET_ARWEAVE_ADDRESS_LEN  43

/**
 * Compute Arweave wallet address from JWK JSON.
 * address_buf must be at least WALLET_ARWEAVE_ADDRESS_LEN + 1.
 * Returns 0 on success, negative on error (missing/invalid n, decode/sha256 failure).
 */
int wallet_address_from_jwk(const char *jwk_json, char *address_buf, size_t address_buf_size);

/**
 * Extract the 512-byte owner (RSA modulus n) from JWK as base64url string.
 * Used for "Owner (for signing)" QR so the Vite app can build the data item.
 * owner_buf must be at least 700 bytes (base64url of 512 bytes ~ 684 chars + null).
 * Returns 0 on success, negative on error (missing/invalid n in JWK).
 */
int wallet_owner_b64url_from_jwk(const char *jwk_json, char *owner_buf, size_t owner_buf_size);

#endif
