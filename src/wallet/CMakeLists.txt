SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_setting_screen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_profile2_png.c
    images/ui_img_settingsicon_png.c
    fonts/ui_font_Pixel.c)

add_library(ui ${SOURCES})
