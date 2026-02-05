/* Waiting screen: "Generating public address" then Continue -> note_for_password. */

#ifndef UI_GENERATING_PUBLIC_ADDRESS_H
#define UI_GENERATING_PUBLIC_ADDRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_generating_public_address;

void ui_generating_public_address_screen_init(void);
void ui_generating_public_address_screen_destroy(void);

/** Show the screen, compute address from pending JWK, then show Continue. */
void ui_generating_public_address_show(void);

#ifdef __cplusplus
}
#endif

#endif
