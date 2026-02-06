/**
 * Arweave address from JWK: n (base64url) -> decode -> SHA256 -> base64url.
 * WALLET_RECIPE.md section 3.
 */

#include "wallet_address.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"

static const char *TAG = "wallet_address";

#define RSA4096_N_BYTES  512
#define SHA256_BYTES     32
/* Base64url encode of 32 bytes: ceil(32*8/6) = 43 chars, no padding */

static int b64url_to_standard(char *buf, size_t buf_size, size_t *out_len)
{
	size_t len = strlen(buf);
	if (len + 4 > buf_size) return -1;
	for (size_t i = 0; i < len; i++) {
		if (buf[i] == '-') buf[i] = '+';
		else if (buf[i] == '_') buf[i] = '/';
	}
	size_t pad = (4 - (len % 4)) % 4;
	for (size_t i = 0; i < pad; i++)
		buf[len + i] = '=';
	buf[len + pad] = '\0';
	*out_len = len + pad;
	return 0;
}

static void standard_to_b64url(char *buf)
{
	for (size_t i = 0; buf[i]; i++) {
		if (buf[i] == '+') buf[i] = '-';
		else if (buf[i] == '/') buf[i] = '_';
	}
	/* Strip padding */
	char *p = strchr(buf, '=');
	if (p) *p = '\0';
}

int wallet_address_from_jwk(const char *jwk_json, char *address_buf, size_t address_buf_size)
{
	if (!jwk_json || !address_buf || address_buf_size < WALLET_ARWEAVE_ADDRESS_LEN + 1)
		return -1;

	const char *n_key = "\"n\":\"";
	const char *p = strstr(jwk_json, n_key);
	if (!p) {
		ESP_LOGE(TAG, "JWK missing 'n'");
		return -2;
	}
	p += strlen(n_key);
	const char *end = strchr(p, '"');
	if (!end || end <= p) {
		ESP_LOGE(TAG, "JWK invalid n value");
		return -3;
	}
	size_t n_b64_len = (size_t)(end - p);
	if (n_b64_len > 1024) return -4;

	/* Copy n (base64url) into mutable buffer and convert to standard base64 for mbedtls */
	char *n_b64 = (char *)malloc(n_b64_len + 8);
	if (!n_b64) return -5;
	memcpy(n_b64, p, n_b64_len);
	n_b64[n_b64_len] = '\0';

	size_t std_len;
	if (b64url_to_standard(n_b64, n_b64_len + 8, &std_len) != 0) {
		free(n_b64);
		return -6;
	}

	unsigned char n_bytes[RSA4096_N_BYTES];
	size_t olen;
	int ret = mbedtls_base64_decode(n_bytes, sizeof(n_bytes), &olen, (const unsigned char *)n_b64, std_len);
	free(n_b64);
	if (ret != 0 || olen != RSA4096_N_BYTES) {
		ESP_LOGE(TAG, "base64 decode n failed %d olen=%u", ret, (unsigned)olen);
		return -7;
	}

	unsigned char hash[SHA256_BYTES];
	mbedtls_sha256(n_bytes, olen, hash, 0);

	/* Base64 encode (standard), then convert to base64url and strip padding */
	char b64_buf[64]; /* 32 bytes -> 44 chars with padding */
	size_t b64_len;
	ret = mbedtls_base64_encode((unsigned char *)b64_buf, sizeof(b64_buf), &b64_len, hash, SHA256_BYTES);
	if (ret != 0 || b64_len != 44) {
		ESP_LOGE(TAG, "base64 encode hash failed %d len=%u", ret, (unsigned)b64_len);
		return -8;
	}
	b64_buf[b64_len] = '\0';
	standard_to_b64url(b64_buf);
	size_t addr_len = strlen(b64_buf);
	if (addr_len != WALLET_ARWEAVE_ADDRESS_LEN) {
		ESP_LOGE(TAG, "address length %u expected 43", (unsigned)addr_len);
		return -9;
	}
	memcpy(address_buf, b64_buf, addr_len + 1);
	return 0;
}

int wallet_owner_b64url_from_jwk(const char *jwk_json, char *owner_buf, size_t owner_buf_size)
{
	if (!jwk_json || !owner_buf || owner_buf_size < 512)
		return -1;

	const char *n_key = "\"n\":\"";
	const char *p = strstr(jwk_json, n_key);
	if (!p) {
		ESP_LOGE(TAG, "JWK missing 'n'");
		return -2;
	}
	p += strlen(n_key);
	const char *end = strchr(p, '"');
	if (!end || end <= p) {
		ESP_LOGE(TAG, "JWK invalid n value");
		return -3;
	}
	size_t n_len = (size_t)(end - p);
	if (n_len >= owner_buf_size) {
		ESP_LOGE(TAG, "n value too long %u", (unsigned)n_len);
		return -4;
	}
	memcpy(owner_buf, p, n_len);
	owner_buf[n_len] = '\0';
	return 0;
}
