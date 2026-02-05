/**
 * Hold JWK in memory after wallet generation until user sets encryption password.
 * Never written to SD as plaintext; cleared after encrypted save.
 */

#ifndef WALLET_JWK_PENDING_H
#define WALLET_JWK_PENDING_H

#include <stddef.h>
#include <stdbool.h>

/** Store JWK (call from wallet_gen_done_cb; do not save to SD yet). */
void wallet_jwk_pending_set(const char *jwk_json);

/** Return true if pending JWK is set. */
bool wallet_jwk_pending_has(void);

/**
 * Copy pending JWK into buf. Returns length copied (excluding NUL), or 0 if none.
 * buf_size must be > 0. Caller should use WALLET_GEN_JWK_MAX from arweave_wallet_gen.h.
 */
size_t wallet_jwk_pending_get(char *buf, size_t buf_size);

/** Clear pending JWK (call after successful encrypted save). */
void wallet_jwk_pending_clear(void);

#endif
