# Test project – SquareLine UI integration flow

This project mirrors the main **verifiable_wallet** setup so you can try the “new UI export → integrate” flow without touching the main repo.

## Layout

- **`src/main.cpp`** – Same init sequence as main project: NVS, I2C, IO expander, display port, touch port, brightness, LVGL port, then `ui_init()`. No camera/QR/button.
- **`src/test_ui/`** – SquareLine Studio UI (single screen) copied here and ported to LVGL 9.
- **`components/`** – LCD-only: `esp_port` (3.5" display + touch + brightness), `esp_lcd_st7796`, `esp_lcd_touch_ft6336`.

## Flow (what was done)

1. **Project setup** – `platformio.ini` (board `esp32-s3-touch-lcd-35`), root `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`.
2. **Components** – Copied display/touch components from main; `esp_port` only registers `esp_3inch5_lcd_port.cpp` (no camera).
3. **`src/`** – `CMakeLists.txt` (GLOB all sources under `src/`, include `test_ui`), `idf_component.yml` (LVGL 9.1, esp_lvgl_port, esp_io_expander_tca9554, esp_lcd_touch; no camera/button/quirc), `main.cpp` (init + `ui_init()`).
4. **test_ui** – Copied from `test_ui/` into `src/test_ui/` and ported to LVGL 9:
   - **`ui.c`**: `lv_disp_t` → `lv_display_t`, `lv_disp_set_theme` → `lv_display_set_theme`, `lv_disp_load_scr` → `lv_screen_load`.
   - **`screens/ui_Screen1.c`**: style selectors cast to `(lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT)` (and scrollbar) for LVGL 9.

## Build and run

From this directory (with PlatformIO in your PATH):

```bash
pio run
pio run -t upload
pio device monitor
```

Display config matches the main project: rotation 0, `mirror_x=1`, `swap_bytes=1`, same LCD/touch/brightness init.

## Reusing this flow for a new UI

When you export a new UI from SquareLine Studio into a folder (e.g. `my_new_ui/`):

1. Copy that folder into `src/` (e.g. `src/my_new_ui/`).
2. In `src/CMakeLists.txt`, add `my_new_ui` to `INCLUDE_DIRS` if you use a different folder name.
3. In `main.cpp`, call the new UI’s init (e.g. `#include "my_new_ui/ui.h"` and `my_new_ui_init()` or whatever the export provides).
4. Port the generated code to LVGL 9:
   - In the main `ui.c`: `lv_display_t`, `lv_display_set_theme`, `lv_screen_load`.
   - In screen files: use `(lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT)` (and similar) for all style setters that take a selector.
5. Build; fix any remaining LVGL 9 API renames (e.g. `lv_image_set_src` signature, `lv_obj_del` vs `lv_obj_delete` as in your tree).

This test project is the reference for that flow.
