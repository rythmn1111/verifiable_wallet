# QR decoder

Stub implementation: always returns "no code". For real QR decoding, integrate **quirc** (https://github.com/dlbeer/quirc):

1. Clone quirc and copy `lib/*.c` and `lib/*.h` into this component (or add as submodule).
2. In `qr_decoder.c`: convert RGB565 to grayscale, call `quirc_new()`, `quirc_resize()`, `quirc_begin()`/fill/`quirc_end()`, `quirc_count()`, `quirc_extract()`/`quirc_decode()`, then copy payload to `out_str`.
