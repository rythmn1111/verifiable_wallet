/* Store the 12 mnemonic words for display on word_count / last_word_count screens. */

#ifndef WALLET_MNEMONIC_DISPLAY_H
#define WALLET_MNEMONIC_DISPLAY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parse space-separated mnemonic and store up to 12 words. Call when wallet is generated. */
void wallet_mnemonic_display_set(const char *mnemonic);

/* Copy word at index (0..11) into buf. Returns 0 on success, -1 if no words or bad index. */
int wallet_mnemonic_display_get_word(int index, char *buf, size_t buf_size);

/* Number of words stored (0 or 12). */
int wallet_mnemonic_display_count(void);

#ifdef __cplusplus
}
#endif

#endif
