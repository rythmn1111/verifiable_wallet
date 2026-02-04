/**
 * Arweave wallet generator: 12-word BIP39 -> seed -> RSA 4096 -> JWK.
 * Runs in a FreeRTOS task; prints words and JWK to serial and calls done_cb.
 */

#include "arweave_wallet_gen.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

#include "mbedtls/rsa.h"
#include "mbedtls/bignum.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pkcs5.h"

static const char *TAG = "wallet_gen";

#define ENTROPY_BITS_12  128
#define ENTROPY_BYTES_12 (ENTROPY_BITS_12 / 8)
#define SEED_BYTES       64
#define MNEMONIC_SALT     "mnemonic"
#define PBKDF2_ITER      2048

/* Base64url encode (no padding, + -> -, / -> _) */
static size_t base64url_encode(const unsigned char *in, size_t in_len, char *out, size_t out_max)
{
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	size_t i, j = 0;
	uint32_t v;

	for (i = 0; i + 3 <= in_len && j + 4 <= out_max; i += 3) {
		v = (uint32_t)in[i] << 16 | (uint32_t)in[i + 1] << 8 | in[i + 2];
		out[j++] = b64[(v >> 18) & 63];
		out[j++] = b64[(v >> 12) & 63];
		out[j++] = b64[(v >> 6) & 63];
		out[j++] = b64[v & 63];
	}
	if (i < in_len && j < out_max) {
		v = (uint32_t)in[i] << 16;
		if (i + 1 < in_len) v |= (uint32_t)in[i + 1] << 8;
		out[j++] = b64[(v >> 18) & 63];
		out[j++] = b64[(v >> 12) & 63];
		if (i + 1 < in_len) out[j++] = b64[(v >> 6) & 63];
	}
	out[j] = '\0';
	return j;
}

/* BIP39 encode: 16 bytes entropy -> 12 words (with checksum). Bits are MSB-first. */
static void bip39_encode_12(const unsigned char *entropy, char *words_out, size_t words_out_size)
{
	unsigned char hash[32];
	unsigned char bits[17];  /* 128 bits entropy + 4 bits checksum = 132 bits */
	size_t i, b;
	uint32_t idx;

	mbedtls_sha256(entropy, ENTROPY_BYTES_12, hash, 0);
	memcpy(bits, entropy, ENTROPY_BYTES_12);
	bits[ENTROPY_BYTES_12] = (unsigned char)(hash[0] >> 4);  /* 4 checksum bits in high nibble */

	words_out[0] = '\0';
	for (i = 0; i < 12; i++) {
		uint32_t start = (uint32_t)(i * 11);
		idx = 0;
		for (b = 0; b < 11; b++) {
			uint32_t bit_pos = start + b;
			idx = (idx << 1) | ((bits[bit_pos / 8] >> (7 - (bit_pos % 8))) & 1);
		}
		if (idx >= BIP39_WORDLIST_SIZE) idx = BIP39_WORDLIST_SIZE - 1;
		const char *w = bip39_wordlist[idx];
		if (i > 0) { (void)strlcat(words_out, " ", words_out_size); }
		(void)strlcat(words_out, w, words_out_size);
	}
}

/* Entropy callback: return our seed buffer (for deterministic DRBG) */
typedef struct {
	const unsigned char *seed;
	size_t seed_len;
	size_t read_pos;
} seed_entropy_ctx_t;

static int seed_entropy_cb(void *ctx, unsigned char *out, size_t len)
{
	seed_entropy_ctx_t *s = (seed_entropy_ctx_t *)ctx;
	size_t left = s->seed_len - s->read_pos;
	if (left > len) left = len;
	memcpy(out, s->seed + s->read_pos, left);
	if (left < len)
		memset(out + left, 0, len - left);
	s->read_pos += left;
	return 0;
}

#define B64_N_MAX  1024
#define B64_E_MAX  32
#define B64_D_MAX  1024
#define B64_PQ_MAX 512

static void wallet_gen_task(void *arg)
{
	arweave_wallet_gen_done_cb_t done_cb = (arweave_wallet_gen_done_cb_t)arg;
	unsigned char entropy[ENTROPY_BYTES_12];
	char mnemonic[WALLET_GEN_MNEMONIC_MAX];
	unsigned char seed[SEED_BYTES];
	char *jwk_buf = NULL;
	int ret;

	/* Allocate JWK and base64 buffers on heap to avoid stack overflow (task stack is limited) */
	jwk_buf = (char *)malloc(WALLET_GEN_JWK_MAX);
	if (!jwk_buf) {
		ESP_LOGE(TAG, "malloc jwk_buf failed");
		if (done_cb) done_cb("", "");
		vTaskDelete(NULL);
		return;
	}

	/* 1. Random entropy */
	esp_fill_random(entropy, sizeof(entropy));
	/* 2. BIP39 encode -> 12 words */
	bip39_encode_12(entropy, mnemonic, sizeof(mnemonic));

	ESP_LOGI(TAG, "Mnemonic (12 words): %s", mnemonic);

	/* 3. Mnemonic -> seed (PBKDF2-HMAC-SHA512) */
	ret = mbedtls_pkcs5_pbkdf2_hmac_ext(
		MBEDTLS_MD_SHA512,
		(const unsigned char *)mnemonic, strlen(mnemonic),
		(const unsigned char *)MNEMONIC_SALT, sizeof(MNEMONIC_SALT) - 1,
		PBKDF2_ITER, SEED_BYTES, seed);
	if (ret != 0) {
		ESP_LOGE(TAG, "pbkdf2 failed %d", ret);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}

	/* 4. Seed -> deterministic RSA 4096 via CTR_DRBG */
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ctr_drbg_init(&ctr_drbg);
	seed_entropy_ctx_t seed_ctx = { .seed = seed, .seed_len = SEED_BYTES, .read_pos = 0 };
	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, seed_entropy_cb, &seed_ctx, NULL, 0);
	if (ret != 0) {
		ESP_LOGE(TAG, "ctr_drbg_seed failed %d", ret);
		mbedtls_ctr_drbg_free(&ctr_drbg);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}

	mbedtls_rsa_context rsa;
	mbedtls_rsa_init(&rsa);
	ret = mbedtls_rsa_gen_key(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, 4096, 65537);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_gen_key failed %d", ret);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}

	/* 5. RSA -> JWK (base64url of n,e,d,p,q,dp,dq,qi) via mbedtls 3.x export API */
	size_t rsa_len = mbedtls_rsa_get_len(&rsa);  /* modulus size in bytes (512 for 4096-bit) */
	size_t n_len = rsa_len;
	size_t e_len = 4;   /* 65537 fits in 4 bytes */
	size_t d_len = rsa_len;
	size_t p_len = rsa_len / 2;
	size_t q_len = rsa_len / 2;

	unsigned char *n_bin = malloc(n_len);
	unsigned char *e_bin = malloc(e_len);
	unsigned char *d_bin = malloc(d_len);
	unsigned char *p_bin = malloc(p_len);
	unsigned char *q_bin = malloc(q_len);

	if (!n_bin || !e_bin || !d_bin || !p_bin || !q_bin) {
		ESP_LOGE(TAG, "malloc for JWK params failed");
		free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}

	ret = mbedtls_rsa_export_raw(&rsa, n_bin, n_len, p_bin, p_len, q_bin, q_len, d_bin, d_len, e_bin, e_len);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_export_raw failed %d", ret);
		free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}

	mbedtls_mpi DP_mpi, DQ_mpi, QP_mpi;
	mbedtls_mpi_init(&DP_mpi);
	mbedtls_mpi_init(&DQ_mpi);
	mbedtls_mpi_init(&QP_mpi);
	ret = mbedtls_rsa_export_crt(&rsa, &DP_mpi, &DQ_mpi, &QP_mpi);
	if (ret != 0) {
		ESP_LOGE(TAG, "rsa_export_crt failed %d", ret);
		mbedtls_mpi_free(&DP_mpi);
		mbedtls_mpi_free(&DQ_mpi);
		mbedtls_mpi_free(&QP_mpi);
		free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}
	size_t dp_len = mbedtls_mpi_size(&DP_mpi);
	size_t dq_len = mbedtls_mpi_size(&DQ_mpi);
	size_t qi_len = mbedtls_mpi_size(&QP_mpi);
	unsigned char *dp_bin = malloc(dp_len);
	unsigned char *dq_bin = malloc(dq_len);
	unsigned char *qi_bin = malloc(qi_len);
	if (!dp_bin || !dq_bin || !qi_bin) {
		free(dp_bin); free(dq_bin); free(qi_bin);
		mbedtls_mpi_free(&DP_mpi); mbedtls_mpi_free(&DQ_mpi); mbedtls_mpi_free(&QP_mpi);
		free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}
	mbedtls_mpi_write_binary(&DP_mpi, dp_bin, dp_len);
	mbedtls_mpi_write_binary(&DQ_mpi, dq_bin, dq_len);
	mbedtls_mpi_write_binary(&QP_mpi, qi_bin, qi_len);
	mbedtls_mpi_free(&DP_mpi);
	mbedtls_mpi_free(&DQ_mpi);
	mbedtls_mpi_free(&QP_mpi);

	/* Base64 buffers on heap to keep task stack small */
	char *n_b64 = malloc(B64_N_MAX);
	char *e_b64 = malloc(B64_E_MAX);
	char *d_b64 = malloc(B64_D_MAX);
	char *p_b64 = malloc(B64_PQ_MAX);
	char *q_b64 = malloc(B64_PQ_MAX);
	char *dp_b64 = malloc(B64_PQ_MAX);
	char *dq_b64 = malloc(B64_PQ_MAX);
	char *qi_b64 = malloc(B64_PQ_MAX);
	if (!n_b64 || !e_b64 || !d_b64 || !p_b64 || !q_b64 || !dp_b64 || !dq_b64 || !qi_b64) {
		free(n_b64); free(e_b64); free(d_b64); free(p_b64); free(q_b64);
		free(dp_b64); free(dq_b64); free(qi_b64);
		free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
		free(dp_bin); free(dq_bin); free(qi_bin);
		mbedtls_rsa_free(&rsa);
		free(jwk_buf);
		if (done_cb) done_cb(mnemonic, "");
		vTaskDelete(NULL);
		return;
	}
	base64url_encode(n_bin, n_len, n_b64, B64_N_MAX);
	base64url_encode(e_bin, e_len, e_b64, B64_E_MAX);
	base64url_encode(d_bin, d_len, d_b64, B64_D_MAX);
	base64url_encode(p_bin, p_len, p_b64, B64_PQ_MAX);
	base64url_encode(q_bin, q_len, q_b64, B64_PQ_MAX);
	base64url_encode(dp_bin, dp_len, dp_b64, B64_PQ_MAX);
	base64url_encode(dq_bin, dq_len, dq_b64, B64_PQ_MAX);
	base64url_encode(qi_bin, qi_len, qi_b64, B64_PQ_MAX);

	(void)snprintf(jwk_buf, WALLET_GEN_JWK_MAX,
		"{\"kty\":\"RSA\",\"e\":\"%s\",\"n\":\"%s\",\"d\":\"%s\",\"p\":\"%s\",\"q\":\"%s\",\"dp\":\"%s\",\"dq\":\"%s\",\"qi\":\"%s\"}",
		e_b64, n_b64, d_b64, p_b64, q_b64, dp_b64, dq_b64, qi_b64);

	ESP_LOGI(TAG, "JWK: %s", jwk_buf);

	free(n_b64); free(e_b64); free(d_b64); free(p_b64); free(q_b64);
	free(dp_b64); free(dq_b64); free(qi_b64);
	free(n_bin); free(e_bin); free(d_bin); free(p_bin); free(q_bin);
	free(dp_bin); free(dq_bin); free(qi_bin);
	mbedtls_rsa_free(&rsa);

	if (done_cb) done_cb(mnemonic, jwk_buf);
	free(jwk_buf);
	vTaskDelete(NULL);
}

/* Task stack: 16KB — mbedtls RSA 4096 key gen and bignum ops need significant stack */
#define WALLET_GEN_TASK_STACK 16384

void arweave_wallet_gen_start(arweave_wallet_gen_done_cb_t done_cb)
{
	xTaskCreate(wallet_gen_task, "wallet_gen", WALLET_GEN_TASK_STACK, (void *)done_cb, 5, NULL);
}
