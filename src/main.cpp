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
#include "esp_heap_caps.h"

#include "ui.h"
#include "ui_events.h"

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
    STATE_QR_HOME,
    STATE_SCANNING,
    STATE_RESULT,
} qr_state_t;

static volatile qr_state_t s_qr_state = STATE_QR_HOME;
static char s_qr_result[QR_RESULT_MAX_LEN];
static SemaphoreHandle_t s_result_mux = NULL;
static bool s_camera_inited = false;

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_io_expander_handle_t s_expander_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static lv_display_t *s_lvgl_disp = NULL;

/* QR UI on Screen 2 (wallet Screen 2) */
static lv_obj_t *s_qr_home_panel = NULL;
static lv_obj_t *s_qr_scan_panel = NULL;
static lv_obj_t *s_qr_result_panel = NULL;
static lv_obj_t *s_img_camera = NULL;
static lv_obj_t *s_label_result_text = NULL;

#define CAM_W  480
#define CAM_H  320
#define CAM_FB_BYTES  (CAM_W * CAM_H * 2)

/* Own buffer for camera frame so we don't return fb to driver while LVGL is drawing */
static uint8_t *s_cam_copy = NULL;

/* LVGL 9 image descriptor for camera frame (data pointer set at runtime) */
static lv_image_dsc_t s_cam_img_dsc = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .flags = 0,
        .w = (uint32_t)CAM_W,
        .h = (uint32_t)CAM_H,
        .stride = (uint32_t)(CAM_W * 2),
        .reserved_2 = 0,
    },
    .data_size = (uint32_t)(CAM_W * CAM_H * 2),
    .data = NULL,
    .reserved = NULL,
};

static void i2c_bus_init(void);
static void io_expander_init(void);
static void lv_port_init(void);
static void scan_task(void *arg);
static void app_screen2_qr_ui_init(lv_obj_t *screen2);
static void qr_ui_show_home(void);
static void qr_ui_show_scanning(void);
static void qr_ui_show_result(const char *text);

static void boot_btn_cb(void *btn_handle, void *usr_data)
{
    (void)btn_handle;
    (void)usr_data;
    /* Only react when user is on Screen 2 (QR scanner) */
    if (lv_screen_active() != ui_Screen2) {
        return;
    }
    qr_state_t st = s_qr_state;
    if (st == STATE_QR_HOME) {
        s_qr_state = STATE_SCANNING;
        qr_ui_show_scanning();
    } else if (st == STATE_RESULT) {
        s_qr_state = STATE_SCANNING;
        s_qr_result[0] = '\0';
        qr_ui_show_scanning();
    }
}

static void scan_task(void *arg)
{
    (void)arg;
    camera_fb_t *fb = NULL;
    while (1) {
        if (s_qr_state != STATE_SCANNING) {
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
        if (fb->format != PIXFORMAT_RGB565) {
            esp_camera_fb_return(fb);
            continue;
        }
        int dec_w = (fb->width > 0 && fb->height > 0) ? (int)fb->width : CAM_W;
        int dec_h = (fb->width > 0 && fb->height > 0) ? (int)fb->height : CAM_H;
        size_t expected_len = (size_t)(dec_w * dec_h * 2);
        if (fb->len < expected_len) {
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        if (s_cam_copy == NULL) {
            s_cam_copy = (uint8_t *)heap_caps_malloc(CAM_FB_BYTES, MALLOC_CAP_SPIRAM);
        }
        if (s_cam_copy != NULL && lvgl_port_lock(0)) {
            memcpy(s_cam_copy, fb->buf, expected_len);
            /* Camera often outputs big-endian RGB565; swap bytes for LVGL display */
            for (size_t i = 0; i < expected_len; i += 2) {
                uint8_t t = s_cam_copy[i];
                s_cam_copy[i] = s_cam_copy[i + 1];
                s_cam_copy[i + 1] = t;
            }
            s_cam_img_dsc.data = s_cam_copy;
            s_cam_img_dsc.header.w = (uint32_t)dec_w;
            s_cam_img_dsc.header.h = (uint32_t)dec_h;
            s_cam_img_dsc.header.stride = (uint32_t)(dec_w * 2);
            s_cam_img_dsc.data_size = expected_len;
            if (s_img_camera) {
                lv_image_set_src(s_img_camera, &s_cam_img_dsc);
            }
            lvgl_port_unlock();
        }
        char decoded[QR_RESULT_MAX_LEN];
        if (qr_decoder_decode_rgb565(fb->buf, dec_w, dec_h, decoded, sizeof(decoded))) {
            xSemaphoreTake(s_result_mux, portMAX_DELAY);
            strncpy(s_qr_result, decoded, QR_RESULT_MAX_LEN - 1);
            s_qr_result[QR_RESULT_MAX_LEN - 1] = '\0';
            xSemaphoreGive(s_result_mux);
            s_qr_state = STATE_RESULT;
            if (lvgl_port_lock(0)) {
                qr_ui_show_result(s_qr_result);
                lvgl_port_unlock();
            }
        }
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void app_screen2_qr_ui_init(lv_obj_t *screen2)
{
    if (screen2 == NULL) return;

    lv_obj_set_style_bg_color(screen2, lv_color_hex(0x1a1a2e), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

    /* Panel: "Press BOOT to start camera" (QR home) */
    s_qr_home_panel = lv_obj_create(screen2);
    lv_obj_set_size(s_qr_home_panel, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_align(s_qr_home_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_qr_home_panel, lv_color_hex(0x1a1a2e), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_remove_flag(s_qr_home_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_qr_home_panel);
    lv_label_set_text(title, "Scan QR");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(title, lv_color_hex(0xeeeeff), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_center(title);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *sub = lv_label_create(s_qr_home_panel);
    lv_label_set_text(sub, "Press BOOT to start camera");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(sub, lv_color_hex(0xaaaaaa), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_center(sub);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 20);

    /* Panel: camera + "Scanning..." */
    s_qr_scan_panel = lv_obj_create(screen2);
    lv_obj_set_size(s_qr_scan_panel, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_align(s_qr_scan_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_qr_scan_panel, lv_color_black(), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_remove_flag(s_qr_scan_panel, LV_OBJ_FLAG_SCROLLABLE);

    s_img_camera = lv_image_create(s_qr_scan_panel);
    lv_obj_set_size(s_img_camera, CAM_W, CAM_H);
    lv_obj_align(s_img_camera, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *scan_label = lv_label_create(s_qr_scan_panel);
    lv_label_set_text(scan_label, "Scanning...");
    lv_obj_set_style_text_color(scan_label, lv_color_white(), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_align(scan_label, LV_ALIGN_TOP_MID, 0, 10);

    /* Panel: result text + "Re-scan? Press BOOT" */
    s_qr_result_panel = lv_obj_create(screen2);
    lv_obj_set_size(s_qr_result_panel, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_align(s_qr_result_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_qr_result_panel, lv_color_hex(0x1a1a2e), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_remove_flag(s_qr_result_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *res_title = lv_label_create(s_qr_result_panel);
    lv_label_set_text(res_title, "QR content:");
    lv_obj_set_style_text_font(res_title, &lv_font_montserrat_14, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(res_title, lv_color_white(), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_align(res_title, LV_ALIGN_TOP_LEFT, 10, 10);

    s_label_result_text = lv_label_create(s_qr_result_panel);
    lv_label_set_text(s_label_result_text, "");
    lv_obj_set_style_text_font(s_label_result_text, &lv_font_montserrat_14, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(s_label_result_text, lv_color_hex(0xaaffaa), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_width(s_label_result_text, EXAMPLE_LCD_H_RES - 20);
    lv_obj_align(s_label_result_text, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_label_set_long_mode(s_label_result_text, LV_LABEL_LONG_WRAP);

    lv_obj_t *rescan_label = lv_label_create(s_qr_result_panel);
    lv_label_set_text(rescan_label, "Re-scan? Press BOOT");
    lv_obj_set_style_text_font(rescan_label, &lv_font_montserrat_14, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(rescan_label, lv_color_hex(0x8888ff), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_align(rescan_label, LV_ALIGN_BOTTOM_MID, 0, -30);

    /* Show home panel by default */
    qr_ui_show_home();
}

static void qr_ui_show_home(void)
{
    if (s_qr_home_panel) lv_obj_remove_flag(s_qr_home_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_scan_panel) lv_obj_add_flag(s_qr_scan_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_result_panel) lv_obj_add_flag(s_qr_result_panel, LV_OBJ_FLAG_HIDDEN);
}

static void qr_ui_show_scanning(void)
{
    if (s_qr_home_panel) lv_obj_add_flag(s_qr_home_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_scan_panel) lv_obj_remove_flag(s_qr_scan_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_result_panel) lv_obj_add_flag(s_qr_result_panel, LV_OBJ_FLAG_HIDDEN);
}

static void qr_ui_show_result(const char *text)
{
    if (s_qr_home_panel) lv_obj_add_flag(s_qr_home_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_scan_panel) lv_obj_add_flag(s_qr_scan_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_qr_result_panel) lv_obj_remove_flag(s_qr_result_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_label_result_text) lv_label_set_text(s_label_result_text, text ? text : "");
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

    app_screen2_init_cb = app_screen2_qr_ui_init;

    if (lvgl_port_lock(0)) {
        ui_init();
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

    xTaskCreatePinnedToCore(scan_task, "scan", 24 * 1024, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "Ready. Swipe right for QR scanner, then Press BOOT to scan.");
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
    /* mirror_x=1 so text/images are not reversed (board default). sw_rotate=0 so panel mirror is actually applied! */
    display_cfg.rotation = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 };
#if LVGL_VERSION_MAJOR >= 9
    display_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    display_cfg.flags.buff_dma = 0;
    display_cfg.flags.buff_spiram = 1;
    display_cfg.flags.sw_rotate = 0;  /* must be 0 or mirror_x/mirror_y are never applied to panel */
    display_cfg.flags.full_refresh = 0;
    display_cfg.flags.direct_mode = 0;
    /* Panel is BGR; swap bytes so LVGL RGB565 is sent as BGR565 (fixes violet/wrong colors) */
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
