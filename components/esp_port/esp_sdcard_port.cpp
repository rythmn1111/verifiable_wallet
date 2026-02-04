#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"

static const char *TAG = "sdcard";

static bool s_sdcard_mounted = false;

/* ESP32-S3-Touch-LCD-3.5 board: SDMMC 1-line pins (same as board_examples 08_lvgl_image) */
#define EXAMPLE_PIN_SD_CMD  GPIO_NUM_10
#define EXAMPLE_PIN_SD_D0  GPIO_NUM_9
#define EXAMPLE_PIN_SD_CLK GPIO_NUM_11

bool esp_sdcard_port_is_mounted(void)
{
    return s_sdcard_mounted;
}

bool esp_sdcard_port_init(void)
{
    s_sdcard_mounted = false;
    esp_err_t ret;

    ESP_LOGI(TAG, "SD card: initializing (SDMMC 1-line, CMD=%d D0=%d CLK=%d)",
             EXAMPLE_PIN_SD_CMD, EXAMPLE_PIN_SD_D0, EXAMPLE_PIN_SD_CLK);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    const char mount_point[] = "/sdcard";
    sdmmc_card_t *card = NULL;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = EXAMPLE_PIN_SD_CLK;
    slot_config.cmd = EXAMPLE_PIN_SD_CMD;
    slot_config.d0 = EXAMPLE_PIN_SD_D0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "SD card: mounting filesystem at %s", mount_point);
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "SD card: not detected or failed to mount (format_if_mount_failed=false)");
        } else {
            ESP_LOGW(TAG, "SD card: init failed (%s). Check pull-ups on SD lines.", esp_err_to_name(ret));
        }
        return false;
    }

    s_sdcard_mounted = true;
    ESP_LOGI(TAG, "SD card: detected and mounted at %s", mount_point);
    if (card != NULL) {
        ESP_LOGI(TAG, "SD card: capacity %llu MB, sector size %lu",
                 (unsigned long long)((uint64_t)card->csd.capacity * card->csd.sector_size) / (1024 * 1024),
                 (unsigned long)card->csd.sector_size);
    }
    return true;
}
