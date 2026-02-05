// Wallet already exists screen: message + Back to Screen2

#ifndef UI_WALLET_EXISTS_SCREEN_H
#define UI_WALLET_EXISTS_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_wallet_exists_screen_screen_init(void);
extern void ui_wallet_exists_screen_screen_destroy(void);
extern lv_obj_t *ui_wallet_exists_screen;

/** Show the "wallet already exists" screen (call when wallet_sd_exists() is true). */
void ui_wallet_exists_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
