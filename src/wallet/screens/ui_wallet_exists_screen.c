/* Wallet exists: scrollable menu (Public key, Sign tx, etc.) + circle home. Buttons do nothing for now. */

#include "../ui.h"
#include "ui_wallet_exists_screen.h"
#include "ui_Screen2.h"
#include "ui_public_address_qr_screen.h"
#include "ui_upload_public_qr_screen.h"
#include "ui_owner_qr_screen.h"
#include "ui_export_key_screen.h"
#include "boot_mode.h"
#include "ui_sign_tx_password_screen.h"
#include "wallet_sd.h"

lv_obj_t *ui_wallet_exists_screen = NULL;
static lv_obj_t *s_menu_list = NULL;
static lv_obj_t *s_home_btn = NULL;

#define MENU_ACTION_PUBLIC_KEY     ((void *)1)
#define MENU_ACTION_UPLOAD_PUBLIC  ((void *)5)
#define MENU_ACTION_OWNER_QR       ((void *)4)
#define MENU_ACTION_SIGN_TX        ((void *)6)
#define MENU_ACTION_EXPORT_KEY     ((void *)2)
#define MENU_ACTION_DELETE         ((void *)3)

#define SIGN_TX_HASH_BUF_SIZE  256

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void menu_item_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	lv_obj_t *btn = lv_event_get_target(e);
	void *action = lv_obj_get_user_data(btn);
	if (action == MENU_ACTION_PUBLIC_KEY) {
		ui_public_address_qr_screen_show();
		return;
	}
	if (action == MENU_ACTION_UPLOAD_PUBLIC) {
		ui_upload_public_qr_screen_show();
		return;
	}
	if (action == MENU_ACTION_OWNER_QR) {
		ui_owner_qr_screen_show();
		return;
	}
	if (action == MENU_ACTION_SIGN_TX) {
		/* If we have a pending hash from scanner (temp_sig), show password/sign flow; else reboot to camera. */
		if (wallet_sd_temp_sig_exists()) {
			static char hash_buf[SIGN_TX_HASH_BUF_SIZE];
			if (wallet_sd_temp_sig_read(hash_buf, sizeof(hash_buf)))
				ui_sign_tx_password_screen_show(hash_buf);
		} else {
			boot_mode_request_scanner_and_reboot();
		}
		return;
	}
	if (action == MENU_ACTION_EXPORT_KEY) {
		ui_export_key_screen_show();
		return;
	}
	if (action == MENU_ACTION_DELETE) {
		wallet_sd_delete();
		_ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_Screen2_screen_init);
	}
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

	lv_obj_t *title = lv_label_create(ui_wallet_exists_screen);
	lv_obj_set_width(title, 240);
	lv_obj_set_height(title, LV_SIZE_CONTENT);
	lv_obj_set_align(title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(title, 8);
	lv_label_set_text(title, "Wallet");
	lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_menu_list = lv_list_create(ui_wallet_exists_screen);
	lv_obj_set_width(s_menu_list, 260);
	lv_obj_set_height(s_menu_list, 320);
	lv_obj_set_align(s_menu_list, LV_ALIGN_CENTER);
	lv_obj_set_y(s_menu_list, 10);

	static const char *items[] = {
		"Public key",
		"Upload public QR",
		"Owner (for signing)",
		"Sign tx",
		"Export key",
		"Verify message",
		"Delete wallet",
		NULL
	};
	static const void *actions[] = { MENU_ACTION_PUBLIC_KEY, MENU_ACTION_UPLOAD_PUBLIC, MENU_ACTION_OWNER_QR, MENU_ACTION_SIGN_TX, MENU_ACTION_EXPORT_KEY, NULL, MENU_ACTION_DELETE };
	for (int i = 0; items[i] != NULL; i++) {
		lv_obj_t *btn = lv_list_add_button(s_menu_list, NULL, items[i]);
		lv_obj_set_style_text_font(btn, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
		lv_obj_set_user_data(btn, (void *)actions[i]);
		lv_obj_add_event_cb(btn, menu_item_cb, LV_EVENT_CLICKED, NULL);
	}

	add_circle_home_button(ui_wallet_exists_screen);
}

void ui_wallet_exists_screen_screen_destroy(void)
{
	if (ui_wallet_exists_screen)
		lv_obj_del(ui_wallet_exists_screen);
	ui_wallet_exists_screen = NULL;
	s_menu_list = NULL;
	s_home_btn = NULL;
}
