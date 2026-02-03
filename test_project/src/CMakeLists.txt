FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "test_ui"
                    REQUIRES "esp_port" "nvs_flash" "driver" "esp_io_expander_tca9554" "espressif__esp_lvgl_port" "lvgl__lvgl" "espressif__esp_lcd_touch")
