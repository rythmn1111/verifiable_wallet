# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
                    REQUIRES "esp_port" "qr_decoder" "nvs_flash" "driver" "esp_io_expander_tca9554" "esp_lvgl_port" "lvgl" "esp_lcd_touch" "esp32-camera" "button")
