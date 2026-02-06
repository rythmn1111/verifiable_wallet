/**
 * Sign 48-byte deep hash (base64url) with JWK -> 512-byte signature (base64url).
 * Uses RSA-PSS with SHA-256 to match arbundles/gateway (createSign("sha256")).
 * The 48-byte deep hash is hashed with SHA-256 before PSS signing.
 */

#include "wallet_sign.h"
#include "esp_log.h"
#include "esp_random.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mbedtls/rsa.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

static const char *TAG = "wallet_sign";

#define DEEP_HASH_LEN  48   /* SHA-384 deep hash from arbundles */
#define PSS_HASH_LEN   32   /* SHA-256 digest we sign (matches Node createSign("sha256")) */
#define RSA4096_BYTES  512

static int rng_wrapper(void *ctx, unsigned char *buf, size_t len)
{
	(void)ctx;
	esp_fill_random(buf, len);
	return 0;
}

/* Find key in JSON and return pointer to value string (after opening "). End is next ". */
static const char *jwk_find_key(const char *jwk_json, const char *key)
{
	/* key is e.g. "n" - we search for "key": then optional space then " */
	char pattern[16];
	if (strlen(key) + 4 >= sizeof(pattern))
		return NULL;
	snprintf(pattern, sizeof(pattern), "\"%s\":", key);
	const char *p = strstr(jwk_json, pattern);
	if (!p)
		return NULL;
	p += strlen(pattern); /* past "key": */
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p != '"')
		return NULL;
	return p + 1; /* first char of value */
}

/* Copy JWK key value (base64url) to buf, convert to standard base64 for mbedtls, decode to out_bytes. */
static int jwk_b64url_to_bytes(const char *jwk_json, const char *key_name,
                               unsigned char *out_bytes, size_t out_size, size_t *out_len)
{
	const char *p = jwk_find_key(jwk_json, key_name);
	if (!p)
		return -1;
	const char *end = strchr(p, '"');
	if (!end || end <= p)
		return -2;
	size_t b64_len = (size_t)(end - p);
	if (b64_len > 1024)
		return -3;

	char *b64 = (char *)malloc(b64_len + 8);
	if (!b64)
		return -4;
	memcpy(b64, p, b64_len);
	b64[b64_len] = '\0';
	for (size_t i = 0; i < b64_len; i++) {
		if (b64[i] == '-') b64[i] = '+';
		else if (b64[i] == '_') b64[i] = '/';
	}
	size_t pad = (4 - (b64_len % 4)) % 4;
	for (size_t i = 0; i < pad; i++)
		b64[b64_len + i] = '=';
	b64[b64_len + pad] = '\0';

	size_t olen;
	int ret = mbedtls_base64_decode(out_bytes, out_size, &olen, (const unsigned char *)b64, b64_len + pad);
	free(b64);
	if (ret != 0) {
		ESP_LOGE(TAG, "base64 decode %s failed %d", key_name, ret);
		return -5;
	}
	*out_len = olen;
	return 0;
}

int wallet_sign_hash_from_jwk(const char *jwk_json, const char *hash_b64url,
                              char *sig_b64url_out, size_t sig_b64url_size)
{
	if (!jwk_json || !hash_b64url || !sig_b64url_out || sig_b64url_size < 700)
		return -1;

	/* Decode hash (48 bytes) */
	char hash_b64[128];
	size_t hash_b64_len = strnlen(hash_b64url, 96);
	if (hash_b64_len >= sizeof(hash_b64) - 4)
		return -2;
	memcpy(hash_b64, hash_b64url, hash_b64_len + 1);
	for (size_t i = 0; i < hash_b64_len; i++) {
		if (hash_b64[i] == '-') hash_b64[i] = '+';
		else if (hash_b64[i] == '_') hash_b64[i] = '/';
	}
	size_t pad = (4 - (hash_b64_len % 4)) % 4;
	for (size_t i = 0; i < pad; i++)
		hash_b64[hash_b64_len + i] = '=';
	hash_b64[hash_b64_len + pad] = '\0';

	unsigned char hash_bin[DEEP_HASH_LEN];
	size_t hash_dec_len;
	int ret = mbedtls_base64_decode(hash_bin, sizeof(hash_bin), &hash_dec_len,
	                                (const unsigned char *)hash_b64, hash_b64_len + pad);
	if (ret != 0 || hash_dec_len != DEEP_HASH_LEN) {
		ESP_LOGE(TAG, "hash base64 decode failed %d len=%u", ret, (unsigned)hash_dec_len);
		return -3;
	}

	/* Gateway expects RSA-PSS with SHA-256; Node hashes the message with SHA-256 then signs */
	unsigned char hash256[PSS_HASH_LEN];
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md_info) {
		ESP_LOGE(TAG, "SHA-256 not available");
		return -10;
	}
	ret = mbedtls_md(md_info, hash_bin, DEEP_HASH_LEN, hash256);
	if (ret != 0) {
		ESP_LOGE(TAG, "SHA-256 hash failed %d", ret);
		return -11;
	}

	/* Parse JWK and import RSA */
	unsigned char n_bin[RSA4096_BYTES], e_bin[4], d_bin[RSA4096_BYTES];
	unsigned char p_bin[RSA4096_BYTES/2], q_bin[RSA4096_BYTES/2];
	size_t n_len, e_len, d_len, p_len, q_len;

	if (jwk_b64url_to_bytes(jwk_json, "n", n_bin, sizeof(n_bin), &n_len) != 0 ||
	    jwk_b64url_to_bytes(jwk_json, "e", e_bin, sizeof(e_bin), &e_len) != 0 ||
	    jwk_b64url_to_bytes(jwk_json, "d", d_bin, sizeof(d_bin), &d_len) != 0 ||
	    jwk_b64url_to_bytes(jwk_json, "p", p_bin, sizeof(p_bin), &p_len) != 0 ||
	    jwk_b64url_to_bytes(jwk_json, "q", q_bin, sizeof(q_bin), &q_len) != 0) {
		ESP_LOGE(TAG, "JWK parse failed");
		return -4;
	}

	mbedtls_rsa_context rsa;
	mbedtls_rsa_init(&rsa);
	ret = mbedtls_rsa_import_raw(&rsa, n_bin, n_len, p_bin, p_len, q_bin, q_len, d_bin, d_len, e_bin, e_len);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_import_raw failed %d", ret);
		mbedtls_rsa_free(&rsa);
		return -5;
	}
	ret = mbedtls_rsa_complete(&rsa);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_complete failed %d", ret);
		mbedtls_rsa_free(&rsa);
		return -6;
	}
	ret = mbedtls_rsa_set_padding(&rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_set_padding failed %d", ret);
		mbedtls_rsa_free(&rsa);
		return -7;
	}

	unsigned char sig_bin[RSA4096_BYTES];
	ret = mbedtls_rsa_rsassa_pss_sign(&rsa, rng_wrapper, NULL, MBEDTLS_MD_SHA256, PSS_HASH_LEN, hash256, sig_bin);
	mbedtls_rsa_free(&rsa);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_rsassa_pss_sign failed %d", ret);
		return -8;
	}

	/* Encode signature to base64url */
	size_t olen;
	ret = mbedtls_base64_encode((unsigned char *)sig_b64url_out, sig_b64url_size, &olen, sig_bin, RSA4096_BYTES);
	if (ret != 0) {
		ESP_LOGE(TAG, "base64 encode sig failed %d", ret);
		return -9;
	}
	sig_b64url_out[olen] = '\0';
	for (size_t i = 0; i < olen; i++) {
		if (sig_b64url_out[i] == '+') sig_b64url_out[i] = '-';
		else if (sig_b64url_out[i] == '/') sig_b64url_out[i] = '_';
	}
	/* Strip padding */
	char *eq = strchr(sig_b64url_out, '=');
	if (eq) *eq = '\0';

	return 0;
}
