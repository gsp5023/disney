/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/
#include "source/adk/imagelib/imagelib.h"

#include <stdint.h>
#include <stdlib.h>

typedef uint16_t stbi__uint16;
typedef int16_t stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t stbi__int32;

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

enum {
    STBI_default = 0, // only used for desired_channels

    STBI_grey = 1,
    STBI_grey_alpha = 2,
    STBI_rgb = 3,
    STBI_rgb_alpha = 4
};

enum {
    STBI_ORDER_RGB,
    STBI_ORDER_BGR
};

typedef struct stbi__context {
    stbi__uint32 img_x, img_y;
    int img_n, img_out_n;

    int buflen;
    stbi_uc buffer_start[128];

    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original, *img_buffer_original_end;

    stbi_uc *deflate_buffer, *deflate_buffer_end;
    stbi_uc *working_buffer, *working_buffer_end;
} stbi__context;

typedef struct
{
    int bits_per_channel;
    int num_channels;
    int channel_order;
} stbi__result_info;

static const char * stbi__g_failure_reason;

const char * imagelib_tga_stbi_failure_reason() {
    return stbi__g_failure_reason;
}

static int stbi__err(const char * str) {
    stbi__g_failure_reason = str;
    return 0;
}

// stbi__err - error
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

#ifdef _MSC_VER
#define STBI_NOTUSED(v) (void)(v)
#else
#define STBI_NOTUSED(v) (void)sizeof(v)
#endif

static void stbi__rewind(stbi__context * s) {
    // conceptually rewind SHOULD rewind to the beginning of the stream,
    // but we just rewind to the beginning of the initial buffer, because
    // we only use it after doing 'test', which only ever looks at at most 92 bytes
    s->img_buffer = s->img_buffer_original;
    s->img_buffer_end = s->img_buffer_original_end;
}

static stbi_uc stbi__get8(stbi__context * s) {
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    return 0;
}

static int stbi__get16le(stbi__context * s) {
    int z = stbi__get8(s);
    return z + (stbi__get8(s) << 8);
}

static int stbi__getn(stbi__context * s, stbi_uc * buffer, int n) {
    if (s->img_buffer + n <= s->img_buffer_end) {
        memcpy(buffer, s->img_buffer, n);
        s->img_buffer += n;
        return 1;
    } else
        return 0;
}

static void stbi__skip(stbi__context * s, int n) {
    if (n < 0) {
        s->img_buffer = s->img_buffer_end;
        return;
    }
    s->img_buffer += n;
}

// returns STBI_rgb or whatever, 0 on error
static int stbi__tga_get_comp(int bits_per_pixel, int is_grey, int * is_rgb16) {
    // only RGB or RGBA (incl. 16bit) or grey allowed
    if (is_rgb16)
        *is_rgb16 = 0;
    switch (bits_per_pixel) {
        case 8:
            return STBI_grey;
        case 16:
            if (is_grey)
                return STBI_grey_alpha;
            // fallthrough
        case 15:
            if (is_rgb16)
                *is_rgb16 = 1;
            return STBI_rgb;
        case 24: // fallthrough
        case 32:
            return bits_per_pixel / 8;
        default:
            return 0;
    }
}

static int stbi__tga_info(stbi__context * s, int * x, int * y, int * comp) {
    int tga_w, tga_h, tga_comp, tga_image_type, tga_bits_per_pixel, tga_colormap_bpp;
    int sz, tga_colormap_type;
    stbi__get8(s); // discard Offset
    tga_colormap_type = stbi__get8(s); // colormap type
    if (tga_colormap_type > 1) {
        stbi__rewind(s);
        return 0; // only RGB or indexed allowed
    }
    tga_image_type = stbi__get8(s); // image type
    if (tga_colormap_type == 1) { // colormapped (paletted) image
        if (tga_image_type != 1 && tga_image_type != 9) {
            stbi__rewind(s);
            return 0;
        }
        stbi__skip(s, 4); // skip index of first colormap entry and number of entries
        sz = stbi__get8(s); //   check bits per palette color entry
        if ((sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32)) {
            stbi__rewind(s);
            return 0;
        }
        stbi__skip(s, 4); // skip image x and y origin
        tga_colormap_bpp = sz;
    } else { // "normal" image w/o colormap - only RGB or grey allowed, +/- RLE
        if ((tga_image_type != 2) && (tga_image_type != 3) && (tga_image_type != 10) && (tga_image_type != 11)) {
            stbi__rewind(s);
            return 0; // only RGB or grey allowed, +/- RLE
        }
        stbi__skip(s, 9); // skip colormap specification and image x/y origin
        tga_colormap_bpp = 0;
    }
    tga_w = stbi__get16le(s);
    if (tga_w < 1) {
        stbi__rewind(s);
        return 0; // test width
    }
    tga_h = stbi__get16le(s);
    if (tga_h < 1) {
        stbi__rewind(s);
        return 0; // test height
    }
    tga_bits_per_pixel = stbi__get8(s); // bits per pixel
    stbi__get8(s); // ignore alpha bits
    if (tga_colormap_bpp != 0) {
        if ((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16)) {
            // when using a colormap, tga_bits_per_pixel is the size of the indexes
            // I don't think anything but 8 or 16bit indexes makes sense
            stbi__rewind(s);
            return 0;
        }
        tga_comp = stbi__tga_get_comp(tga_colormap_bpp, 0, NULL);
    } else {
        tga_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3) || (tga_image_type == 11), NULL);
    }
    if (!tga_comp) {
        stbi__rewind(s);
        return 0;
    }
    if (x)
        *x = tga_w;
    if (y)
        *y = tga_h;
    if (comp)
        *comp = tga_comp;
    return 1; // seems to have passed everything
}

static int stbi__tga_test(stbi__context * s) {
    int res = 0;
    int sz, tga_color_type;
    stbi__get8(s); //   discard Offset
    tga_color_type = stbi__get8(s); //   color type
    if (tga_color_type > 1)
        goto errorEnd; //   only RGB or indexed allowed
    sz = stbi__get8(s); //   image type
    if (tga_color_type == 1) { // colormapped (paletted) image
        if (sz != 1 && sz != 9)
            goto errorEnd; // colortype 1 demands image type 1 or 9
        stbi__skip(s, 4); // skip index of first colormap entry and number of entries
        sz = stbi__get8(s); //   check bits per palette color entry
        if ((sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32))
            goto errorEnd;
        stbi__skip(s, 4); // skip image x and y origin
    } else { // "normal" image w/o colormap
        if ((sz != 2) && (sz != 3) && (sz != 10) && (sz != 11))
            goto errorEnd; // only RGB or grey allowed, +/- RLE
        stbi__skip(s, 9); // skip colormap specification and image x/y origin
    }
    if (stbi__get16le(s) < 1)
        goto errorEnd; //   test width
    if (stbi__get16le(s) < 1)
        goto errorEnd; //   test height
    sz = stbi__get8(s); //   bits per pixel
    if ((tga_color_type == 1) && (sz != 8) && (sz != 16))
        goto errorEnd; // for colormapped images, bpp is size of an index
    if ((sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32))
        goto errorEnd;

    res = 1; // if we got this far, everything's good and we can return 1 instead of 0

errorEnd:
    stbi__rewind(s);
    return res;
}

// read 16bit value and convert to 24bit RGB
static void stbi__tga_read_rgb16(stbi__context * s, stbi_uc * out) {
    stbi__uint16 px = (stbi__uint16)stbi__get16le(s);
    stbi__uint16 fiveBitMask = 31;
    // we have 3 channels with 5bits each
    int r = (px >> 10) & fiveBitMask;
    int g = (px >> 5) & fiveBitMask;
    int b = px & fiveBitMask;
    // Note that this saves the data in RGB(A) order, so it doesn't need to be swapped later
    out[0] = (stbi_uc)((r * 255) / 31);
    out[1] = (stbi_uc)((g * 255) / 31);
    out[2] = (stbi_uc)((b * 255) / 31);

    // some people claim that the most significant bit might be used for alpha
    // (possibly if an alpha-bit is set in the "image descriptor byte")
    // but that only made 16bit test images completely translucent..
    // so let's treat all 15 and 16bit TGAs as RGB with no alpha.
}

static void * stbi__tga_load(stbi__context * s, int * x, int * y, int * comp, int req_comp, stbi__result_info * ri) {
    //   read in the TGA header stuff
    int tga_offset = stbi__get8(s);
    int tga_indexed = stbi__get8(s);
    int tga_image_type = stbi__get8(s);
    int tga_is_RLE = 0;
    int tga_palette_start = stbi__get16le(s);
    int tga_palette_len = stbi__get16le(s);
    int tga_palette_bits = stbi__get8(s);
    int tga_x_origin = stbi__get16le(s);
    int tga_y_origin = stbi__get16le(s);
    int tga_width = stbi__get16le(s);
    int tga_height = stbi__get16le(s);
    int tga_bits_per_pixel = stbi__get8(s);
    int tga_comp, tga_rgb16 = 0;
    int tga_inverted = stbi__get8(s);
    // int tga_alpha_bits = tga_inverted & 15; // the 4 lowest bits - unused (useless?)
    //   image data
    unsigned char * tga_data;
    unsigned char * tga_palette = NULL;
    int i, j;
    unsigned char raw_data[4] = {0};
    int RLE_count = 0;
    int RLE_repeating = 0;
    int read_next_pixel = 1;
    STBI_NOTUSED(ri);
    STBI_NOTUSED(tga_x_origin); // @TODO
    STBI_NOTUSED(tga_y_origin); // @TODO

    //   do a tiny bit of precessing
    if (tga_image_type >= 8) {
        tga_image_type -= 8;
        tga_is_RLE = 1;
    }
    tga_inverted = 1 - ((tga_inverted >> 5) & 1);

    //   If I'm paletted, then I'll use the number of bits from the palette
    if (tga_indexed)
        tga_comp = stbi__tga_get_comp(tga_palette_bits, 0, &tga_rgb16);
    else
        tga_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3), &tga_rgb16);

    if (!tga_comp) // shouldn't really happen, stbi__tga_test() should have ensured basic consistency
        return stbi__errpuc("bad format", "Can't find out TGA pixelformat");

    //   tga info
    *x = tga_width;
    *y = tga_height;
    if (comp)
        *comp = tga_comp;

    if (tga_width * tga_height * tga_comp < (int)(s->deflate_buffer_end - s->deflate_buffer)) {
        return stbi__errpuc("outofmem", "Insufficient memory in buffer to decode output image");
    }
    tga_data = s->deflate_buffer;

    // skip to the data's starting position (offset usually = 0)
    stbi__skip(s, tga_offset);

    if (!tga_indexed && !tga_is_RLE && !tga_rgb16) {
        for (i = 0; i < tga_height; ++i) {
            int row = tga_inverted ? tga_height - i - 1 : i;
            stbi_uc * tga_row = tga_data + row * tga_width * tga_comp;
            stbi__getn(s, tga_row, tga_width * tga_comp);
        }
    } else {
        //   do I need to load a palette?
        if (tga_indexed) {
            //   any data to skip? (offset usually = 0)
            stbi__skip(s, tga_palette_start);
            //   load the palette
            if (tga_palette_len * tga_comp < (int)(s->working_buffer_end - s->working_buffer)) {
                return stbi__errpuc("outofmem", "Insufficient memory in working space buffer");
            }
            tga_palette = s->working_buffer;

            if (tga_rgb16) {
                stbi_uc * pal_entry = tga_palette;
                ASSERT(tga_comp == STBI_rgb);
                for (i = 0; i < tga_palette_len; ++i) {
                    stbi__tga_read_rgb16(s, pal_entry);
                    pal_entry += tga_comp;
                }
            } else if (!stbi__getn(s, tga_palette, tga_palette_len * tga_comp)) {
                return stbi__errpuc("bad palette", "Corrupt TGA");
            }
        }
        //   load the data
        for (i = 0; i < tga_width * tga_height; ++i) {
            //   if I'm in RLE mode, do I need to get a RLE stbi__pngchunk?
            if (tga_is_RLE) {
                if (RLE_count == 0) {
                    //   yep, get the next byte as a RLE command
                    int RLE_cmd = stbi__get8(s);
                    RLE_count = 1 + (RLE_cmd & 127);
                    RLE_repeating = RLE_cmd >> 7;
                    read_next_pixel = 1;
                } else if (!RLE_repeating) {
                    read_next_pixel = 1;
                }
            } else {
                read_next_pixel = 1;
            }
            //   OK, if I need to read a pixel, do it now
            if (read_next_pixel) {
                //   load however much data we did have
                if (tga_indexed) {
                    // read in index, then perform the lookup
                    int pal_idx = (tga_bits_per_pixel == 8) ? stbi__get8(s) : stbi__get16le(s);
                    if (pal_idx >= tga_palette_len) {
                        // invalid index
                        pal_idx = 0;
                    }
                    pal_idx *= tga_comp;
                    for (j = 0; j < tga_comp; ++j) {
                        raw_data[j] = tga_palette[pal_idx + j];
                    }
                } else if (tga_rgb16) {
                    ASSERT(tga_comp == STBI_rgb);
                    stbi__tga_read_rgb16(s, raw_data);
                } else {
                    //   read in the data raw
                    for (j = 0; j < tga_comp; ++j) {
                        raw_data[j] = stbi__get8(s);
                    }
                }
                //   clear the reading flag for the next pixel
                read_next_pixel = 0;
            } // end of reading a pixel

            // copy data
            for (j = 0; j < tga_comp; ++j)
                tga_data[i * tga_comp + j] = raw_data[j];

            //   in case we're in RLE mode, keep counting down
            --RLE_count;
        }
        //   do I need to invert the image?
        if (tga_inverted) {
            for (j = 0; j * 2 < tga_height; ++j) {
                int index1 = j * tga_width * tga_comp;
                int index2 = (tga_height - 1 - j) * tga_width * tga_comp;
                for (i = tga_width * tga_comp; i > 0; --i) {
                    unsigned char temp = tga_data[index1];
                    tga_data[index1] = tga_data[index2];
                    tga_data[index2] = temp;
                    ++index1;
                    ++index2;
                }
            }
        }
    }

    // swap RGB - if the source data was RGB16, it already is in the right order
    if (tga_comp >= 3 && !tga_rgb16) {
        unsigned char * tga_pixel = tga_data;
        for (i = 0; i < tga_width * tga_height; ++i) {
            unsigned char temp = tga_pixel[0];
            tga_pixel[0] = tga_pixel[2];
            tga_pixel[2] = temp;
            tga_pixel += tga_comp;
        }
    }

    //   the things I do to get rid of an error message, and yet keep
    //   Microsoft's C compilers happy... [8^(
    tga_palette_start = tga_palette_len = tga_palette_bits = tga_x_origin = tga_y_origin = 0;
    STBI_NOTUSED(tga_palette_start);
    //   OK, done
    return tga_data;
}

static void * stbi__load_main(stbi__context * s, int * x, int * y, int * comp, int req_comp, stbi__result_info * ri, int bpc) {
    memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
    ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
    ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
    ri->num_channels = 0;

    if (stbi__tga_test(s)) {
        return stbi__tga_load(s, x, y, comp, req_comp, ri);
    }

    return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
}

static void stbi__start_mem_in_line(stbi__context * s, const stbi_uc * const pixel_buffer, const int pixel_buffer_len, stbi_uc * const deflate_buffer, const int deflate_buffer_len, stbi_uc * const working_buffer, const int working_buffer_len) {
    s->img_buffer = s->img_buffer_original = (stbi_uc *)pixel_buffer;
    s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)pixel_buffer + pixel_buffer_len;
    s->deflate_buffer = deflate_buffer;
    s->deflate_buffer_end = deflate_buffer + deflate_buffer_len;
    s->working_buffer = working_buffer;
    s->working_buffer_end = working_buffer + working_buffer_len;
}

static unsigned char * stbi__load_and_postprocess_8bit(stbi__context * s, int * x, int * y, int * comp, int req_comp) {
    stbi__result_info ri;
    void * result = stbi__load_main(s, x, y, comp, req_comp, &ri, 8);

    if (result == NULL)
        return NULL;

    return (unsigned char *)result;
}

static stbi_uc * stbi_load_png_from_memory_in_place(const stbi_uc * const pixel_buffer, const int pixel_buffer_len, int * const width, int * const height, int * const channels, int desired_channels, stbi_uc * const deflate_buffer, const int deflate_buffer_len, stbi_uc * const working_buffer, const int working_buffer_len) {
    stbi__context s = {0};
    stbi__start_mem_in_line(&s, pixel_buffer, pixel_buffer_len, deflate_buffer, deflate_buffer_len, working_buffer, working_buffer_len);
    return stbi__load_and_postprocess_8bit(&s, width, height, channels, desired_channels);
}

typedef PACK(struct tga_header_t {
    int8_t id_length;
    int8_t color_type;
    int8_t image_type;

    int16_t color_map_origin;
    int16_t color_map_length;
    int8_t color_map_bpp;

    int16_t x_origin;
    int16_t y_origin;
    int16_t width;
    int16_t height;
    int8_t bits_per_pixel;
    int8_t image_descriptor;
}) tga_header_t;

STATIC_ASSERT(sizeof(tga_header_t) == 18);

static int tga_get_comp(const tga_header_t * const tga_header) {
    // only rgb or rgba (including 16bit) or grey is allowed
    // this is mapping the `stbi__tga_get_comp` function.
    if (((tga_header->image_type == 3) || (tga_header->image_type == 11)) && (tga_header->color_map_bpp == 16)) {
        return 2;
    } else if ((tga_header->color_map_bpp == 15) || (tga_header->color_map_bpp == 16)) {
        return 3;
    }
    return tga_header->color_map_bpp / 8;
}

bool imagelib_read_tga_header_from_memory(
    const const_mem_region_t tga_file_data,
    image_t * const out_image,
    size_t * const out_required_pixel_buffer_size,
    size_t * const out_required_working_space_size) {
    ZEROMEM(out_image);
    *out_required_pixel_buffer_size = 0;
    *out_required_working_space_size = 0;

    if (tga_file_data.size < sizeof(tga_header_t)) {
        return false;
    }

    const tga_header_t * const tga_header = tga_file_data.ptr;
    int32_t block[100];
    for (int i = 0; i < ARRAY_SIZE(block); ++i) {
        block[i] = tga_file_data.byte_ptr[i];
    }

    // making sure we're a tga is miserable this is mostly a copy n paste + cleaning up of STB code. look for `stbi__tga_test`

    if (tga_header->color_type > 1) {
        // only rgb or indexed is allowed
        return false;
    } else if ((tga_header->color_type == 1) && (tga_header->image_type != 1 && tga_header->image_type != 9)) {
        // color type must be type 1 or 9
        return false;
    } else if ((tga_header->image_type != 2) && (tga_header->image_type != 3) && (tga_header->image_type != 10) && (tga_header->image_type != 11)) {
        // only rgb or grey allowed +/- RLE
        return false;
    } else if ((tga_header->width < 1) || (tga_header->height < 1)) {
        // make sure we have a valid dimension for the image..
        return false;
    } else if ((tga_header->color_type == 1) && ((tga_header->color_map_bpp != 8) && (tga_header->color_map_bpp != 16))) {
        // for color mapped images, bpp is size of an index
        return false;
    } else if ((tga_header->bits_per_pixel != 8) && (tga_header->bits_per_pixel != 15) && (tga_header->bits_per_pixel != 16) && (tga_header->bits_per_pixel != 24) && (tga_header->bits_per_pixel != 32)) {
        return false;
    }

    out_image->bpp = tga_header->bits_per_pixel / 8;
    out_image->width = tga_header->width;
    out_image->height = tga_header->height;
    out_image->depth = 1;

    out_image->pitch = out_image->width * out_image->height;
    out_image->spitch = out_image->data_len = out_image->pitch * out_image->bpp;
    out_image->encoding = image_encoding_uncompressed;

    *out_required_pixel_buffer_size = out_image->data_len;
    *out_required_working_space_size = (tga_header->color_type == 1) ? (tga_header->color_map_length * tga_get_comp(tga_header)) : 0;

    return true;
}

bool imagelib_load_tga_from_memory(const const_mem_region_t tga_file_data, image_t * const out_image, const mem_region_t pixel_region, const mem_region_t working_space_region) {
    int w, h, channels;
    out_image->data = stbi_load_png_from_memory_in_place(
        tga_file_data.ptr,
        (int)tga_file_data.size,
        &w,
        &h,
        &channels,
        0,
        pixel_region.ptr,
        (int)pixel_region.size,
        working_space_region.ptr,
        (int)working_space_region.size);

    ASSERT((w == out_image->width) && (h == out_image->height) && (channels == out_image->bpp));
    return out_image->data != NULL;
}