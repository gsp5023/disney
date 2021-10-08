/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 imagelib_gif.c

 stb_image.h modified/trimmed to support gif loading 'in place' in a specified memory region
 and support for imagelib loading gifs
 */

// stb image portions are licensed/copyright under:
/* stb_image - v2.23 - public domain image loader - http://nothings.org/stb
								   no warranty implied; use at your own risk
								   */

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/file_stbi.h"
#include "source/adk/runtime/runtime.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define IMAGELIB_GIF_TAG FOURCC('I', 'G', 'I', 'F')

#ifdef _MSC_VER
#define STBI_NOTUSED(v) (void)(v)

typedef unsigned short stbi__uint16;
typedef signed short stbi__int16;
typedef unsigned int stbi__uint32;
typedef signed int stbi__int32;

#define stbi_inline __forceinline
#else
#define STBI_NOTUSED(v) (void)sizeof(v)

typedef uint16_t stbi__uint16;
typedef int16_t stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t stbi__int32;

#ifdef __cplusplus
#define stbi_inline inline
#else
#define stbi_inline
#endif
#endif

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#define STBI_ASSERT ASSERT

#define STBI__BYTECAST(x) ((stbi_uc)((x)&255)) // truncate int to byte without warnings

enum {
    STBI__SCAN_load = 0,
    STBI__SCAN_type,
    STBI__SCAN_header
};

enum {
    STBI_ORDER_RGB,
    STBI_ORDER_BGR
};

enum {
    imagelib_interlace_huffman_max_overhead = (258 * 8)
};

///////////////////////////////////////////////
//
//  stbi__context struct and start_xxx functions

// stbi__context structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct stbi__context {
    stbi__uint32 img_x, img_y;
    int img_n, img_out_n;

    int buflen;
    stbi_uc buffer_start[128];

    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original, *img_buffer_original_end;

    stbi_uc *deflate_buffer, *deflate_buffer_end;
    stbi_uc *working_buffer, *working_buffer_end;
    int working_buffer_used;

} stbi__context;

static void stbi__start_mem_in_line(stbi__context * s, const stbi_uc * const pixel_buffer, const int pixel_buffer_len, stbi_uc * const deflate_buffer, const int deflate_buffer_len, stbi_uc * const working_buffer, const int working_buffer_len) {
    s->img_buffer = s->img_buffer_original = (stbi_uc *)pixel_buffer;
    s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)pixel_buffer + pixel_buffer_len;
    s->deflate_buffer = deflate_buffer;
    s->deflate_buffer_end = deflate_buffer + deflate_buffer_len;
    s->working_buffer = working_buffer;
    s->working_buffer_end = working_buffer + working_buffer_len;
    s->working_buffer_used = 0;
}

typedef struct
{
    int bits_per_channel;
    int num_channels;
    int channel_order;
} stbi__result_info;

static const char * stbi__g_failure_reason;

static const char * imagelib_gif_stbi_failure_reason(void) {
    return stbi__g_failure_reason;
}

static int stbi__err(const char * str) {
    stbi__g_failure_reason = str;
    return 0;
}

// stbi__errpf - error returning pointer to float
// stbi__errpuc - error returning pointer to unsigned char

#ifdef STBI_NO_FAILURE_STRINGS
#define stbi__err(x, y) 0
#elif defined(STBI_FAILURE_USERMSG)
#define stbi__err(x, y) stbi__err(y)
#else
#define stbi__err(x, y) stbi__err(x)
#endif

#define stbi__errpf(x, y) ((float *)(size_t)(stbi__err(x, y) ? NULL : NULL))
#define stbi__errpuc(x, y) ((unsigned char *)(size_t)(stbi__err(x, y) ? NULL : NULL))

// return 1 if the sum is valid, 0 on overflow.
// negative terms are considered invalid.
static int stbi__addsizes_valid(int a, int b) {
    if (b < 0)
        return 0;
    // now 0 <= b <= INT_MAX, hence also
    // 0 <= INT_MAX - b <= INTMAX.
    // And "a + b <= INT_MAX" (which might overflow) is the
    // same as a <= INT_MAX - b (no overflow)
    return a <= INT_MAX - b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__mul2sizes_valid(int a, int b) {
    if (a < 0 || b < 0)
        return 0;
    if (b == 0)
        return 1; // mul-by-0 is always safe
    // portable way to check for no overflows in a*b
    return a <= INT_MAX / b;
}

// returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow
static int stbi__mad3sizes_valid(int a, int b, int c, int add) {
    return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a * b, c) && stbi__addsizes_valid(a * b * c, add);
}

static void stbi__skip(stbi__context * s, int n) {
    if (n < 0) {
        s->img_buffer = s->img_buffer_end;
        return;
    }
    s->img_buffer += n;
}

stbi_inline static stbi_uc stbi__get8(stbi__context * s) {
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    return 0;
}

static int stbi__get16le(stbi__context * s) {
    int z = stbi__get8(s);
    return z + (stbi__get8(s) << 8);
}

// *************************************************************************************************
// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

typedef struct
{
    stbi__int16 prefix;
    stbi_uc first;
    stbi_uc suffix;
} stbi__gif_lzw;

typedef struct
{
    int w, h;
    stbi_uc * out; // output buffer (always 4 components)
    stbi_uc * background; // The current "background" as far as a gif is concerned
    stbi_uc * history;
    int flags, bgindex, ratio, transparent, eflags;
    stbi_uc pal[256][4];
    stbi_uc lpal[256][4];
    stbi__gif_lzw codes[8192];
    stbi_uc * color_table;
    int parse, step;
    int lflags;
    int start_x, start_y;
    int max_x, max_y;
    int cur_x, cur_y;
    int line_size;
    int delay;
} stbi__gif;

static void stbi__gif_parse_colortable(stbi__context * s, stbi_uc pal[256][4], int num_entries, int transp) {
    int i;
    for (i = 0; i < num_entries; ++i) {
        pal[i][2] = stbi__get8(s);
        pal[i][1] = stbi__get8(s);
        pal[i][0] = stbi__get8(s);
        pal[i][3] = transp == i ? 0 : 255;
    }
}

static int stbi__gif_header(stbi__context * s, stbi__gif * g, int * comp, int is_info) {
    stbi_uc version;
    if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
        return stbi__err("not GIF", "Corrupt GIF");

    version = stbi__get8(s);
    if (version != '7' && version != '9')
        return stbi__err("not GIF", "Corrupt GIF");
    if (stbi__get8(s) != 'a')
        return stbi__err("not GIF", "Corrupt GIF");

    stbi__g_failure_reason = "";
    g->w = stbi__get16le(s);
    g->h = stbi__get16le(s);
    g->flags = stbi__get8(s);
    g->bgindex = stbi__get8(s);
    g->ratio = stbi__get8(s);
    g->transparent = -1;

    if (comp != 0)
        *comp = 4; // can't actually tell whether it's 3 or 4 until we parse the comments

    if (is_info)
        return 1;

    if (g->flags & 0x80)
        stbi__gif_parse_colortable(s, g->pal, 2 << (g->flags & 7), -1);

    return 1;
}

static void stbi__out_gif_code(stbi__gif * g, stbi__uint16 code) {
    stbi_uc *p, *c;
    int idx;

    // recurse to decode the prefixes, since the linked-list is backwards,
    // and working backwards through an interleaved image would be nasty
    if (g->codes[code].prefix >= 0)
        stbi__out_gif_code(g, g->codes[code].prefix);

    if (g->cur_y >= g->max_y)
        return;

    idx = g->cur_x + g->cur_y;
    p = &g->out[idx];
    g->history[idx / 4] = 1;

    c = &g->color_table[g->codes[code].suffix * 4];
    if (c[3] > 128) { // don't render transparent pixels;
        p[0] = c[2];
        p[1] = c[1];
        p[2] = c[0];
        p[3] = c[3];
    }
    g->cur_x += 4;

    if (g->cur_x >= g->max_x) {
        g->cur_x = g->start_x;
        g->cur_y += g->step;

        while (g->cur_y >= g->max_y && g->parse > 0) {
            g->step = (1 << g->parse) * g->line_size;
            g->cur_y = g->start_y + (g->step >> 1);
            --g->parse;
        }
    }
}

static stbi_uc * stbi__process_gif_raster(stbi__context * s, stbi__gif * g) {
    stbi_uc lzw_cs;
    stbi__int32 len, init_code;
    stbi__uint32 first;
    stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
    stbi__gif_lzw * p;

    lzw_cs = stbi__get8(s);
    if (lzw_cs > 12)
        return NULL;
    clear = 1 << lzw_cs;
    first = 1;
    codesize = lzw_cs + 1;
    codemask = (1 << codesize) - 1;
    bits = 0;
    valid_bits = 0;
    for (init_code = 0; init_code < clear; init_code++) {
        g->codes[init_code].prefix = -1;
        g->codes[init_code].first = (stbi_uc)init_code;
        g->codes[init_code].suffix = (stbi_uc)init_code;
    }

    // support no starting clear code
    avail = clear + 2;
    oldcode = -1;

    len = 0;
    for (;;) {
        if (valid_bits < codesize) {
            if (len == 0) {
                len = stbi__get8(s); // start new block
                if (len == 0)
                    return g->out;
            }
            --len;
            bits |= (stbi__int32)stbi__get8(s) << valid_bits;
            valid_bits += 8;
        } else {
            stbi__int32 code = bits & codemask;
            bits >>= codesize;
            valid_bits -= codesize;
            // @OPTIMIZE: is there some way we can accelerate the non-clear path?
            if (code == clear) { // clear code
                codesize = lzw_cs + 1;
                codemask = (1 << codesize) - 1;
                avail = clear + 2;
                oldcode = -1;
                first = 0;
            } else if (code == clear + 1) { // end of stream code
                stbi__skip(s, len);
                while ((len = stbi__get8(s)) > 0)
                    stbi__skip(s, len);
                return g->out;
            } else if (code <= avail) {
                if (first) {
                    return stbi__errpuc("no clear code", "Corrupt GIF");
                }

                if (oldcode >= 0) {
                    p = &g->codes[avail++];
                    if (avail > 8192) {
                        return stbi__errpuc("too many codes", "Corrupt GIF");
                    }

                    p->prefix = (stbi__int16)oldcode;
                    p->first = g->codes[oldcode].first;
                    p->suffix = (code == avail) ? p->first : g->codes[code].first;
                } else if (code == avail)
                    return stbi__errpuc("illegal code in raster", "Corrupt GIF");

                stbi__out_gif_code(g, (stbi__uint16)code);

                if ((avail & codemask) == 0 && avail <= 0x0FFF) {
                    codesize++;
                    codemask = (1 << codesize) - 1;
                }

                oldcode = code;
            } else {
                return stbi__errpuc("illegal code in raster", "Corrupt GIF");
            }
        }
    }
}

// this function is designed to support animated gifs, although stb_image doesn't support it
// two back is the image from two frames ago, used for a very specific disposal format
static stbi_uc * stbi__gif_load_next(stbi__context * s, stbi__gif * g, int * comp, int req_comp, stbi_uc * two_back) {
    int dispose;
    int first_frame;
    int pi;
    int pcount;
    STBI_NOTUSED(req_comp);

    // on first frame, any non-written pixels get the background colour (non-transparent)
    first_frame = 0;
    if (g->out == 0) {
        if (!stbi__gif_header(s, g, comp, 0))
            return 0; // stbi__g_failure_reason set by stbi__gif_header
        if (!stbi__mad3sizes_valid(4, g->w, g->h, 0))
            return stbi__errpuc("too large", "GIF image is too large");
        pcount = g->w * g->h;
        const int64_t deflate_buffer_size = s->deflate_buffer_end - s->deflate_buffer;
        const int64_t working_buffer_size = s->working_buffer_end - s->working_buffer;
        if ((deflate_buffer_size < (4 * pcount)) || (working_buffer_size < (2 * 4 * pcount))) {
            return NULL;
        }
        g->out = s->deflate_buffer;
        g->background = s->working_buffer;
        g->history = s->working_buffer + (4 * pcount);
        if (!g->out || !g->background || !g->history)
            return stbi__errpuc("outofmem", "Out of memory");

        // image is treated as "transparent" at the start - ie, nothing overwrites the current background;
        // background colour is only used for pixels that are not rendered first frame, after that "background"
        // color refers to the color that was there the previous frame.
        memset(g->out, 0x00, 4 * pcount);
        memset(g->background, 0x00, 4 * pcount); // state of the background (starts transparent)
        memset(g->history, 0x00, pcount); // pixels that were affected previous frame
        first_frame = 1;
    } else {
        // second frame - how do we dispoase of the previous one?
        dispose = (g->eflags & 0x1C) >> 2;
        pcount = g->w * g->h;

        if ((dispose == 3) && (two_back == 0)) {
            dispose = 2; // if I don't have an image to revert back to, default to the old background
        }

        if (dispose == 3) { // use previous graphic
            for (pi = 0; pi < pcount; ++pi) {
                if (g->history[pi]) {
                    memcpy(&g->out[pi * 4], &two_back[pi * 4], 4);
                }
            }
        } else if (dispose == 2) {
            // restore what was changed last frame to background before that frame;
            for (pi = 0; pi < pcount; ++pi) {
                if (g->history[pi]) {
                    memcpy(&g->out[pi * 4], &g->background[pi * 4], 4);
                }
            }
        } else {
            // This is a non-disposal case eithe way, so just
            // leave the pixels as is, and they will become the new background
            // 1: do not dispose
            // 0:  not specified.
        }

        // background is what out is after the undoing of the previou frame;
        memcpy(g->background, g->out, 4 * g->w * g->h);
    }

    // clear my history;
    memset(g->history, 0x00, g->w * g->h); // pixels that were affected previous frame

    for (;;) {
        int tag = stbi__get8(s);
        switch (tag) {
            case 0x2C: /* Image Descriptor */
            {
                stbi__int32 x, y, w, h;
                stbi_uc * o;

                x = stbi__get16le(s);
                y = stbi__get16le(s);
                w = stbi__get16le(s);
                h = stbi__get16le(s);
                if (((x + w) > (g->w)) || ((y + h) > (g->h)))
                    return stbi__errpuc("bad Image Descriptor", "Corrupt GIF");

                g->line_size = g->w * 4;
                g->start_x = x * 4;
                g->start_y = y * g->line_size;
                g->max_x = g->start_x + w * 4;
                g->max_y = g->start_y + h * g->line_size;
                g->cur_x = g->start_x;
                g->cur_y = g->start_y;

                // if the width of the specified rectangle is 0, that means
                // we may not see *any* pixels or the image is malformed;
                // to make sure this is caught, move the current y down to
                // max_y (which is what out_gif_code checks).
                if (w == 0)
                    g->cur_y = g->max_y;

                g->lflags = stbi__get8(s);

                if (g->lflags & 0x40) {
                    g->step = 8 * g->line_size; // first interlaced spacing
                    g->parse = 3;
                } else {
                    g->step = g->line_size;
                    g->parse = 0;
                }

                if (g->lflags & 0x80) {
                    stbi__gif_parse_colortable(s, g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
                    g->color_table = (stbi_uc *)g->lpal;
                } else if (g->flags & 0x80) {
                    g->color_table = (stbi_uc *)g->pal;
                } else
                    return stbi__errpuc("missing color table", "Corrupt GIF");

                o = stbi__process_gif_raster(s, g);
                if (!o)
                    return NULL;

                // if this was the first frame,
                pcount = g->w * g->h;
                if (first_frame && (g->bgindex > 0)) {
                    // if first frame, any pixel not drawn to gets the background color
                    for (pi = 0; pi < pcount; ++pi) {
                        if (g->history[pi] == 0) {
                            g->pal[g->bgindex][3] = 255; // just in case it was made transparent, undo that; It will be reset next frame if need be;
                            memcpy(&g->out[pi * 4], &g->pal[g->bgindex], 4);
                        }
                    }
                }

                return o;
            }

            case 0x21: // Comment Extension.
            {
                int len;
                int ext = stbi__get8(s);
                if (ext == 0xF9) { // Graphic Control Extension.
                    len = stbi__get8(s);
                    if (len == 4) {
                        g->eflags = stbi__get8(s);
                        g->delay = 10 * stbi__get16le(s); // delay - 1/100th of a second, saving as 1/1000ths.

                        // unset old transparent
                        if (g->transparent >= 0) {
                            g->pal[g->transparent][3] = 255;
                        }
                        if (g->eflags & 0x01) {
                            g->transparent = stbi__get8(s);
                            if (g->transparent >= 0) {
                                g->pal[g->transparent][3] = 0;
                            }
                        } else {
                            // don't need transparent
                            stbi__skip(s, 1);
                            g->transparent = -1;
                        }
                    } else {
                        stbi__skip(s, len);
                        break;
                    }
                }
                while ((len = stbi__get8(s)) != 0) {
                    stbi__skip(s, len);
                }
                break;
            }

            case 0x3B: // gif stream termination code
                return (stbi_uc *)s; // using '1' causes warning on some compilers

            default:
                return stbi__errpuc("unknown code", "Corrupt GIF");
        }
    }
}

static int16_t little_endian_to_native_int16_t(const int16_t num) {
    static const int16_t endian_constant = 0x0102;
    if (((char *)&endian_constant)[0] == (char)0x01) {
        return ((num & 0x00ff) << 8) | ((num & 0xff00) >> 8);
    } else {
        return num;
    }
}

typedef struct imagelib_gif_decode_context_t {
    stbi__context stbi_context;
    stbi__gif gif_context;
} imagelib_gif_decode_context_t;

bool imagelib_read_gif_header_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size) {
    ASSERT(out_image && out_required_pixel_buffer_size && out_required_working_space_size);

    const uint8_t gif_header1[] = "GIF87a";
    const uint8_t gif_header2[] = "GIF89a";

    if ((memcmp(gif_header1, gif_file_data.ptr, sizeof(gif_header1) - 1) != 0) && (memcmp(gif_header2, gif_file_data.ptr, sizeof(gif_header2) - 1) != 0)) {
        return false;
    }

    const int16_t * const dim_start = (const int16_t *)(gif_file_data.byte_ptr + 6);
    out_image->width = little_endian_to_native_int16_t(*dim_start);
    out_image->height = little_endian_to_native_int16_t(*(dim_start + 1));
    out_image->bpp = 4; // stb assumes that all gifs will give us 4 channels (and doesn't support anything else)

    out_image->depth = 1;
    out_image->pitch = out_image->width * out_image->bpp;
    out_image->spitch = out_image->data_len = out_image->width * out_image->height * out_image->bpp;
    out_image->encoding = image_encoding_uncompressed;

    const size_t gif_decode_context_aligned = ALIGN_INT(sizeof(imagelib_gif_decode_context_t), 8);
    *out_required_working_space_size = gif_decode_context_aligned + out_image->data_len * 2; // space to store the context inline, and history + background
    *out_required_pixel_buffer_size = out_image->data_len;

    return true;
}

bool imagelib_gif_load_first_frame_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, milliseconds_t * const out_image_delay_in_ms, const mem_region_t pixel_region, const mem_region_t working_space_region) {
    ASSERT(out_image_delay_in_ms);
    ASSERT((pixel_region.size > 0) && (working_space_region.size > 0));

    const size_t gif_decode_context_aligned = ALIGN_INT(sizeof(imagelib_gif_decode_context_t), 8);
    imagelib_gif_decode_context_t * const gif_decode_context = working_space_region.ptr;
    ZEROMEM(gif_decode_context);

    stbi__start_mem_in_line(
        &gif_decode_context->stbi_context,
        gif_file_data.ptr,
        (int)gif_file_data.size,
        pixel_region.ptr,
        (int)pixel_region.size,
        working_space_region.byte_ptr + gif_decode_context_aligned,
        (int)(working_space_region.size - gif_decode_context_aligned));

    return imagelib_gif_load_next_frame_from_memory(gif_file_data, out_image, out_image_delay_in_ms, pixel_region, working_space_region, imagelib_gif_continue);
}

static void imagelib_gif_restart(const mem_region_t working_space_region) {
    imagelib_gif_decode_context_t * const gif_decode_context = working_space_region.ptr;
    gif_decode_context->stbi_context.img_buffer = gif_decode_context->stbi_context.img_buffer_original;
    gif_decode_context->stbi_context.img_buffer_end = gif_decode_context->stbi_context.img_buffer_original_end;

    ZEROMEM(&gif_decode_context->gif_context);
}

bool imagelib_gif_load_next_frame_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, milliseconds_t * const out_image_delay_in_ms, const mem_region_t pixel_region, const mem_region_t working_space_region, const imagelib_gif_restart_mode_e restart_mode) {
    ASSERT(out_image_delay_in_ms);
    ASSERT(working_space_region.size > 0);
    ASSERT((restart_mode == imagelib_gif_force_restart) || (restart_mode == imagelib_gif_continue));

    imagelib_gif_decode_context_t * const gif_decode_context = working_space_region.ptr;
    if (restart_mode == imagelib_gif_continue) {
        out_image->data = stbi__gif_load_next(&gif_decode_context->stbi_context, &gif_decode_context->gif_context, NULL, 4, NULL);
    }
    if ((restart_mode == imagelib_gif_force_restart) || !out_image->data || (out_image->data == (uint8_t *)&gif_decode_context->stbi_context)) {
        imagelib_gif_restart(working_space_region);
        out_image->data = stbi__gif_load_next(&gif_decode_context->stbi_context, &gif_decode_context->gif_context, NULL, 4, NULL);
    }

    if (!out_image->data) {
        LOG_WARN(IMAGELIB_GIF_TAG, "Failed to decode gif frame");
    }

    *out_image_delay_in_ms = (milliseconds_t){(uint32_t)gif_decode_context->gif_context.delay};
    return out_image->data != NULL;
}