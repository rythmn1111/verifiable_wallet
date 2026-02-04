// Wi-Fi password screen (from SquareLine Studio wallet3)

#ifndef UI_WIFI_PASSWORD_SCREEN_H
#define UI_WIFI_PASSWORD_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_wifi_password_screen_screen_init(void);
extern void ui_wifi_password_screen_screen_destroy(void);
extern lv_obj_t *ui_wifi_password_screen;
extern lv_obj_t *ui_input_password_label;
extern lv_obj_t *ui_password_box;
extern lv_obj_t *ui_wifi_device_name;
extern lv_obj_t *ui_Keyboard1;

/** Show the password screen for the given SSID and switch to it. */
void ui_wifi_password_screen_show(const char *ssid);

/** Notify connect result (1 = success, 0 = failed). Call from wifi/IP event handlers. */
void ui_wifi_password_screen_on_connect_result(int success);

#ifdef __cplusplus
}
#endif

#endif
