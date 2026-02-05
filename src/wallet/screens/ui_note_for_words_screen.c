/* Note-for-words screen: intro before 12-word display. Ported from wallet3; LVGL 9. */

#include "../ui.h"
#include "ui_note_for_words_screen.h"

lv_obj_t *ui_note_for_words_screen = NULL;
lv_obj_t *ui_time5 = NULL;
lv_obj_t *ui_wifi_enable5 = NULL;
lv_obj_t *ui_wifi_not_enable5 = NULL;
lv_obj_t *ui_Note = NULL;
lv_obj_t *ui_note_text = NULL;
lv_obj_t *ui_lets_go_button = NULL;

void ui_note_for_words_screen_screen_init(void)
{
	ui_note_for_words_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_note_for_words_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_note_for_words_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_note_for_words_screen, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_time5 = lv_label_create(ui_note_for_words_screen);
	lv_obj_set_width(ui_time5, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_time5, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_time5, -34);
	lv_obj_set_y(ui_time5, -216);
	lv_obj_set_align(ui_time5, LV_ALIGN_CENTER);
	lv_label_set_text(ui_time5, "12:30PM");
	lv_obj_set_style_text_font(ui_time5, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	/* no transform_scale: avoids taskLVGL blocking and watchdog (expensive on ESP32) */

	ui_wifi_enable5 = lv_image_create(ui_note_for_words_screen);
	lv_image_set_src(ui_wifi_enable5, &ui_img_wifi_enable_png);
	lv_obj_set_width(ui_wifi_enable5, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_enable5, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_enable5, 113);
	lv_obj_set_y(ui_wifi_enable5, -213);
	lv_obj_set_align(ui_wifi_enable5, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_enable5, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_enable5, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_enable5, 100);

	ui_wifi_not_enable5 = lv_image_create(ui_note_for_words_screen);
	lv_image_set_src(ui_wifi_not_enable5, &ui_img_wifi_not_enable_png);
	lv_obj_set_width(ui_wifi_not_enable5, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_not_enable5, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_not_enable5, 113);
	lv_obj_set_y(ui_wifi_not_enable5, -213);
	lv_obj_set_align(ui_wifi_not_enable5, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_not_enable5, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_not_enable5, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_not_enable5, 100);

	ui_Note = lv_label_create(ui_note_for_words_screen);
	lv_obj_set_width(ui_Note, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Note, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Note, -42);
	lv_obj_set_y(ui_Note, -146);
	lv_obj_set_align(ui_Note, LV_ALIGN_CENTER);
	lv_label_set_text(ui_Note, "Note");
	lv_obj_set_style_text_decor(ui_Note, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_Note, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	/* no transform_scale: avoids taskLVGL blocking and watchdog */

	ui_note_text = lv_label_create(ui_note_for_words_screen);
	lv_obj_set_width(ui_note_text, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_note_text, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_note_text, 63);
	lv_obj_set_y(ui_note_text, 69);
	lv_obj_set_align(ui_note_text, LV_ALIGN_CENTER);
	lv_label_set_text(ui_note_text,
		"> On the next screen you \n\nwill be displayed 12\n\nunique characters.\n\n\n> Make sure to write them\n\ndown somewhere safe.\n\n\n> If you loose them there \n\nis no way to recover your\n\nwallet ");
	lv_obj_set_style_text_align(ui_note_text, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_note_text, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	/* no transform_scale: avoids taskLVGL blocking and watchdog */

	/* Let's go: text button (no image asset in src/wallet) */
	ui_lets_go_button = lv_button_create(ui_note_for_words_screen);
	lv_obj_set_width(ui_lets_go_button, 105);
	lv_obj_set_height(ui_lets_go_button, 50);
	lv_obj_set_x(ui_lets_go_button, -8);
	lv_obj_set_y(ui_lets_go_button, 176);
	lv_obj_set_align(ui_lets_go_button, LV_ALIGN_CENTER);
	lv_obj_t *btn_label = lv_label_create(ui_lets_go_button);
	lv_label_set_text(btn_label, "Let's go");
	lv_obj_center(btn_label);
	lv_obj_set_style_text_font(btn_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
}

void ui_note_for_words_screen_screen_destroy(void)
{
	if (ui_note_for_words_screen)
		lv_obj_del(ui_note_for_words_screen);
	ui_note_for_words_screen = NULL;
	ui_time5 = NULL;
	ui_wifi_enable5 = NULL;
	ui_wifi_not_enable5 = NULL;
	ui_Note = NULL;
	ui_note_text = NULL;
	ui_lets_go_button = NULL;
}
