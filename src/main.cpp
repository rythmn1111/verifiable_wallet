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
#include "freertos/queue.h"
#include "driver/i2c_master.h"

#include "boot_mode.h"
#include "esp_camera.h"
#include "esp_camera_port.h"
#include "qr_decoder.h"
#include "esp_heap_caps.h"
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
#include "screens/ui_sign_tx_password_screen.h"
#include "wallet_sd.h"
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
/** Scanner mode: display + camera + QR; on done saves result and reboots. Never returns. */
static void run_scanner_mode(void);

/** Wallet path: after boot, if scanner just finished with success, hash is here until we show password screen. */
static char s_pending_sign_tx_hash[SIGN_TX_HASH_MAX];

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

    /* One firmware, two modes: scanner runs minimal UI then reboots; wallet runs full UI. */
    if (boot_mode_is_scanner()) {
        boot_mode_set_wallet();
        run_scanner_mode();
        /* never returns */
    }

    /* Wallet path: read sign_tx result from scanner (if it just ran with success). */
    s_pending_sign_tx_hash[0] = '\0';
    {
        size_t pending_len = 0;
        (void)sign_tx_get_result(s_pending_sign_tx_hash, sizeof(s_pending_sign_tx_hash), &pending_len);
    }

    /* SD card: init and log result; used for WiFi creds and temp_sig (pending hash to sign). */
    s_last_ssid[0] = s_last_password[0] = '\0';
    if (esp_sdcard_port_init()) {
        ESP_LOGI(TAG, "SD card: detected and mounted");
    } else {
        ESP_LOGI(TAG, "SD card: not detected or mount failed");
    }

    /* If scanner just finished with success, save hash to SD temp_sig so user can tap Sign Tx to sign. */
    if (s_pending_sign_tx_hash[0] != '\0' && esp_sdcard_port_is_mounted()) {
        wallet_sd_temp_sig_write(s_pending_sign_tx_hash);
        sign_tx_clear_result();
    }
    s_pending_sign_tx_hash[0] = '\0';

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

/* --- Scanner mode (one firmware, two modes): display + camera + QR; on done reboot. --- */
#define SCAN_PREVIEW_W      240
#define SCAN_PREVIEW_H      240
#define SCAN_PREVIEW_ROW    (SCAN_PREVIEW_W * 2)
#define SCAN_QUEUE_LEN      2
#define SCAN_TASK_STACK     (24 * 1024)
#define SCAN_HASH_BUF_SIZE  256
/* Max frame size for packed copy (decoder expects contiguous rows). */
#define SCAN_DECODE_MAX_W   320
#define SCAN_DECODE_MAX_H   240
#define SCAN_DECODE_BUF_SIZE ((size_t)SCAN_DECODE_MAX_W * SCAN_DECODE_MAX_H * 2u)

static QueueHandle_t s_scan_queue = NULL;
static uint8_t *s_scan_decode_buf = NULL;  /* packed RGB565 for QR decode (handles camera stride) */
static char s_scan_decoded_hash[SCAN_HASH_BUF_SIZE];
/** Double-buffer: task writes to buf[write_idx], then swaps; timer reads from buf[1-write_idx] to avoid tearing. */
static uint8_t *s_scan_preview_buf[2] = { NULL, NULL };
static volatile int s_scan_preview_write_idx = 0;
static uint8_t *s_scan_canvas_buf = NULL;
static lv_obj_t *s_scan_canvas = NULL;
static volatile bool s_scan_preview_dirty = false;
static lv_timer_t *s_scan_poll_timer = NULL;
static TaskHandle_t s_scan_task_handle = NULL;
static int s_scanner_cam_ok = -1;

static void scanner_task(void *pv)
{
    (void)pv;
    if (s_scanner_cam_ok != 0) {
        uint32_t msg = 1;
        xQueueSend(s_scan_queue, &msg, 0);
        vTaskDelete(NULL);
        return;
    }
    char decode_buf[SCAN_HASH_BUF_SIZE];
    while (1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        uint8_t *dst = s_scan_preview_buf[s_scan_preview_write_idx];
        if (dst && fb->width > 0 && fb->height > 0) {
            int cw = (fb->width <= SCAN_PREVIEW_W) ? fb->width : SCAN_PREVIEW_W;
            int ch = (fb->height <= SCAN_PREVIEW_H) ? fb->height : SCAN_PREVIEW_H;
            size_t src_stride = (size_t)fb->width * 2u;
            /* Copy with byte-swap per pixel so preview matches display (LCD expects swap_bytes). */
            for (int y = 0; y < ch; y++) {
                const uint8_t *src_row = fb->buf + (size_t)y * src_stride;
                uint8_t *dst_row = dst + (size_t)y * SCAN_PREVIEW_ROW;
                for (int x = 0; x < cw; x++) {
                    uint16_t px = (uint16_t)src_row[x * 2] | ((uint16_t)src_row[x * 2 + 1] << 8);
                    px = (px >> 8) | (px << 8);
                    dst_row[x * 2] = (uint8_t)(px & 0xff);
                    dst_row[x * 2 + 1] = (uint8_t)(px >> 8);
                }
            }
            s_scan_preview_write_idx = 1 - s_scan_preview_write_idx;
            s_scan_preview_dirty = true;
        }
        /* Decoder expects contiguous rows; camera DMA may use a different stride. Copy to packed buffer. */
        bool ok = false;
        if (s_scan_decode_buf && fb->width > 0 && fb->height > 0 &&
            (size_t)fb->width <= SCAN_DECODE_MAX_W && (size_t)fb->height <= SCAN_DECODE_MAX_H) {
            size_t src_stride = ((size_t)fb->width * 2u);
            if (fb->len > 0 && (size_t)fb->height > 0) {
                size_t stride_from_len = fb->len / (size_t)fb->height;
                if (stride_from_len >= src_stride)
                    src_stride = stride_from_len;
            }
            size_t row_bytes = (size_t)fb->width * 2u;
            for (int y = 0; y < fb->height; y++)
                memcpy(s_scan_decode_buf + (size_t)y * row_bytes,
                       fb->buf + (size_t)y * src_stride, row_bytes);
            ok = qr_decoder_decode_rgb565(s_scan_decode_buf, fb->width, fb->height, decode_buf, sizeof(decode_buf));
        } else {
            ok = qr_decoder_decode_rgb565(fb->buf, fb->width, fb->height, decode_buf, sizeof(decode_buf));
        }
        esp_camera_fb_return(fb);
        if (ok && decode_buf[0] != '\0') {
            size_t len = strnlen(decode_buf, sizeof(s_scan_decoded_hash) - 1);
            if (len > 0) {
                memcpy(s_scan_decoded_hash, decode_buf, len + 1);
                uint32_t msg = 0;
                xQueueSend(s_scan_queue, &msg, 0);
            }
            vTaskDelete(NULL);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

static void scanner_poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_scan_queue) return;
    /* Copy from the buffer we're not currently writing to (last complete frame). */
    uint8_t *src = s_scan_preview_buf[1 - s_scan_preview_write_idx];
    if (s_scan_preview_dirty && s_scan_canvas_buf && src && s_scan_canvas) {
        for (int y = 0; y < SCAN_PREVIEW_H; y++)
            memcpy(s_scan_canvas_buf + (size_t)y * SCAN_PREVIEW_ROW, src + (size_t)y * SCAN_PREVIEW_ROW, SCAN_PREVIEW_ROW);
        s_scan_preview_dirty = false;
        lv_obj_invalidate(s_scan_canvas);
    }
    uint32_t msg;
    if (xQueueReceive(s_scan_queue, &msg, 0) != pdPASS) return;
    if (s_scan_poll_timer) { lv_timer_del(s_scan_poll_timer); s_scan_poll_timer = NULL; }
    s_scan_task_handle = NULL;
    if (msg == 1) {
        sign_tx_save_cancelled();
        esp_restart();
    }
    sign_tx_save_success(s_scan_decoded_hash);
    esp_restart();
}

static void scanner_cancel_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    sign_tx_save_cancelled();
    esp_restart();
}

static void run_scanner_mode(void)
{
    ESP_LOGI(TAG, "Scanner mode: init display + camera + QR");
    i2c_bus_init();
    io_expander_init();
    esp_3inch5_display_port_init(&s_io_handle, &s_panel_handle, LCD_BUFFER_SIZE);
    esp_3inch5_touch_port_init(&s_touch_handle, s_i2c_bus, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_DISPLAY_ROTATION);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_3inch5_brightness_port_init();
    esp_3inch5_brightness_port_set(80);
    lv_port_init();

    vTaskDelay(pdMS_TO_TICKS(200));
    s_scanner_cam_ok = esp_camera_port_init_c(0);

    s_scan_queue = xQueueCreate(SCAN_QUEUE_LEN, sizeof(uint32_t));
    s_scan_decoded_hash[0] = '\0';
    s_scan_decode_buf = (uint8_t *)heap_caps_malloc(SCAN_DECODE_BUF_SIZE, MALLOC_CAP_SPIRAM);

    lv_obj_t *scan_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(scan_screen, LV_OBJ_FLAG_SCROLLABLE);
    const lv_style_selector_t sel = (lv_style_selector_t)((int)LV_PART_MAIN | (int)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(scan_screen, lv_color_hex(0x88BF6C), sel);
    lv_obj_set_style_bg_opa(scan_screen, LV_OPA_COVER, sel);

    lv_obj_t *scan_title = lv_label_create(scan_screen);
    lv_obj_set_width(scan_title, 280);
    lv_obj_set_align(scan_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(scan_title, 12);
    lv_label_set_text(scan_title, "Sign Tx - Scan hash QR");
    lv_obj_set_style_text_align(scan_title, LV_TEXT_ALIGN_CENTER, sel);

    size_t canvas_size = (size_t)SCAN_PREVIEW_ROW * SCAN_PREVIEW_H;
    s_scan_preview_buf[0] = (uint8_t *)heap_caps_malloc((size_t)SCAN_PREVIEW_ROW * SCAN_PREVIEW_H, MALLOC_CAP_SPIRAM);
    s_scan_preview_buf[1] = (uint8_t *)heap_caps_malloc((size_t)SCAN_PREVIEW_ROW * SCAN_PREVIEW_H, MALLOC_CAP_SPIRAM);
    s_scan_canvas_buf = (uint8_t *)heap_caps_malloc(canvas_size, MALLOC_CAP_SPIRAM);
    if (s_scan_canvas_buf && s_scan_preview_buf[0] && s_scan_preview_buf[1]) {
        for (int y = 0; y < SCAN_PREVIEW_H; y++) {
            uint16_t *row = (uint16_t *)(s_scan_canvas_buf + (size_t)y * SCAN_PREVIEW_ROW);
            for (int x = 0; x < SCAN_PREVIEW_W; x++) row[x] = 0x3186;
        }
        s_scan_canvas = lv_canvas_create(scan_screen);
        lv_canvas_set_buffer(s_scan_canvas, s_scan_canvas_buf, SCAN_PREVIEW_W, SCAN_PREVIEW_H, LV_COLOR_FORMAT_RGB565);
        lv_obj_set_width(s_scan_canvas, SCAN_PREVIEW_W);
        lv_obj_set_height(s_scan_canvas, SCAN_PREVIEW_H);
        lv_obj_set_align(s_scan_canvas, LV_ALIGN_TOP_MID);
        lv_obj_set_y(s_scan_canvas, 36);
    }

    lv_obj_t *scan_cancel_btn = lv_btn_create(scan_screen);
    lv_obj_set_width(scan_cancel_btn, 120);
    lv_obj_set_height(scan_cancel_btn, 48);
    lv_obj_set_align(scan_cancel_btn, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(scan_cancel_btn, -24);
    lv_obj_t *cancel_label = lv_label_create(scan_cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);

    lv_obj_add_event_cb(scan_cancel_btn, scanner_cancel_cb, LV_EVENT_CLICKED, NULL);

    s_scan_poll_timer = lv_timer_create(scanner_poll_timer_cb, 200, NULL);
    lv_scr_load(scan_screen);
    xTaskCreatePinnedToCore(scanner_task, "scan", SCAN_TASK_STACK, NULL, 3, &s_scan_task_handle, 1);

    for (;;)
        vTaskDelay(pdMS_TO_TICKS(1000));
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
            ui_new_note_for_word_count_set_wifi_connected(0);
            ui_word_count_set_wifi_connected(0);
            ui_last_word_count_set_wifi_connected(0);
            ui_note_for_password_set_wifi_connected(0);
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
            ui_new_note_for_word_count_set_wifi_connected(1);
            ui_word_count_set_wifi_connected(1);
            ui_last_word_count_set_wifi_connected(1);
            ui_note_for_password_set_wifi_connected(1);
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
                vTaskDelay(pdMS_TO_TICKS(20)); /* yield so LVGL/IDLE can run and watchdog is fed */
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
