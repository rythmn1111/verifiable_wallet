#ifndef UI_SIGN_TX_SIGNATURE_QR_SCREEN_H
#define UI_SIGN_TX_SIGNATURE_QR_SCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_sign_tx_signature_qr_screen;

void ui_sign_tx_signature_qr_screen_screen_init(void);
void ui_sign_tx_signature_qr_screen_screen_destroy(void);
/** Show signature (base64url) as big QR. */
void ui_sign_tx_signature_qr_screen_show(const char *signature_b64url);

#ifdef __cplusplus
}
#endif

#endif
