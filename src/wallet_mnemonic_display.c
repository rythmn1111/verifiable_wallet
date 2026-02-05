/* Store the 12 mnemonic words for display. Never saved to SD. */

#include "wallet_mnemonic_display.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mnemonic_display";

#define MAX_WORDS  12
#define WORD_LEN   12

static char s_words[MAX_WORDS][WORD_LEN];
static int  s_count = 0;

void wallet_mnemonic_display_set(const char *mnemonic)
{
	s_count = 0;
	if (!mnemonic) return;
	/* Serial print for confirmation only; not saved to SD */
	ESP_LOGI(TAG, "12 words (display only, not saved to SD): %s", mnemonic);
	const char *p = mnemonic;
	for (int i = 0; i < MAX_WORDS && *p; i++) {
		while (*p == ' ') p++;
		if (!*p) break;
		size_t n = 0;
		while (p[n] && p[n] != ' ' && n < (WORD_LEN - 1)) n++;
		if (n > 0) {
			memcpy(s_words[i], p, n);
			s_words[i][n] = '\0';
			s_count++;
		}
		p += n;
	}
}

int wallet_mnemonic_display_get_word(int index, char *buf, size_t buf_size)
{
	if (buf_size == 0 || index < 0 || index >= s_count) return -1;
	if (buf) {
		strncpy(buf, s_words[index], buf_size - 1);
		buf[buf_size - 1] = '\0';
	}
	return 0;
}

int wallet_mnemonic_display_count(void)
{
	return s_count;
}
