/**
 * Hold the computed Arweave address in RAM until we save it with the encrypted wallet.
 */

#ifndef WALLET_ADDRESS_PENDING_H
#define WALLET_ADDRESS_PENDING_H

#include <stdbool.h>
#include <stddef.h>

void wallet_address_pending_set(const char *address);
bool wallet_address_pending_has(void);
size_t wallet_address_pending_get(char *buf, size_t buf_size);
void wallet_address_pending_clear(void);

#endif
