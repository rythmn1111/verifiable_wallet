/**
 * QR decoder using espressif/quirc (from qrcode-demo).
 * RGB565 -> grayscale conversion and quirc flow based on Espressif qrcode-demo.
 */
#include "qr_decoder.h"
#include "quirc.h"
#include <string.h>
#include <sys/param.h>

static struct quirc *s_qr;
static int s_last_w, s_last_h;

/* RGB565 to grayscale (from qrcode-demo) */
typedef union {
    uint16_t val;
    struct {
        uint16_t b : 5;
        uint16_t g : 6;
        uint16_t r : 5;
    };
} rgb565_t;

static uint8_t rgb565_to_grayscale_pixel(const uint8_t *img, int use_bswap)
{
    uint16_t *img_16 = (uint16_t *)img;
    uint16_t raw = *img_16;
    rgb565_t rgb = { .val = use_bswap ? __builtin_bswap16(raw) : raw };
    uint16_t val = (rgb.r * 8 + rgb.g * 4 + rgb.b * 8) / 3;
    return (uint8_t)MIN(255, val);
}

static void rgb565_to_grayscale_buf(const uint8_t *src, uint8_t *dst, int width, int height, int use_bswap)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            dst[y * width + x] = rgb565_to_grayscale_pixel(&src[(y * width + x) * 2], use_bswap);
        }
    }
}

static void flip_grayscale_vertical(uint8_t *buf, int width, int height)
{
    int half = height / 2;
    for (int y = 0; y < half; y++) {
        int row_upper = y * width;
        int row_lower = (height - 1 - y) * width;
        for (int x = 0; x < width; x++) {
            uint8_t t = buf[row_upper + x];
            buf[row_upper + x] = buf[row_lower + x];
            buf[row_lower + x] = t;
        }
    }
}

static int try_decode(struct quirc *qr, uint8_t *qr_buf, int width, int height,
                      char *out_str, size_t out_len, int try_flip_code)
{
    int count = quirc_count(qr);
    if (count <= 0) {
        return 0;
    }
    struct quirc_code code;
    struct quirc_data qr_data;
    quirc_extract(qr, 0, &code);
    if (try_flip_code) {
        quirc_flip(&code);
    }
    if (quirc_decode(&code, &qr_data) != 0) {
        return 0;
    }
    size_t copy_len = (size_t)qr_data.payload_len;
    if (copy_len >= out_len) {
        copy_len = out_len - 1;
    }
    memcpy(out_str, qr_data.payload, copy_len);
    out_str[copy_len] = '\0';
    return 1;
}

bool qr_decoder_decode_rgb565(const uint8_t *rgb565, int width, int height,
                              char *out_str, size_t out_len)
{
    if (rgb565 == NULL || width <= 0 || height <= 0 || out_str == NULL || out_len == 0) {
        return false;
    }
    out_str[0] = '\0';

    if (s_qr == NULL) {
        s_qr = quirc_new();
        if (s_qr == NULL) {
            return false;
        }
        s_last_w = s_last_h = 0;
    }

    if (s_last_w != width || s_last_h != height) {
        if (quirc_resize(s_qr, width, height) < 0) {
            return false;
        }
        s_last_w = width;
        s_last_h = height;
    }

    uint8_t *qr_buf = quirc_begin(s_qr, NULL, NULL);
    if (qr_buf == NULL) {
        return false;
    }

    /* Try big-endian RGB565 first (bswap), then little-endian (no bswap). OV5640/OV2640 vary. */
    for (int use_bswap = 1; use_bswap >= 0; use_bswap--) {
        rgb565_to_grayscale_buf(rgb565, qr_buf, width, height, use_bswap);
        quirc_end(s_qr);
        if (try_decode(s_qr, qr_buf, width, height, out_str, out_len, 1)) {
            return true;
        }
        if (try_decode(s_qr, qr_buf, width, height, out_str, out_len, 0)) {
            return true;
        }
        /* Retry with image flipped vertically (upside-down codes) */
        qr_buf = quirc_begin(s_qr, NULL, NULL);
        if (qr_buf == NULL) {
            return false;
        }
        rgb565_to_grayscale_buf(rgb565, qr_buf, width, height, use_bswap);
        flip_grayscale_vertical(qr_buf, width, height);
        quirc_end(s_qr);
        if (try_decode(s_qr, qr_buf, width, height, out_str, out_len, 1)) {
            return true;
        }
        if (try_decode(s_qr, qr_buf, width, height, out_str, out_len, 0)) {
            return true;
        }
        qr_buf = quirc_begin(s_qr, NULL, NULL);
        if (qr_buf == NULL) {
            return false;
        }
    }
    return false;
}
