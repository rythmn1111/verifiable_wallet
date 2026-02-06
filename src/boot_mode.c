/**
 * Boot mode and Sign Tx handoff via NVS.
 */
#include "boot_mode.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include <string.h>

#define NVS_NAMESPACE   "wallet"
#define NVS_KEY_MODE    "boot_mode"
#define NVS_KEY_STATUS  "sign_tx_status"
#define NVS_KEY_HASH    "sign_tx_hash"

static bool read_nvs_str(nvs_handle_t h, const char *key, char *out, size_t out_size)
{
    size_t len = out_size;
    esp_err_t err = nvs_get_str(h, key, out, &len);
    if (err != ESP_OK || out_size == 0) {
        if (out_size) out[0] = '\0';
        return false;
    }
    return true;
}

static void write_nvs_str(nvs_handle_t h, const char *key, const char *val)
{
    if (val) nvs_set_str(h, key, val);
}

bool boot_mode_is_scanner(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
    char mode[16];
    bool is_scanner = read_nvs_str(h, NVS_KEY_MODE, mode, sizeof(mode)) && (strcmp(mode, BOOT_MODE_SCANNER) == 0);
    nvs_close(h);
    return is_scanner;
}

void boot_mode_set_wallet(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    write_nvs_str(h, NVS_KEY_MODE, BOOT_MODE_WALLET);
    nvs_commit(h);
    nvs_close(h);
}

void boot_mode_request_scanner_and_reboot(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    write_nvs_str(h, NVS_KEY_MODE, BOOT_MODE_SCANNER);
    nvs_commit(h);
    nvs_close(h);
    esp_restart();
}

void sign_tx_save_success(const char *hash_str)
{
    if (!hash_str) return;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    write_nvs_str(h, NVS_KEY_STATUS, SIGN_TX_STATUS_SUCCESS);
    write_nvs_str(h, NVS_KEY_HASH, hash_str);
    nvs_commit(h);
    nvs_close(h);
}

void sign_tx_save_cancelled(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    write_nvs_str(h, NVS_KEY_STATUS, SIGN_TX_STATUS_CANCELLED);
    nvs_erase_key(h, NVS_KEY_HASH);
    nvs_commit(h);
    nvs_close(h);
}

bool sign_tx_get_result(char *hash_out, size_t hash_max, size_t *hash_out_len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
    char status[16];
    if (!read_nvs_str(h, NVS_KEY_STATUS, status, sizeof(status))) {
        nvs_close(h);
        return false;
    }
    if (strcmp(status, SIGN_TX_STATUS_SUCCESS) != 0) {
        nvs_close(h);
        sign_tx_clear_result();
        return false;
    }
    if (hash_out && hash_max) {
        size_t len = hash_max;
        if (nvs_get_str(h, NVS_KEY_HASH, hash_out, &len) == ESP_OK && hash_out_len)
            *hash_out_len = strnlen(hash_out, hash_max - 1);
        else if (hash_out_len) *hash_out_len = 0;
    }
    nvs_close(h);
    return true;
}

void sign_tx_clear_result(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_key(h, NVS_KEY_STATUS);
    nvs_erase_key(h, NVS_KEY_HASH);
    nvs_commit(h);
    nvs_close(h);
}
