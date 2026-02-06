/* Sign Tx: enter password -> decrypt JWK -> sign hash -> show signature QR. */

#include "../ui.h"
#include "ui_sign_tx_password_screen.h"
#include "ui_sign_tx_signature_qr_screen.h"
#include "ui_Screen1.h"
#include "wallet_sd.h"
#include "wallet_encrypt.h"
#include "wallet_sign.h"
#include "arweave_wallet_gen.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "sign_tx_pwd";

#define PASSWORD_MAX_LEN    64
#define HASH_B64_MAX        128
#define SIG_B64_MAX         720
#define CT_B64_SIZE         (((WALLET_GEN_JWK_MAX + 16 + 2) / 3) * 4 + 1)
#define SALT_HEX_SIZE       (WALLET_ENCRYPT_SALT_HEX_LEN + 2)
#define IV_HEX_SIZE         (WALLET_ENCRYPT_IV_HEX_LEN + 2)

lv_obj_t *ui_sign_tx_password_screen = NULL;
static lv_obj_t *s_title = NULL;
static lv_obj_t *s_pwd_ta = NULL;
static lv_obj_t *s_sign_btn = NULL;
static lv_obj_t *s_msg = NULL;
static lv_obj_t *s_home_btn = NULL;
static lv_obj_t *s_keyboard = NULL;
static char s_hash_b64[HASH_B64_MAX];

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

/** Extract hash from scanned string: raw base64url or JSON {"v":1,"hash":"..."} */
static void extract_hash(const char *input, char *out_hash, size_t out_size)
{
	out_hash[0] = '\0';
	if (!input) return;
	while (*input == ' ') input++;
	if (*input == '{') {
		const char *key = "\"hash\":\"";
		const char *p = strstr(input, key);
		if (p) {
			p += strlen(key);
			const char *end = strchr(p, '"');
			if (end && (size_t)(end - p) < out_size) {
				size_t len = (size_t)(end - p);
				memcpy(out_hash, p, len);
				out_hash[len] = '\0';
				return;
			}
		}
	}
	strncpy(out_hash, input, out_size - 1);
	out_hash[out_size - 1] = '\0';
}

static void do_sign(void)
{
	const char *pwd = s_pwd_ta ? lv_textarea_get_text(s_pwd_ta) : "";
	if (!pwd) pwd = "";
	if (pwd[0] == '\0') {
		if (s_msg) { lv_label_set_text(s_msg, "Enter password"); lv_obj_remove_flag(s_msg, LV_OBJ_FLAG_HIDDEN); }
		return;
	}

	static char salt_hex[SALT_HEX_SIZE], iv_hex[IV_HEX_SIZE], ct_b64[CT_B64_SIZE];
	if (!wallet_sd_read_encrypted_jwk(salt_hex, sizeof(salt_hex), iv_hex, sizeof(iv_hex), ct_b64, sizeof(ct_b64))) {
		if (s_msg) { lv_label_set_text(s_msg, "Read wallet failed"); lv_obj_remove_flag(s_msg, LV_OBJ_FLAG_HIDDEN); }
		return;
	}

	static char jwk_buf[WALLET_GEN_JWK_MAX];
	int ret = wallet_decrypt_jwk(pwd, salt_hex, iv_hex, ct_b64, jwk_buf, sizeof(jwk_buf));
	if (ret != 0) {
		if (s_msg) { lv_label_set_text(s_msg, "Wrong password"); lv_obj_remove_flag(s_msg, LV_OBJ_FLAG_HIDDEN); }
		return;
	}

	static char sig_b64[SIG_B64_MAX];
	ret = wallet_sign_hash_from_jwk(jwk_buf, s_hash_b64, sig_b64, sizeof(sig_b64));
	if (ret != 0) {
		if (s_msg) { lv_label_set_text(s_msg, "Sign failed"); lv_obj_remove_flag(s_msg, LV_OBJ_FLAG_HIDDEN); }
		return;
	}

	ESP_LOGI(TAG, "signed ok");
	ui_sign_tx_signature_qr_screen_show(sig_b64);
}

static void sign_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	do_sign();
}

static void ta_ready_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_READY) return;
	do_sign();
}

static void pwd_click_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	if (s_keyboard && s_pwd_ta) {
		_ui_keyboard_set_target(s_keyboard, s_pwd_ta);
		lv_obj_remove_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
	}
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

void ui_sign_tx_password_screen_show(const char *hash_str)
{
	if (ui_sign_tx_password_screen == NULL)
		ui_sign_tx_password_screen_screen_init();

	extract_hash(hash_str, s_hash_b64, sizeof(s_hash_b64));
	if (s_pwd_ta) lv_textarea_set_text(s_pwd_ta, "");
	if (s_msg) { lv_label_set_text(s_msg, "Enter password to sign"); lv_obj_add_flag(s_msg, LV_OBJ_FLAG_HIDDEN); }

	_ui_screen_change(&ui_sign_tx_password_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_sign_tx_password_screen_screen_init(void)
{
	ui_sign_tx_password_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_sign_tx_password_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_sign_tx_password_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_sign_tx_password_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_title = lv_label_create(ui_sign_tx_password_screen);
	lv_obj_set_width(s_title, 260);
	lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(s_title, 8);
	lv_label_set_text(s_title, "Sign Tx - Password");
	lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	lv_obj_t *pwd_lbl = lv_label_create(ui_sign_tx_password_screen);
	lv_obj_set_align(pwd_lbl, LV_ALIGN_TOP_LEFT);
	lv_obj_set_x(pwd_lbl, 20);
	lv_obj_set_y(pwd_lbl, 45);
	lv_label_set_text(pwd_lbl, "Password:");
	lv_obj_set_style_text_font(pwd_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_pwd_ta = lv_textarea_create(ui_sign_tx_password_screen);
	lv_obj_set_width(s_pwd_ta, 200);
	lv_obj_set_height(s_pwd_ta, 40);
	lv_obj_set_align(s_pwd_ta, LV_ALIGN_TOP_LEFT);
	lv_obj_set_x(s_pwd_ta, 20);
	lv_obj_set_y(s_pwd_ta, 68);
	lv_textarea_set_password_mode(s_pwd_ta, true);
	lv_textarea_set_one_line(s_pwd_ta, true);
	lv_textarea_set_max_length(s_pwd_ta, PASSWORD_MAX_LEN);
	lv_obj_add_event_cb(s_pwd_ta, pwd_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(s_pwd_ta, ta_ready_cb, LV_EVENT_READY, NULL);

	s_sign_btn = lv_button_create(ui_sign_tx_password_screen);
	lv_obj_set_width(s_sign_btn, 140);
	lv_obj_set_height(s_sign_btn, 44);
	lv_obj_set_align(s_sign_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_sign_btn, 20);
	lv_obj_add_event_cb(s_sign_btn, sign_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *sign_lbl = lv_label_create(s_sign_btn);
	lv_label_set_text(sign_lbl, "Sign");
	lv_obj_center(sign_lbl);
	lv_obj_set_style_text_font(sign_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_msg = lv_label_create(ui_sign_tx_password_screen);
	lv_obj_set_width(s_msg, 260);
	lv_obj_set_align(s_msg, LV_ALIGN_CENTER);
	lv_obj_set_y(s_msg, 75);
	lv_label_set_text(s_msg, "");
	lv_obj_set_style_text_align(s_msg, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_msg, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_flag(s_msg, LV_OBJ_FLAG_HIDDEN);

	s_keyboard = lv_keyboard_create(ui_sign_tx_password_screen);
	lv_obj_set_width(s_keyboard, 321);
	lv_obj_set_height(s_keyboard, 194);
	lv_obj_set_align(s_keyboard, LV_ALIGN_BOTTOM_MID);
	lv_keyboard_set_textarea(s_keyboard, s_pwd_ta);
	lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);

	add_circle_home(ui_sign_tx_password_screen);
}

void ui_sign_tx_password_screen_screen_destroy(void)
{
	if (ui_sign_tx_password_screen)
		lv_obj_del(ui_sign_tx_password_screen);
	ui_sign_tx_password_screen = NULL;
	s_title = NULL;
	s_pwd_ta = NULL;
	s_sign_btn = NULL;
	s_msg = NULL;
	s_home_btn = NULL;
	s_keyboard = NULL;
}
