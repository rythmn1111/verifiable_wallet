idf_component_register(SRCS "esp_3inch5_lcd_port.cpp" "esp_camera_port.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "driver" "esp_lcd_st7796" "esp_lcd_touch_ft6336" "esp32-camera")
