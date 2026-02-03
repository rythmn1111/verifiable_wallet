/**
 * ESP32-S3-Touch-LCD-3.5 (Waveshare) pin definitions
 * Matches board_examples/05_lvgl_axp2101 components/esp_port/esp_3inch5_lcd_port.cpp
 */

#pragma once

#include "driver/gpio.h"

/* LCD (ST7796, SPI2) */
#define BOARD_LCD_SPI_HOST        SPI2_HOST
#define BOARD_LCD_PIN_MISO       GPIO_NUM_NC
#define BOARD_LCD_PIN_MOSI       GPIO_NUM_1
#define BOARD_LCD_PIN_SCLK       GPIO_NUM_5
#define BOARD_LCD_PIN_CS         GPIO_NUM_NC
#define BOARD_LCD_PIN_DC         GPIO_NUM_3
#define BOARD_LCD_PIN_RST        GPIO_NUM_NC
#define BOARD_LCD_PIN_BL         GPIO_NUM_6

/* Touch (FT6336, I2C) - header SCL=GPIO7, SDA=GPIO8 */
#define BOARD_TOUCH_I2C_SCL      GPIO_NUM_7
#define BOARD_TOUCH_I2C_SDA      GPIO_NUM_8
#define BOARD_TOUCH_PIN_INT      GPIO_NUM_NC
#define BOARD_TOUCH_PIN_RST      GPIO_NUM_NC

/* PSRAM (from sdkconfig) */
#define BOARD_PSRAM_CLK_IO        30
#define BOARD_PSRAM_CS_IO        26
