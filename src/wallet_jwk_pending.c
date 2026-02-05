#include "wallet_jwk_pending.h"
#include "arweave_wallet_gen.h"
#include <string.h>

static char s_pending[WALLET_GEN_JWK_MAX];
static bool s_has = false;

void wallet_jwk_pending_set(const char *jwk_json)
{
	s_has = false;
	if (!jwk_json) return;
	size_t len = strnlen(jwk_json, WALLET_GEN_JWK_MAX);
	if (len >= WALLET_GEN_JWK_MAX) return;
	memcpy(s_pending, jwk_json, len + 1);
	s_has = true;
}

bool wallet_jwk_pending_has(void)
{
	return s_has;
}

size_t wallet_jwk_pending_get(char *buf, size_t buf_size)
{
	if (!s_has || !buf || buf_size == 0) return 0;
	size_t len = strnlen(s_pending, WALLET_GEN_JWK_MAX);
	if (len >= buf_size) len = buf_size - 1;
	memcpy(buf, s_pending, len);
	buf[len] = '\0';
	return len;
}

void wallet_jwk_pending_clear(void)
{
	s_has = false;
	(void)memset(s_pending, 0, sizeof(s_pending));
}
