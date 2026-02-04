/* Wallet generation screen: "Generating wallet..." then "Wallet generated!" */

#include "../ui.h"
#include "ui_wallet_gen_screen.h"
#include "arweave_wallet_gen.h"
#include "esp_lvgl_port.h"
#include <string.h>

lv_obj_t *ui_wallet_gen_screen = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_done_btn = NULL;

static void done_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED && ui_Screen2)
		_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_Screen2_screen_init);
}

static void wallet_gen_done_cb(const char *words, const char *jwk)
{
	(void)words;
	(void)jwk;
	if (lvgl_port_lock(pdMS_TO_TICKS(100))) {
		if (s_status_label)
			lv_label_set_text(s_status_label, "Wallet generated!");
		if (s_done_btn)
			lv_obj_remove_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);
		lvgl_port_unlock();
	}
}

void ui_wallet_gen_screen_start(void)
{
	if (ui_wallet_gen_screen == NULL)
		ui_wallet_gen_screen_screen_init();
	if (s_status_label)
		lv_label_set_text(s_status_label, "Generating wallet...");
	if (s_done_btn)
		lv_obj_add_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);
	_ui_screen_change(&ui_wallet_gen_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
	arweave_wallet_gen_start(wallet_gen_done_cb);
}

void ui_wallet_gen_screen_screen_init(void)
{
	ui_wallet_gen_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_wallet_gen_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_wallet_gen_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_wallet_gen_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_status_label = lv_label_create(ui_wallet_gen_screen);
	lv_obj_set_width(s_status_label, 280);
	lv_obj_set_height(s_status_label, LV_SIZE_CONTENT);
	lv_obj_set_align(s_status_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_status_label, -60);
	lv_label_set_text(s_status_label, "Generating wallet...");
	lv_label_set_long_mode(s_status_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_status_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(s_status_label, 300, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_done_btn = lv_button_create(ui_wallet_gen_screen);
	lv_obj_set_width(s_done_btn, 160);
	lv_obj_set_height(s_done_btn, 60);
	lv_obj_set_align(s_done_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_done_btn, 80);
	lv_obj_add_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);  /* shown when generation done */
	lv_obj_add_event_cb(s_done_btn, done_btn_cb, LV_EVENT_CLICKED, NULL);

	lv_obj_t *btn_label = lv_label_create(s_done_btn);
	lv_label_set_text(btn_label, "Done");
	lv_obj_center(btn_label);
	lv_obj_set_style_text_font(btn_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
}

void ui_wallet_gen_screen_screen_destroy(void)
{
	if (ui_wallet_gen_screen)
		lv_obj_del(ui_wallet_gen_screen);
	ui_wallet_gen_screen = NULL;
	s_status_label = NULL;
	s_done_btn = NULL;
}
