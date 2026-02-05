// New note for word count screen (from wallet3): no transform_scale, safe for ESP32

#ifndef UI_NEW_NOTE_FOR_WORD_COUNT_H
#define UI_NEW_NOTE_FOR_WORD_COUNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_new_note_for_word_count_screen_init(void);
extern void ui_new_note_for_word_count_screen_destroy(void);
extern void ui_new_note_for_word_count_set_wifi_connected(int connected);
extern lv_obj_t *ui_new_note_for_word_count;
extern lv_obj_t *ui_time8;
extern lv_obj_t *ui_wifi_enable8;
extern lv_obj_t *ui_wifi_not_enable8;
extern lv_obj_t *ui_Label10;
extern lv_obj_t *ui_Label11;
extern lv_obj_t *ui_ImgButton8;

#ifdef __cplusplus
}
#endif

#endif
