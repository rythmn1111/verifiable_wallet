#include "wallet_sd.h"
#include "wallet_encrypt.h"
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

bool wallet_sd_save_jwk_only(const char *jwk_json)
{
	if (!esp_sdcard_port_is_mounted()) {
		ESP_LOGE(TAG, "SD not mounted");
		return false;
	}
	if (!jwk_json || jwk_json[0] == '\0') {
		ESP_LOGD(TAG, "jwk empty, not saving");
		return false;
	}
	if (!ensure_wallet_dir())
		return false;

	FILE *f = fopen(WALLET_JWK_FILE, "w");
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

	ESP_LOGI(TAG, "wallet (JWK only) saved to %s (mnemonic not saved)", WALLET_SD_DIR);
	return true;
}

bool wallet_sd_save_encrypted_jwk(const char *salt_hex, const char *iv_hex, const char *ct_b64)
{
	if (!esp_sdcard_port_is_mounted()) {
		ESP_LOGE(TAG, "SD not mounted");
		return false;
	}
	if (!salt_hex || !iv_hex || !ct_b64) {
		ESP_LOGE(TAG, "null encrypted jwk params");
		return false;
	}
	if (!ensure_wallet_dir())
		return false;

	FILE *f = fopen(WALLET_JWK_FILE, "w");
	if (!f) {
		ESP_LOGE(TAG, "fopen %s: %s", WALLET_JWK_FILE, strerror(errno));
		return false;
	}
	fprintf(f, "v1\n%s\n%s\n%s\n", salt_hex, iv_hex, ct_b64);
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

	ESP_LOGI(TAG, "encrypted JWK saved to %s", WALLET_SD_DIR);
	return true;
}

bool wallet_sd_save_public_address(const char *address)
{
	if (!esp_sdcard_port_is_mounted()) {
		ESP_LOGE(TAG, "SD not mounted");
		return false;
	}
	if (!address || strlen(address) != 43) {
		ESP_LOGE(TAG, "invalid address (must be 43 chars)");
		return false;
	}
	if (!ensure_wallet_dir())
		return false;

	FILE *f = fopen(WALLET_ADDRESS_FILE, "w");
	if (!f) {
		ESP_LOGE(TAG, "fopen %s: %s", WALLET_ADDRESS_FILE, strerror(errno));
		return false;
	}
	if (fputs(address, f) < 0) {
		ESP_LOGE(TAG, "write address failed");
		fclose(f);
		return false;
	}
	if (fclose(f) != 0) {
		ESP_LOGE(TAG, "fclose address: %s", strerror(errno));
		return false;
	}
	ESP_LOGI(TAG, "public address saved to %s", WALLET_ADDRESS_FILE);
	return true;
}

bool wallet_sd_get_public_address(char *buf, size_t buf_size)
{
	if (!esp_sdcard_port_is_mounted() || !buf || buf_size < 44)
		return false;
	FILE *f = fopen(WALLET_ADDRESS_FILE, "r");
	if (!f)
		return false;
	size_t n = fread(buf, 1, buf_size - 1, f);
	fclose(f);
	if (n == 0 || n > 43) {
		buf[0] = '\0';
		return false;
	}
	buf[n] = '\0';
	/* Trim newline if present */
	if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
		buf[--n] = '\0';
	}
	if (n != 43) {
		buf[0] = '\0';
		return false;
	}
	return true;
}

bool wallet_sd_read_encrypted_jwk(char *salt_hex, size_t salt_hex_size,
                                  char *iv_hex, size_t iv_hex_size,
                                  char *ct_b64, size_t ct_b64_size)
{
	if (!esp_sdcard_port_is_mounted() || !salt_hex || !iv_hex || !ct_b64)
		return false;
	/* Need +2 so fgets consumes newline (32+1 salt, 24+1 iv) */
	if (salt_hex_size < (WALLET_ENCRYPT_SALT_HEX_LEN + 2) ||
	    iv_hex_size < (WALLET_ENCRYPT_IV_HEX_LEN + 2) ||
	    ct_b64_size < 100)
		return false;
	FILE *f = fopen(WALLET_JWK_FILE, "r");
	if (!f)
		return false;
	char line_buf[256];
	bool ok = false;
	if (!fgets(line_buf, sizeof(line_buf), f))
		goto out;
	if (strstr(line_buf, "v1") != line_buf)
		goto out;
	/* fgets with size N reads at most N-1 chars + null; use size so newline is consumed */
	if (!fgets(salt_hex, (size_t)salt_hex_size, f))
		goto out;
	if (!fgets(iv_hex, (size_t)iv_hex_size, f))
		goto out;
	if (!fgets(ct_b64, (size_t)ct_b64_size, f))
		goto out;
	/* Trim trailing newline/cr so hex strings are exact length */
	for (char *p = salt_hex; *p; p++)
		if (*p == '\n' || *p == '\r') { *p = '\0'; break; }
	for (char *p = iv_hex; *p; p++)
		if (*p == '\n' || *p == '\r') { *p = '\0'; break; }
	for (char *p = ct_b64; *p; p++)
		if (*p == '\n' || *p == '\r') { *p = '\0'; break; }
	ok = true;
out:
	fclose(f);
	return ok;
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
	if (remove(WALLET_ADDRESS_FILE) != 0 && errno != ENOENT) {
		ESP_LOGE(TAG, "remove %s: %s", WALLET_ADDRESS_FILE, strerror(errno));
		ok = false;
	}
	if (ok)
		ESP_LOGI(TAG, "wallet deleted from %s", WALLET_SD_DIR);
	return ok;
}
