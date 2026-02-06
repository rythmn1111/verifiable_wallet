/* Owner (512-byte) QR screen: password -> decrypt JWK -> extract n (base64url) -> show as QR for Vite app. */

#include "../ui.h"
#include "ui_owner_qr_screen.h"
#include "ui_Screen1.h"
#include "wallet_sd.h"
#include "wallet_encrypt.h"
#include "wallet_address.h"
#include "arweave_wallet_gen.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "owner_qr";

#define PASSWORD_MAX_LEN    64
#define OWNER_B64_MAX       720   /* base64url(512) ~ 684 + null */
#define CT_B64_SIZE         (((WALLET_GEN_JWK_MAX + 16 + 2) / 3) * 4 + 1)
#define SALT_HEX_SIZE       (WALLET_ENCRYPT_SALT_HEX_LEN + 2)
#define IV_HEX_SIZE         (WALLET_ENCRYPT_IV_HEX_LEN + 2)
#define QR_SIZE_OWNER       180  /* larger QR for ~684 chars */

#if LV_USE_QRCODE

lv_obj_t *ui_owner_qr_screen = NULL;
static lv_obj_t *s_title = NULL;
static lv_obj_t *s_pwd_ta = NULL;
static lv_obj_t *s_show_btn = NULL;
static lv_obj_t *s_qr = NULL;
static lv_obj_t *s_msg_label = NULL;
static lv_obj_t *s_home_btn = NULL;
static lv_obj_t *s_keyboard = NULL;

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void do_show_owner_qr(void)
{
	const char *pwd = s_pwd_ta ? lv_textarea_get_text(s_pwd_ta) : "";
	if (!pwd) pwd = "";
	if (pwd[0] == '\0') {
		if (s_msg_label) lv_label_set_text(s_msg_label, "Enter password");
		lv_obj_remove_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	static char salt_hex[SALT_HEX_SIZE];
	static char iv_hex[IV_HEX_SIZE];
	static char ct_b64[CT_B64_SIZE];
	if (!wallet_sd_read_encrypted_jwk(salt_hex, sizeof(salt_hex), iv_hex, sizeof(iv_hex), ct_b64, sizeof(ct_b64))) {
		if (s_msg_label) lv_label_set_text(s_msg_label, "Read failed");
		lv_obj_remove_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	static char jwk_buf[WALLET_GEN_JWK_MAX];
	int ret = wallet_decrypt_jwk(pwd, salt_hex, iv_hex, ct_b64, jwk_buf, sizeof(jwk_buf));
	if (ret != 0) {
		if (s_msg_label) lv_label_set_text(s_msg_label, "Wrong password");
		lv_obj_remove_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	static char owner_b64[OWNER_B64_MAX];
	ret = wallet_owner_b64url_from_jwk(jwk_buf, owner_b64, sizeof(owner_b64));
	if (ret != 0) {
		if (s_msg_label) lv_label_set_text(s_msg_label, "Invalid JWK");
		lv_obj_remove_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	ESP_LOGI(TAG, "owner (n) len=%u", (unsigned)strlen(owner_b64));
	if (s_msg_label) {
		lv_label_set_text(s_msg_label, "Owner (for signing) - scan in Vite app");
		lv_obj_remove_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_qr) {
		lv_qrcode_update(s_qr, owner_b64, (uint32_t)strlen(owner_b64));
		lv_obj_remove_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
	}
}

static void show_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	do_show_owner_qr();
}

static void ta_ready_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_READY) return;
	do_show_owner_qr();
}

static void pwd_click_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED && s_keyboard)
		_ui_keyboard_set_target(s_keyboard, s_pwd_ta);
}

void ui_owner_qr_screen_show(void)
{
	if (ui_owner_qr_screen == NULL)
		ui_owner_qr_screen_screen_init();
	if (s_msg_label) {
		lv_label_set_text(s_msg_label, "");
		lv_obj_add_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_qr)
		lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
	if (s_pwd_ta)
		lv_textarea_set_text(s_pwd_ta, "");
	_ui_screen_change(&ui_owner_qr_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_owner_qr_screen_screen_init(void)
{
	ui_owner_qr_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_owner_qr_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_owner_qr_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_owner_qr_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_title = lv_label_create(ui_owner_qr_screen);
	lv_obj_set_width(s_title, 240);
	lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(s_title, 6);
	lv_label_set_text(s_title, "Owner (for signing)");
	lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	lv_obj_t *pwd_lbl = lv_label_create(ui_owner_qr_screen);
	lv_obj_set_align(pwd_lbl, LV_ALIGN_CENTER);
	lv_obj_set_y(pwd_lbl, -155);
	lv_label_set_text(pwd_lbl, "Password");
	lv_obj_set_style_text_font(pwd_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_pwd_ta = lv_textarea_create(ui_owner_qr_screen);
	lv_obj_set_width(s_pwd_ta, 180);
	lv_obj_set_height(s_pwd_ta, 40);
	lv_obj_set_align(s_pwd_ta, LV_ALIGN_CENTER);
	lv_obj_set_y(s_pwd_ta, -105);
	lv_textarea_set_placeholder_text(s_pwd_ta, "Password...");
	lv_textarea_set_max_length(s_pwd_ta, PASSWORD_MAX_LEN);
	lv_textarea_set_password_mode(s_pwd_ta, true);
	lv_obj_add_event_cb(s_pwd_ta, pwd_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(s_pwd_ta, ta_ready_cb, LV_EVENT_READY, NULL);

	s_show_btn = lv_button_create(ui_owner_qr_screen);
	lv_obj_set_width(s_show_btn, 140);
	lv_obj_set_height(s_show_btn, 36);
	lv_obj_set_align(s_show_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_show_btn, -55);
	lv_obj_add_event_cb(s_show_btn, show_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *btn_lbl = lv_label_create(s_show_btn);
	lv_label_set_text(btn_lbl, "Show owner QR");
	lv_obj_center(btn_lbl);
	lv_obj_set_style_text_font(btn_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_qr = lv_qrcode_create(ui_owner_qr_screen);
	lv_qrcode_set_size(s_qr, QR_SIZE_OWNER);
	lv_qrcode_set_dark_color(s_qr, lv_color_hex(0x000000));
	lv_qrcode_set_light_color(s_qr, lv_color_hex(0xFFFFFF));
	lv_obj_set_align(s_qr, LV_ALIGN_CENTER);
	lv_obj_set_y(s_qr, 30);
	lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);

	s_msg_label = lv_label_create(ui_owner_qr_screen);
	lv_obj_set_width(s_msg_label, 260);
	lv_obj_set_align(s_msg_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_msg_label, 130);
	lv_label_set_text(s_msg_label, "");
	lv_label_set_long_mode(s_msg_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(s_msg_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_msg_label, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_flag(s_msg_label, LV_OBJ_FLAG_HIDDEN);

	s_home_btn = lv_button_create(ui_owner_qr_screen);
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

	s_keyboard = lv_keyboard_create(ui_owner_qr_screen);
	lv_obj_set_width(s_keyboard, 321);
	lv_obj_set_height(s_keyboard, 194);
	lv_obj_set_y(s_keyboard, 143);
	lv_obj_set_align(s_keyboard, LV_ALIGN_CENTER);
	lv_obj_set_style_bg_color(s_keyboard, lv_color_hex(0x8BBE6A), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_keyboard_set_textarea(s_keyboard, s_pwd_ta);
}

void ui_owner_qr_screen_screen_destroy(void)
{
	if (ui_owner_qr_screen)
		lv_obj_del(ui_owner_qr_screen);
	ui_owner_qr_screen = NULL;
	s_title = NULL;
	s_pwd_ta = NULL;
	s_show_btn = NULL;
	s_qr = NULL;
	s_msg_label = NULL;
	s_home_btn = NULL;
	s_keyboard = NULL;
}

#else

lv_obj_t *ui_owner_qr_screen = NULL;

void ui_owner_qr_screen_show(void)
{
	(void)0;
}

void ui_owner_qr_screen_screen_init(void)
{
	ui_owner_qr_screen = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(ui_owner_qr_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_t *lbl = lv_label_create(ui_owner_qr_screen);
	lv_label_set_text(lbl, "QR not available");
	lv_obj_center(lbl);
}

void ui_owner_qr_screen_screen_destroy(void)
{
	if (ui_owner_qr_screen)
		lv_obj_del(ui_owner_qr_screen);
	ui_owner_qr_screen = NULL;
}

#endif
