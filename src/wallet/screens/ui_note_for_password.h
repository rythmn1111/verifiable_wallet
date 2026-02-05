/* Note for password screen (from wallet3): LVGL 9, no transform_scale. */

#ifndef UI_NOTE_FOR_PASSWORD_H
#define UI_NOTE_FOR_PASSWORD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_note_for_password_screen_init(void);
extern void ui_note_for_password_screen_destroy(void);
extern void ui_note_for_password_set_wifi_connected(int connected);

extern lv_obj_t *ui_note_for_password;
extern lv_obj_t *ui_time5_pwd;
extern lv_obj_t *ui_wifi_enable5_pwd;
extern lv_obj_t *ui_wifi_not_enable5_pwd;
extern lv_obj_t *ui_Label2;
extern lv_obj_t *ui_note_;
extern lv_obj_t *ui_ImgButton1;

#ifdef __cplusplus
}
#endif

#endif
