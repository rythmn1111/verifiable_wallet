#ifndef UI_WALLET_GEN_SCREEN_H
#define UI_WALLET_GEN_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_wallet_gen_screen;

void ui_wallet_gen_screen_screen_init(void);
void ui_wallet_gen_screen_screen_destroy(void);

/** Show "Generating wallet..." and start background generation. When done, serial gets words+JWK and screen shows "Wallet generated!". */
void ui_wallet_gen_screen_start(void);

#ifdef __cplusplus
}
#endif

#endif
