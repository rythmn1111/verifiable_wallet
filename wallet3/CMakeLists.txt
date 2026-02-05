SET(SOURCES ui_Screen1.c
    ui_Screen2.c
    ui_setting_screen.c
    ui_Screen3.c
    ui_wallet_generation_waiting_screen.c
    ui_word_count.c
    ui_wifi_password_screen.c
    ui_new_note_for_word_count.c
    ui_note_for_password.c
    ui_password_for_encription.c
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
    ui_img_wallet_watiting_png.c
    ui_img_backbutton_png.c
    ui_img_nextbutton_png.c
    ui_img_letsgobutton_png.c
    fonts/ui_font_Pixel.c
    fonts/ui_font_pixel_heading.c
    fonts/ui_font_pixel_wordings.c
    fonts/ui_font_pixel_wordings_small.c)

add_library(ui ${SOURCES})
