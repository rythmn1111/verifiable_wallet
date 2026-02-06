/* Upload public QR: show saved 512-byte owner (owner.txt) as QR, no password. */

#ifndef UI_UPLOAD_PUBLIC_QR_SCREEN_H
#define UI_UPLOAD_PUBLIC_QR_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_upload_public_qr_screen;

void ui_upload_public_qr_screen_screen_init(void);
void ui_upload_public_qr_screen_screen_destroy(void);

/** Show the screen: load owner from SD (owner.txt), display QR; no password. */
void ui_upload_public_qr_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
