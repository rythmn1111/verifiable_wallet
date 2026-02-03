# ESP32-S3-Touch-LCD-3.5: Project Setup & SquareLine Studio UI — Full Guide

This document is the single source of truth for:

1. Setting up a **new PlatformIO/ESP-IDF project** for the ESP32-S3-Touch-LCD-3.5 (3.5" 320×480 touch LCD).
2. Integrating a **SquareLine Studio** UI and avoiding every pitfall we hit (LVGL 9, display, camera, paths).

**Audience:** You (or an AI with fresh context) should be able to follow this from scratch and get a working UI + optional camera/QR without re-discovering the same bugs.

---

## Table of contents

1. [Project path: no spaces](#1-project-path-no-spaces)
2. [New PlatformIO project setup](#2-new-platformio-project-setup)
3. [Display + touch + LVGL port](#3-display--touch--lvgl-port)
4. [Display pitfalls (rotation, colors, sw_rotate)](#4-display-pitfalls-rotation-colors-sw_rotate)
5. [SquareLine Studio UI integration](#5-squareline-studio-ui-integration)
6. [LVGL 8 → 9 port (SquareLine export)](#6-lvgl-8--9-port-squareline-export)
7. [Camera feed (if you use it)](#7-camera-feed-if-you-use-it)
8. [Adding custom UI to a screen (e.g. QR scanner)](#8-adding-custom-ui-to-a-screen-eg-qr-scanner)
9. [Checklist and quick reference](#9-checklist-and-quick-reference)

---

## 1. Project path: no spaces

**Mistake:** Project folder name contains a space (e.g. `test project`).

**Symptom:** PlatformIO build fails with:
```text
Error: Detected a whitespace character in project paths.
```

**Fix:** Use a path with **no spaces**. Rename the folder (e.g. `test project` → `test_project`). Open that folder as the project root and build again.

---

## 2. New PlatformIO project setup

Target: **ESP32-S3-Touch-LCD-3.5** (Waveshare 3.5" 320×480, ST7796, FT6336 touch). Board is often selected as `esp32-s3-devkitc-1`; PSRAM and flash are set via `sdkconfig.defaults`.

### 2.1 platformio.ini

```ini
[env:esp32-s3-touch-lcd-35]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
```

### 2.2 Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(your_project_name)
```

### 2.3 sdkconfig.defaults

Required for this board: 16MB flash, 8MB PSRAM (OCT), custom partition table, LVGL in 16-bit.

```text
CONFIG_IDF_TARGET="esp32s3"

CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_CLK_IO=30
CONFIG_SPIRAM_CS_IO=26

CONFIG_LV_COLOR_16_SWAP=y
CONFIG_LV_MEM_CUSTOM=y
CONFIG_LV_USE_LOG=n
```

### 2.4 partitions.csv

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 6M,
```

### 2.5 Main component (src/)

- **src/CMakeLists.txt** — Register all app sources and include the UI folder. Example (UI folder name `wallet`):

```cmake
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "wallet"
                    REQUIRES "esp_port" "nvs_flash" "driver" "esp_io_expander_tca9554" "espressif__esp_lvgl_port" "lvgl__lvgl" "espressif__esp_lcd_touch")
```

- Omit camera/button/quirc from `REQUIRES` if you don’t use them. Add them when you add camera/QR.

- **src/idf_component.yml** — LVGL 9 and display stack (no camera if not needed):

```yaml
dependencies:
  idf: ">=5.1"
  lvgl/lvgl: "~9.1.0"
  espressif/esp_lcd_touch: "^1.1.2"
  espressif/esp_io_expander_tca9554: "^2.0.0"
  espressif/esp_lvgl_port: "^2.5.0"
```

Add `espressif/esp32-camera`, `espressif/button`, `espressif/quirc` only if you use camera/QR.

---

## 3. Display + touch + LVGL port

### 3.1 Components you need

- **esp_port** — Board-specific LCD + touch + backlight:
  - `esp_3inch5_display_port_init(io_handle, panel_handle, max_transfer_sz)`
  - `esp_3inch5_touch_port_init(touch_handle, i2c_bus, xmax, ymax, rotation)`
  - `esp_3inch5_brightness_port_init()` and `esp_3inch5_brightness_port_set(0..100)`
- **esp_lcd_st7796** — ST7796 driver.
- **esp_lcd_touch_ft6336** — FT6336 touch driver.

LCD reset is **not** on a GPIO; it’s via the **TCA9554** I2C expander. So init order is:

1. NVS.
2. I2C bus (e.g. SDA=8, SCL=7).
3. TCA9554 (e.g. address 0x20), set pin 1 as output and toggle to reset LCD.
4. Display port init (SPI + ST7796).
5. Touch port init (I2C + FT6336).
6. Short delay, then brightness init and set (e.g. 80%).
7. LVGL port init (see below), then your `ui_init()`.

### 3.2 LVGL port display config (critical)

Use the **exact** pattern below. Wrong values cause wrong orientation or wrong colors (see next section).

```c
#define EXAMPLE_DISPLAY_ROTATION 0

#if EXAMPLE_DISPLAY_ROTATION == 90 || EXAMPLE_DISPLAY_ROTATION == 270
#define EXAMPLE_LCD_H_RES 480
#define EXAMPLE_LCD_V_RES  320
#else
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES  480
#endif

#define LCD_BUFFER_SIZE  (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8)

// In lv_port_init():
lvgl_port_display_cfg_t display_cfg = {};
display_cfg.io_handle = s_io_handle;
display_cfg.panel_handle = s_panel_handle;
display_cfg.control_handle = NULL;
display_cfg.buffer_size = LCD_BUFFER_SIZE;
display_cfg.double_buffer = true;
display_cfg.trans_size = 0;
display_cfg.hres = EXAMPLE_LCD_H_RES;
display_cfg.vres = EXAMPLE_LCD_V_RES;
display_cfg.monochrome = false;
/* Rotation 0: swap_xy=0, mirror_x=1, mirror_y=0 matches this board. */
display_cfg.rotation = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 };

#if LVGL_VERSION_MAJOR >= 9
display_cfg.color_format = LV_COLOR_FORMAT_RGB565;
display_cfg.flags.buff_dma = 0;
display_cfg.flags.buff_spiram = 1;
display_cfg.flags.sw_rotate = 0;   /* MUST be 0 — see Section 4 */
display_cfg.flags.full_refresh = 0;
display_cfg.flags.direct_mode = 0;
display_cfg.flags.swap_bytes = 1; /* Panel is BGR — see Section 4 */
#else
display_cfg.flags.buff_dma = 0;
display_cfg.flags.buff_spiram = 1;
display_cfg.flags.sw_rotate = 0;
display_cfg.flags.full_refresh = 0;
display_cfg.flags.direct_mode = 0;
#endif

s_lvgl_disp = lvgl_port_add_disp(&display_cfg);

const lvgl_port_touch_cfg_t touch_cfg = {
    .disp = s_lvgl_disp,
    .handle = s_touch_handle,
    .scale = { .x = 1.0f, .y = 1.0f },
};
lvgl_port_add_touch(&touch_cfg);
```

Touch port must use the **same** `EXAMPLE_DISPLAY_ROTATION` so touch coordinates match the display.

---

## 4. Display pitfalls (rotation, colors, sw_rotate)

These are the exact issues we hit and how to avoid them.

### 4.1 sw_rotate must be 0 for mirror to work

**What’s wrong:** In `managed_components/espressif__esp_lvgl_port/src/lvgl9/esp_lvgl_port_disp.c`, the function `lvgl_port_disp_rotation_update()` does this:

```c
if (disp_ctx->flags.sw_rotate) {
    return;  // early return — panel mirror/swap_xy are NEVER applied
}
esp_lcd_panel_swap_xy(...);
esp_lcd_panel_mirror(...);
```

So if `display_cfg.flags.sw_rotate = 1`, the driver **never** calls `esp_lcd_panel_mirror()` or `esp_lcd_panel_swap_xy()`. Your `rotation.mirror_x` / `mirror_y` settings have **no effect**.

**Fix:** Set `display_cfg.flags.sw_rotate = 0` when using rotation 0 with mirror. Then the port applies mirror/swap_xy to the panel and the image is correct.

### 4.2 Rotation 0 + mirror for this board

For **EXAMPLE_DISPLAY_ROTATION 0** (320×480 portrait), the known-good config is:

- `swap_xy = 0`, `mirror_x = 1`, `mirror_y = 0`

Match this in both the LVGL display config and in the touch port init (touch driver uses the same rotation value so coordinates align).

### 4.3 Wrong colors (violet / inverted look)

**Cause:** Panel is wired for **BGR** and may have color inversion. LVGL draws **RGB565**. If you send RGB565 directly, colors are wrong (e.g. blacks look violet).

**Fix:**

1. In the LCD panel driver (e.g. ST7796), keep `esp_lcd_panel_invert_color(panel, true)` if the board needs it (common for ST7796).
2. In the LVGL port config, set **`display_cfg.flags.swap_bytes = 1`**. This swaps the two bytes of each 16-bit pixel so that what LVGL produces (RGB565) is sent as BGR565 to the panel. That fixes the violet/wrong colors.

### 4.4 Summary of display flags

| Setting        | Value | Why |
|----------------|-------|-----|
| `sw_rotate`    | **0** | So that mirror_x/mirror_y are actually applied by the driver. |
| `rotation`     | swap_xy=0, mirror_x=**1**, mirror_y=0 | Correct orientation for this board at rotation 0. |
| `swap_bytes`   | **1** | Panel expects BGR; swap converts LVGL RGB565 → BGR565. |
| `invert_color` | true (in panel init) | Often required for ST7796. |

---

## 5. SquareLine Studio UI integration

### 5.1 Export from SquareLine

- In SquareLine Studio, set the project to **LVGL 9.1.0** and export.
- You get a folder with: `ui.c`, `ui.h`, `ui_helpers.c/h`, `ui_events.h`, `screens/` (e.g. `ui_Screen1.c/h`), `fonts/`, `images/`, `components/` (e.g. `ui_comp_hook.c`).

### 5.2 Where to put the UI in your project

- Copy the **entire** UI folder into **src/** (e.g. `src/wallet/` or `src/my_ui/`).
- In **src/CMakeLists.txt**, add that folder name to **INCLUDE_DIRS** (e.g. `"wallet"`) so that `#include "ui.h"` and `#include "screens/ui_Screen1.h"` resolve when building the main component.
- Do **not** make the UI a separate IDF component unless you want to manage `lvgl__lvgl` and paths carefully; in-tree under `src/` is simpler.

### 5.3 Main app flow

In `main.cpp` (or your main app file):

1. After `lvgl_port_init()` and adding display + touch, take the LVGL lock.
2. Call your UI init (e.g. `ui_init()` from `ui.h`).
3. Release the lock.

Example:

```c
if (lvgl_port_lock(0)) {
    ui_init();
    lvgl_port_unlock();
}
```

### 5.4 LV_COLOR_DEPTH

SquareLine typically exports for 16-bit color. In generated code you’ll see:

```c
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
```

Your `sdkconfig.defaults` (and LVGL config) must use 16-bit (e.g. `CONFIG_LV_COLOR_16_SWAP=y` and 16b color depth). Don’t change to 32-bit without re-exporting or fixing assets.

---

## 6. LVGL 8 → 9 port (SquareLine export)

SquareLine may generate LVGL 8 style APIs. Our stack uses **LVGL 9**. Port the generated UI as follows.

### 6.1 In ui.c (init and load screen)

| LVGL 8 (wrong)           | LVGL 9 (correct)            |
|--------------------------|-----------------------------|
| `lv_disp_t *dispp = ...` | `lv_display_t *dispp = ...` |
| `lv_disp_set_theme(dispp, theme)` | `lv_display_set_theme(dispp, theme)` |
| `lv_disp_load_scr(ui_Screen1)`    | `lv_screen_load(ui_Screen1)` |

So in `ui_init()` use:

```c
lv_display_t *dispp = lv_display_get_default();
lv_theme_t *theme = lv_theme_default_init(dispp, ...);
lv_display_set_theme(dispp, theme);
// ... screen inits ...
lv_screen_load(ui_Screen1);
```

### 6.2 In every screen .c file (style setters)

Style APIs in LVGL 9 use a single selector type; mixing `LV_PART_MAIN` and `LV_STATE_DEFAULT` with a bitwise OR can trigger enum warnings or errors. Cast the selector:

**Wrong (can warn or fail):**
```c
lv_obj_set_style_bg_color(obj, lv_color_hex(0x88BF6C), LV_PART_MAIN | LV_STATE_DEFAULT);
```

**Correct:**
```c
lv_obj_set_style_bg_color(obj, lv_color_hex(0x88BF6C), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
```

Do this for **all** style calls that take a selector: `lv_obj_set_style_*`, for both `LV_PART_MAIN` and `LV_PART_SCROLLBAR` (and any other part/state combo).

### 6.3 Runtime image descriptor (LVGL 9) — e.g. for camera

If you set an image source from a buffer at runtime (e.g. camera frame), use `lv_image_dsc_t` with a proper `lv_image_header_t`. Field order and all fields matter; wrong order can cause “designator order does not match declaration order” or corruption.

Example (camera, RGB565):

```c
static lv_image_dsc_t s_cam_img_dsc = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .flags = 0,
        .w = (uint32_t)width,
        .h = (uint32_t)height,
        .stride = (uint32_t)(width * 2),
        .reserved_2 = 0,
    },
    .data_size = (uint32_t)(width * height * 2),
    .data = NULL,
    .reserved = NULL,
};
```

Then set `s_cam_img_dsc.data` to your buffer and call `lv_image_set_src(image_obj, &s_cam_img_dsc)`.

### 6.4 Screen delete

We kept `lv_obj_del(screen)` in screen destroy; it exists in LVGL 9. If you see `lv_obj_delete` in docs, either is fine as long as it matches your LVGL version.

---

## 7. Camera feed (if you use it)

If you show a live camera image in an LVGL image object, two mistakes will corrupt the image or freeze the system.

### 7.1 Never pass the camera buffer directly to LVGL

**Mistake:** Doing `lv_image_set_src(img, &desc)` where `desc.data` points at `fb->buf` (the frame buffer from `esp_camera_fb_get()`), then calling `esp_camera_fb_return(fb)` right after. LVGL keeps a **pointer** to the buffer and draws asynchronously. The camera driver **reuses** that buffer for the next frame. So the buffer changes while LVGL is still drawing → **corruption, glitches, wrong colors**.

**Fix:**

1. Allocate your **own** buffer (e.g. in PSRAM): `s_cam_copy = heap_caps_malloc(CAM_W * CAM_H * 2, MALLOC_CAP_SPIRAM)`.
2. For each frame: copy the frame into your buffer (`memcpy(s_cam_copy, fb->buf, expected_len)`), set your image descriptor’s `.data = s_cam_copy`, call `lv_image_set_src(s_img_camera, &s_cam_img_dsc)` **inside** an LVGL lock if you’re on another task.
3. **Then** call `esp_camera_fb_return(fb)` so the driver can reuse its buffer. LVGL now uses your copy, so no reuse conflict.

### 7.2 Byte order (big-endian vs little-endian)

**Mistake:** Camera (e.g. OV2640/OV5640) often outputs RGB565 in **big-endian**. LVGL expects **little-endian** RGB565. If you pass the buffer as-is (even in your copy), colors are wrong (e.g. red/blue swapped).

**Fix:** After copying the frame into your buffer, **swap bytes** for each 16-bit pixel:

```c
for (size_t i = 0; i < expected_len; i += 2) {
    uint8_t t = s_cam_copy[i];
    s_cam_copy[i] = s_cam_copy[i + 1];
    s_cam_copy[i + 1] = t;
}
```

Then set the descriptor and `lv_image_set_src`.

### 7.3 Sanity checks

- Only use frames with `fb->format == PIXFORMAT_RGB565`.
- Check `fb->len >= (width * height * 2)` (and use `fb->width`/`fb->height` if valid) so you don’t read out of bounds or set wrong stride/size in the image descriptor.

---

## 8. Adding custom UI to a screen (e.g. QR scanner)

If one of your SquareLine screens is just a container and you want to add custom widgets (e.g. camera view, QR result labels) from C code:

- In the **generated** screen init (e.g. `ui_Screen2_screen_init()`), after creating the base screen object, call a **callback** that your app provides.
- In `main` (or a UI hook file), implement that callback: create panels, labels, image object for camera, etc., as children of that screen.

Example pattern:

**In your UI (e.g. ui_events.h or a shared header):**
```c
extern void (*app_screen2_init_cb)(lv_obj_t *screen2);
```

**In generated ui_Screen2.c:**
```c
void ui_Screen2_screen_init(void) {
    ui_Screen2 = lv_obj_create(NULL);
    // ...
    if (app_screen2_init_cb != NULL) {
        app_screen2_init_cb(ui_Screen2);
    }
}
```

**In main.cpp (before ui_init()):**
```c
app_screen2_init_cb = my_qr_ui_init;  // your function that creates QR panels/labels/camera image
```

Then in `my_qr_ui_init(screen2)` you create all custom widgets as children of `screen2`. Use the same style selector cast as in section 6.2 for any style calls.

---

## 9. Checklist and quick reference

### New project

- [ ] Project folder path has **no spaces**.
- [ ] `platformio.ini`: board, framework = espidf.
- [ ] Root `CMakeLists.txt` + `sdkconfig.defaults` (PSRAM, partition, LVGL 16-bit) + `partitions.csv`.
- [ ] `src/CMakeLists.txt`: GLOB sources, INCLUDE_DIRS including UI folder, REQUIRES for nvs_flash, driver, esp_port, esp_lvgl_port, lvgl, esp_lcd_touch, esp_io_expander_tca9554 (and camera/button/quirc only if used).
- [ ] `src/idf_component.yml`: lvgl ~9.1.0, esp_lvgl_port, esp_io_expander_tca9554, esp_lcd_touch (+ camera/button/quirc if used).

### Display

- [ ] Init order: NVS → I2C → TCA9554 (reset) → display port → touch port → delay → brightness → LVGL port.
- [ ] `display_cfg.flags.sw_rotate = 0` so mirror is applied.
- [ ] `display_cfg.rotation = { .swap_xy = 0, .mirror_x = 1, .mirror_y = 0 }` for rotation 0 on this board.
- [ ] `display_cfg.flags.swap_bytes = 1` for correct colors (BGR panel).
- [ ] Touch port uses same `EXAMPLE_DISPLAY_ROTATION` as display.

### SquareLine UI

- [ ] UI folder copied under `src/`; folder name in INCLUDE_DIRS.
- [ ] `ui.c`: `lv_display_t`, `lv_display_set_theme`, `lv_screen_load`.
- [ ] All screen style calls: `(lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT)` (and other part/state combos).
- [ ] Runtime image (e.g. camera): `lv_image_dsc_t` with correct `.header` field order and your own buffer.

### Camera (if used)

- [ ] Own PSRAM buffer; copy each frame into it, then pass to LVGL; then `esp_camera_fb_return(fb)`.
- [ ] Byte-swap each 16-bit pixel in your copy if camera is big-endian RGB565.
- [ ] Check `fb->format == PIXFORMAT_RGB565` and `fb->len >= width*height*2`.

### Build and run

```bash
pio run
pio run -t upload
pio device monitor
```

---

*This document was written so that a fresh context (human or AI) can set up the project and integrate SquareLine UI without re-hitting the same display, LVGL 9, and camera pitfalls.*
