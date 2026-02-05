/* New note for word count: ported from wallet3. Uses font sizes instead of transform_scale. */

#include "../ui.h"
#include "ui_new_note_for_word_count.h"
#include "ui_word_count.h"

lv_obj_t *ui_new_note_for_word_count = NULL;

static void lets_go_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code != LV_EVENT_CLICKED) return;
	ui_word_count_set_index(0);
	ui_word_count_refresh_labels();
	_ui_screen_change(&ui_word_count, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_word_count_screen_init);
}

void ui_new_note_for_word_count_set_wifi_connected(int connected)
{
	if (ui_wifi_not_enable8 && ui_wifi_enable8) {
		if (connected) {
			lv_obj_add_flag(ui_wifi_not_enable8, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(ui_wifi_enable8, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_remove_flag(ui_wifi_not_enable8, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(ui_wifi_enable8, LV_OBJ_FLAG_HIDDEN);
		}
	}
}
lv_obj_t *ui_time8 = NULL;
lv_obj_t *ui_wifi_enable8 = NULL;
lv_obj_t *ui_wifi_not_enable8 = NULL;
lv_obj_t *ui_Label10 = NULL;
lv_obj_t *ui_Label11 = NULL;
lv_obj_t *ui_ImgButton8 = NULL;

void ui_new_note_for_word_count_screen_init(void)
{
	ui_new_note_for_word_count = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_new_note_for_word_count, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_new_note_for_word_count, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_new_note_for_word_count, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_time8 = lv_label_create(ui_new_note_for_word_count);
	lv_obj_set_width(ui_time8, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_time8, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_time8, -34);
	lv_obj_set_y(ui_time8, -216);
	lv_obj_set_align(ui_time8, LV_ALIGN_CENTER);
	lv_label_set_text(ui_time8, "12:30PM");
	lv_obj_set_style_text_font(ui_time8, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	/* no transform_scale to avoid taskLVGL watchdog */

	ui_wifi_enable8 = lv_image_create(ui_new_note_for_word_count);
	lv_image_set_src(ui_wifi_enable8, &ui_img_wifi_enable_png);
	lv_obj_set_width(ui_wifi_enable8, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_enable8, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_enable8, 113);
	lv_obj_set_y(ui_wifi_enable8, -213);
	lv_obj_set_align(ui_wifi_enable8, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_enable8, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_enable8, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_enable8, 100);

	ui_wifi_not_enable8 = lv_image_create(ui_new_note_for_word_count);
	lv_image_set_src(ui_wifi_not_enable8, &ui_img_wifi_not_enable_png);
	lv_obj_set_width(ui_wifi_not_enable8, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_not_enable8, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_not_enable8, 113);
	lv_obj_set_y(ui_wifi_not_enable8, -213);
	lv_obj_set_align(ui_wifi_not_enable8, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_not_enable8, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_not_enable8, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_not_enable8, 100);

	ui_Label10 = lv_label_create(ui_new_note_for_word_count);
	lv_obj_set_width(ui_Label10, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Label10, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Label10, -3);
	lv_obj_set_y(ui_Label10, -140);
	lv_obj_set_align(ui_Label10, LV_ALIGN_CENTER);
	lv_label_set_text(ui_Label10, "Note");
	lv_obj_set_style_text_decor(ui_Label10, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_Label10, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_Label11 = lv_label_create(ui_new_note_for_word_count);
	lv_obj_set_width(ui_Label11, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Label11, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Label11, 2);
	lv_obj_set_y(ui_Label11, -2);
	lv_obj_set_align(ui_Label11, LV_ALIGN_CENTER);
	lv_label_set_text(ui_Label11,
		"> On the next screen you \n\nwill be displayed 12\n\nunique characters.\n\n\n> Make sure to write them\n\ndown somewhere safe.\n\n\n> If you loose them there \n\nis no way to recover your\n\nwallet ");
	lv_obj_set_style_text_align(ui_Label11, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_Label11, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	/* Let's go: original image button → Screen2 on click */
	ui_ImgButton8 = lv_imagebutton_create(ui_new_note_for_word_count);
	lv_imagebutton_set_src(ui_ImgButton8, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_letsgobutton_png, NULL);
	lv_obj_set_width(ui_ImgButton8, 105);
	lv_obj_set_height(ui_ImgButton8, 54);
	lv_obj_set_x(ui_ImgButton8, -1);
	lv_obj_set_y(ui_ImgButton8, 146);
	lv_obj_set_align(ui_ImgButton8, LV_ALIGN_CENTER);
	lv_obj_add_event_cb(ui_ImgButton8, lets_go_btn_cb, LV_EVENT_CLICKED, NULL);
}

void ui_new_note_for_word_count_screen_destroy(void)
{
	if (ui_new_note_for_word_count)
		lv_obj_del(ui_new_note_for_word_count);
	ui_new_note_for_word_count = NULL;
	ui_time8 = NULL;
	ui_wifi_enable8 = NULL;
	ui_wifi_not_enable8 = NULL;
	ui_Label10 = NULL;
	ui_Label11 = NULL;
	ui_ImgButton8 = NULL;
}
