/* Public address QR screen: QR code of 43-char address + label; circle home. */

#include "../ui.h"
#include "ui_public_address_qr_screen.h"
#include "wallet_sd.h"
#include <string.h>

#if LV_USE_QRCODE

lv_obj_t *ui_public_address_qr_screen = NULL;
static lv_obj_t *s_title = NULL;
static lv_obj_t *s_qr = NULL;
static lv_obj_t *s_address_label = NULL;
static lv_obj_t *s_home_btn = NULL;
static lv_obj_t *s_error_label = NULL;

#define ADDRESS_BUF_SIZE  48
#define QR_SIZE           140

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
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

void ui_public_address_qr_screen_show(void)
{
	if (ui_public_address_qr_screen == NULL)
		ui_public_address_qr_screen_screen_init();

	static char addr_buf[ADDRESS_BUF_SIZE];
	bool ok = wallet_sd_get_public_address(addr_buf, sizeof(addr_buf));

	if (s_error_label) {
		if (ok)
			lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
		else {
			lv_label_set_text(s_error_label, "Address not available");
			lv_obj_remove_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
		}
	}
	if (s_address_label) {
		if (ok) {
			lv_label_set_text(s_address_label, addr_buf);
			lv_obj_remove_flag(s_address_label, LV_OBJ_FLAG_HIDDEN);
		} else
			lv_obj_add_flag(s_address_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (s_qr) {
		if (ok) {
			lv_qrcode_update(s_qr, addr_buf, (uint32_t)strlen(addr_buf));
			lv_obj_remove_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
		} else
			lv_obj_add_flag(s_qr, LV_OBJ_FLAG_HIDDEN);
	}

	_ui_screen_change(&ui_public_address_qr_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_public_address_qr_screen_screen_init(void)
{
	ui_public_address_qr_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_public_address_qr_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_public_address_qr_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_public_address_qr_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_title = lv_label_create(ui_public_address_qr_screen);
	lv_obj_set_width(s_title, 240);
	lv_obj_set_height(s_title, LV_SIZE_CONTENT);
	lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(s_title, 6);
	lv_label_set_text(s_title, "Public address");
	lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_qr = lv_qrcode_create(ui_public_address_qr_screen);
	lv_qrcode_set_size(s_qr, QR_SIZE);
	lv_qrcode_set_dark_color(s_qr, lv_color_hex(0x000000));
	lv_qrcode_set_light_color(s_qr, lv_color_hex(0xFFFFFF));
	lv_obj_set_align(s_qr, LV_ALIGN_CENTER);
	lv_obj_set_y(s_qr, -15);

	s_address_label = lv_label_create(ui_public_address_qr_screen);
	lv_obj_set_width(s_address_label, 260);
	lv_obj_set_height(s_address_label, LV_SIZE_CONTENT);
	lv_obj_set_align(s_address_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_address_label, 75);
	lv_label_set_text(s_address_label, "");
	lv_obj_set_style_text_align(s_address_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_address_label, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_error_label = lv_label_create(ui_public_address_qr_screen);
	lv_obj_set_width(s_error_label, 260);
	lv_obj_set_height(s_error_label, LV_SIZE_CONTENT);
	lv_obj_set_align(s_error_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_error_label, 0);
	lv_label_set_text(s_error_label, "Address not available");
	lv_obj_set_style_text_align(s_error_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_error_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);

	add_circle_home_button(ui_public_address_qr_screen);
}

void ui_public_address_qr_screen_screen_destroy(void)
{
	if (ui_public_address_qr_screen)
		lv_obj_del(ui_public_address_qr_screen);
	ui_public_address_qr_screen = NULL;
	s_title = NULL;
	s_qr = NULL;
	s_address_label = NULL;
	s_home_btn = NULL;
	s_error_label = NULL;
}

#else /* !LV_USE_QRCODE */

lv_obj_t *ui_public_address_qr_screen = NULL;
static lv_obj_t *s_stub_home_btn = NULL;

static void stub_home_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

void ui_public_address_qr_screen_show(void)
{
	if (ui_public_address_qr_screen == NULL)
		ui_public_address_qr_screen_screen_init();
	_ui_screen_change(&ui_public_address_qr_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_public_address_qr_screen_screen_init(void)
{
	ui_public_address_qr_screen = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(ui_public_address_qr_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_t *lbl = lv_label_create(ui_public_address_qr_screen);
	lv_label_set_text(lbl, "QR not available");
	lv_obj_center(lbl);
	s_stub_home_btn = lv_button_create(ui_public_address_qr_screen);
	lv_obj_set_width(s_stub_home_btn, 49);
	lv_obj_set_height(s_stub_home_btn, 47);
	lv_obj_set_y(s_stub_home_btn, 205);
	lv_obj_set_align(s_stub_home_btn, LV_ALIGN_CENTER);
	lv_obj_set_style_radius(s_stub_home_btn, 1000, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_color(s_stub_home_btn, lv_color_hex(0xFFFFFF), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_event_cb(s_stub_home_btn, stub_home_cb, LV_EVENT_CLICKED, NULL);
}

void ui_public_address_qr_screen_screen_destroy(void)
{
	if (ui_public_address_qr_screen)
		lv_obj_del(ui_public_address_qr_screen);
	ui_public_address_qr_screen = NULL;
	s_stub_home_btn = NULL;
}

#endif /* LV_USE_QRCODE */
