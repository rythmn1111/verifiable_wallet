/* No wallet screen: one button "Generate" with + ; circle home (same as settings). */

#include "../ui.h"
#include "ui_wallet_no_wallet_screen.h"
#include "ui_wallet_generation_waiting_screen.h"

lv_obj_t *ui_wallet_no_wallet_screen = NULL;
static lv_obj_t *s_generate_btn = NULL;
static lv_obj_t *s_home_btn = NULL;

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void generate_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	ui_wallet_generation_waiting_screen_start();
}

void ui_wallet_no_wallet_screen_show(void)
{
	if (ui_wallet_no_wallet_screen == NULL)
		ui_wallet_no_wallet_screen_screen_init();
	_ui_screen_change(&ui_wallet_no_wallet_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

static void add_circle_home_button(lv_obj_t *parent)
{
	s_home_btn = lv_button_create(parent);
	lv_obj_set_width(s_home_btn, 49);
	lv_obj_set_height(s_home_btn, 47);
	lv_obj_set_x(s_home_btn, -6);
	lv_obj_set_y(s_home_btn, 205);
	lv_obj_set_align(s_home_btn, LV_ALIGN_CENTER);
	lv_obj_set_ext_click_area(s_home_btn, 8);
	lv_obj_add_flag(s_home_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_remove_flag(s_home_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(s_home_btn, 1000, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_color(s_home_btn, lv_color_hex(0xFFFFFF), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(s_home_btn, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_color(s_home_btn, lv_color_hex(0x000000), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_opa(s_home_btn, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_width(s_home_btn, 10, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_spread(s_home_btn, 1, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_event_cb(s_home_btn, home_btn_cb, LV_EVENT_CLICKED, NULL);
}

void ui_wallet_no_wallet_screen_screen_init(void)
{
	ui_wallet_no_wallet_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_wallet_no_wallet_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_wallet_no_wallet_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_wallet_no_wallet_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	lv_obj_t *title = lv_label_create(ui_wallet_no_wallet_screen);
	lv_obj_set_width(title, 240);
	lv_obj_set_height(title, LV_SIZE_CONTENT);
	lv_obj_set_align(title, LV_ALIGN_CENTER);
	lv_obj_set_y(title, -60);
	lv_label_set_text(title, "No wallet");
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_generate_btn = lv_button_create(ui_wallet_no_wallet_screen);
	lv_obj_set_width(s_generate_btn, 180);
	lv_obj_set_height(s_generate_btn, 52);
	lv_obj_set_align(s_generate_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_generate_btn, 0);
	lv_obj_add_event_cb(s_generate_btn, generate_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *gen_lbl = lv_label_create(s_generate_btn);
	lv_label_set_text(gen_lbl, LV_SYMBOL_PLUS " Generate");
	lv_obj_center(gen_lbl);
	lv_obj_set_style_text_font(gen_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	add_circle_home_button(ui_wallet_no_wallet_screen);
}

void ui_wallet_no_wallet_screen_screen_destroy(void)
{
	if (ui_wallet_no_wallet_screen)
		lv_obj_del(ui_wallet_no_wallet_screen);
	ui_wallet_no_wallet_screen = NULL;
	s_generate_btn = NULL;
	s_home_btn = NULL;
}
