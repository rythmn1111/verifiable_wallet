#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#include "esp_io_expander_tca9554.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "esp_3inch5_lcd_port.h"
#include "ui.h"

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

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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
