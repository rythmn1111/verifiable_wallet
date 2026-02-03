#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "esp_io_expander_tca9554.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "iot_button.h"
#include "button_gpio.h"

#include "esp_camera.h"
#include "esp_camera_port.h"
#include "esp_3inch5_lcd_port.h"
#include "qr_decoder.h"

#define EXAMPLE_PIN_I2C_SDA   GPIO_NUM_8
#define EXAMPLE_PIN_I2C_SCL   GPIO_NUM_7
#define EXAMPLE_PIN_BOOT      GPIO_NUM_0
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
#define QR_RESULT_MAX_LEN 256

static const char *TAG = "qr_scan";

typedef enum {
    STATE_HOME,
    STATE_SCANNING,
    STATE_RESULT,
} app_state_t;

static volatile app_state_t s_state = STATE_HOME;
static char s_qr_result[QR_RESULT_MAX_LEN];
static SemaphoreHandle_t s_result_mux = NULL;
static bool s_camera_inited = false;

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_io_expander_handle_t s_expander_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static lv_display_t *s_lvgl_disp = NULL;

static lv_obj_t *s_screen_home = NULL;
static lv_obj_t *s_screen_scan = NULL;
static lv_obj_t *s_screen_result = NULL;
static lv_obj_t *s_img_camera = NULL;
static lv_obj_t *s_label_result_text = NULL;
static lv_obj_t *s_label_rescan = NULL;

/* Camera uses FRAMESIZE_HVGA (480x320) with standard esp32-camera */
#define CAM_W  480
#define CAM_H  320

/* LVGL 8 lv_img_header_t order: cf, always_zero, reserved, w, h */
static lv_img_dsc_t s_cam_img_dsc = {
    .header = {
        .cf = LV_IMG_CF_TRUE_COLOR,
        .always_zero = 0,
        .reserved = 0,
        .w = CAM_W,
        .h = CAM_H,
    },
    .data_size = CAM_W * CAM_H * 2,
    .data = NULL,
};

static void i2c_bus_init(void);
static void io_expander_init(void);
static void lv_port_init(void);
static void ui_create_screens(void);
static void ui_show_home(void);
static void ui_show_scanning(void);
static void ui_show_result(const char *text);

static void scan_task(void *arg);

static void boot_btn_cb(void *btn_handle, void *usr_data)
{
    (void)btn_handle;
    (void)usr_data;
    app_state_t st = s_state;
    if (st == STATE_HOME) {
        s_state = STATE_SCANNING;
        ui_show_scanning();
    } else if (st == STATE_RESULT) {
        s_state = STATE_SCANNING;
        s_qr_result[0] = '\0';
        ui_show_scanning();
    }
    /* Camera init is done in scan_task to avoid stack overflow / watchdog in callback */
}

static void scan_task(void *arg)
{
    (void)arg;
    camera_fb_t *fb = NULL;
    while (1) {
        if (s_state != STATE_SCANNING) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (!s_camera_inited) {
            esp_camera_port_init(I2C_PORT_NUM);
            s_camera_inited = true;
        }
        fb = esp_camera_fb_get();
        if (fb == NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        int dec_w = (fb->width > 0 && fb->height > 0) ? (int)fb->width : CAM_W;
        int dec_h = (fb->width > 0 && fb->height > 0) ? (int)fb->height : CAM_H;
        size_t expected_len = (size_t)(dec_w * dec_h * 2);
        if (fb->len >= expected_len) {
            if (lvgl_port_lock(0)) {
                s_cam_img_dsc.data = fb->buf;
                lv_img_set_src(s_img_camera, &s_cam_img_dsc);
                lvgl_port_unlock();
            }
            char decoded[QR_RESULT_MAX_LEN];
            if (qr_decoder_decode_rgb565(fb->buf, dec_w, dec_h, decoded, sizeof(decoded))) {
                xSemaphoreTake(s_result_mux, portMAX_DELAY);
                strncpy(s_qr_result, decoded, QR_RESULT_MAX_LEN - 1);
                s_qr_result[QR_RESULT_MAX_LEN - 1] = '\0';
                xSemaphoreGive(s_result_mux);
                s_state = STATE_RESULT;
                if (lvgl_port_lock(0)) {
                    ui_show_result(s_qr_result);
                    lvgl_port_unlock();
                }
            }
        }
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void ui_create_screens(void)
{
    s_screen_home = lv_scr_act();
    lv_obj_set_style_bg_color(s_screen_home, lv_color_hex(0x1a1a2e), 0);

    lv_obj_t *title = lv_label_create(s_screen_home);
    lv_label_set_text(title, "Scan QR");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xeeeeff), 0);
    lv_obj_center(title);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *sub = lv_label_create(s_screen_home);
    lv_label_set_text(sub, "Press BOOT to start camera");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xaaaaaa), 0);
    lv_obj_center(sub);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 20);

    s_screen_scan = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen_scan, lv_color_black(), 0);
    s_img_camera = lv_img_create(s_screen_scan);
    lv_obj_set_size(s_img_camera, CAM_W, CAM_H);
    lv_obj_align(s_img_camera, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *scan_label = lv_label_create(s_screen_scan);
    lv_label_set_text(scan_label, "Scanning...");
    lv_obj_set_style_text_color(scan_label, lv_color_white(), 0);
    lv_obj_align(scan_label, LV_ALIGN_TOP_MID, 0, 10);

    s_screen_result = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen_result, lv_color_hex(0x1a1a2e), 0);
    lv_obj_t *res_title = lv_label_create(s_screen_result);
    lv_label_set_text(res_title, "QR content:");
    lv_obj_set_style_text_font(res_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(res_title, lv_color_white(), 0);
    lv_obj_align(res_title, LV_ALIGN_TOP_LEFT, 10, 10);
    s_label_result_text = lv_label_create(s_screen_result);
    lv_label_set_text(s_label_result_text, "");
    lv_obj_set_style_text_font(s_label_result_text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_label_result_text, lv_color_hex(0xaaffaa), 0);
    lv_obj_set_width(s_label_result_text, EXAMPLE_LCD_H_RES - 20);
    lv_obj_align(s_label_result_text, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_label_set_long_mode(s_label_result_text, LV_LABEL_LONG_WRAP);
    s_label_rescan = lv_label_create(s_screen_result);
    lv_label_set_text(s_label_rescan, "Re-scan? Press BOOT");
    lv_obj_set_style_text_font(s_label_rescan, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_label_rescan, lv_color_hex(0x8888ff), 0);
    lv_obj_align(s_label_rescan, LV_ALIGN_BOTTOM_MID, 0, -30);
}

static void ui_show_home(void)
{
    lv_scr_load_anim(s_screen_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void ui_show_scanning(void)
{
    lv_scr_load_anim(s_screen_scan, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void ui_show_result(const char *text)
{
    lv_label_set_text(s_label_result_text, text ? text : "");
    lv_scr_load_anim(s_screen_result, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_result_mux = xSemaphoreCreateMutex();
    if (s_result_mux == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    i2c_bus_init();
    io_expander_init();
    esp_3inch5_display_port_init(&s_io_handle, &s_panel_handle, LCD_BUFFER_SIZE);
    esp_3inch5_touch_port_init(&s_touch_handle, s_i2c_bus, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_DISPLAY_ROTATION);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_3inch5_brightness_port_init();
    esp_3inch5_brightness_port_set(80);
    lv_port_init();

    if (lvgl_port_lock(0)) {
        ui_create_screens();
        ui_show_home();
        lvgl_port_unlock();
    }

    button_config_t btn_cfg = {};
    button_gpio_config_t gpio_cfg = {};
    gpio_cfg.gpio_num = EXAMPLE_PIN_BOOT;
    gpio_cfg.active_level = 0;
    button_handle_t btn = NULL;
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn) == ESP_OK && btn) {
        iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, boot_btn_cb, NULL);
    }

    /* DISPLAY_SETUP.md: quirc + camera + display need ~24 KB stack to avoid overflow/reboot */
    xTaskCreatePinnedToCore(scan_task, "scan", 24 * 1024, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "Ready. Press BOOT to scan QR.");
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

    lvgl_port_display_cfg_t display_cfg = {
        .io_handle = s_io_handle,
        .panel_handle = s_panel_handle,
        .control_handle = NULL,
        .buffer_size = LCD_BUFFER_SIZE,
        .double_buffer = true,
        .trans_size = 0,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .rotation = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 },
        .flags = { .buff_dma = 0, .buff_spiram = 1, .sw_rotate = 1, .full_refresh = 0, .direct_mode = 0 },
    };
    s_lvgl_disp = lvgl_port_add_disp(&display_cfg);
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = s_lvgl_disp,
        .handle = s_touch_handle,
    };
    lvgl_port_add_touch(&touch_cfg);
}
