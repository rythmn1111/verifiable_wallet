#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decode QR code from RGB565 camera frame.
 * @param rgb565   Frame buffer (width * height * 2 bytes)
 * @param width    Frame width (e.g. 320)
 * @param height   Frame height (e.g. 480)
 * @param out_str  Output buffer for decoded text
 * @param out_len  Size of out_str
 * @return true if a QR code was decoded, false otherwise
 */
bool qr_decoder_decode_rgb565(const uint8_t *rgb565, int width, int height,
                              char *out_str, size_t out_len);

#ifdef __cplusplus
}
#endif
