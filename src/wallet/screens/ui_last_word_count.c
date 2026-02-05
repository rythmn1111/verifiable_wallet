/* Last word count screen (word 12): ported from wallet3, no transform_scale. */

#include "../ui.h"
#include "ui_last_word_count.h"
#include "ui_word_count.h"
#include "ui_note_for_password.h"
#include "wallet_mnemonic_display.h"
#include <string.h>

#define WORD_BUF_SIZE 32

lv_obj_t *ui_last_word_count = NULL;
lv_obj_t *ui_time1 = NULL;
lv_obj_t *ui_wifi_enable1 = NULL;
lv_obj_t *ui_wifi_not_enable1 = NULL;
lv_obj_t *ui_Label1 = NULL;
lv_obj_t *ui_word1 = NULL;
lv_obj_t *ui_back_button = NULL;
lv_obj_t *ui_finish_button = NULL;

static void back_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code != LV_EVENT_CLICKED) return;
	ui_word_count_set_index(10);
	ui_word_count_refresh_labels();
	_ui_screen_change(&ui_word_count, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_word_count_screen_init);
}

static void finish_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_note_for_password, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_note_for_password_screen_init);
}

void ui_last_word_count_refresh_word(void)
{
	char buf[WORD_BUF_SIZE];
	if (ui_word1 && wallet_mnemonic_display_get_word(11, buf, sizeof(buf)) == 0)
		lv_label_set_text(ui_word1, buf);
}

void ui_last_word_count_set_wifi_connected(int connected)
{
	if (ui_wifi_not_enable1 && ui_wifi_enable1) {
		if (connected) {
			lv_obj_add_flag(ui_wifi_not_enable1, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(ui_wifi_enable1, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_remove_flag(ui_wifi_not_enable1, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(ui_wifi_enable1, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void ui_last_word_count_screen_init(void)
{
	char buf[WORD_BUF_SIZE];

	ui_last_word_count = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_last_word_count, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_last_word_count, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_last_word_count, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_time1 = lv_label_create(ui_last_word_count);
	lv_obj_set_width(ui_time1, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_time1, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_time1, -34);
	lv_obj_set_y(ui_time1, -216);
	lv_obj_set_align(ui_time1, LV_ALIGN_CENTER);
	lv_label_set_text(ui_time1, "12:30PM");
	lv_obj_set_style_text_font(ui_time1, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_wifi_enable1 = lv_image_create(ui_last_word_count);
	lv_image_set_src(ui_wifi_enable1, &ui_img_wifi_enable_png);
	lv_obj_set_width(ui_wifi_enable1, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_enable1, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_enable1, 113);
	lv_obj_set_y(ui_wifi_enable1, -213);
	lv_obj_set_align(ui_wifi_enable1, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_enable1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_enable1, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_enable1, 100);

	ui_wifi_not_enable1 = lv_image_create(ui_last_word_count);
	lv_image_set_src(ui_wifi_not_enable1, &ui_img_wifi_not_enable_png);
	lv_obj_set_width(ui_wifi_not_enable1, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_not_enable1, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_not_enable1, 113);
	lv_obj_set_y(ui_wifi_not_enable1, -213);
	lv_obj_set_align(ui_wifi_not_enable1, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_not_enable1, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_not_enable1, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_not_enable1, 100);

	ui_Label1 = lv_label_create(ui_last_word_count);
	lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Label1, -7);
	lv_obj_set_y(ui_Label1, -99);
	lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
	lv_label_set_text(ui_Label1, "12/12");
	lv_obj_set_style_text_font(ui_Label1, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_word1 = lv_label_create(ui_last_word_count);
	lv_obj_set_width(ui_word1, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_word1, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_word1, 0);
	lv_obj_set_y(ui_word1, -27);
	lv_obj_set_align(ui_word1, LV_ALIGN_CENTER);
	if (wallet_mnemonic_display_get_word(11, buf, sizeof(buf)) == 0)
		lv_label_set_text(ui_word1, buf);
	else
		lv_label_set_text(ui_word1, "");
	lv_obj_set_style_text_decor(ui_word1, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_word1, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_back_button = lv_imagebutton_create(ui_last_word_count);
	lv_imagebutton_set_src(ui_back_button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_backbutton_png, NULL);
	lv_obj_set_width(ui_back_button, 147);
	lv_obj_set_height(ui_back_button, 71);
	lv_obj_set_x(ui_back_button, -74);
	lv_obj_set_y(ui_back_button, 85);
	lv_obj_set_align(ui_back_button, LV_ALIGN_CENTER);
	lv_obj_add_event_cb(ui_back_button, back_btn_cb, LV_EVENT_CLICKED, NULL);

	ui_finish_button = lv_imagebutton_create(ui_last_word_count);
	lv_imagebutton_set_src(ui_finish_button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_finishbutton_png, NULL);
	lv_obj_set_width(ui_finish_button, 143);
	lv_obj_set_height(ui_finish_button, 70);
	lv_obj_set_x(ui_finish_button, 81);
	lv_obj_set_y(ui_finish_button, 83);
	lv_obj_set_align(ui_finish_button, LV_ALIGN_CENTER);
	lv_obj_add_event_cb(ui_finish_button, finish_btn_cb, LV_EVENT_CLICKED, NULL);
}

void ui_last_word_count_screen_destroy(void)
{
	if (ui_last_word_count)
		lv_obj_del(ui_last_word_count);
	ui_last_word_count = NULL;
	ui_time1 = NULL;
	ui_wifi_enable1 = NULL;
	ui_wifi_not_enable1 = NULL;
	ui_Label1 = NULL;
	ui_word1 = NULL;
	ui_back_button = NULL;
	ui_finish_button = NULL;
}
