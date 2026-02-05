/* Password for encryption screen: input + confirm password, then encrypt JWK and save. */

#ifndef UI_PASSWORD_FOR_ENCRYPTION_H
#define UI_PASSWORD_FOR_ENCRYPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_password_for_encryption_screen_init(void);
extern void ui_password_for_encryption_screen_destroy(void);
/** Clear password/confirm fields and message; call when showing screen. */
extern void ui_password_for_encryption_clear_inputs(void);

extern lv_obj_t *ui_password_for_encryption;
extern lv_obj_t *ui_input_password_label_enc;
extern lv_obj_t *ui_password_box_enc;
extern lv_obj_t *ui_confirm_password_label;
extern lv_obj_t *ui_confirm_password_box;
extern lv_obj_t *ui_Keyboard_enc;
extern lv_obj_t *ui_submit_btn_enc;
extern lv_obj_t *ui_msg_label_enc;

#ifdef __cplusplus
}
#endif

#endif
