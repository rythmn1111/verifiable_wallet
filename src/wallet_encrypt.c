/**
 * Encrypt JWK with password: PBKDF2-SHA256 + AES-256-GCM.
 */

#include "wallet_encrypt.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mbedtls/gcm.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/base64.h"
#include "mbedtls/cipher.h"

static const char *TAG = "wallet_encrypt";

#define WALLET_SALT_BYTES  16
#define WALLET_GCM_IV_BYTES  12
#define WALLET_KEY_BYTES   32
#define WALLET_GCM_TAG_BYTES  16

static void bytes_to_hex(const unsigned char *buf, size_t len, char *hex)
{
	for (size_t i = 0; i < len; i++)
		sprintf(hex + i * 2, "%02x", buf[i]);
	hex[len * 2] = '\0';
}

int wallet_encrypt_jwk(const char *password, const char *jwk_json,
                       char *salt_hex, char *iv_hex, char *ct_b64, size_t ct_b64_size)
{
	if (!password || !jwk_json || !salt_hex || !iv_hex || !ct_b64)
		return -1;

	size_t jwk_len = strlen(jwk_json);
	if (jwk_len == 0)
		return -2;

	/* Minimum ct_b64_size: base64((jwk_len + WALLET_GCM_TAG_BYTES)) */
	size_t ct_len = jwk_len + WALLET_GCM_TAG_BYTES;
	size_t b64_min = ((ct_len + 2) / 3) * 4 + 1;
	if (ct_b64_size < b64_min)
		return -3;

	unsigned char salt[WALLET_SALT_BYTES];
	unsigned char iv[WALLET_GCM_IV_BYTES];
	unsigned char key[WALLET_KEY_BYTES];

	esp_fill_random(salt, WALLET_SALT_BYTES);
	esp_fill_random(iv, WALLET_GCM_IV_BYTES);

	int ret = mbedtls_pkcs5_pbkdf2_hmac_ext(
		MBEDTLS_MD_SHA256,
		(const unsigned char *)password, strlen(password),
		salt, WALLET_SALT_BYTES,
		WALLET_ENCRYPT_PBKDF2_ITER, WALLET_KEY_BYTES, key);
	if (ret != 0) {
		ESP_LOGE(TAG, "PBKDF2 failed %d", ret);
		return -4;
	}

	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);
	ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, WALLET_KEY_BYTES * 8);
	if (ret != 0) {
		mbedtls_gcm_free(&gcm);
		return -5;
	}

	unsigned char *ct = (unsigned char *)malloc(ct_len);
	if (!ct) {
		mbedtls_gcm_free(&gcm);
		return -6;
	}

	unsigned char tag[WALLET_GCM_TAG_BYTES];
	ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, jwk_len,
		iv, WALLET_GCM_IV_BYTES, NULL, 0,
		(const unsigned char *)jwk_json, ct, WALLET_GCM_TAG_BYTES, tag);
	mbedtls_gcm_free(&gcm);
	if (ret != 0) {
		ESP_LOGE(TAG, "GCM encrypt failed %d", ret);
		free(ct);
		return -7;
	}

	/* Store ciphertext || tag for decryption later */
	memcpy(ct + jwk_len, tag, WALLET_GCM_TAG_BYTES);

	bytes_to_hex(salt, WALLET_SALT_BYTES, salt_hex);
	bytes_to_hex(iv, WALLET_GCM_IV_BYTES, iv_hex);

	size_t olen;
	ret = mbedtls_base64_encode((unsigned char *)ct_b64, ct_b64_size, &olen, ct, (size_t)(jwk_len + WALLET_GCM_TAG_BYTES));
	free(ct);
	if (ret != 0) {
		ESP_LOGE(TAG, "base64 encode failed %d", ret);
		return -8;
	}
	ct_b64[olen] = '\0';

	return 0;
}
