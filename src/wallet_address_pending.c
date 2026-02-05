#include "wallet_address_pending.h"
#include "wallet_address.h"
#include <string.h>

static char s_pending[WALLET_ARWEAVE_ADDRESS_LEN + 1];
static bool s_has = false;

void wallet_address_pending_set(const char *address)
{
	s_has = false;
	if (!address) return;
	size_t len = strnlen(address, WALLET_ARWEAVE_ADDRESS_LEN + 1);
	if (len != WALLET_ARWEAVE_ADDRESS_LEN) return;
	memcpy(s_pending, address, len + 1);
	s_has = true;
}

bool wallet_address_pending_has(void)
{
	return s_has;
}

size_t wallet_address_pending_get(char *buf, size_t buf_size)
{
	if (!s_has || !buf || buf_size == 0) return 0;
	size_t len = strnlen(s_pending, WALLET_ARWEAVE_ADDRESS_LEN + 1);
	if (len >= buf_size) len = buf_size - 1;
	memcpy(buf, s_pending, len);
	buf[len] = '\0';
	return len;
}

void wallet_address_pending_clear(void)
{
	s_has = false;
	(void)memset(s_pending, 0, sizeof(s_pending));
}
