#pragma once

#include <stdbool.h>

/** Initialize SD card (SDMMC 1-line), mount FAT at /sdcard. Returns true if mounted, false otherwise. Logs everything. */
bool esp_sdcard_port_init(void);

/** Returns true if SD was successfully mounted (so /sdcard is available). */
bool esp_sdcard_port_is_mounted(void);
