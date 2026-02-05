/* Public address QR screen: show 43-char address as QR + optional text; circle home. */

#ifndef UI_PUBLIC_ADDRESS_QR_SCREEN_H
#define UI_PUBLIC_ADDRESS_QR_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_public_address_qr_screen;

void ui_public_address_qr_screen_screen_init(void);
void ui_public_address_qr_screen_screen_destroy(void);

/** Show the screen: load address from SD, display QR + text; circle home goes to Screen1. */
void ui_public_address_qr_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
