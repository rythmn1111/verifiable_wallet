# Learnings — ESP32-S3-Touch-LCD-3.5 QR Scanner

What we learned getting the QR scanner working on this board.

---

## 1. Don’t do heavy work in the button callback

**Problem:** Pressing BOOT caused a restart.

**Cause:** The BOOT button callback was calling `esp_camera_port_init()` directly. That runs in a small-stack context (button/timer task). Camera init does GPIO, I2C, PSRAM allocation, and sensor setup — too much stack and blocking time → stack overflow or task watchdog → reboot.

**Fix:** In the callback only set state and switch UI (e.g. `s_state = STATE_SCANNING`, `ui_show_scanning()`). Do camera init in a dedicated task (e.g. `scan_task`) the first time it sees `STATE_SCANNING` and `!s_camera_inited`. That task has a proper stack and can block without killing the system.

---

## 2. Give the scan task enough stack

**Problem:** After moving camera init to `scan_task`, it could still restart when scanning started.

**Cause:** `scan_task` was created with 4 KB stack. Running quirc decode + camera + LVGL updates in one task needs much more. DISPLAY_SETUP.md (and similar Arduino notes) say the default loop stack (8 KB) wasn’t enough and caused “stack overflow in task loopTask” and reboots.

**Fix:** Create the scan task with **24 KB** stack, e.g. `xTaskCreatePinnedToCore(scan_task, "scan", 24 * 1024, ...)`.

---

## 3. quirc_resize is (width, height), not (height, width)

**Problem:** Camera and UI worked, but QR codes were never decoded — always “Scanning…”.

**Cause:** We called `quirc_resize(s_qr, height, width)` (320, 480). The API is `quirc_resize(q, w, h)` — **width first, then height**. So quirc thought the image was 320×480 (320 columns, 480 rows) while we filled the buffer as 480×320. The logical image was wrong → detection failed.

**Fix:** Call `quirc_resize(s_qr, width, height)` (480, 320).

---

## 4. Use the camera’s actual frame dimensions

**Problem:** Relying only on compile-time `CAM_W`/`CAM_H` can mismatch the real buffer.

**Cause:** The driver sets `fb->width` and `fb->height` from the resolution table. If the sensor or driver ever differs (e.g. OV5640 vs OV2640, or different frame size), decoding must use the same dimensions as the buffer layout.

**Fix:** When calling the decoder, use `dec_w`/`dec_h` from `fb->width`/`fb->height` when they’re set, otherwise fall back to `CAM_W`/`CAM_H`. Check `fb->len >= dec_w * dec_h * 2` before decoding.

---

## 5. RGB565 byte order depends on the camera

**Problem:** Even with correct dimensions and resize, some boards/cameras still didn’t decode.

**Cause:** Grayscale was computed assuming **big-endian** RGB565 (as in qrcode-demo) using `__builtin_bswap16()`. Some drivers (e.g. certain OV5640 setups) output **little-endian** RGB565. Wrong byte order → wrong grayscale → quirc can’t lock onto the code.

**Fix:** Try both interpretations: first convert with bswap (big-endian), run quirc; if that doesn’t decode, convert **without** bswap (little-endian) and run quirc again. Use a `use_bswap` flag in the grayscale conversion and try both in the decoder.

---

## 6. Try multiple decode paths (flip code + flip image)

**Problem:** Some QR codes still didn’t decode (orientation, ECC, etc.).

**Cause:** quirc can find a code but fail to decode if the code is rotated/upside-down. The demo uses `quirc_flip(&code)` after extract; sometimes decoding works with flip, sometimes without. Also, the **image** can be upside-down (e.g. camera mount), so flipping the grayscale buffer and re-running detection helps.

**Fix:** For each grayscale image we try:
- Decode **with** `quirc_flip(&code)`.
- Decode **without** flip (re-extract, don’t flip).
- If both fail, **flip the grayscale buffer vertically**, run `quirc_end` again, then try decode with and without `quirc_flip(&code)`.

So we combine: two byte orders (bswap / no bswap) × normal image + flipped image × flip code / no flip code. One of these usually succeeds.

---

## 7. Header must define uint8_t for the decoder API

**Problem:** Build error: `unknown type name 'uint8_t'` and “conflicting types” for `qr_decoder_decode_rgb565` in the header.

**Cause:** The public header declared the function with `const uint8_t *` but didn’t include `<stdint.h>`. Without it, `uint8_t` was undefined (or interpreted differently), so the declaration didn’t match the implementation.

**Fix:** In `components/qr_decoder/include/qr_decoder.h`, add `#include <stdint.h>`.

---

## 8. LVGL task watchdog: don’t create heavy screens at startup (Wallet freeze)

**Problem:** Tapping the Wallet button caused the device to freeze; logs showed "Task watchdog got triggered" with `taskLVGL` running on CPU 0 and IDLE0 not resetting in time.

**Cause:** When we added the password flow (note-for-password screen and password-for-encryption screen with keyboard), we started creating **both screens at app startup** in `ui_init()`. That meant more LVGL objects in memory and more work for the LVGL task every frame. The Wallet tap still did the same work (SD existence check + optionally creating the wallet-waiting screen), but the **baseline load** from the two extra screens (especially the full on-screen keyboard) pushed the LVGL task over the watchdog limit when the user then tapped Wallet.

**Fix:** Create those screens **on demand** instead of at startup — same pattern as the wallet generation waiting screen. Remove `ui_note_for_password_screen_init()` and `ui_password_for_encryption_screen_init()` from `ui_init()`. Navigate to them with `_ui_screen_change(..., &ui_note_for_password_screen_init)` (and the same for the password screen) so they are created the first time the user reaches that step. At boot we're back to the previous footprint; the new screens only exist when the user is in that flow.

**Takeaway:** On resource-limited targets (e.g. ESP32), avoid creating heavy LVGL screens (keyboards, many widgets) at startup. Create them on first use so the LVGL task stays under the task watchdog threshold.

---

## Quick reference

| Issue | Fix |
|-------|-----|
| BOOT press → restart | Don’t init camera in button callback; init in scan_task when entering SCANNING. |
| Restart when scanning starts | Give scan_task 24 KB stack. |
| QR never decodes (dimensions) | Use `quirc_resize(qr, width, height)` and pass real `fb->width`/`fb->height` to decoder. |
| QR never decodes (byte order) | Try grayscale with and without `__builtin_bswap16` (big-endian vs little-endian RGB565). |
| Orientation / ECC failures | Try decode with and without `quirc_flip(&code)`; try vertically flipped grayscale image. |
| uint8_t / build errors in qr_decoder.h | Add `#include <stdint.h>`. |
| Wallet tap → freeze / task watchdog (taskLVGL) | Don't create note-for-password and password-for-encryption screens at startup; create them on demand when navigating to them. |

---

*Captured so we don’t forget: callback light, stack big, quirc (w,h), use fb dimensions, try both byte orders and flip code + flip image; heavy LVGL screens (e.g. keyboard) on demand, not at init.*
