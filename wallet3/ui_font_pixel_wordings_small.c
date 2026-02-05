/*******************************************************************************
 * Size: 9 px
 * Bpp: 1
 * Opts: --bpp 1 --size 9 --font /Users/rythmn/SquareLine/assets/PressStart2P-Regular.ttf -o /Users/rythmn/SquareLine/assets/ui_font_pixel_wordings_small.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "ui.h"

#ifndef UI_FONT_PIXEL_WORDINGS_SMALL
#define UI_FONT_PIXEL_WORDINGS_SMALL 1
#endif

#if UI_FONT_PIXEL_WORDINGS_SMALL

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xed, 0x86,

    /* U+0022 "\"" */
    0xcf, 0x3c, 0xc0,

    /* U+0023 "#" */
    0x6c, 0xff, 0x6c, 0x6c, 0x6c, 0x6c, 0xff, 0x6c,

    /* U+0024 "$" */
    0x10, 0x7e, 0xd0, 0x7e, 0x12, 0x13, 0xfe, 0x10,

    /* U+0025 "%" */
    0x61, 0xa2, 0xc4, 0x10, 0x0, 0x23, 0x45, 0x86,

    /* U+0026 "&" */
    0x70, 0xcc, 0xcc, 0x70, 0x50, 0xcd, 0xc6, 0x7f,

    /* U+0027 "'" */
    0xfc,

    /* U+0028 "(" */
    0x36, 0xcc, 0xc4, 0x63,

    /* U+0029 ")" */
    0xc6, 0x33, 0x32, 0x6c,

    /* U+002A "*" */
    0x66, 0x28, 0x38, 0xff, 0x38, 0x66,

    /* U+002B "+" */
    0x30, 0x60, 0xc7, 0xf3, 0x6, 0x0,

    /* U+002C "," */
    0x6f, 0x0,

    /* U+002D "-" */
    0xfe,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x1, 0x2, 0x4, 0x10, 0x0, 0x20, 0x40, 0x80,

    /* U+0030 "0" */
    0x38, 0x46, 0xc3, 0xc3, 0xc3, 0x42, 0x62, 0x3c,

    /* U+0031 "1" */
    0x30, 0xe0, 0xc1, 0x83, 0x6, 0xc, 0x7f,

    /* U+0032 "2" */
    0x7e, 0xc3, 0xf, 0xe, 0x3e, 0x7c, 0xe0, 0xff,

    /* U+0033 "3" */
    0x7f, 0x6, 0xc, 0x3e, 0x2, 0x3, 0xc3, 0x7e,

    /* U+0034 "4" */
    0x1c, 0x79, 0xb2, 0x6c, 0xdf, 0xc3, 0x6,

    /* U+0035 "5" */
    0xfe, 0xc0, 0xfe, 0x2, 0x3, 0x3, 0xc3, 0x7e,

    /* U+0036 "6" */
    0x3e, 0x60, 0xc0, 0xff, 0xc3, 0x42, 0x42, 0x7e,

    /* U+0037 "7" */
    0xff, 0xc3, 0x6, 0x6, 0x18, 0x30, 0x30, 0x30,

    /* U+0038 "8" */
    0x7c, 0xc2, 0xe2, 0x64, 0x9c, 0x9f, 0x83, 0x7e,

    /* U+0039 "9" */
    0x7e, 0xc3, 0xc3, 0x7f, 0x2, 0x2, 0x6, 0x7c,

    /* U+003A ":" */
    0xf0, 0xf0,

    /* U+003B ";" */
    0x6c, 0x6, 0xf0,

    /* U+003C "<" */
    0xc, 0xc6, 0x30, 0x40, 0x83, 0x3,

    /* U+003D "=" */
    0xff, 0x0, 0xff,

    /* U+003E ">" */
    0xc1, 0x83, 0x3, 0x10, 0x86, 0x30,

    /* U+003F "?" */
    0x7e, 0xff, 0xc3, 0x2, 0x6, 0x38, 0x0, 0x38,

    /* U+0040 "@" */
    0x7e, 0x81, 0xbd, 0xa5, 0xa5, 0xbf, 0x80, 0x7e,

    /* U+0041 "A" */
    0x38, 0x66, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3,

    /* U+0042 "B" */
    0xfe, 0xc3, 0xc3, 0xfe, 0xc2, 0xc3, 0xc3, 0xfe,

    /* U+0043 "C" */
    0x3e, 0x63, 0xc0, 0xc0, 0xc0, 0x40, 0x63, 0x3e,

    /* U+0044 "D" */
    0xfc, 0xc6, 0xc3, 0xc3, 0xc3, 0xc2, 0xc6, 0xfc,

    /* U+0045 "E" */
    0xff, 0xc0, 0xc0, 0xfe, 0xc0, 0xc0, 0xc0, 0xff,

    /* U+0046 "F" */
    0xff, 0xc0, 0xc0, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+0047 "G" */
    0x3f, 0x60, 0xc0, 0xcf, 0xc3, 0x43, 0x63, 0x3f,

    /* U+0048 "H" */
    0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3,

    /* U+0049 "I" */
    0xfe, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x7f,

    /* U+004A "J" */
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0xc3, 0x7e,

    /* U+004B "K" */
    0xc3, 0xc6, 0xcc, 0xc8, 0xf8, 0xf4, 0xc6, 0xc7,

    /* U+004C "L" */
    0xc1, 0x83, 0x6, 0xc, 0x18, 0x30, 0x7f,

    /* U+004D "M" */
    0xc3, 0xef, 0xff, 0xd3, 0xd3, 0xc3, 0xc3, 0xc3,

    /* U+004E "N" */
    0xc3, 0xe3, 0xfb, 0xdb, 0xc7, 0xc7, 0xc3, 0xc3,

    /* U+004F "O" */
    0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e,

    /* U+0050 "P" */
    0xfe, 0xc3, 0xc3, 0xc3, 0xc2, 0xfe, 0xc0, 0xc0,

    /* U+0051 "Q" */
    0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0xdf, 0xc6, 0x7d,

    /* U+0052 "R" */
    0xfe, 0xc3, 0xc3, 0xc7, 0xc4, 0xfc, 0xce, 0xc7,

    /* U+0053 "S" */
    0x7e, 0xc3, 0xc0, 0x7e, 0x2, 0x3, 0xc3, 0x7e,

    /* U+0054 "T" */
    0xfe, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x18,

    /* U+0055 "U" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e,

    /* U+0056 "V" */
    0xc3, 0xc3, 0xc3, 0x42, 0x2c, 0x38, 0x38, 0x10,

    /* U+0057 "W" */
    0xd3, 0xd3, 0xd3, 0xd3, 0xd3, 0xff, 0xef, 0x42,

    /* U+0058 "X" */
    0xc3, 0xc3, 0x66, 0x38, 0x28, 0x46, 0xc3, 0xc3,

    /* U+0059 "Y" */
    0xc7, 0x8f, 0x1a, 0x24, 0x86, 0xc, 0x18,

    /* U+005A "Z" */
    0xff, 0x7, 0xe, 0x38, 0x70, 0x60, 0xe0, 0xff,

    /* U+005B "[" */
    0xfc, 0xcc, 0xcc, 0xcf,

    /* U+005C "\\" */
    0x80, 0x40, 0x20, 0x10, 0x0, 0x4, 0x2, 0x1,

    /* U+005D "]" */
    0xf3, 0x33, 0x33, 0x3f,

    /* U+005E "^" */
    0x73, 0x30,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x7e, 0x3, 0x3, 0x7f, 0xc3, 0x7f,

    /* U+0062 "b" */
    0xc0, 0xc0, 0xfe, 0xc2, 0xc3, 0xc3, 0xc3, 0x7e,

    /* U+0063 "c" */
    0x7f, 0x40, 0xc0, 0xc0, 0xc0, 0x7f,

    /* U+0064 "d" */
    0x3, 0x3, 0x7f, 0x43, 0xc3, 0xc3, 0xc3, 0x7f,

    /* U+0065 "e" */
    0x7e, 0x42, 0xc3, 0xff, 0xc0, 0x7e,

    /* U+0066 "f" */
    0xe, 0x63, 0xf9, 0x83, 0x6, 0xc, 0x18,

    /* U+0067 "g" */
    0x7f, 0xc3, 0xc3, 0x43, 0x7f, 0x3, 0x7e,

    /* U+0068 "h" */
    0xc0, 0xc0, 0xfe, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3,

    /* U+0069 "i" */
    0x30, 0x1, 0xc1, 0x83, 0x6, 0xc, 0x7f,

    /* U+006A "j" */
    0xc, 0x1, 0xc3, 0xc, 0x30, 0xc3, 0xf8,

    /* U+006B "k" */
    0xc0, 0xc0, 0xc3, 0xc2, 0xc4, 0xfc, 0xc6, 0xc3,

    /* U+006C "l" */
    0x70, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x7f,

    /* U+006D "m" */
    0xfe, 0xb2, 0xb2, 0xb3, 0xb3, 0xb3,

    /* U+006E "n" */
    0xfe, 0xc2, 0xc2, 0xc3, 0xc3, 0xc3,

    /* U+006F "o" */
    0x7e, 0x42, 0xc3, 0xc3, 0xc3, 0x7e,

    /* U+0070 "p" */
    0xfe, 0xc3, 0xc3, 0xc2, 0xfe, 0xc0, 0xc0,

    /* U+0071 "q" */
    0x7f, 0xc3, 0xc3, 0x43, 0x7f, 0x3, 0x3,

    /* U+0072 "r" */
    0xcf, 0xe3, 0x6, 0xc, 0x18, 0x0,

    /* U+0073 "s" */
    0x7e, 0xc0, 0x40, 0x7e, 0x3, 0xfe,

    /* U+0074 "t" */
    0x30, 0x63, 0xf9, 0x83, 0x6, 0xc, 0x18,

    /* U+0075 "u" */
    0xc3, 0xc3, 0xc3, 0x43, 0x43, 0x7f,

    /* U+0076 "v" */
    0xc7, 0x8f, 0x1a, 0x23, 0x86, 0x0,

    /* U+0077 "w" */
    0xd3, 0xd3, 0xd3, 0x42, 0x42, 0x66,

    /* U+0078 "x" */
    0xc3, 0x42, 0x2c, 0x38, 0x66, 0xc3,

    /* U+0079 "y" */
    0xc3, 0xc3, 0xc3, 0x43, 0x7f, 0x3, 0x7e,

    /* U+007A "z" */
    0xff, 0xe, 0x38, 0x38, 0x70, 0xff,

    /* U+007B "{" */
    0x36, 0x6c, 0x66, 0x63,

    /* U+007C "|" */
    0xff, 0xff,

    /* U+007D "}" */
    0xc6, 0x63, 0x66, 0x6c,

    /* U+007E "~" */
    0x70, 0xb9, 0xe,

    /* U+007F "" */
    0xcf, 0x30
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 144, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 144, .box_w = 3, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 4, .adv_w = 144, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 7, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 15, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 23, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 31, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 39, .adv_w = 144, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 40, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 44, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 48, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 54, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 60, .adv_w = 144, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 144, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 63, .adv_w = 144, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 64, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 72, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 80, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 87, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 95, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 103, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 110, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 118, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 126, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 134, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 142, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 150, .adv_w = 144, .box_w = 2, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 152, .adv_w = 144, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 155, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 161, .adv_w = 144, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 164, .adv_w = 144, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 170, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 178, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 186, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 194, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 202, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 210, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 218, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 226, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 234, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 242, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 250, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 257, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 265, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 273, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 280, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 288, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 296, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 304, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 312, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 320, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 328, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 336, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 343, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 351, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 359, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 367, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 375, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 382, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 390, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 394, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 402, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 406, .adv_w = 144, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 408, .adv_w = 144, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 409, .adv_w = 144, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 410, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 416, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 424, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 430, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 438, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 444, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 451, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 466, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 473, .adv_w = 144, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 488, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 495, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 501, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 507, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 513, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 520, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 533, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 539, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 546, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 552, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 558, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 564, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 570, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 577, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 583, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 587, .adv_w = 144, .box_w = 2, .box_h = 8, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 589, .adv_w = 144, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 593, .adv_w = 144, .box_w = 8, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 596, .adv_w = 144, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 96, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_pixel_wordings_small = {
#else
lv_font_t ui_font_pixel_wordings_small = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 9,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_PIXEL_WORDINGS_SMALL*/

