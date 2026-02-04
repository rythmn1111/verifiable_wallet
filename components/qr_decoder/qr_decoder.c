/**
 * QR decoder stub when camera/QR is disabled.
 * Always returns "no code". For real decoding, add espressif/quirc and restore the quirc-based implementation.
 */
#include "qr_decoder.h"

bool qr_decoder_decode_rgb565(const uint8_t *rgb565, int width, int height,
                              char *out_str, size_t out_len)
{
    (void)rgb565;
    (void)width;
    (void)height;
    if (out_str != NULL && out_len > 0) {
        out_str[0] = '\0';
    }
    return false;
}
