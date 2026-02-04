// Wi-Fi password screen (ported from SquareLine Studio wallet3, LVGL 9)

#include "../ui.h"
#include "app_wifi.h"
#include <stdint.h>
#include <string.h>

#define WIFI_PASSWORD_SSID_MAX 32
#define RESULT_MSG_DURATION_MS 2500

lv_obj_t *ui_wifi_password_screen = NULL;
lv_obj_t *ui_input_password_label = NULL;
lv_obj_t *ui_password_box = NULL;
lv_obj_t *ui_wifi_device_name = NULL;
lv_obj_t *ui_Keyboard1 = NULL;
lv_obj_t *ui_connect_btn = NULL;
lv_obj_t *ui_home_btn = NULL;

static char s_wifi_password_ssid[WIFI_PASSWORD_SSID_MAX + 1];
static bool s_connect_pending = false;
static lv_obj_t *s_loading_spinner = NULL;
static lv_obj_t *s_loading_label = NULL;
static lv_obj_t *s_result_label = NULL;
static lv_timer_t *s_result_timer = NULL;

static void result_timer_cb(lv_timer_t *timer)
{
	int go_home = (int)(intptr_t)lv_timer_get_user_data(timer);
	lv_timer_del(timer);
	s_result_timer = NULL;
	if (s_result_label)
		lv_obj_add_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);
	if (go_home && ui_wifi_password_screen && ui_Screen1)
		_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void show_loading(bool show)
{
	if (s_loading_spinner) {
		if (show)
			lv_obj_remove_flag(s_loading_spinner, LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_add_flag(s_loading_spinner, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_loading_label) {
		if (show)
			lv_obj_remove_flag(s_loading_label, LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_add_flag(s_loading_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (ui_connect_btn) {
		if (show)
			lv_obj_add_flag(ui_connect_btn, LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_remove_flag(ui_connect_btn, LV_OBJ_FLAG_HIDDEN);
	}
}

static void show_result(const char *msg, int go_home_after_ms)
{
	show_loading(false);
	s_connect_pending = false;
	if (s_result_label) {
		lv_label_set_text(s_result_label, msg);
		lv_obj_remove_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_result_timer)
		lv_timer_del(s_result_timer);
	s_result_timer = lv_timer_create(result_timer_cb,
		go_home_after_ms > 0 ? (uint32_t)go_home_after_ms : (uint32_t)RESULT_MSG_DURATION_MS,
		(void *)(intptr_t)(go_home_after_ms > 0 ? 1 : 0));
}

void ui_wifi_password_screen_on_connect_result(int success)
{
	if (!s_connect_pending)
		return;
	if (success)
		show_result("Connected!", 1500);
	else
		show_result("Failed", 0);
}

static void ui_event_password_box(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
		_ui_keyboard_set_target(ui_Keyboard1, ui_password_box);
}

static void ui_event_home_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED)
		return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void ui_event_connect_btn(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED)
		return;
	if (s_connect_pending)
		return;
	/* Hide keyboard when connecting so loading/result and Home button are visible */
	if (ui_Keyboard1) {
		lv_keyboard_set_textarea(ui_Keyboard1, NULL);
		lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
	}
	const char *pass = lv_textarea_get_text(ui_password_box);
	s_connect_pending = true;
	show_loading(true);
	app_wifi_connect(s_wifi_password_ssid, pass && pass[0] ? pass : NULL);
}

void ui_wifi_password_screen_show(const char *ssid)
{
	if (!ssid)
		return;
	size_t len = strnlen(ssid, WIFI_PASSWORD_SSID_MAX);
	memcpy(s_wifi_password_ssid, ssid, len);
	s_wifi_password_ssid[len] = '\0';

	if (ui_wifi_password_screen == NULL)
		ui_wifi_password_screen_screen_init();

	if (ui_wifi_device_name)
		lv_label_set_text(ui_wifi_device_name, s_wifi_password_ssid);
	if (ui_password_box) {
		lv_textarea_set_text(ui_password_box, "");
	}
	/* Show keyboard again when opening this screen */
	if (ui_Keyboard1) {
		lv_obj_remove_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
		lv_keyboard_set_textarea(ui_Keyboard1, ui_password_box);
	}
	_ui_screen_change(&ui_wifi_password_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, NULL);
}

void ui_wifi_password_screen_screen_init(void)
{
	ui_wifi_password_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_wifi_password_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_wifi_password_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_wifi_password_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_input_password_label = lv_label_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_input_password_label, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_input_password_label, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_input_password_label, -27);
	lv_obj_set_y(ui_input_password_label, -192);
	lv_obj_set_align(ui_input_password_label, LV_ALIGN_CENTER);
	lv_label_set_text(ui_input_password_label, "input\npassword");
	lv_obj_set_style_text_align(ui_input_password_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_decor(ui_input_password_label, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_input_password_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(ui_input_password_label, 350, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_password_box = lv_textarea_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_password_box, 212);
	lv_obj_set_height(ui_password_box, 70);
	lv_obj_set_x(ui_password_box, 1);
	lv_obj_set_y(ui_password_box, -46);
	lv_obj_set_align(ui_password_box, LV_ALIGN_CENTER);
	lv_textarea_set_placeholder_text(ui_password_box, "Password...");
	lv_textarea_set_password_mode(ui_password_box, false);  /* visible so user can verify on small keyboard */
	lv_obj_set_style_border_color(ui_password_box, lv_color_hex(0x000000), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_border_opa(ui_password_box, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_event_cb(ui_password_box, ui_event_password_box, LV_EVENT_CLICKED, NULL);

	ui_wifi_device_name = lv_label_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_wifi_device_name, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_wifi_device_name, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_wifi_device_name, -9);
	lv_obj_set_y(ui_wifi_device_name, -104);
	lv_obj_set_align(ui_wifi_device_name, LV_ALIGN_CENTER);
	lv_label_set_text(ui_wifi_device_name, "");
	lv_obj_set_style_text_align(ui_wifi_device_name, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_wifi_device_name, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(ui_wifi_device_name, 250, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_Keyboard1 = lv_keyboard_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_Keyboard1, 321);
	lv_obj_set_height(ui_Keyboard1, 194);
	lv_obj_set_x(ui_Keyboard1, 0);
	lv_obj_set_y(ui_Keyboard1, 143);
	lv_obj_set_align(ui_Keyboard1, LV_ALIGN_CENTER);
	lv_obj_set_style_bg_color(ui_Keyboard1, lv_color_hex(0x8BBE6A), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_Keyboard1, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_keyboard_set_textarea(ui_Keyboard1, ui_password_box);

	/* Connect button: above keyboard so keyboard remains usable */
	ui_connect_btn = lv_button_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_connect_btn, 120);
	lv_obj_set_height(ui_connect_btn, 40);
	lv_obj_set_x(ui_connect_btn, 0);
	lv_obj_set_y(ui_connect_btn, 8);
	lv_obj_set_align(ui_connect_btn, LV_ALIGN_CENTER);
	lv_obj_t *connect_lbl = lv_label_create(ui_connect_btn);
	lv_label_set_text(connect_lbl, "Connect");
	lv_obj_center(connect_lbl);
	lv_obj_add_event_cb(ui_connect_btn, ui_event_connect_btn, LV_EVENT_CLICKED, NULL);

	/* Loading: spinner + label, hidden by default */
	s_loading_spinner = lv_spinner_create(ui_wifi_password_screen);
	lv_obj_set_size(s_loading_spinner, 50, 50);
	lv_obj_set_x(s_loading_spinner, -30);
	lv_obj_set_y(s_loading_spinner, 8);
	lv_obj_set_align(s_loading_spinner, LV_ALIGN_CENTER);
	lv_spinner_set_anim_params(s_loading_spinner, 1000, 60);
	lv_obj_add_flag(s_loading_spinner, LV_OBJ_FLAG_HIDDEN);

	s_loading_label = lv_label_create(ui_wifi_password_screen);
	lv_label_set_text(s_loading_label, "Connecting...");
	lv_obj_set_style_text_font(s_loading_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_x(s_loading_label, 35);
	lv_obj_set_y(s_loading_label, 8);
	lv_obj_set_align(s_loading_label, LV_ALIGN_CENTER);
	lv_obj_add_flag(s_loading_label, LV_OBJ_FLAG_HIDDEN);

	/* Result message: "Connected!" or "Failed", hidden by default */
	s_result_label = lv_label_create(ui_wifi_password_screen);
	lv_label_set_text(s_result_label, "");
	lv_obj_set_style_text_font(s_result_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_transform_scale(s_result_label, 300, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_x(s_result_label, 0);
	lv_obj_set_y(s_result_label, 8);
	lv_obj_set_align(s_result_label, LV_ALIGN_CENTER);
	lv_obj_add_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);

	/* Home button: go back when keyboard is hidden (after Connect or anytime) */
	ui_home_btn = lv_button_create(ui_wifi_password_screen);
	lv_obj_set_width(ui_home_btn, 80);
	lv_obj_set_height(ui_home_btn, 36);
	lv_obj_set_x(ui_home_btn, 110);
	lv_obj_set_y(ui_home_btn, -210);
	lv_obj_set_align(ui_home_btn, LV_ALIGN_CENTER);
	lv_obj_t *home_lbl = lv_label_create(ui_home_btn);
	lv_label_set_text(home_lbl, "Home");
	lv_obj_center(home_lbl);
	lv_obj_add_event_cb(ui_home_btn, ui_event_home_btn, LV_EVENT_CLICKED, NULL);
}

void ui_wifi_password_screen_screen_destroy(void)
{
	if (s_result_timer) {
		lv_timer_del(s_result_timer);
		s_result_timer = NULL;
	}
	s_connect_pending = false;
	s_loading_spinner = NULL;
	s_loading_label = NULL;
	s_result_label = NULL;
	if (ui_wifi_password_screen)
		lv_obj_del(ui_wifi_password_screen);
	ui_wifi_password_screen = NULL;
	ui_input_password_label = NULL;
	ui_password_box = NULL;
	ui_wifi_device_name = NULL;
	ui_Keyboard1 = NULL;
	ui_connect_btn = NULL;
	ui_home_btn = NULL;
}
