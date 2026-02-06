#ifndef UI_SIGN_TX_PASSWORD_SCREEN_H
#define UI_SIGN_TX_PASSWORD_SCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_sign_tx_password_screen;

void ui_sign_tx_password_screen_screen_init(void);
void ui_sign_tx_password_screen_screen_destroy(void);
/** Show password screen to sign the given hash (base64url or JSON {"v":1,"hash":"..."}). */
void ui_sign_tx_password_screen_show(const char *hash_str);

#ifdef __cplusplus
}
#endif

#endif
