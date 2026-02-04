#ifndef ARWEAVE_WALLET_GEN_H
#define ARWEAVE_WALLET_GEN_H

#include <stddef.h>

#define BIP39_WORDLIST_SIZE 2048
#define BIP39_WORDS_12     12
#define WALLET_GEN_MNEMONIC_MAX   (12 * 10 + 11)   /* 12 words + spaces */
#define WALLET_GEN_JWK_MAX        6144             /* JWK JSON size (4096-bit RSA base64url fields) */

/* BIP39 wordlist (defined in bip39_wordlist.c) */
extern const char *const bip39_wordlist[BIP39_WORDLIST_SIZE];

/**
 * Callback when wallet generation finishes.
 * Called from the generator task. Use lvgl_port_lock before updating UI.
 * words: 12 words separated by spaces (null-terminated).
 * jwk:   JWK JSON string (null-terminated).
 */
typedef void (*arweave_wallet_gen_done_cb_t)(const char *words, const char *jwk);

/**
 * Start wallet generation in a background task.
 * - Fills 128-bit entropy, encodes to 12 BIP39 words.
 * - Derives 64-byte seed (PBKDF2-HMAC-SHA512).
 * - Generates 4096-bit RSA from seed (deterministic), exports JWK.
 * - Prints words and JWK to ESP_LOG, then calls done_cb.
 * done_cb can be NULL (still prints to serial).
 */
void arweave_wallet_gen_start(arweave_wallet_gen_done_cb_t done_cb);

#endif /* ARWEAVE_WALLET_GEN_H */
