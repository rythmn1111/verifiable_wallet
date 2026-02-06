/* Owner (512-byte) QR screen: password -> decrypt -> show JWK "n" as QR for Vite app signing. */

#ifndef UI_OWNER_QR_SCREEN_H
#define UI_OWNER_QR_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_owner_qr_screen;

void ui_owner_qr_screen_screen_init(void);
void ui_owner_qr_screen_screen_destroy(void);

/** Show the screen: password + "Show owner QR" -> decrypt, extract n, display QR. */
void ui_owner_qr_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
