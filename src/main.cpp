#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_io_expander_tca9554.h"
#include "esp_http_client.h"
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) && CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "esp_3inch5_lcd_port.h"
#include "esp_sdcard_port.h"
#include "ui.h"
#include "screens/ui_setting_screen.h"
#include "screens/ui_Screen1.h"
#include "screens/ui_Screen2.h"
#include "screens/ui_Screen3.h"
#include "screens/ui_wifi_password_screen.h"
#include "app_wifi.h"

#define EXAMPLE_PIN_I2C_SDA   GPIO_NUM_8
#define EXAMPLE_PIN_I2C_SCL   GPIO_NUM_7
#define EXAMPLE_DISPLAY_ROTATION 0

#if EXAMPLE_DISPLAY_ROTATION == 90 || EXAMPLE_DISPLAY_ROTATION == 270
#define EXAMPLE_LCD_H_RES 480
#define EXAMPLE_LCD_V_RES  320
#else
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES  480
#endif

#define LCD_BUFFER_SIZE   (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8)
#define I2C_PORT_NUM      0

static const char *TAG = "wallet";

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_io_expander_handle_t s_expander_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static lv_display_t *s_lvgl_disp = NULL;

static void i2c_bus_init(void);
static void io_expander_init(void);
static void lv_port_init(void);

/* Wi-Fi scan (lazy init, one-shot task) */
static bool s_wifi_inited = false;
static SemaphoreHandle_t s_wifi_scan_done = NULL;
static void wifi_init_sta(void);
static void wifi_scan_task(void *arg);

/* NTP: start once when we get IP */
static bool s_sntp_started = false;
static void start_sntp_once(void);

/* AO/AR price fetch from CoinGecko when connected */
static void price_fetch_task(void *arg);

/* WiFi creds on SD: /sdcard/wifi/creds (line1=SSID, line2=password). Written on successful connect. */
#define WIFI_CREDS_DIR   "/sdcard/wifi"
#define WIFI_CREDS_FILE  "/sdcard/wifi/creds"
#define WIFI_SSID_MAX    32
#define WIFI_PASS_MAX    64
static char s_last_ssid[WIFI_SSID_MAX + 1];
static char s_last_password[WIFI_PASS_MAX + 1];

/** Read SSID and password from WIFI_CREDS_FILE. Returns true if both lines read. */
static bool wifi_creds_read_from_sd(char *ssid_out, size_t ssid_size, char *pass_out, size_t pass_size)
{
    if (!esp_sdcard_port_is_mounted() || ssid_out == NULL || pass_size == 0) {
        return false;
    }
    FILE *f = fopen(WIFI_CREDS_FILE, "r");
    if (f == NULL) {
        ESP_LOGI(TAG, "SD wifi: creds file not found (%s)", WIFI_CREDS_FILE);
        return false;
    }
    bool ok = false;
    if (fgets(ssid_out, (int)ssid_size, f) != NULL) {
        size_t n = strnlen(ssid_out, ssid_size);
        if (n > 0 && ssid_out[n - 1] == '\n') {
            ssid_out[n - 1] = '\0';
        }
        if (pass_out && fgets(pass_out, (int)pass_size, f) != NULL) {
            n = strnlen(pass_out, pass_size);
            if (n > 0 && pass_out[n - 1] == '\n') {
                pass_out[n - 1] = '\0';
            }
            ok = ssid_out[0] != '\0';
        }
    }
    fclose(f);
    if (ok) {
        ESP_LOGI(TAG, "SD wifi: read creds from %s (ssid='%s')", WIFI_CREDS_FILE, ssid_out);
    }
    return ok;
}

/** Create WIFI_CREDS_DIR if missing, then write SSID and password to WIFI_CREDS_FILE. */
static void wifi_creds_write_to_sd(const char *ssid, const char *password)
{
    if (!esp_sdcard_port_is_mounted()) {
        ESP_LOGI(TAG, "SD wifi: skip write (SD not mounted)");
        return;
    }
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGI(TAG, "SD wifi: skip write (no ssid)");
        return;
    }
    ESP_LOGI(TAG, "SD wifi: creating dir %s if not exists", WIFI_CREDS_DIR);
    if (mkdir(WIFI_CREDS_DIR, 0755) != 0) {
        /* EEXIST is fine */
        if (errno != EEXIST) {
            ESP_LOGW(TAG, "SD wifi: mkdir %s failed: %d", WIFI_CREDS_DIR, errno);
        }
    }
    ESP_LOGI(TAG, "SD wifi: writing creds to %s (ssid='%s')", WIFI_CREDS_FILE, ssid);
    FILE *f = fopen(WIFI_CREDS_FILE, "w");
    if (f == NULL) {
        ESP_LOGW(TAG, "SD wifi: failed to open %s for write", WIFI_CREDS_FILE);
        return;
    }
    fprintf(f, "%s\n", ssid);
    if (password) {
        fprintf(f, "%s\n", password);
    } else {
        fprintf(f, "\n");
    }
    fclose(f);
    ESP_LOGI(TAG, "SD wifi: creds written successfully");
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* SD card: init and log result; used for WiFi creds persistence */
    s_last_ssid[0] = s_last_password[0] = '\0';
    if (esp_sdcard_port_init()) {
        ESP_LOGI(TAG, "SD card: detected and mounted");
    } else {
        ESP_LOGI(TAG, "SD card: not detected or mount failed");
    }

    i2c_bus_init();
    io_expander_init();
    esp_3inch5_display_port_init(&s_io_handle, &s_panel_handle, LCD_BUFFER_SIZE);
    esp_3inch5_touch_port_init(&s_touch_handle, s_i2c_bus, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_DISPLAY_ROTATION);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_3inch5_brightness_port_init();
    esp_3inch5_brightness_port_set(80);
    lv_port_init();

    /* Indian Standard Time (UTC+5:30) for real-time clock display */
    setenv("TZ", "IST-5:30", 1);
    tzset();

    if (lvgl_port_lock(0)) {
        ui_init();
        lvgl_port_unlock();
    }

    /* If we have WiFi creds on SD, connect at boot */
    if (esp_sdcard_port_is_mounted()) {
        char ssid[WIFI_SSID_MAX + 1];
        char pass[WIFI_PASS_MAX + 1];
        if (wifi_creds_read_from_sd(ssid, sizeof(ssid), pass, sizeof(pass))) {
            ESP_LOGI(TAG, "SD wifi: auto-connecting to '%s'", ssid);
            wifi_init_sta();
            if (s_wifi_inited) {
                wifi_config_t wifi_config = {};
                size_t ssid_len = strnlen(ssid, WIFI_SSID_MAX);
                memcpy(wifi_config.sta.ssid, ssid, ssid_len + 1);
                if (pass[0] != '\0') {
                    size_t pass_len = strnlen(pass, WIFI_PASS_MAX);
                    if (pass_len < sizeof(wifi_config.sta.password)) {
                        memcpy(wifi_config.sta.password, pass, pass_len + 1);
                    }
                    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
                } else {
                    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
                }
                esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
                esp_wifi_connect();
                /* Save for writing back to SD on GOT_IP (in case we overwrite with same) */
                strncpy(s_last_ssid, ssid, WIFI_SSID_MAX);
                s_last_ssid[WIFI_SSID_MAX] = '\0';
                strncpy(s_last_password, pass, WIFI_PASS_MAX);
                s_last_password[WIFI_PASS_MAX] = '\0';
            }
        }
    }

    ESP_LOGI(TAG, "Ready. Wallet3 UI.");
}

static void i2c_bus_init(void)
{
    i2c_master_bus_config_t cfg = {};
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.i2c_port = (i2c_port_num_t)I2C_PORT_NUM;
    cfg.scl_io_num = EXAMPLE_PIN_I2C_SCL;
    cfg.sda_io_num = EXAMPLE_PIN_I2C_SDA;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = 1;
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_i2c_bus));
}

static void io_expander_init(void)
{
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(s_i2c_bus, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &s_expander_handle));
    ESP_ERROR_CHECK(esp_io_expander_set_dir(s_expander_handle, IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(s_expander_handle, IO_EXPANDER_PIN_NUM_1, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(s_expander_handle, IO_EXPANDER_PIN_NUM_1, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void lv_port_init(void)
{
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);

    lvgl_port_display_cfg_t display_cfg = {};
    display_cfg.io_handle = s_io_handle;
    display_cfg.panel_handle = s_panel_handle;
    display_cfg.control_handle = NULL;
    display_cfg.buffer_size = LCD_BUFFER_SIZE;
    display_cfg.double_buffer = true;
    display_cfg.trans_size = 0;
    display_cfg.hres = EXAMPLE_LCD_H_RES;
    display_cfg.vres = EXAMPLE_LCD_V_RES;
    display_cfg.monochrome = false;
    display_cfg.rotation = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 };
#if LVGL_VERSION_MAJOR >= 9
    display_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    display_cfg.flags.buff_dma = 0;
    display_cfg.flags.buff_spiram = 1;
    display_cfg.flags.sw_rotate = 0;
    display_cfg.flags.full_refresh = 0;
    display_cfg.flags.direct_mode = 0;
    display_cfg.flags.swap_bytes = 1;
#else
    display_cfg.flags.buff_dma = 0;
    display_cfg.flags.buff_spiram = 1;
    display_cfg.flags.sw_rotate = 0;
    display_cfg.flags.full_refresh = 0;
    display_cfg.flags.direct_mode = 0;
#endif
    s_lvgl_disp = lvgl_port_add_disp(&display_cfg);
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = s_lvgl_disp,
        .handle = s_touch_handle,
        .scale = { .x = 1.0f, .y = 1.0f },
    };
    lvgl_port_add_touch(&touch_cfg);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE && s_wifi_scan_done) {
        xSemaphoreGive(s_wifi_scan_done);
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (lvgl_port_lock(pdMS_TO_TICKS(100))) {
            ui_Screen1_set_wifi_connected(0);
            ui_Screen2_set_wifi_connected(0);
            ui_Screen3_set_wifi_connected(0);
            ui_wifi_password_screen_on_connect_result(0);  /* failed if connect was pending */
            lvgl_port_unlock();
        }
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    (void)event_data;
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        /* Persist WiFi creds to SD on successful connection */
        if (s_last_ssid[0] != '\0') {
            wifi_creds_write_to_sd(s_last_ssid, s_last_password);
        }
        start_sntp_once();
        xTaskCreate(price_fetch_task, "price_fetch", 4096, NULL, 4, NULL);
        if (lvgl_port_lock(pdMS_TO_TICKS(100))) {
            ui_Screen1_set_wifi_connected(1);
            ui_Screen2_set_wifi_connected(1);
            ui_Screen3_set_wifi_connected(1);
            ui_wifi_password_screen_on_connect_result(1);  /* success */
            lvgl_port_unlock();
        }
    }
}

static void wifi_init_sta(void)
{
    if (s_wifi_inited) {
        return;
    }
    s_wifi_scan_done = xSemaphoreCreateBinary();
    if (!s_wifi_scan_done) {
        ESP_LOGE(TAG, "wifi scan sem create failed");
        return;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_wifi_inited = true;
}

static void wifi_scan_task(void *arg)
{
    wifi_init_sta();
    if (!s_wifi_inited) {
        vTaskDelete(NULL);
        return;
    }
    esp_wifi_start();
    vTaskDelay(pdMS_TO_TICKS(200));
    if (lvgl_port_lock(pdMS_TO_TICKS(2000))) {
        ui_setting_screen_show_scanning();
        lvgl_port_unlock();
    }
    esp_err_t err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
            ui_setting_screen_show_networks(NULL, NULL, 0);
            lvgl_port_unlock();
        }
        vTaskDelete(NULL);
        return;
    }
    if (xSemaphoreTake(s_wifi_scan_done, pdMS_TO_TICKS(10000)) != pdTRUE) {
        esp_wifi_scan_stop();
        if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
            ui_setting_screen_show_networks(NULL, NULL, 0);
            lvgl_port_unlock();
        }
        vTaskDelete(NULL);
        return;
    }
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > 32) {
        ap_count = 32;
    }
    wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_list) {
        ap_count = 0;
    } else {
        uint16_t got = ap_count;
        esp_wifi_scan_get_ap_records(&got, ap_list);
        ap_count = got;
    }
    if (lvgl_port_lock(pdMS_TO_TICKS(2000))) {
        if (ap_count && ap_list) {
            static const char *ssids[32];
            static int8_t rssis[32];
            for (uint16_t i = 0; i < ap_count; i++) {
                ap_list[i].ssid[32 - 1] = '\0';
                ssids[i] = (const char *)ap_list[i].ssid;
                rssis[i] = ap_list[i].rssi;
            }
            ui_setting_screen_show_networks(ssids, rssis, ap_count);
        } else {
            ui_setting_screen_show_networks(NULL, NULL, 0);
        }
        lvgl_port_unlock();
    }
    if (ap_list) {
        free(ap_list);
    }
    vTaskDelete(NULL);
}

extern "C" void app_wifi_scan_start(void)
{
    xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL);
}

extern "C" void app_wifi_scan_stop(void)
{
    if (s_wifi_inited) {
        esp_wifi_stop();
    }
    if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
        ui_setting_screen_clear_networks();
        lvgl_port_unlock();
    }
}

static void start_sntp_once(void)
{
    if (s_sntp_started) {
        return;
    }
    s_sntp_started = true;
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = NULL;
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SNTP started (pool.ntp.org)");
    } else {
        ESP_LOGW(TAG, "SNTP init failed: %s", esp_err_to_name(err));
        s_sntp_started = false;
    }
}

#define PRICE_BUF_SIZE 512
#define PRICE_STR_SIZE 24

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} price_fetch_ctx_t;

static esp_err_t price_http_event_handler(esp_http_client_event_t *evt)
{
    price_fetch_ctx_t *ctx = (price_fetch_ctx_t *)evt->user_data;
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (ctx && ctx->buf && evt->data_len > 0) {
            size_t need = ctx->len + evt->data_len + 1;
            if (need <= ctx->cap) {
                memcpy(ctx->buf + ctx->len, evt->data, evt->data_len);
                ctx->len += evt->data_len;
                ctx->buf[ctx->len] = '\0';
            }
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

/* Parse "id":{"usd":N.N} from JSON; only look for "usd" inside that id's object (so empty {} gives -1) */
static float parse_usd_price(const char *json, const char *id)
{
    const char *p = strstr(json, id);
    if (!p) return -1.f;
    p = strchr(p, '{');  /* start of this id's value object */
    if (!p) return -1.f;
    const char *end = strchr(p + 1, '}');  /* end of this object */
    if (!end) return -1.f;
    p = strstr(p, "\"usd\"");  /* "usd" must be inside this {} */
    if (!p || p >= end) return -1.f;
    p += 5;
    while (*p && (*p == ' ' || *p == ':')) p++;
    return (float)atof(p);
}

static void price_fetch_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(2000));  /* let network settle */

    char *buf = (char *)malloc(PRICE_BUF_SIZE);
    if (!buf) {
        vTaskDelete(NULL);
        return;
    }

    price_fetch_ctx_t ctx = { .buf = buf, .len = 0, .cap = PRICE_BUF_SIZE };

    const char *price_url = "https://api.coingecko.com/api/v3/simple/price?ids=arweave,ao&vs_currencies=usd";
    ESP_LOGI(TAG, "price fetch request: %s", price_url);

    esp_http_client_config_t cfg = {};
    cfg.url = price_url;
    cfg.timeout_ms = 10000;
    cfg.event_handler = price_http_event_handler;
    cfg.user_data = &ctx;
#if defined(CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) && CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
#else
    cfg.skip_cert_common_name_check = true;  /* requires CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY in sdkconfig */
#endif
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        free(buf);
        vTaskDelete(NULL);
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && ctx.len > 0) {
        buf[ctx.len] = '\0';
        /* Log response snippet (first 160 chars) for debugging */
        size_t log_len = (ctx.len < 160u) ? ctx.len : 160u;
        ESP_LOGI(TAG, "price response (%u bytes): %.*s", (unsigned)ctx.len, (int)log_len, buf);

        float ar_usd = parse_usd_price(buf, "\"arweave\"");
        float ao_usd = parse_usd_price(buf, "\"ao\"");
        ESP_LOGI(TAG, "parsed prices: AR=%.2f AO=%.2f", (double)ar_usd, (double)ao_usd);

        char ao_str[PRICE_STR_SIZE];
        char ar_str[PRICE_STR_SIZE];
        ao_str[0] = ar_str[0] = '\0';
        if (ao_usd >= 0.f) {
            snprintf(ao_str, sizeof(ao_str), "AO$%.2f", (double)ao_usd);
        }
        if (ar_usd >= 0.f) {
            snprintf(ar_str, sizeof(ar_str), "AR$%.2f", (double)ar_usd);
        }
        ESP_LOGI(TAG, "UI strings: ao='%s' ar='%s'", ao_str, ar_str);

        if (ao_str[0] || ar_str[0]) {
            if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
                ui_Screen1_set_prices(ao_str[0] ? ao_str : NULL, ar_str[0] ? ar_str : NULL);
                lvgl_port_unlock();
            }
        }
    } else {
        ESP_LOGW(TAG, "price fetch failed or empty (err=%s, len=%u)", esp_err_to_name(err), (unsigned)ctx.len);
    }

    free(buf);
    vTaskDelete(NULL);
}

extern "C" void app_wifi_connect(const char *ssid, const char *password)
{
    wifi_init_sta();
    if (!s_wifi_inited || !ssid) {
        return;
    }
    /* Save for writing to SD on GOT_IP */
    strncpy(s_last_ssid, ssid, WIFI_SSID_MAX);
    s_last_ssid[WIFI_SSID_MAX] = '\0';
    if (password) {
        strncpy(s_last_password, password, WIFI_PASS_MAX);
        s_last_password[WIFI_PASS_MAX] = '\0';
    } else {
        s_last_password[0] = '\0';
    }
    ESP_LOGI(TAG, "wifi: connecting to '%s' (creds will be saved to SD on success)", ssid);

    wifi_config_t wifi_config = {};
    size_t ssid_len = strnlen(ssid, sizeof(wifi_config.sta.ssid));
    if (ssid_len >= sizeof(wifi_config.sta.ssid)) {
        return;
    }
    memcpy(wifi_config.sta.ssid, ssid, ssid_len + 1);
    if (password) {
        size_t pass_len = strnlen(password, sizeof(wifi_config.sta.password));
        if (pass_len < sizeof(wifi_config.sta.password)) {
            memcpy(wifi_config.sta.password, password, pass_len + 1);
        }
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();
}
