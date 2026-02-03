# ESP32-S3-Touch-LCD-3.5 Display Setup — What We Learned

Documentation of how we got "Hello World" on the **ESP32-S3-Touch-LCD-3.5** display, for future reference.

---

## Board & Display

- **Board:** ESP32-S3-Touch-LCD-3.5 (3.5" 320×480 touch LCD)
- **Display driver:** ST7796 (IPS)
- **Resolution:** 320 × 480

---

## Pin Definitions (Final Working Config)

| Purpose     | GPIO | Notes |
|------------|------|--------|
| **Backlight** | 6  | Must be driven HIGH or screen stays black. |
| **LCD DC**    | 3  | Data/Command (from schematic NLIO3). |
| **LCD CS**    | -1 | No hardware CS — only device on SPI bus. |
| **LCD RST**   | -1 | Reset is **not** a GPIO; it’s via TCA9554 (see below). |
| **SPI SCK**   | 5  | NLIO5. |
| **SPI MOSI**  | 1  | NLIO1. |
| **SPI MISO**  | 2  | NLIO2. |
| **I2C SDA**   | 8  | TCA9554 I2C. |
| **I2C SCL**   | 7  | TCA9554 I2C. |

**Important:** Using GPIOs 4 or 7 for CS/RST does **not** work on this board. CS and RST must be `-1`; reset is done via the TCA9554.

---

## Why TCA9554?

The LCD reset line is not connected to a direct GPIO. It’s driven by the **TCA9554** I2C port expander (address **0x20**).

- **TCA9554 pin 1** = LCD reset.
- We use I2C (SDA=8, SCL=7) to talk to the TCA9554, then toggle pin 1 to reset the display.

**Reset sequence (must run before `gfx->begin()`):**

1. `Wire.begin(I2C_SDA, I2C_SCL);`
2. `TCA.begin();`
3. `TCA.pinMode1(1, OUTPUT);`
4. `lcd_reset()`:
   - TCA pin 1 → HIGH, delay 10 ms  
   - TCA pin 1 → LOW, delay 10 ms  
   - TCA pin 1 → HIGH, delay 200 ms  

Then call `gfx->begin()`.

---

## SPI Bus: Arduino_HWSPI vs Arduino_ESP32SPI

- **Arduino_ESP32SPI** (used in the vendor demo) can pull in `esp32-hal-periman.h`, which may not exist or match in all Arduino-ESP32 versions.
- We use **Arduino_HWSPI** with:
  - DC = 3  
  - CS = -1  
  - SCK = 5, MOSI = 1, MISO = 2  

So we avoid the periman dependency while keeping the same pinout and behavior.

---

## Libraries

- **Arduino_GFX** — display driver (e.g. from GitHub: `moononournation/Arduino_GFX`).
- **TCA9554** — I2C port expander for LCD reset. We use the implementation from the **ESP32-S3-Touch-LCD-3.5-Demo** (`Arduino/libraries/TCA9554`), copied into this project’s `lib/TCA9554/` (both `.h` and `.cpp`; header includes derived classes so the `.cpp` compiles).

---

## Init Order in `setup()`

1. `Serial.begin(115200);`
2. `Wire.begin(I2C_SDA, I2C_SCL);`
3. `TCA.begin();` and `TCA.pinMode1(1, OUTPUT);`
4. `lcd_reset();`
5. `gfx->begin();`
6. Backlight: `pinMode(LCD_BL, OUTPUT); digitalWrite(LCD_BL, HIGH);`
7. Then clear screen, set text, draw "Hello World", etc.

---

## Quick Reference Snippet

```cpp
#define LCD_BL    6
#define LCD_DC    3
#define LCD_CS   -1
#define LCD_RST  -1
#define SPI_SCLK  5
#define SPI_MOSI  1
#define SPI_MISO  2
#define I2C_SDA   8
#define I2C_SCL   7

TCA9554 TCA(0x20);
Arduino_DataBus *bus = new Arduino_HWSPI(LCD_DC, LCD_CS, SPI_SCLK, SPI_MOSI, SPI_MISO);
Arduino_GFX *gfx = new Arduino_ST7796(bus, LCD_RST, 0, true, 320, 480);

void lcd_reset(void) {
  TCA.write1(1, 1); delay(10);
  TCA.write1(1, 0); delay(10);
  TCA.write1(1, 1); delay(200);
}
```

---

## Build & Upload

- **PlatformIO** env: `esp32-s3-touch-lcd-35` (or equivalent for this board).
- Upload: use PlatformIO’s Upload, or the VS Code task that auto-detects the port (see `.vscode/tasks.json`). Don’t hardcode a port that might change.

---

*Learned and documented so we don’t forget: CS/RST = -1, reset via TCA9554 pin 1, Arduino_HWSPI, backlight on 6.*

---

# QR Code Scanner — Full Documentation

This section documents the **live camera + continuous QR scan** implementation that runs on the same ESP32-S3-Touch-LCD-3.5 board. It uses the display for a live preview (top 320×240) and a status/results area (bottom 240 px), and decodes QR codes from the camera feed without a capture button.

---

## QR Scanner Overview

- **Behavior:** Continuous live preview from the camera; QR decode runs every few frames on the same frame we display. No button — just point the camera at a QR code.
- **Camera:** OV2640 (or compatible) on the ESP32-S3-Touch-LCD-3.5 module. Uses **esp32-camera** and a custom pin set for this board.
- **Decoder:** **quirc** (QR code recognition library), bundled inside `lib/ESP32QRCodeReader` (we use the quirc sources and `ESP32CameraPins.h` from that lib; we do *not* use the ESP32QRCodeReader background task — we run decode in the main loop).
- **Display layout:** Top 320×240 = live preview (center crop from 480×320). Bottom 240 px = status line + decoded text or last error message.

---

## Camera Pins (ESP32-S3-Touch-LCD-3.5)

The camera pin set is defined as `CAMERA_MODEL_ESP32_S3_TOUCH_LCD_35` in `lib/ESP32QRCodeReader/include/ESP32CameraPins.h`. **Do not use a different board’s pin set** or the camera will not work.

| Pin role   | GPIO | Notes |
|------------|------|--------|
| XCLK       | 38   | |
| PCLK       | 41   | |
| VSYNC      | 17   | |
| HREF       | 18   | |
| SIOD (SDA) | 8    | I2C data for sensor (shared I2C bus with TCA9554). |
| SIOC (SCL) | 7    | I2C clock for sensor. |
| Y2–Y9      | 45, 47, 48, 46, 42, 40, 39, 21 | Data pins D0–D7. |
| PWDN       | -1   | |
| RESET      | -1   | |

**Config in code:** We use `camera_config_t` with `pin_sccb_sda` / `pin_sccb_scl` (not the deprecated `pin_sscb_*`). Frame size is `FRAMESIZE_HVGA` (480×320), pixel format `PIXFORMAT_RGB565`, `fb_count = 1`, `fb_location = CAMERA_FB_IN_PSRAM`, `grab_mode = CAMERA_GRAB_WHEN_EMPTY`.

---

## Why HVGA (480×320)?

- **QVGA (320×240)** was too low resolution: quirc often saw the finder patterns but failed with ECC/format errors (not enough pixels per QR module).
- **HVGA (480×320)** gives ~1.5× more pixels so quirc can resolve the QR modules and decode reliably.
- The **preview** is still 320×240: we take a **center crop** (320×240) from the 480×320 frame and draw that on the top half of the screen. The **full 480×320** frame is converted to grayscale and passed to quirc for decode.

---

## Loop Task Stack Size (Critical)

The default Arduino loop task stack (**8 KB**) is **not enough**. Running quirc decode + display + camera in the same task caused **stack overflow in task loopTask** and reboots.

- **Fix:** Increase the loop task stack **before** any other includes that pull in Arduino loop setup:
  ```cpp
  #include <Arduino.h>
  SET_LOOP_TASK_STACK_SIZE(24 * 1024);   // 24 KB
  ```
- **Placement:** Must be right after `#include <Arduino.h>`. 24 KB is enough for this app; 16 KB might be marginal.
- **Reference:** Arduino-ESP32 provides `SET_LOOP_TASK_STACK_SIZE()`; see `Arduino.h` and the ArduinoStackSize example.

---

## Grayscale Conversion (From qrcode-demo)

quirc expects **grayscale** input. We capture **RGB565** for a correct-looking preview, then convert the same buffer to grayscale for decode. Two details from Espressif’s **qrcode-demo** made decoding reliable:

1. **Byte order:** The camera/display buffer is treated as **big-endian** RGB565. We read each 16-bit pixel with `__builtin_bswap16()` so R/G/B are interpreted correctly. Without this, the grayscale image can be wrong and quirc may fail or never lock onto a code.
2. **Formula:** Grayscale is computed as `(r*8 + g*4 + b*8) / 3` (with R/G/B from the 5/6/5 bits). This matches the qrcode-demo and works well for quirc.

**Snippet (concept):**
```cpp
uint16_t p = __builtin_bswap16(*((const uint16_t *)&src[i * 2]));
int r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
gray[i] = (uint8_t)((r * 8 + g * 4 + b * 8) / 3);
```

---

## Orientation Retry (Flip)

quirc can find a QR but fail to decode if the code is **rotated or upside-down** (e.g. ECC/format errors). The qrcode-demo uses `quirc_flip(&code)` after extract; our bundled quirc doesn’t expose that API.

- **What we do:** After a failed decode, we **flip the grayscale image vertically in-place**, call quirc again, then flip back. So we effectively try “normal” and “upside-down” orientations without adding `quirc_flip` to the library.
- **Implementation:** `flipGrayVertical(gray, w, h)` swaps rows; we call it before the retry and again after to restore the buffer for the next frame.

---

## Buffers and Memory

- **grayBuf:** Grayscale copy of the full frame for quirc. Size = `CAM_W * CAM_H` = 480×320 = 153 600 bytes. Prefer PSRAM (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`).
- **previewCropBuf:** RGB565 center crop for the preview. Size = 320×240×2 = 153 600 bytes. Also prefer PSRAM.
- **quirc:** One `struct quirc *` from `quirc_new()`; `quirc_resize(qr, w, h)` allocates internal buffers for that size.
- **Camera:** Single frame buffer in PSRAM (`fb_count = 1`). We get a frame, draw preview + (every Nth frame) convert to gray and decode, then return the frame.

---

## Scan Frequency

- We run QR decode **every 3rd frame** (`SCAN_EVERY_N_FRAMES 3`) to balance responsiveness and CPU load. The preview is updated every frame.
- `delay(1)` is used inside the scan block to yield to the watchdog and other tasks.
- The bottom status area is updated when we decode: on success we show “Scanned:” + payload; on failure we show “Scanning...” + last error (e.g. “No QR in frame” or “Decode: ECC failure”) so you can see what the pipeline is doing.

---

## Init Order (Display + Camera + QR)

1. Serial, Wire, TCA, `lcd_reset()`, `gfx->begin()`, backlight on, `fillScreen(BLACK)`.
2. **Camera:** `initCamera()` — sets `camera_config_t` from `CAMERA_MODEL_ESP32_S3_TOUCH_LCD_35`, `esp_camera_init()`, then `sensor_t::set_vflip(s, 1)` so the preview is right-side up.
3. **Buffers:** Allocate `grayBuf` and `previewCropBuf` (PSRAM first, fallback to heap).
4. **quirc:** `qr = quirc_new()`.
5. Draw initial bottom area (“Scanning...”, “Point at QR (HVGA)”).

**Camera init failure:** If `esp_camera_init` fails, we show “Camera init failed. Check PSRAM & wiring.” and halt. PSRAM is required for the frame buffer.

---

## Libraries and Dependencies

| What | Where / how |
|------|------------------|
| **Display** | Arduino_GFX (ST7796), TCA9554 — see display section above. |
| **Camera** | `espressif/esp32-camera` (PlatformIO `lib_deps`). |
| **Camera pins** | `lib/ESP32QRCodeReader/include/ESP32CameraPins.h` — use `CAMERA_MODEL_ESP32_S3_TOUCH_LCD_35`. |
| **quirc** | `lib/ESP32QRCodeReader/src/quirc/` (quirc.c, identify.c, decode.c, quirc.h). We include `quirc/quirc.h` and link the quirc sources; we do not use the ESP32QRCodeReader wrapper task. |
| **Reference demo** | `qrcode-demo/` (Espressif ESP-IDF qrcode-demo) — we reused the grayscale conversion and the idea of trying an alternate orientation (flip) to improve decode. |

---

## Build Flags / PlatformIO

- **PSRAM:** `BOARD_HAS_PSRAM` (or equivalent) so the camera and our buffers can use PSRAM.
- **USB CDC:** `ARDUINO_USB_CDC_ON_BOOT=1` if you use USB serial for upload/monitor.
- Board: `esp32-s3-devkitc-1` (or the one that matches ESP32-S3-Touch-LCD-3.5).

---

## Tips for Best Scan Results

- **Lighting:** Good, even lighting. Avoid strong backlight or shadows across the QR.
- **Distance / size:** Fill a reasonable part of the frame; don’t put a tiny QR in a corner. HVGA helps, but the code still needs enough pixels per module.
- **Stability:** Hold steady for a moment so quirc can lock and decode; the flip retry often helps if the code is upside-down.
- **Content:** Works with URLs, text, Wi‑Fi credentials, etc. Decoded payload is shown in the bottom area and sent over Serial.

---

## Quick Reference: Key Defines and Flow

```text
CAM_W=480, CAM_H=320     — Camera resolution (HVGA).
PREVIEW_W=320, PREVIEW_H=240, BOTTOM_Y=240  — Display layout.
SCAN_EVERY_N_FRAMES=3    — Decode every 3rd frame.
SET_LOOP_TASK_STACK_SIZE(24*1024)  — Required for quirc + display.

Loop:
  fb = esp_camera_fb_get()
  crop 320×240 from center of fb → previewCropBuf, draw to (0,0)
  every 3rd frame:
    rgb565ToGray(fb->buf, grayBuf, w, h)   // byte-swap + (r*8+g*4+b*8)/3
    decodeQR(grayBuf, w, h)                // decode; on fail, flip gray and retry
    update bottom area with "Scanned: ..." or "Scanning..." + lastResult
  esp_camera_fb_return(fb)
  delay(40)
```

---

*QR scanner documented: HVGA, TCA9554 + display init first, camera pins from ESP32CameraPins.h, 24 KB loop stack, qrcode-demo grayscale + flip retry, center-crop preview, decode every 3 frames.*
