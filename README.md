# Verifiable Wallet (Arweave hardware wallet)

An **ESP32-S3** firmware that turns a touchscreen + camera board into an **Arweave-compatible hardware wallet**: generate a wallet on-device, show public key/address as QR codes, and sign data-item hashes for uploads (e.g. via the included web demo). The same firmware supports **wallet mode** and **scanner mode** (scan a hash QR → sign → reboot back to wallet).

---

## What you need

### Hardware (tested setup)

- **Board:** ESP32-S3-Touch-LCD-3.5 (Waveshare) — 3.5" 320×480 touch LCD, ST7796, FT6336 touch
- **SD card:** Inserted in the board’s slot (used for wallet storage and optional Wi‑Fi creds)
- **Camera:** Board with OV3660/OV5640 or compatible camera (used in scanner mode to scan hash QR codes)
- **USB:** For flashing and serial log

Pinout is defined for this board in `include/board_pins.h` and in `components/esp_port/` (LCD, touch, SD, camera). To use a **different board**, you’ll need to adjust those pins and possibly the display/touch drivers.

### Software

- **ESP-IDF v5.1 or newer**  
  Install and activate: [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
- **Python 3** (for ESP-IDF tools)

---

## Build and flash

1. **Clone the repo** (and submodules if any):
   ```bash
   cd verifiable_wallet
   ```

2. **Set the target and build:**
   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```

3. **Flash and open serial monitor:**
   ```bash
   idf.py -p /dev/cu.usbserial-XXXX flash monitor
   ```
   Replace the port with your board’s serial port (e.g. on Windows: `COM3`).

4. **Optional:** Use the project’s `sdkconfig.defaults` (16 MB flash, PSRAM, partition table, etc.):
   ```bash
   idf.py fullclean
   idf.py set-target esp32s3
   idf.py build
   ```

---

## Using the device

### First run: create a wallet

1. Power on with an **SD card** inserted.
2. From the home screen, go to **Wallet** and choose **Create wallet** (or equivalent).
3. Follow the flow: choose 12-word mnemonic, write down the words, set a **password** to encrypt the key on SD.
4. After creation, the wallet is stored under `/sdcard/wallet/` (encrypted JWK, address, 512-byte owner for “Full Public Key” QR). **Back up your mnemonic and password.**

### Wallet menu (after opening the wallet)

- **Public key** — Shows the **Arweave address** (32-byte → 43-character base64url) as a QR and text. Use this for receiving AR or identifying the wallet.
- **Full Public Key** — Shows the **512-byte owner** (RSA public key) as a QR. Use this in the upload demo when doing **hardware-signed** uploads (the web app needs the full owner, not the short address).
- **Sign tx** — Starts the “sign a data-item hash” flow (see below).
- **Export key** — Export the JWK (password required) for use in other apps (e.g. Wander, Arweave.app).
- **Delete wallet** — Deletes wallet data on SD (irreversible).

### Signing a transaction (hash) for uploads

The web demo uses a **hash QR → device signs → signature QR** flow:

1. On the **website**: select a file, scan your wallet’s **Full Public Key** QR (512-byte), then the app shows a **hash QR**.
2. On the **device**: tap **Sign tx**.
   - If a “pending” hash was saved (e.g. from scanner mode), the device shows the password screen, then the **signature QR**.
   - If not, the device **reboots into scanner mode**: point the camera at the **hash QR** on the website. On success it saves the hash and reboots back to wallet mode.
3. On the device again: open **Wallet → Sign tx**. Enter password; the device shows the **signature QR**.
4. On the **website**: scan the **signature QR**, then click **Upload (hardware-signed)**.

Signatures are **RSA-PSS with SHA-256** (512-byte), matching Arweave/ANS-104 and the upload gateway.

---

## Web upload demo (hardware-signed uploads)

The repo includes a small **Vite + React** app that builds a data-item hash, shows it as a QR, then accepts a hardware signature and uploads to Arweave (via Turbo).

1. **Go to the app:**
   ```bash
   cd uploading_app_demo/uploading_app_demo
   npm install
   npm run dev
   ```
2. Open the URL shown (e.g. `http://localhost:5173`).
3. **Hardware flow:**
   - **Scan public key QR** — Use the device’s **Full Public Key** (Wallet → Full Public Key), not the short “Public key” address.
   - **Select image** (e.g. under 100 KB).
   - **Show hash QR** — The app displays a QR; in scanner mode the device scans it, or you can use “Sign tx” after a scanner reboot to get the hash from SD.
   - **Sign on device** — Enter password and show the **signature QR**.
   - **Scan signature QR** in the app, then **Upload (hardware-signed)**.

Protocol details (hash format, 512-byte owner, 512-byte signature) are in `uploading_app_demo/uploading_app_demo/HARDWARE_PROTOCOL.md`.

---

## Project layout (high level)

| Path | Purpose |
|------|--------|
| `src/main.cpp` | Entry, NVS, display/touch/camera/SD init, wallet vs scanner boot, Wi‑Fi, LVGL |
| `src/wallet/` | LVGL UI: screens (wallet menu, QR screens, password, sign tx, export, etc.) |
| `src/wallet_*.c`, `src/arweave_wallet_gen.c` | Wallet gen (BIP39 + RSA JWK), address, encryption, SD read/write, signing |
| `src/boot_mode.c` | NVS “boot mode” and “pending sign hash” (scanner → wallet) |
| `components/esp_port/` | Board-specific: LCD, touch, SD card, camera (pins and init) |
| `include/board_pins.h` | LCD/touch pin definitions for ESP32-S3-Touch-LCD-3.5 |
| `uploading_app_demo/` | Web app: build hash, show hash QR, scan owner + signature, upload signed data item |
| `WALLET_RECIPE.md` | Arweave wallet format: BIP39 → seed → RSA 4096 → JWK, address = base64url(SHA256(n)) |

---

## Adapting to your own device

1. **Board / pins**
   - Set **LCD and touch** pins in `include/board_pins.h` and in the display/touch init used by `main.cpp` (e.g. `esp_3inch5_lcd_port`, `esp_lvgl_port`).
   - Set **SD card** pins in `components/esp_port/esp_sdcard_port.cpp` (e.g. `EXAMPLE_PIN_SD_CMD`, `EXAMPLE_PIN_SD_D0`, `EXAMPLE_PIN_SD_CLK`). SDMMC 1-line is used by default.
   - Set **camera** pins and sensor in `components/esp_port/esp_camera_port.cpp` (D0–D7, XCLK, PCLK, VSYNC, HREF, SCCB/I2C). Match your module’s schematic.

2. **Display driver**
   - The current code assumes an ST7796-style LCD and the port in `esp_3inch5_lcd_port`. For another display, replace or adapt that port and the resolution (320×480) in `main.cpp` (`EXAMPLE_LCD_H_RES`, `EXAMPLE_LCD_V_RES`).

3. **LVGL**
   - UI is in `src/wallet/` (SquareLine-style screens). Resolution and rotation are set in `main.cpp`; ensure they match your display.

4. **PSRAM / flash**
   - `sdkconfig.defaults` enables 8 MB PSRAM (OCT) and 16 MB flash. If your board has less, change PSRAM/flash settings and possibly reduce LVGL or camera buffers.

5. **No camera**
   - You can still use “Sign tx” if the hash is provided by another path (e.g. copy to SD as `temp_sig`). Scanner mode and the hash QR scan step won’t work without a camera.

---

## Security notes

- **Mnemonic and password:** The device does not transmit the mnemonic or password. The key is encrypted on SD; signing uses the password only on-device.
- **Backup:** Export the JWK (or save the 12 words + password) to restore the wallet elsewhere.
- **Full Public Key:** The 512-byte owner is public (like a public key). Only share the **signature QR** after you intend to complete an upload; the signature is bound to the hash the app showed.

---

## References

- **Wallet format:** `WALLET_RECIPE.md` (BIP39, RSA 4096, JWK, Arweave address).
- **Hardware ↔ app protocol:** `uploading_app_demo/uploading_app_demo/HARDWARE_PROTOCOL.md`.
- **Display bring-up:** `DISPLAY_SETUP.md` (for the 3.5" board used in development).
