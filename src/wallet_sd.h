#pragma once

#include <stdbool.h>
#include <stddef.h>

/** Base path for wallet on SD (FAT mounted at /sdcard). */
#define WALLET_SD_DIR        "/sdcard/wallet"
#define WALLET_WORDS_FILE    "/sdcard/wallet/words.txt"
#define WALLET_JWK_FILE      "/sdcard/wallet/jwk.txt"
#define WALLET_ADDRESS_FILE  "/sdcard/wallet/address.txt"
#define WALLET_OWNER_FILE    "/sdcard/wallet/owner.txt"  /* 512-byte owner base64url for Upload public QR */
#define WALLET_FLAG_FILE     "/sdcard/wallet/flag"

/**
 * Save wallet to SD card: creates WALLET_SD_DIR, writes words to words.txt,
 * jwk_json to jwk.txt, and writes "red" to flag (wallet exists).
 * Returns true if all writes succeeded. Requires SD mounted (esp_sdcard_port_is_mounted()).
 */
bool wallet_sd_save(const char *words, const char *jwk_json);

/**
 * Save only JWK and flag to SD (no mnemonic words). Use after wallet generation
 * so the 12 words are never written to SD. Returns true if write succeeded.
 */
bool wallet_sd_save_jwk_only(const char *jwk_json);

/**
 * Save encrypted JWK to jwk.txt (format: v1, salt_hex, iv_hex, ct_b64).
 * Use after user sets encryption password. Also writes flag file.
 */
bool wallet_sd_save_encrypted_jwk(const char *salt_hex, const char *iv_hex, const char *ct_b64);

/**
 * Save public Arweave address (43 chars) to address.txt.
 * Call after saving encrypted JWK so both are persisted.
 */
bool wallet_sd_save_public_address(const char *address);

/**
 * Read public Arweave address from address.txt into buf (at least 44 bytes).
 * Returns true if read succeeded and address is 43 chars; false otherwise.
 */
bool wallet_sd_get_public_address(char *buf, size_t buf_size);

/**
 * Save 512-byte owner (base64url string, ~684 chars) to owner.txt.
 * Call when saving the wallet (same time as address). No encryption.
 */
bool wallet_sd_save_owner_b64url(const char *owner_b64url);

/**
 * Read owner base64url from owner.txt into buf (at least 720 bytes recommended).
 * Returns true if read succeeded; false if file missing or too small.
 */
bool wallet_sd_get_owner_b64url(char *buf, size_t buf_size);

/**
 * Read encrypted JWK from jwk.txt (format: v1\nsalt_hex\niv_hex\nct_b64).
 * Buffers must be at least: salt_hex 34, iv_hex 26 (to consume newlines), ct_b64 >= 8192 recommended.
 * Returns true if read succeeded and first line is "v1"; false otherwise.
 */
bool wallet_sd_read_encrypted_jwk(char *salt_hex, size_t salt_hex_size,
                                  char *iv_hex, size_t iv_hex_size,
                                  char *ct_b64, size_t ct_b64_size);

/**
 * Return true if a wallet is already stored (flag file contains "red",
 * or both words.txt and jwk.txt exist). Safe to call without SD mounted (returns false).
 */
bool wallet_sd_exists(void);

/**
 * Delete the stored wallet from SD: remove words.txt, jwk.txt, address.txt, owner.txt, and flag.
 * Returns true if removal succeeded (or nothing was there). Safe to call without SD mounted (returns false).
 */
bool wallet_sd_delete(void);
