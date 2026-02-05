// Note-for-words screen (ported from wallet3): intro before showing 12 words

#ifndef UI_NOTE_FOR_WORDS_SCREEN_H
#define UI_NOTE_FOR_WORDS_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_note_for_words_screen_screen_init(void);
extern void ui_note_for_words_screen_screen_destroy(void);
extern lv_obj_t *ui_note_for_words_screen;
extern lv_obj_t *ui_time5;
extern lv_obj_t *ui_wifi_enable5;
extern lv_obj_t *ui_wifi_not_enable5;
extern lv_obj_t *ui_Note;
extern lv_obj_t *ui_note_text;
extern lv_obj_t *ui_lets_go_button;

#ifdef __cplusplus
}
#endif

#endif
