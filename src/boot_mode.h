/**
 * Boot mode and Sign Tx handoff via NVS.
 * One firmware, two modes: "wallet" (default) and "scanner".
 * Scanner mode runs camera + QR; on done it saves result and reboots back to wallet.
 */
#ifndef BOOT_MODE_H
#define BOOT_MODE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_MODE_WALLET   "wallet"
#define BOOT_MODE_SCANNER  "scanner"

#define SIGN_TX_STATUS_SUCCESS   "success"
#define SIGN_TX_STATUS_CANCELLED "cancelled"

#define SIGN_TX_HASH_MAX  256

/** Return true if NVS says next boot should be scanner mode. */
bool boot_mode_is_scanner(void);

/** Set boot mode to wallet (for next boot). */
void boot_mode_set_wallet(void);

/** Set boot mode to scanner and reboot (call from wallet when user taps Sign Tx). */
void boot_mode_request_scanner_and_reboot(void);

/** Scanner: save decoded hash and set status success, then caller reboots. */
void sign_tx_save_success(const char *hash_str);

/** Scanner: set status cancelled, then caller reboots. */
void sign_tx_save_cancelled(void);

/**
 * Wallet: read result from scanner. Returns true if scanner just ran.
 * If status was success, hash_out is filled (up to hash_max) and hash_out_len set.
 * Caller should clear NVS after reading (sign_tx_clear_result).
 */
bool sign_tx_get_result(char *hash_out, size_t hash_max, size_t *hash_out_len);

/** Clear sign_tx status and hash from NVS after wallet has consumed them. */
void sign_tx_clear_result(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_MODE_H */
