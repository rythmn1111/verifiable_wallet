/* Password for encryption: two fields (password + confirm), match then encrypt JWK and save. */

#include "../ui.h"
#include "ui_password_for_encryption.h"
#include "ui_Screen1.h"
#include "ui_note_for_password.h"
#include "wallet_jwk_pending.h"
#include "wallet_encrypt.h"
#include "wallet_sd.h"
#include "arweave_wallet_gen.h"
#include <string.h>
#include <stdio.h>

#define PASSWORD_MAX_LEN  64
#define CT_B64_SIZE       (((WALLET_GEN_JWK_MAX + 16 + 2) / 3) * 4 + 1)

lv_obj_t *ui_password_for_encryption = NULL;
lv_obj_t *ui_input_password_label_enc = NULL;
lv_obj_t *ui_password_box_enc = NULL;
lv_obj_t *ui_confirm_password_label = NULL;
lv_obj_t *ui_confirm_password_box = NULL;
lv_obj_t *ui_Keyboard_enc = NULL;
lv_obj_t *ui_submit_btn_enc = NULL;
lv_obj_t *ui_msg_label_enc = NULL;

static void pwd_box_enc_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
		_ui_keyboard_set_target(ui_Keyboard_enc, ui_password_box_enc);
}

static void confirm_box_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) == LV_EVENT_CLICKED)
		_ui_keyboard_set_target(ui_Keyboard_enc, ui_confirm_password_box);
}

static void show_msg(const char *msg)
{
	if (ui_msg_label_enc) {
		lv_label_set_text(ui_msg_label_enc, msg);
		lv_obj_remove_flag(ui_msg_label_enc, LV_OBJ_FLAG_HIDDEN);
	}
}

void ui_password_for_encryption_clear_inputs(void)
{
	if (ui_password_box_enc)
		lv_textarea_set_text(ui_password_box_enc, "");
	if (ui_confirm_password_box)
		lv_textarea_set_text(ui_confirm_password_box, "");
	if (ui_msg_label_enc) {
		lv_label_set_text(ui_msg_label_enc, "");
		lv_obj_add_flag(ui_msg_label_enc, LV_OBJ_FLAG_HIDDEN);
	}
}

static void submit_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

	const char *p1 = lv_textarea_get_text(ui_password_box_enc);
	const char *p2 = lv_textarea_get_text(ui_confirm_password_box);
	if (!p1) p1 = "";
	if (!p2) p2 = "";

	if (strcmp(p1, p2) != 0) {
		show_msg("Passwords do not match");
		return;
	}
	if (p1[0] == '\0') {
		show_msg("Enter a password");
		return;
	}

	if (!wallet_jwk_pending_has()) {
		show_msg("No wallet to encrypt");
		return;
	}

	static char jwk_buf[WALLET_GEN_JWK_MAX];
	static char salt_hex[WALLET_ENCRYPT_SALT_HEX_LEN + 1];
	static char iv_hex[WALLET_ENCRYPT_IV_HEX_LEN + 1];
	static char ct_b64[CT_B64_SIZE];

	size_t len = wallet_jwk_pending_get(jwk_buf, sizeof(jwk_buf));
	if (len == 0) {
		show_msg("No wallet data");
		return;
	}
	jwk_buf[len] = '\0';

	int ret = wallet_encrypt_jwk(p1, jwk_buf, salt_hex, iv_hex, ct_b64, sizeof(ct_b64));
	if (ret != 0) {
		show_msg("Encrypt failed");
		return;
	}

	if (!wallet_sd_save_encrypted_jwk(salt_hex, iv_hex, ct_b64)) {
		show_msg("Save failed");
		return;
	}

	wallet_jwk_pending_clear();
	show_msg("Saved!");
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_Screen1_screen_init);
}

void ui_password_for_encryption_screen_init(void)
{
	ui_password_for_encryption = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_password_for_encryption, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_password_for_encryption, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_password_for_encryption, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_input_password_label_enc = lv_label_create(ui_password_for_encryption);
	lv_obj_set_width(ui_input_password_label_enc, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_input_password_label_enc, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_input_password_label_enc, -3);
	lv_obj_set_y(ui_input_password_label_enc, -175);
	lv_obj_set_align(ui_input_password_label_enc, LV_ALIGN_CENTER);
	lv_label_set_text(ui_input_password_label_enc, "Input password");
	lv_obj_set_style_text_align(ui_input_password_label_enc, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_decor(ui_input_password_label_enc, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_input_password_label_enc, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_password_box_enc = lv_textarea_create(ui_password_for_encryption);
	lv_obj_set_width(ui_password_box_enc, 212);
	lv_obj_set_height(ui_password_box_enc, 50);
	lv_obj_set_x(ui_password_box_enc, 0);
	lv_obj_set_y(ui_password_box_enc, -120);
	lv_obj_set_align(ui_password_box_enc, LV_ALIGN_CENTER);
	lv_textarea_set_placeholder_text(ui_password_box_enc, "Password...");
	lv_textarea_set_max_length(ui_password_box_enc, PASSWORD_MAX_LEN);
	lv_textarea_set_password_mode(ui_password_box_enc, true);
	lv_obj_add_event_cb(ui_password_box_enc, pwd_box_enc_cb, LV_EVENT_CLICKED, NULL);

	ui_confirm_password_label = lv_label_create(ui_password_for_encryption);
	lv_obj_set_width(ui_confirm_password_label, LV_SIZE_CONTENT);
	lv_obj_set_height(ui_confirm_password_label, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_confirm_password_label, -3);
	lv_obj_set_y(ui_confirm_password_label, -55);
	lv_obj_set_align(ui_confirm_password_label, LV_ALIGN_CENTER);
	lv_label_set_text(ui_confirm_password_label, "Confirm password");
	lv_obj_set_style_text_align(ui_confirm_password_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_decor(ui_confirm_password_label, LV_TEXT_DECOR_UNDERLINE, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_confirm_password_label, &ui_font_pixel_heading, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_confirm_password_box = lv_textarea_create(ui_password_for_encryption);
	lv_obj_set_width(ui_confirm_password_box, 212);
	lv_obj_set_height(ui_confirm_password_box, 50);
	lv_obj_set_x(ui_confirm_password_box, 0);
	lv_obj_set_y(ui_confirm_password_box, 0);
	lv_obj_set_align(ui_confirm_password_box, LV_ALIGN_CENTER);
	lv_textarea_set_placeholder_text(ui_confirm_password_box, "Confirm...");
	lv_textarea_set_max_length(ui_confirm_password_box, PASSWORD_MAX_LEN);
	lv_textarea_set_password_mode(ui_confirm_password_box, true);
	lv_obj_add_event_cb(ui_confirm_password_box, confirm_box_cb, LV_EVENT_CLICKED, NULL);

	ui_msg_label_enc = lv_label_create(ui_password_for_encryption);
	lv_obj_set_width(ui_msg_label_enc, 280);
	lv_obj_set_height(ui_msg_label_enc, LV_SIZE_CONTENT);
	lv_obj_set_x(ui_msg_label_enc, 0);
	lv_obj_set_y(ui_msg_label_enc, 55);
	lv_obj_set_align(ui_msg_label_enc, LV_ALIGN_CENTER);
	lv_label_set_text(ui_msg_label_enc, "");
	lv_obj_set_style_text_align(ui_msg_label_enc, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(ui_msg_label_enc, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_flag(ui_msg_label_enc, LV_OBJ_FLAG_HIDDEN);

	ui_submit_btn_enc = lv_button_create(ui_password_for_encryption);
	lv_obj_set_width(ui_submit_btn_enc, 140);
	lv_obj_set_height(ui_submit_btn_enc, 44);
	lv_obj_set_x(ui_submit_btn_enc, 0);
	lv_obj_set_y(ui_submit_btn_enc, 95);
	lv_obj_set_align(ui_submit_btn_enc, LV_ALIGN_CENTER);
	lv_obj_add_event_cb(ui_submit_btn_enc, submit_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *btn_lbl = lv_label_create(ui_submit_btn_enc);
	lv_label_set_text(btn_lbl, "Submit");
	lv_obj_center(btn_lbl);
	lv_obj_set_style_text_font(btn_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	ui_Keyboard_enc = lv_keyboard_create(ui_password_for_encryption);
	lv_obj_set_width(ui_Keyboard_enc, 321);
	lv_obj_set_height(ui_Keyboard_enc, 194);
	lv_obj_set_x(ui_Keyboard_enc, 0);
	lv_obj_set_y(ui_Keyboard_enc, 143);
	lv_obj_set_align(ui_Keyboard_enc, LV_ALIGN_CENTER);
	lv_obj_set_style_bg_color(ui_Keyboard_enc, lv_color_hex(0x8BBE6A), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_Keyboard_enc, 255, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_keyboard_set_textarea(ui_Keyboard_enc, ui_password_box_enc);
}

void ui_password_for_encryption_screen_destroy(void)
{
	if (ui_password_for_encryption)
		lv_obj_del(ui_password_for_encryption);
	ui_password_for_encryption = NULL;
	ui_input_password_label_enc = NULL;
	ui_password_box_enc = NULL;
	ui_confirm_password_label = NULL;
	ui_confirm_password_box = NULL;
	ui_Keyboard_enc = NULL;
	ui_submit_btn_enc = NULL;
	ui_msg_label_enc = NULL;
}
