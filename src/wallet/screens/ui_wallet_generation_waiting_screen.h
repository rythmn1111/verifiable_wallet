// From SquareLine Studio wallet3 - wallet generation waiting screen

#ifndef UI_WALLET_GENERATION_WAITING_SCREEN_H
#define UI_WALLET_GENERATION_WAITING_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_wallet_generation_waiting_screen_screen_init(void);
extern void ui_wallet_generation_waiting_screen_screen_destroy(void);
extern lv_obj_t *ui_wallet_generation_waiting_screen;

/** Show waiting screen and start background generation. When done, serial gets words+JWK and screen shows "Wallet generated!" + Done. */
void ui_wallet_generation_waiting_screen_start(void);

#ifdef __cplusplus
}
#endif

#endif
