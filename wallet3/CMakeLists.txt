SET(SOURCES ui_Screen1.c
    ui_Screen2.c
    ui_setting_screen.c
    ui_wifi_password_screen.c
    ui_Screen3.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_profile2_png.c
    ui_img_wifi_not_enable_png.c
    ui_img_wifi_enable_png.c
    ui_img_settingsicon_png.c
    ui_img_wallet_png.c
    ui_img_under_image_png.c
    ui_img_verification_png.c
    fonts/ui_font_Pixel.c)

add_library(ui ${SOURCES})
