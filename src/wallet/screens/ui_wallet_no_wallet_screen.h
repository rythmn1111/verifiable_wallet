/* No wallet screen: option to generate with "+" button; circle home. */

#ifndef UI_WALLET_NO_WALLET_SCREEN_H
#define UI_WALLET_NO_WALLET_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_wallet_no_wallet_screen;

void ui_wallet_no_wallet_screen_screen_init(void);
void ui_wallet_no_wallet_screen_screen_destroy(void);

/** Show the "no wallet" screen (call when wallet_sd_exists() is false). */
void ui_wallet_no_wallet_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
