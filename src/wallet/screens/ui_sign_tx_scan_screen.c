/* Sign Tx: scan hash QR from website (camera + quirc), then go to password screen. */

#include "../ui.h"
#include "ui_sign_tx_scan_screen.h"
#include "ui_sign_tx_password_screen.h"
#include "ui_Screen1.h"
#include "ui_wallet_exists_screen.h"
#include "ui_helpers.h"
#include "esp_camera.h"
#include "esp_camera_port.h"
#include "qr_decoder.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "sign_tx_scan";

#define SCAN_QUEUE_LEN      2
#define SCANNED_BUF_SIZE    256
#define SCAN_TASK_STACK     (24 * 1024)
#define PREVIEW_W           240
#define PREVIEW_H           240
#define PREVIEW_ROW_BYTES   (PREVIEW_W * 2)   /* RGB565: 2 bytes per pixel */

lv_obj_t *ui_sign_tx_scan_screen = NULL;
static lv_obj_t *s_title = NULL;
static lv_obj_t *s_msg = NULL;
static lv_obj_t *s_cancel_btn = NULL;
static lv_obj_t *s_home_btn = NULL;
static lv_timer_t *s_poll_timer = NULL;
static lv_obj_t *s_preview_canvas = NULL;
static uint8_t *s_preview_buf = NULL;   /* written by scan task (packed PREVIEW_ROW_BYTES per row) */
static uint8_t *s_canvas_buf = NULL;    /* used by LVGL canvas (stride per row) */
static uint32_t s_canvas_stride = 0;    /* bytes per row for canvas (from lv_draw_buf_width_to_stride) */
static volatile bool s_preview_dirty = false;

static QueueHandle_t s_scan_queue = NULL;
static TaskHandle_t s_scan_task_handle = NULL;
static char s_scanned_buf[SCANNED_BUF_SIZE];
static volatile bool s_scan_cancel = false;
static lv_timer_t *s_start_scan_timer = NULL;  /* one-shot: start scan task after UI has settled */

static void poll_timer_cb(lv_timer_t *timer);

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	s_scan_cancel = true;
	if (s_start_scan_timer) {
		lv_timer_del(s_start_scan_timer);
		s_start_scan_timer = NULL;
	}
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void cancel_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
	s_scan_cancel = true;
	if (s_start_scan_timer) {
		lv_timer_del(s_start_scan_timer);
		s_start_scan_timer = NULL;
	}
	_ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen1_screen_init);
}

static void scan_task(void *pv)
{
	(void)pv;
	/* Yield so LVGL can finish screen transition and avoid I2C/display contention with camera init */
	vTaskDelay(pdMS_TO_TICKS(300));
	if (s_scan_cancel) {
		vTaskDelete(NULL);
		return;
	}
	if (esp_camera_port_init_c(0) != 0) {
		ESP_LOGW(TAG, "camera init failed");
		uint32_t err = 1;
		xQueueSend(s_scan_queue, &err, 0);
		vTaskDelete(NULL);
		return;
	}

	camera_fb_t *fb = NULL;
	char decode_buf[SCANNED_BUF_SIZE];
	while (!s_scan_cancel) {
		fb = esp_camera_fb_get();
		if (!fb) {
			vTaskDelay(pdMS_TO_TICKS(50));
			continue;
		}
		/* Copy frame to preview buffer for UI (crop to PREVIEW_W x PREVIEW_H if needed) */
		if (s_preview_buf && fb->width > 0 && fb->height > 0) {
			int copy_w = (fb->width <= PREVIEW_W) ? fb->width : PREVIEW_W;
			int copy_h = (fb->height <= PREVIEW_H) ? fb->height : PREVIEW_H;
			int row_bytes = copy_w * 2;
			for (int y = 0; y < copy_h; y++)
				memcpy(s_preview_buf + (size_t)y * PREVIEW_ROW_BYTES, fb->buf + (size_t)y * fb->width * 2, (size_t)row_bytes);
			s_preview_dirty = true;
		}
		bool ok = qr_decoder_decode_rgb565(fb->buf, fb->width, fb->height, decode_buf, sizeof(decode_buf));
		esp_camera_fb_return(fb);
		if (ok && decode_buf[0] != '\0') {
			size_t len = strnlen(decode_buf, sizeof(s_scanned_buf) - 1);
			if (len > 0) {
				memcpy(s_scanned_buf, decode_buf, len + 1);
				uint32_t done = 0;
				xQueueSend(s_scan_queue, &done, 0);
			}
			vTaskDelete(NULL);
			return;
		}
		vTaskDelay(pdMS_TO_TICKS(30));
	}
	vTaskDelete(NULL);
}

static void start_scan_timer_cb(lv_timer_t *timer)
{
	(void)timer;
	s_start_scan_timer = NULL;
	if (s_scan_cancel || !s_scan_queue || s_scan_task_handle != NULL) return;
	BaseType_t created = xTaskCreatePinnedToCore(scan_task, "sign_scan", SCAN_TASK_STACK, NULL, 3, &s_scan_task_handle, 1);
	if (created == pdPASS && s_poll_timer == NULL)
		s_poll_timer = lv_timer_create(poll_timer_cb, 200, NULL);
	if (s_msg)
		lv_label_set_text(s_msg, "Point camera at hash QR on website...");
}

static void poll_timer_cb(lv_timer_t *timer)
{
	(void)timer;
	if (!s_scan_queue) return;

	/* Update preview from scan task's buffer; copy row-by-row so stride matches LVGL canvas */
	if (s_preview_dirty && s_canvas_buf && s_preview_buf && s_preview_canvas && s_canvas_stride > 0) {
		for (int y = 0; y < PREVIEW_H; y++)
			memcpy(s_canvas_buf + (size_t)y * s_canvas_stride, s_preview_buf + (size_t)y * PREVIEW_ROW_BYTES, PREVIEW_ROW_BYTES);
		s_preview_dirty = false;
		lv_obj_invalidate(s_preview_canvas);
	}

	uint32_t msg;
	if (xQueueReceive(s_scan_queue, &msg, 0) != pdPASS) return;

	lv_timer_del(s_poll_timer);
	s_poll_timer = NULL;
	s_scan_task_handle = NULL;

	if (msg == 1) {
		/* Camera init failed: go back to wallet menu so LVGL doesn't get stuck on this screen */
		_ui_screen_change(&ui_wallet_exists_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_wallet_exists_screen_screen_init);
		return;
	}
	/* msg == 0: success, s_scanned_buf has the hash (or JSON) */
	ui_sign_tx_password_screen_show(s_scanned_buf);
}

static void add_circle_home(lv_obj_t *parent)
{
	s_home_btn = lv_button_create(parent);
	lv_obj_set_width(s_home_btn, 49);
	lv_obj_set_height(s_home_btn, 47);
	lv_obj_set_x(s_home_btn, -6);
	lv_obj_set_y(s_home_btn, 205);
	lv_obj_set_align(s_home_btn, LV_ALIGN_CENTER);
	lv_obj_set_ext_click_area(s_home_btn, 8);
	lv_obj_add_flag(s_home_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	lv_obj_remove_flag(s_home_btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(s_home_btn, 1000, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_color(s_home_btn, lv_color_hex(0xFFFFFF), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(s_home_btn, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_color(s_home_btn, lv_color_hex(0x000000), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_opa(s_home_btn, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_width(s_home_btn, 10, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_shadow_spread(s_home_btn, 1, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_add_event_cb(s_home_btn, home_btn_cb, LV_EVENT_CLICKED, NULL);
}

void ui_sign_tx_scan_screen_show(void)
{
	if (ui_sign_tx_scan_screen == NULL)
		ui_sign_tx_scan_screen_screen_init();

	s_scan_cancel = false;
	s_scanned_buf[0] = '\0';
	if (s_scan_queue == NULL)
		s_scan_queue = xQueueCreate(SCAN_QUEUE_LEN, sizeof(uint32_t));
	if (s_msg)
		lv_label_set_text(s_msg, "Starting camera...");

	/* Defer starting the scan task so LVGL can finish the screen transition and avoid
	 * I2C/SPI contention (camera and touch/display may share bus). Start scan after 600ms. */
	if (s_start_scan_timer)
		lv_timer_del(s_start_scan_timer);
	s_start_scan_timer = lv_timer_create(start_scan_timer_cb, 600, NULL);
	lv_timer_set_repeat_count(s_start_scan_timer, 1);

	_ui_screen_change(&ui_sign_tx_scan_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, NULL);
}

void ui_sign_tx_scan_screen_screen_init(void)
{
	ui_sign_tx_scan_screen = lv_obj_create(NULL);
	lv_obj_remove_flag(ui_sign_tx_scan_screen, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(ui_sign_tx_scan_screen, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_bg_opa(ui_sign_tx_scan_screen, LV_OPA_COVER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_title = lv_label_create(ui_sign_tx_scan_screen);
	lv_obj_set_width(s_title, 280);
	lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
	lv_obj_set_y(s_title, 12);
	lv_label_set_text(s_title, "Sign Tx - Scan hash QR");
	lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_title, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	/* Camera preview: canvas stride for RGB565 (match LVGL default: width*2, align 1) */
	s_canvas_stride = PREVIEW_ROW_BYTES;
	size_t canvas_size = (size_t)s_canvas_stride * PREVIEW_H;
	s_preview_buf = (uint8_t *)heap_caps_malloc((size_t)PREVIEW_ROW_BYTES * PREVIEW_H, MALLOC_CAP_SPIRAM);
	s_canvas_buf = (uint8_t *)heap_caps_malloc(canvas_size, MALLOC_CAP_SPIRAM);
	if (s_canvas_buf && s_preview_buf) {
		/* Fill with dark gray so preview area is visible before first frame (respect stride) */
		for (int y = 0; y < PREVIEW_H; y++) {
			uint16_t *row = (uint16_t *)(s_canvas_buf + (size_t)y * s_canvas_stride);
			for (int x = 0; x < PREVIEW_W; x++) row[x] = 0x3186;  /* RGB565 dark gray */
		}
		s_preview_canvas = lv_canvas_create(ui_sign_tx_scan_screen);
		lv_canvas_set_buffer(s_preview_canvas, s_canvas_buf, PREVIEW_W, PREVIEW_H, LV_COLOR_FORMAT_RGB565);
		lv_obj_set_width(s_preview_canvas, PREVIEW_W);
		lv_obj_set_height(s_preview_canvas, PREVIEW_H);
		lv_obj_set_align(s_preview_canvas, LV_ALIGN_TOP_MID);
		lv_obj_set_y(s_preview_canvas, 36);
	} else {
		s_preview_canvas = NULL;
		s_canvas_stride = 0;
		if (s_preview_buf) { heap_caps_free(s_preview_buf); s_preview_buf = NULL; }
		if (s_canvas_buf) { heap_caps_free(s_canvas_buf); s_canvas_buf = NULL; }
	}

	s_msg = lv_label_create(ui_sign_tx_scan_screen);
	lv_obj_set_width(s_msg, 280);
	lv_obj_set_align(s_msg, LV_ALIGN_CENTER);
	lv_obj_set_y(s_msg, 20);
	lv_label_set_text(s_msg, "Point camera at hash QR...");
	lv_obj_set_style_text_align(s_msg, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
	lv_obj_set_style_text_font(s_msg, &ui_font_pixel_wordings_small, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	s_cancel_btn = lv_button_create(ui_sign_tx_scan_screen);
	lv_obj_set_width(s_cancel_btn, 120);
	lv_obj_set_height(s_cancel_btn, 44);
	lv_obj_set_align(s_cancel_btn, LV_ALIGN_CENTER);
	lv_obj_set_y(s_cancel_btn, 90);
	lv_obj_add_event_cb(s_cancel_btn, cancel_btn_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_t *cancel_lbl = lv_label_create(s_cancel_btn);
	lv_label_set_text(cancel_lbl, "Cancel");
	lv_obj_center(cancel_lbl);
	lv_obj_set_style_text_font(cancel_lbl, &ui_font_Pixel, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

	add_circle_home(ui_sign_tx_scan_screen);
}

void ui_sign_tx_scan_screen_screen_destroy(void)
{
	s_scan_cancel = true;
	if (s_start_scan_timer) {
		lv_timer_del(s_start_scan_timer);
		s_start_scan_timer = NULL;
	}
	if (s_poll_timer) {
		lv_timer_del(s_poll_timer);
		s_poll_timer = NULL;
	}
	s_scan_task_handle = NULL;
	if (s_scan_queue) {
		vQueueDelete(s_scan_queue);
		s_scan_queue = NULL;
	}
	if (s_preview_buf) {
		heap_caps_free(s_preview_buf);
		s_preview_buf = NULL;
	}
	if (s_canvas_buf) {
		heap_caps_free(s_canvas_buf);
		s_canvas_buf = NULL;
	}
	s_preview_canvas = NULL;
	s_canvas_stride = 0;
	s_preview_dirty = false;
	if (ui_sign_tx_scan_screen)
		lv_obj_del(ui_sign_tx_scan_screen);
	ui_sign_tx_scan_screen = NULL;
	s_title = NULL;
	s_msg = NULL;
	s_cancel_btn = NULL;
	s_home_btn = NULL;
}
