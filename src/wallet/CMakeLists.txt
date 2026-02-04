SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_Screen3.c
    screens/ui_setting_screen.c
    screens/ui_wifi_password_screen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_profile2_png.c
    images/ui_img_settingsicon_png.c
    images/ui_img_wifi_not_enable_png.c
    images/ui_img_wifi_enable_png.c
    images/ui_img_wallet_png.c
    images/ui_img_under_image_png.c
    images/ui_img_verification_png.c
    fonts/ui_font_Pixel.c)

add_library(ui ${SOURCES})
