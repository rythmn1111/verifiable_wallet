/* Wallet generation waiting screen (from wallet3): image + time + wifi; when done, only Done button shown. */

#include "../ui.h"
#include "ui_wallet_generation_waiting_screen.h"
#include "ui_Screen2.h"
#include "arweave_wallet_gen.h"
#include "wallet_sd.h"
#include "esp_lvgl_port.h"

lv_obj_t *ui_wallet_generation_waiting_screen = NULL;
lv_obj_t *ui_Image6 = NULL;
lv_obj_t *ui_time4 = NULL;
lv_obj_t *ui_wifi_enable4 = NULL;
lv_obj_t *ui_wifi_not_enable4 = NULL;

static lv_obj_t *s_done_btn = NULL;

static void done_btn_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED && ui_Screen2)
		_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_Screen2_screen_init);
}

static void wallet_gen_done_cb(const char *words, const char *jwk)
{
	wallet_sd_save(words, jwk ? jwk : "");
	if (lvgl_port_lock(pdMS_TO_TICKS(100))) {
		if (s_done_btn)
			lv_obj_remove_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);
		lvgl_port_unlock();
	}
}

void ui_wallet_generation_waiting_screen_start(void)
{
	if (ui_wallet_generation_waiting_screen == NULL)
		ui_wallet_generation_waiting_screen_screen_init();
	if (s_done_btn)
		lv_obj_add_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);
	_ui_screen_change(&ui_wallet_generation_waiting_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
	arweave_wallet_gen_start(wallet_gen_done_cb);
}

void ui_wallet_generation_waiting_screen_screen_init(void)
{
	ui_wallet_generation_waiting_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_wallet_generation_waiting_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_wallet_generation_waiting_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_wallet_generation_waiting_screen, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_Image6 = lv_image_create(ui_wallet_generation_waiting_screen);
	lv_image_set_src(ui_Image6, &ui_img_wallet_watiting_png);
	lv_obj_set_width(ui_Image6, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_Image6, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_Image6, 6);
	lv_obj_set_y(ui_Image6, 11);
	lv_obj_set_align(ui_Image6, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_Image6, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_Image6, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_Image6, 200);

	ui_time4 = lv_label_create(ui_wallet_generation_waiting_screen);
	lv_obj_set_width(ui_time4, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_time4, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_time4, -34);
	lv_obj_set_y(ui_time4, -216);
	lv_obj_set_align(ui_time4, LV_ALIGN_CENTER);
	lv_label_set_text(ui_time4, "12:30PM");
	lv_obj_set_style_text_font(ui_time4, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(ui_time4, 350, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_wifi_enable4 = lv_image_create(ui_wallet_generation_waiting_screen);
	lv_image_set_src(ui_wifi_enable4, &ui_img_wifi_enable_png);
	lv_obj_set_width(ui_wifi_enable4, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_enable4, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_enable4, 113);
	lv_obj_set_y(ui_wifi_enable4, -213);
	lv_obj_set_align(ui_wifi_enable4, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_enable4, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_enable4, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_enable4, 100);

	ui_wifi_not_enable4 = lv_image_create(ui_wallet_generation_waiting_screen);
	lv_image_set_src(ui_wifi_not_enable4, &ui_img_wifi_not_enable_png);
	lv_obj_set_width(ui_wifi_not_enable4, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_not_enable4, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_not_enable4, 113);
	lv_obj_set_y(ui_wifi_not_enable4, -213);
	lv_obj_set_align(ui_wifi_not_enable4, LV_ALIGN_CENTER);
	lv_obj_add_flag(ui_wifi_not_enable4, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(ui_wifi_not_enable4, LV_OBJ_FLAG_SCROLLABLE);
	lv_image_set_scale(ui_wifi_not_enable4, 100);

	s_done_btn = lv_button_create(ui_wallet_generation_waiting_screen);
	lv_obj_set_width(s_done_btn, 160);
	lv_obj_set_height(s_done_btn, 60);
	lv_obj_set_align(s_done_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_done_btn, 130);
	lv_obj_add_flag(s_done_btn, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(s_done_btn, done_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *btn_label = lv_label_create(s_done_btn);
	lv_label_set_text(btn_label, "Done");
	lv_obj_center(btn_label);
	lv_obj_set_style_text_font(btn_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
}

void ui_wallet_generation_waiting_screen_screen_destroy(void)
{
	if (ui_wallet_generation_waiting_screen)
		lv_obj_del(ui_wallet_generation_waiting_screen);
	ui_wallet_generation_waiting_screen = NULL;
	ui_Image6 = NULL;
	ui_time4 = NULL;
	ui_wifi_enable4 = NULL;
	ui_wifi_not_enable4 = NULL;
	s_done_btn = NULL;
}
