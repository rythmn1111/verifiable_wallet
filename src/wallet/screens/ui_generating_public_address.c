/* Waiting screen: "Generating public address" then Continue -> note_for_password. */

#include "../ui.h"
#include "ui_generating_public_address.h"
#include "ui_note_for_password.h"
#include "arweave_wallet_gen.h"
#include "wallet_jwk_pending.h"
#include "wallet_address.h"
#include "wallet_address_pending.h"
#include <string.h>

lv_obj_t *ui_generating_public_address = NULL;
static lv_obj_t *s_msg_label = NULL;
static lv_obj_t *s_continue_btn = NULL;

static void continue_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	_ui_screen_change(&ui_note_for_password, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_note_for_password_screen_init);
}

void ui_generating_public_address_show(void)
{
	if (ui_generating_public_address == NULL)
		ui_generating_public_address_screen_init();

	/* Compute Arweave address from pending JWK */
	if (wallet_jwk_pending_has()) {
		static char jwk_buf[WALLET_GEN_JWK_MAX];
		static char addr_buf[WALLET_ARWEAVE_ADDRESS_LEN + 1];
		size_t len = wallet_jwk_pending_get(jwk_buf, sizeof(jwk_buf));
		if (len > 0) {
			jwk_buf[len] = '\0';
			if (wallet_address_from_jwk(jwk_buf, addr_buf, sizeof(addr_buf)) == 0)
				wallet_address_pending_set(addr_buf);
		}
	}

	if (s_msg_label)
		lv_label_set_text(s_msg_label, wallet_address_pending_has() ? "Public address ready.\nTap Continue." : "Generating public address...");
	if (s_continue_btn)
		lv_obj_remove_flag(s_continue_btn, LV_OBJ_FLAG_HIDDEN);

	_ui_screen_change(&ui_generating_public_address, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_generating_public_address_screen_init(void)
{
	ui_generating_public_address = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_generating_public_address, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_generating_public_address, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_generating_public_address, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_msg_label = lv_label_create(ui_generating_public_address);
	lv_obj_set_width(s_msg_label, 260);
	lv_obj_set_height(s_msg_label, LV_SIZE_CONTENT);
	lv_obj_set_align(s_msg_label, LV_ALIGN_CENTER);
	lv_obj_set_y(s_msg_label, -30);
	lv_label_set_text(s_msg_label, "Generating public address...");
	lv_obj_set_style_text_align(s_msg_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_msg_label, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_continue_btn = lv_button_create(ui_generating_public_address);
	lv_obj_set_width(s_continue_btn, 160);
	lv_obj_set_height(s_continue_btn, 48);
	lv_obj_set_align(s_continue_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_continue_btn, 50);
	lv_obj_add_event_cb(s_continue_btn, continue_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_flag(s_continue_btn, LV_OBJ_FLAG_HIDDEN);
	lv_obj_t *lbl = lv_label_create(s_continue_btn);
	lv_label_set_text(lbl, "Continue");
	lv_obj_center(lbl);
	lv_obj_set_style_text_font(lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
}

void ui_generating_public_address_screen_destroy(void)
{
	if (ui_generating_public_address)
		lv_obj_del(ui_generating_public_address);
	ui_generating_public_address = NULL;
	s_msg_label = NULL;
	s_continue_btn = NULL;
}
