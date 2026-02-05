#pragma once

#include <stdbool.h>

/** Base path for wallet on SD (FAT mounted at /sdcard). */
#define WALLET_SD_DIR        "/sdcard/wallet"
#define WALLET_WORDS_FILE    "/sdcard/wallet/words.txt"
#define WALLET_JWK_FILE      "/sdcard/wallet/jwk.txt"
#define WALLET_FLAG_FILE     "/sdcard/wallet/flag"

/**
 * Save wallet to SD card: creates WALLET_SD_DIR, writes words to words.txt,
 * jwk_json to keyfile.json, and writes "red" to flag (wallet exists).
 * Returns true if all writes succeeded. Requires SD mounted (esp_sdcard_port_is_mounted()).
 */
bool wallet_sd_save(const char *words, const char *jwk_json);

/**
 * Return true if a wallet is already stored (flag file contains "red",
 * or both words.txt and jwk.txt exist). Safe to call without SD mounted (returns false).
 */
bool wallet_sd_exists(void);

/**
 * Delete the stored wallet from SD: remove words.txt, jwk.txt, and flag.
 * Returns true if removal succeeded (or nothing was there). Safe to call without SD mounted (returns false).
 */
bool wallet_sd_delete(void);
