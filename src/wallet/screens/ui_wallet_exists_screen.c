/* Wallet already exists: message + Back + Delete buttons */

#include "../ui.h"
#include "ui_wallet_exists_screen.h"
#include "ui_Screen2.h"
#include "wallet_sd.h"

lv_obj_t *ui_wallet_exists_screen = NULL;
static lv_obj_t *s_msg_label = NULL;
static lv_obj_t *s_back_btn = NULL;
static lv_obj_t *s_delete_btn = NULL;

static void back_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED && ui_Screen2)
		_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_Screen2_screen_init);
}

static void delete_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code != LV_EVENT_CLICKED)
		return;
	wallet_sd_delete();
	if (ui_Screen2)
		_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_Screen2_screen_init);
}

void ui_wallet_exists_screen_show(void)
{
	if (ui_wallet_exists_screen == NULL)
		ui_wallet_exists_screen_screen_init();
	_ui_screen_change(&ui_wallet_exists_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_wallet_exists_screen_screen_init(void)
{
	ui_wallet_exists_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_wallet_exists_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_wallet_exists_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_wallet_exists_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_msg_label = lv_label_create(ui_wallet_exists_screen);
	lv_obj_set_width(s_msg_label, LV_SIZE_CONTENT);
	lv_obj_set_height(s_msg_label, LV_SIZE_CONTENT);
	lv_obj_set_align(s_msg_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_msg_label, -40);
	lv_label_set_text(s_msg_label, "Wallet already exists");
	lv_obj_set_style_text_align(s_msg_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_msg_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(s_msg_label, 350, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_back_btn = lv_button_create(ui_wallet_exists_screen);
	lv_obj_set_width(s_back_btn, 160);
	lv_obj_set_height(s_back_btn, 60);
	lv_obj_set_align(s_back_btn, LV_ALIGN_CENTER);
	lv_obj_set_x(s_back_btn, -95);
	lv_obj_set_y(s_back_btn, 80);
	lv_obj_add_event_cb(s_back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *back_label = lv_label_create(s_back_btn);
	lv_label_set_text(back_label, "Back");
	lv_obj_center(back_label);
	lv_obj_set_style_text_font(back_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_delete_btn = lv_button_create(ui_wallet_exists_screen);
	lv_obj_set_width(s_delete_btn, 160);
	lv_obj_set_height(s_delete_btn, 60);
	lv_obj_set_align(s_delete_btn, LV_ALIGN_CENTER);
	lv_obj_set_x(s_delete_btn, 95);
	lv_obj_set_y(s_delete_btn, 80);
	lv_obj_add_event_cb(s_delete_btn, delete_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *del_label = lv_label_create(s_delete_btn);
	lv_label_set_text(del_label, "Delete");
	lv_obj_center(del_label);
	lv_obj_set_style_text_font(del_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
}

void ui_wallet_exists_screen_screen_destroy(void)
{
	if (ui_wallet_exists_screen)
		lv_obj_del(ui_wallet_exists_screen);
	ui_wallet_exists_screen = NULL;
	s_msg_label = NULL;
	s_back_btn = NULL;
	s_delete_btn = NULL;
}
