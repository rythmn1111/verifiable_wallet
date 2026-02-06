/* Sign Tx: show signature (base64url) as big QR for website to scan. */

#include "../ui.h"
#include "ui_sign_tx_signature_qr_screen.h"
#include "ui_Screen1.h"
#include "ui_helpers.h"
#include "wallet_sd.h"
#include <string.h>

#if LV_USE_QRCODE

#define SIG_BUF_SIZE  720
#define QR_SIZE       320

lv_obj_t *ui_sign_tx_signature_qr_screen = NULL;
static lv_obj_t *s_qr = NULL;
static lv_obj_t *s_home_btn = NULL;

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	/* Sign flow done: remove temp_sig so next Sign Tx goes to camera. */
	wallet_sd_temp_sig_delete();
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

void ui_sign_tx_signature_qr_screen_show(const char *signature_b64url)
{
	if (ui_sign_tx_signature_qr_screen == NULL)
		ui_sign_tx_signature_qr_screen_screen_init();

	if (signature_b64url && s_qr) {
		size_t len = strnlen(signature_b64url, SIG_BUF_SIZE - 1);
		lv_qrcode_update(s_qr, signature_b64url, (uint32_t)len);
	}

	_ui_screen_change(&ui_sign_tx_signature_qr_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_sign_tx_signature_qr_screen_screen_init(void)
{
	ui_sign_tx_signature_qr_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_sign_tx_signature_qr_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_sign_tx_signature_qr_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_sign_tx_signature_qr_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_qr = lv_qrcode_create(ui_sign_tx_signature_qr_screen);
	lv_qrcode_set_size(s_qr, QR_SIZE);
	lv_qrcode_set_dark_color(s_qr, lv_color_hex(0x000000));
	lv_qrcode_set_light_color(s_qr, lv_color_hex(0xFFFFFF));
	lv_obj_set_align(s_qr, LV_ALIGN_CENTER);
	lv_obj_set_y(s_qr, -25);

	add_circle_home(ui_sign_tx_signature_qr_screen);
}

void ui_sign_tx_signature_qr_screen_screen_destroy(void)
{
	if (ui_sign_tx_signature_qr_screen)
		lv_obj_del(ui_sign_tx_signature_qr_screen);
	ui_sign_tx_signature_qr_screen = NULL;
	s_qr = NULL;
	s_home_btn = NULL;
}

#else

lv_obj_t *ui_sign_tx_signature_qr_screen = NULL;

void ui_sign_tx_signature_qr_screen_show(const char *signature_b64url) { (void)signature_b64url; }
void ui_sign_tx_signature_qr_screen_screen_init(void) {
	ui_sign_tx_signature_qr_screen = lv_obj_create(NULL);
}
void ui_sign_tx_signature_qr_screen_screen_destroy(void) {
	if (ui_sign_tx_signature_qr_screen) lv_obj_del(ui_sign_tx_signature_qr_screen);
	ui_sign_tx_signature_qr_screen = NULL;
}

#endif
