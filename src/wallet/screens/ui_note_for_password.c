/* Note for password screen: ported from wallet3, no transform_scale. */

#include "../ui.h"
#include "ui_note_for_password.h"
#include "ui_password_for_encryption.h"

lv_obj_t *ui_note_for_password = NULL;
lv_obj_t *ui_time5_pwd = NULL;
lv_obj_t *ui_wifi_enable5_pwd = NULL;
lv_obj_t *ui_wifi_not_enable5_pwd = NULL;
lv_obj_t *ui_Label2 = NULL;
lv_obj_t *ui_note_ = NULL;
lv_obj_t *ui_ImgButton1 = NULL;

static void lets_go_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code != LV_EVENT_CLICKED) return;
	ui_password_for_encryption_clear_inputs();
	_ui_screen_change(&ui_password_for_encryption, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_password_for_encryption_screen_init);
}

void ui_note_for_password_set_wifi_connected(int connected)
{
	if (ui_wifi_not_enable5_pwd && ui_wifi_enable5_pwd) {
		if (connected) {
			lv_obj_add_flag(ui_wifi_not_enable5_pwd, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(ui_wifi_enable5_pwd, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_remove_flag(ui_wifi_not_enable5_pwd, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(ui_wifi_enable5_pwd, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void ui_note_for_password_screen_init(void)
{
	ui_note_for_password = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_note_for_password, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_note_for_password, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_note_for_password, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_time5_pwd = lv_label_create(ui_note_for_password);
	lv_obj_set_width(ui_time5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_time5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_time5_pwd, -34);
	lv_obj_set_y(ui_time5_pwd, -216);
	lv_obj_set_align(ui_time5_pwd, LV_ALIGN_CENTER);
	lv_label_set_text(ui_time5_pwd, "12:30PM");
	lv_obj_set_style_text_font(ui_time5_pwd, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_wifi_enable5_pwd = lv_image_create(ui_note_for_password);
	lv_image_set_src(ui_wifi_enable5_pwd, &ui_img_wifi_enable_png);
	lv_obj_set_width(ui_wifi_enable5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_enable5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_enable5_pwd, 113);
	lv_obj_set_y(ui_wifi_enable5_pwd, -213);
	lv_obj_set_align(ui_wifi_enable5_pwd, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_enable5_pwd, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_enable5_pwd, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_enable5_pwd, 100);

	ui_wifi_not_enable5_pwd = lv_image_create(ui_note_for_password);
	lv_image_set_src(ui_wifi_not_enable5_pwd, &ui_img_wifi_not_enable_png);
	lv_obj_set_width(ui_wifi_not_enable5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_not_enable5_pwd, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_not_enable5_pwd, 113);
	lv_obj_set_y(ui_wifi_not_enable5_pwd, -213);
	lv_obj_set_align(ui_wifi_not_enable5_pwd, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_not_enable5_pwd, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_not_enable5_pwd, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_not_enable5_pwd, 100);

	ui_Label2 = lv_label_create(ui_note_for_password);
	lv_obj_set_width(ui_Label2, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Label2, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Label2, -3);
	lv_obj_set_y(ui_Label2, -140);
	lv_obj_set_align(ui_Label2, LV_ALIGN_CENTER);
	lv_label_set_text(ui_Label2, "Note");
	lv_obj_set_style_text_decor(ui_Label2, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_Label2, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_note_ = lv_label_create(ui_note_for_password);
	lv_obj_set_width(ui_note_, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_note_, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_note_, 2);
	lv_obj_set_y(ui_note_, -2);
	lv_obj_set_align(ui_note_, LV_ALIGN_CENTER);
	lv_label_set_text(ui_note_,
		"> On the next screen\n\nyou would be ask to\n\ncreate a password.\n\nkindly write down, or\n\nremember this password,\n\nas it is the main\n\ndecryption key for \n\nsigning tx. ");
	lv_obj_set_style_text_align(ui_note_, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_note_, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_ImgButton1 = lv_imagebutton_create(ui_note_for_password);
	lv_imagebutton_set_src(ui_ImgButton1, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_letsgobutton_png, NULL);
	lv_obj_set_width(ui_ImgButton1, 105);
	lv_obj_set_height(ui_ImgButton1, 54);
	lv_obj_set_x(ui_ImgButton1, -1);
	lv_obj_set_y(ui_ImgButton1, 146);
	lv_obj_set_align(ui_ImgButton1, LV_ALIGN_CENTER);
	lv_obj_add_event_cb(ui_ImgButton1, lets_go_btn_cb, LV_EVENT_CLICKED, NULL);
}

void ui_note_for_password_screen_destroy(void)
{
	if (ui_note_for_password)
		lv_obj_del(ui_note_for_password);
	ui_note_for_password = NULL;
	ui_time5_pwd = NULL;
	ui_wifi_enable5_pwd = NULL;
	ui_wifi_not_enable5_pwd = NULL;
	ui_Label2 = NULL;
	ui_note_ = NULL;
	ui_ImgButton1 = NULL;
}
