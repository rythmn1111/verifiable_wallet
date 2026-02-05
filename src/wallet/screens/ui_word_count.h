/* Word count screen (words 1–11): ported from wallet3, LVGL 9. */

#ifndef UI_WORD_COUNT_H
#define UI_WORD_COUNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_word_count_screen_init(void);
extern void ui_word_count_screen_destroy(void);
extern void ui_word_count_set_wifi_connected(int connected);
/* Set current word index (0..10) and refresh labels; call before showing screen. */
extern void ui_word_count_set_index(int index);
extern void ui_word_count_refresh_labels(void);

extern lv_obj_t *ui_word_count;
extern lv_obj_t *ui_time6;
extern lv_obj_t *ui_wifi_enable6;
extern lv_obj_t *ui_wifi_not_enable6;
extern lv_obj_t *ui_Label12;
extern lv_obj_t *ui_word;
extern lv_obj_t *ui_back_button1;
extern lv_obj_t *ui_next_button;

#ifdef __cplusplus
}
#endif

#endif
