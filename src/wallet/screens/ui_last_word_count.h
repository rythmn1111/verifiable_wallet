/* Last word count screen (word 12): ported from wallet3, LVGL 9. */

#ifndef UI_LAST_WORD_COUNT_H
#define UI_LAST_WORD_COUNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern void ui_last_word_count_screen_init(void);
extern void ui_last_word_count_screen_destroy(void);
extern void ui_last_word_count_set_wifi_connected(int connected);
/* Refresh the 12th word label from storage; call before showing this screen. */
extern void ui_last_word_count_refresh_word(void);

extern lv_obj_t *ui_last_word_count;
extern lv_obj_t *ui_time1;
extern lv_obj_t *ui_wifi_enable1;
extern lv_obj_t *ui_wifi_not_enable1;
extern lv_obj_t *ui_Label1;
extern lv_obj_t *ui_word1;
extern lv_obj_t *ui_back_button;
extern lv_obj_t *ui_finish_button;

#ifdef __cplusplus
}
#endif

#endif
