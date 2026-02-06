#ifndef UI_SIGN_TX_SCAN_SCREEN_H
#define UI_SIGN_TX_SCAN_SCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_sign_tx_scan_screen;

void ui_sign_tx_scan_screen_screen_init(void);
void ui_sign_tx_scan_screen_screen_destroy(void);
void ui_sign_tx_scan_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
