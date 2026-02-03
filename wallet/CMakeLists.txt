# Wallet UI (SquareLine Studio export, LVGL 9)
idf_component_register(
    SRCS
        screens/ui_Screen1.c
        screens/ui_Screen2.c
        ui.c
        components/ui_comp_hook.c
        ui_helpers.c
        images/ui_img_profile2_png.c
        fonts/ui_font_Pixel.c
    INCLUDE_DIRS "."
    REQUIRES lvgl__lvgl espressif__esp_lvgl_port
)
