#include "wallet_sd.h"
#include "esp_sdcard_port.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

static const char *TAG = "wallet_sd";

static bool ensure_wallet_dir(void)
{
	struct stat st;
	if (stat(WALLET_SD_DIR, &st) == 0) {
		if (S_ISDIR(st.st_mode))
			return true;
		ESP_LOGE(TAG, "%s exists but is not a directory", WALLET_SD_DIR);
		return false;
	}
	if (mkdir(WALLET_SD_DIR, 0755) != 0) {
		ESP_LOGE(TAG, "mkdir %s: %s", WALLET_SD_DIR, strerror(errno));
		return false;
	}
	ESP_LOGI(TAG, "created %s", WALLET_SD_DIR);
	return true;
}

bool wallet_sd_save(const char *words, const char *jwk_json)
{
	if (!esp_sdcard_port_is_mounted()) {
		ESP_LOGE(TAG, "SD not mounted");
		return false;
	}
	if (!words || !jwk_json) {
		ESP_LOGE(TAG, "null words or jwk");
		return false;
	}
	if (!ensure_wallet_dir())
		return false;

	FILE *f = fopen(WALLET_WORDS_FILE, "w");
	if (!f) {
		ESP_LOGE(TAG, "fopen %s: %s", WALLET_WORDS_FILE, strerror(errno));
		return false;
	}
	if (fputs(words, f) < 0) {
		ESP_LOGE(TAG, "write words failed");
		fclose(f);
		return false;
	}
	if (fclose(f) != 0) {
		ESP_LOGE(TAG, "fclose words: %s", strerror(errno));
		return false;
	}

	f = fopen(WALLET_JWK_FILE, "w");
	if (!f) {
		ESP_LOGE(TAG, "fopen %s: %s", WALLET_JWK_FILE, strerror(errno));
		return false;
	}
	if (fputs(jwk_json, f) < 0) {
		ESP_LOGE(TAG, "write jwk failed");
		fclose(f);
		return false;
	}
	if (fclose(f) != 0) {
		ESP_LOGE(TAG, "fclose jwk: %s", strerror(errno));
		return false;
	}

	f = fopen(WALLET_FLAG_FILE, "w");
	if (f) {
		fputs("red", f);
		fclose(f);
	} else {
		ESP_LOGW(TAG, "optional flag file write failed: %s", strerror(errno));
	}

	ESP_LOGI(TAG, "wallet saved to %s", WALLET_SD_DIR);
	return true;
}

static bool file_readable(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	fclose(f);
	return true;
}

bool wallet_sd_exists(void)
{
	if (!esp_sdcard_port_is_mounted())
		return false;
	/* Prefer flag: if flag exists and contains "red", wallet exists. */
	FILE *f = fopen(WALLET_FLAG_FILE, "r");
	if (f) {
		char buf[8];
		size_t n = fread(buf, 1, sizeof(buf) - 1, f);
		fclose(f);
		if (n > 0) {
			buf[n] = '\0';
			if (strstr(buf, "red")) {
				ESP_LOGD(TAG, "wallet exists (flag=red)");
				return true;
			}
		}
	}
	/* Auto-detect: both words and jwk files present. */
	if (file_readable(WALLET_WORDS_FILE) && file_readable(WALLET_JWK_FILE)) {
		ESP_LOGD(TAG, "wallet exists (words + jwk present)");
		return true;
	}
	return false;
}

bool wallet_sd_delete(void)
{
	if (!esp_sdcard_port_is_mounted()) {
		ESP_LOGE(TAG, "SD not mounted");
		return false;
	}
	bool ok = true;
	if (remove(WALLET_WORDS_FILE) != 0 && errno != ENOENT) {
		ESP_LOGE(TAG, "remove %s: %s", WALLET_WORDS_FILE, strerror(errno));
		ok = false;
	}
	if (remove(WALLET_JWK_FILE) != 0 && errno != ENOENT) {
		ESP_LOGE(TAG, "remove %s: %s", WALLET_JWK_FILE, strerror(errno));
		ok = false;
	}
	if (remove(WALLET_FLAG_FILE) != 0 && errno != ENOENT) {
		ESP_LOGE(TAG, "remove %s: %s", WALLET_FLAG_FILE, strerror(errno));
		ok = false;
	}
	if (ok)
		ESP_LOGI(TAG, "wallet deleted from %s", WALLET_SD_DIR);
	return ok;
}
