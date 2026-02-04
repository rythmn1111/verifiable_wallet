#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Start Wi-Fi and run a scan; results will be shown on the settings screen. */
void app_wifi_scan_start(void);

/** Stop Wi-Fi and clear the network list on the settings screen. */
void app_wifi_scan_stop(void);

/** Connect to an AP. Calls esp_wifi_set_config + esp_wifi_connect. */
void app_wifi_connect(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif
