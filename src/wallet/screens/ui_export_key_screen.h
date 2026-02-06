/**
 * Export key screen (DEV ONLY - remove in production).
 * Asks for encryption password, decrypts stored JWK, shows keyfile preview.
 */

#ifndef UI_EXPORT_KEY_SCREEN_H
#define UI_EXPORT_KEY_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_export_key_screen;

void ui_export_key_screen_screen_init(void);
void ui_export_key_screen_screen_destroy(void);

void ui_export_key_screen_show(void);

#ifdef __cplusplus
}
#endif

#endif
