/**
 * Export key screen (DEV ONLY - remove in production).
 * Password -> decrypt stored JWK -> show first part of keyfile for confirmation.
 */

#include "../ui.h"
#include "ui_export_key_screen.h"
#include "arweave_wallet_gen.h"
#include "wallet_sd.h"
#include "wallet_encrypt.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "export_key";

#define PASSWORD_MAX_LEN    64
#define JWK_PREVIEW_CHARS  400
#define CT_B64_SIZE         (((WALLET_GEN_JWK_MAX + 16 + 2) / 3) * 4 + 1)
#define SALT_HEX_SIZE       (WALLET_ENCRYPT_SALT_HEX_LEN + 2)  /* +2 so fgets consumes newline */
#define IV_HEX_SIZE         (WALLET_ENCRYPT_IV_HEX_LEN + 2)

lv_obj_t *ui_export_key_screen = NULL;
static lv_obj_t *s_pwd_ta = NULL;
static lv_obj_t *s_keyboard = NULL;
static lv_obj_t *s_result_label = NULL;
static lv_obj_t *s_done_btn = NULL;
static lv_obj_t *s_home_btn = NULL;

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void add_circle_home(lv_obj_t *parent)
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

static void do_decrypt(void)
{
	ESP_LOGI(TAG, "do_decrypt called");

	const char *pwd = s_pwd_ta ? lv_textarea_get_text(s_pwd_ta) : "";
	if (!pwd) pwd = "";
	if (pwd[0] == '\0') {
		ESP_LOGW(TAG, "password empty");
		if (s_result_label)
			lv_label_set_text(s_result_label, "Enter password");
		return;
	}

	ESP_LOGI(TAG, "reading encrypted JWK from SD...");
	static char salt_hex[SALT_HEX_SIZE];
	static char iv_hex[IV_HEX_SIZE];
	static char ct_b64[CT_B64_SIZE];
	if (!wallet_sd_read_encrypted_jwk(salt_hex, sizeof(salt_hex), iv_hex, sizeof(iv_hex), ct_b64, sizeof(ct_b64))) {
		ESP_LOGE(TAG, "read encrypted JWK failed");
		if (s_result_label)
			lv_label_set_text(s_result_label, "Read failed");
		return;
	}
	ESP_LOGI(TAG, "read OK, decryption starting");

	static char jwk_buf[WALLET_GEN_JWK_MAX];
	int ret = wallet_decrypt_jwk(pwd, salt_hex, iv_hex, ct_b64, jwk_buf, sizeof(jwk_buf));
	if (ret != 0) {
		ESP_LOGE(TAG, "decryption failed ret=%d", ret);
		if (s_result_label)
			lv_label_set_text(s_result_label, "Wrong password");
		return;
	}

	ESP_LOGI(TAG, "decryption OK, showing preview");
	/* Show first JWK_PREVIEW_CHARS of keyfile */
	static char preview[JWK_PREVIEW_CHARS + 16];
	size_t len = strnlen(jwk_buf, sizeof(jwk_buf));
	if (len > JWK_PREVIEW_CHARS) {
		memcpy(preview, jwk_buf, JWK_PREVIEW_CHARS);
		preview[JWK_PREVIEW_CHARS] = '\0';
		strcat(preview, "...");
	} else {
		memcpy(preview, jwk_buf, len + 1);
	}
	ESP_LOGI(TAG, "preview %u chars (full keyfile %u chars)", (unsigned)(len > JWK_PREVIEW_CHARS ? JWK_PREVIEW_CHARS : len), (unsigned)len);
	ESP_LOGI(TAG, "decrypted keyfile preview:\n%s", preview);
	if (s_result_label) {
		lv_label_set_text(s_result_label, preview);
		lv_obj_remove_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);
	}
}

static void done_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	ESP_LOGI(TAG, "Show keyfile pressed");
	do_decrypt();
}

static void ta_ready_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_READY) return;
	ESP_LOGI(TAG, "keyboard Done/Enter pressed");
	do_decrypt();
}

static void pwd_click_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && s_keyboard)
		_ui_keyboard_set_target(s_keyboard, s_pwd_ta);
}

void ui_export_key_screen_show(void)
{
	ESP_LOGI(TAG, "export key screen shown");
	if (ui_export_key_screen == NULL)
		ui_export_key_screen_screen_init();
	if (s_result_label) {
		lv_label_set_text(s_result_label, "");
		lv_obj_add_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_pwd_ta)
		lv_textarea_set_text(s_pwd_ta, "");
	_ui_screen_change(&ui_export_key_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_export_key_screen_screen_init(void)
{
	ui_export_key_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_export_key_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_export_key_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_export_key_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	lv_obj_t *title = lv_label_create(ui_export_key_screen);
	lv_obj_set_width(title, 240);
	lv_obj_set_align(title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(title, -175);
	lv_label_set_text(title, "Export key (dev)");
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	lv_obj_t *pwd_lbl = lv_label_create(ui_export_key_screen);
	lv_obj_set_align(pwd_lbl, LV_ALIGN_CENTER);
	lv_obj_set_y(pwd_lbl, -120);
	lv_label_set_text(pwd_lbl, "Encryption password");
	lv_obj_set_style_text_font(pwd_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_pwd_ta = lv_textarea_create(ui_export_key_screen);
	lv_obj_set_width(s_pwd_ta, 212);
	lv_obj_set_height(s_pwd_ta, 50);
	lv_obj_set_align(s_pwd_ta, LV_ALIGN_CENTER);
	lv_obj_set_y(s_pwd_ta, -65);
	lv_textarea_set_placeholder_text(s_pwd_ta, "Password...");
	lv_textarea_set_max_length(s_pwd_ta, PASSWORD_MAX_LEN);
	lv_textarea_set_password_mode(s_pwd_ta, true);
	lv_obj_add_event_cb(s_pwd_ta, pwd_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(s_pwd_ta, ta_ready_cb, LV_EVENT_READY, NULL);

	s_done_btn = lv_button_create(ui_export_key_screen);
	lv_obj_set_width(s_done_btn, 140);
	lv_obj_set_height(s_done_btn, 44);
	lv_obj_set_align(s_done_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_done_btn, 0);
	lv_obj_add_event_cb(s_done_btn, done_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *btn_lbl = lv_label_create(s_done_btn);
	lv_label_set_text(btn_lbl, "Show keyfile");
	lv_obj_center(btn_lbl);
	lv_obj_set_style_text_font(btn_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_result_label = lv_label_create(ui_export_key_screen);
	lv_obj_set_width(s_result_label, 280);
	lv_obj_set_height(s_result_label, 120);
	lv_obj_set_align(s_result_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_result_label, 75);
	lv_label_set_text(s_result_label, "");
	lv_label_set_long_mode(s_result_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_font(s_result_label, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_flag(s_result_label, LV_OBJ_FLAG_HIDDEN);

	s_keyboard = lv_keyboard_create(ui_export_key_screen);
	lv_obj_set_width(s_keyboard, 321);
	lv_obj_set_height(s_keyboard, 194);
	lv_obj_set_y(s_keyboard, 143);
	lv_obj_set_align(s_keyboard, LV_ALIGN_CENTER);
	lv_obj_set_style_bg_color(s_keyboard, lv_color_hex(0x8BBE6A), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_keyboard_set_textarea(s_keyboard, s_pwd_ta);

	add_circle_home(ui_export_key_screen);
}

void ui_export_key_screen_screen_destroy(void)
{
	if (ui_export_key_screen)
		lv_obj_del(ui_export_key_screen);
	ui_export_key_screen = NULL;
	s_pwd_ta = NULL;
	s_keyboard = NULL;
	s_result_label = NULL;
	s_done_btn = NULL;
	s_home_btn = NULL;
}
