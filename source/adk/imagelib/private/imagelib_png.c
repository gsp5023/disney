/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
imagelib_png.c

stb_image.h modified/trimmed to support png loading 'in place' in a specified memory region
and support for loading png images from imagelib + guessing their sizes
*/

// stb image portions are licensed/copyright under:
/* stb_image - v2.23 - public domain image loader - http://nothings.org/stb
								  no warranty implied; use at your own risk
								  */

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/runtime/file_stbi.h"
#include "source/adk/runtime/runtime.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

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

#ifndef STBIDEF
#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
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
    // the maximum size in bytes of a huffman table is 258 bytes.
    // if interlacing is enabled, we have effectively 8 sub images, with unique huffman tables each.
    // when stb calculates the zlib stream decompression size, the output will be off by this amount. (and it's a known upper bound)
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

typedef struct
{
    int bits_per_channel;
    int num_channels;
    int channel_order;
} stbi__result_info;

static const char * stbi__g_failure_reason;

STBIDEF const char * imagelib_png_stbi_failure_reason(void) {
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

// returns 1 if "a*b + add" has no negative terms/factors and doesn't overflow
static int stbi__mad2sizes_valid(int a, int b, int add) {
    return stbi__mul2sizes_valid(a, b) && stbi__addsizes_valid(a * b, add);
}

// returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow
static int stbi__mad3sizes_valid(int a, int b, int c, int add) {
    return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a * b, c) && stbi__addsizes_valid(a * b * c, add);
}

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS 9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK ((1 << STBI__ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
    stbi__uint16 fast[1 << STBI__ZFAST_BITS];
    stbi__uint16 firstcode[16];
    int maxcode[17];
    stbi__uint16 firstsymbol[16];
    stbi_uc size[288];
    stbi__uint16 value[288];
} stbi__zhuffman;

stbi_inline static int stbi__bitreverse16(int n) {
    n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    return n;
}

stbi_inline static int stbi__bit_reverse(int v, int bits) {
    STBI_ASSERT(bits <= 16);
    // to bit reverse n bits, reverse 16 and shift
    // e.g. 11 bits, bit reverse and shift away 5
    return stbi__bitreverse16(v) >> (16 - bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman * z, const stbi_uc * sizelist, int num) {
    int i, k = 0;
    int code, next_code[16], sizes[17];

    // DEFLATE spec for generating codes
    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 0, sizeof(z->fast));
    for (i = 0; i < num; ++i)
        ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i = 1; i < 16; ++i)
        if (sizes[i] > (1 << i))
            return stbi__err("bad sizes", "Corrupt PNG");
    code = 0;
    for (i = 1; i < 16; ++i) {
        next_code[i] = code;
        z->firstcode[i] = (stbi__uint16)code;
        z->firstsymbol[i] = (stbi__uint16)k;
        code = (code + sizes[i]);
        if (sizes[i])
            if (code - 1 >= (1 << i))
                return stbi__err("bad codelengths", "Corrupt PNG");
        z->maxcode[i] = code << (16 - i); // preshift for inner loop
        code <<= 1;
        k += sizes[i];
    }
    z->maxcode[16] = 0x10000; // sentinel
    for (i = 0; i < num; ++i) {
        int s = sizelist[i];
        if (s) {
            int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            stbi__uint16 fastv = (stbi__uint16)((s << 9) | i);
            z->size[c] = (stbi_uc)s;
            z->value[c] = (stbi__uint16)i;
            if (s <= STBI__ZFAST_BITS) {
                int j = stbi__bit_reverse(next_code[s], s);
                while (j < (1 << STBI__ZFAST_BITS)) {
                    z->fast[j] = fastv;
                    j += (1 << s);
                }
            }
            ++next_code[s];
        }
    }
    return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
    stbi_uc *zbuffer, *zbuffer_end;
    int num_bits;
    stbi__uint32 code_buffer;

    char * zout;
    char * zout_start;
    char * zout_end;
    int z_expandable;

    stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

stbi_inline static stbi_uc stbi__zget8(stbi__zbuf * z) {
    if (z->zbuffer >= z->zbuffer_end)
        return 0;
    return *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf * z) {
    do {
        STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
        z->code_buffer |= (unsigned int)stbi__zget8(z) << z->num_bits;
        z->num_bits += 8;
    } while (z->num_bits <= 24);
}

stbi_inline static unsigned int stbi__zreceive(stbi__zbuf * z, int n) {
    unsigned int k;
    if (z->num_bits < n)
        stbi__fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf * a, stbi__zhuffman * z) {
    int b, s, k;
    // not resolved by fast table, so compute it the slow way
    // use jpeg approach, which requires MSbits at top
    k = stbi__bit_reverse(a->code_buffer, 16);
    for (s = STBI__ZFAST_BITS + 1;; ++s)
        if (k < z->maxcode[s])
            break;
    if (s == 16)
        return -1; // invalid code!
    // code size is s, so:
    b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
    STBI_ASSERT(z->size[b] == s);
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}

stbi_inline static int stbi__zhuffman_decode(stbi__zbuf * a, stbi__zhuffman * z) {
    int b, s;
    if (a->num_bits < 16)
        stbi__fill_bits(a);
    b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
    if (b) {
        s = b >> 9;
        a->code_buffer >>= s;
        a->num_bits -= s;
        return b & 511;
    }
    return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zexpand(stbi__context * const context, stbi__zbuf * z, char * zout, int n) // need to make room for n bytes
{
    //char *q;
    //int cur, limit, old_limit;
    z->zout = zout;
    STBI_ASSERT(!z->z_expandable);
    //if (!z->z_expandable)
    return stbi__err("output buffer limit", "Corrupt PNG");
    /*cur = (int)(z->zout - z->zout_start);
	limit = old_limit = (int)(z->zout_end - z->zout_start);
	while (cur + n > limit)
		limit *= 2;
	q = (char *)stbi__realloc_sized(context, z->zout_start, old_limit, limit);
	STBI_NOTUSED(old_limit);
	if (q == NULL) return stbi__err("outofmem", "Out of memory");
	z->zout_start = q;
	z->zout = q + cur;
	z->zout_end = q + limit;
	return 1;*/
}

static const int stbi__zlength_base[31] = {
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    13,
    15,
    17,
    19,
    23,
    27,
    31,
    35,
    43,
    51,
    59,
    67,
    83,
    99,
    115,
    131,
    163,
    195,
    227,
    258,
    0,
    0};

static const int stbi__zlength_extra[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0};

static const int stbi__zdist_base[32] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0};

static const int stbi__zdist_extra[32] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static int stbi__parse_huffman_block(stbi__context * const context, stbi__zbuf * a) {
    char * zout = a->zout;
    for (;;) {
        int z = stbi__zhuffman_decode(a, &a->z_length);
        if (z < 256) {
            if (z < 0)
                return stbi__err("bad huffman code", "Corrupt PNG"); // error in huffman codes
            if (zout >= a->zout_end) {
                if (!stbi__zexpand(context, a, zout, 1))
                    return 0;
                zout = a->zout;
            }
            *zout++ = (char)z;
        } else {
            stbi_uc * p;
            int len, dist;
            if (z == 256) {
                a->zout = zout;
                return 1;
            }
            z -= 257;
            len = stbi__zlength_base[z];
            if (stbi__zlength_extra[z])
                len += stbi__zreceive(a, stbi__zlength_extra[z]);
            z = stbi__zhuffman_decode(a, &a->z_distance);
            if (z < 0)
                return stbi__err("bad huffman code", "Corrupt PNG");
            dist = stbi__zdist_base[z];
            if (stbi__zdist_extra[z])
                dist += stbi__zreceive(a, stbi__zdist_extra[z]);
            if (zout - a->zout_start < dist)
                return stbi__err("bad dist", "Corrupt PNG");
            if (zout + len > a->zout_end) {
                if (!stbi__zexpand(context, a, zout, len))
                    return 0;
                zout = a->zout;
            }
            p = (stbi_uc *)(zout - dist);
            if (dist == 1) { // run of one byte; common in images.
                stbi_uc v = *p;
                if (len) {
                    do
                        *zout++ = v;
                    while (--len);
                }
            } else {
                if (len) {
                    do
                        *zout++ = *p++;
                    while (--len);
                }
            }
        }
    }
}

static int stbi__compute_huffman_codes(stbi__zbuf * a) {
    static const stbi_uc length_dezigzag[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    stbi__zhuffman z_codelength;
    stbi_uc lencodes[286 + 32 + 137]; //padding for maximum single op
    stbi_uc codelength_sizes[19];
    int i, n;

    int hlit = stbi__zreceive(a, 5) + 257;
    int hdist = stbi__zreceive(a, 5) + 1;
    int hclen = stbi__zreceive(a, 4) + 4;
    int ntot = hlit + hdist;

    memset(codelength_sizes, 0, sizeof(codelength_sizes));
    for (i = 0; i < hclen; ++i) {
        int s = stbi__zreceive(a, 3);
        codelength_sizes[length_dezigzag[i]] = (stbi_uc)s;
    }
    if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19))
        return 0;

    n = 0;
    while (n < ntot) {
        int c = stbi__zhuffman_decode(a, &z_codelength);
        if (c < 0 || c >= 19)
            return stbi__err("bad codelengths", "Corrupt PNG");
        if (c < 16)
            lencodes[n++] = (stbi_uc)c;
        else {
            stbi_uc fill = 0;
            if (c == 16) {
                c = stbi__zreceive(a, 2) + 3;
                if (n == 0)
                    return stbi__err("bad codelengths", "Corrupt PNG");
                fill = lencodes[n - 1];
            } else if (c == 17)
                c = stbi__zreceive(a, 3) + 3;
            else {
                STBI_ASSERT(c == 18);
                c = stbi__zreceive(a, 7) + 11;
            }
            if (ntot - n < c)
                return stbi__err("bad codelengths", "Corrupt PNG");
            memset(lencodes + n, fill, c);
            n += c;
        }
    }
    if (n != ntot)
        return stbi__err("bad codelengths", "Corrupt PNG");
    if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit))
        return 0;
    if (!stbi__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist))
        return 0;
    return 1;
}

static int stbi__parse_uncompressed_block(stbi__context * const context, stbi__zbuf * a) {
    stbi_uc header[4];
    int len, nlen, k;
    if (a->num_bits & 7)
        stbi__zreceive(a, a->num_bits & 7); // discard
    // drain the bit-packed data into header
    k = 0;
    while (a->num_bits > 0) {
        header[k++] = (stbi_uc)(a->code_buffer & 255); // suppress MSVC run-time check
        a->code_buffer >>= 8;
        a->num_bits -= 8;
    }
    STBI_ASSERT(a->num_bits == 0);
    // now fill header the normal way
    while (k < 4)
        header[k++] = stbi__zget8(a);
    len = header[1] * 256 + header[0];
    nlen = header[3] * 256 + header[2];
    if (nlen != (len ^ 0xffff))
        return stbi__err("zlib corrupt", "Corrupt PNG");
    if (a->zbuffer + len > a->zbuffer_end)
        return stbi__err("read past buffer", "Corrupt PNG");
    if (a->zout + len > a->zout_end)
        if (!stbi__zexpand(context, a, a->zout, len))
            return 0;
    memcpy(a->zout, a->zbuffer, len);
    a->zbuffer += len;
    a->zout += len;
    return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf * a) {
    int cmf = stbi__zget8(a);
    int cm = cmf & 15;
    /* int cinfo = cmf >> 4; */
    int flg = stbi__zget8(a);
    if ((cmf * 256 + flg) % 31 != 0)
        return stbi__err("bad zlib header", "Corrupt PNG"); // zlib spec
    if (flg & 32)
        return stbi__err("no preset dict", "Corrupt PNG"); // preset dictionary not allowed in png
    if (cm != 8)
        return stbi__err("bad compression", "Corrupt PNG"); // DEFLATE required for png
    // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
    return 1;
}

static const stbi_uc stbi__zdefault_length[288] = {
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8};
static const stbi_uc stbi__zdefault_distance[32] = {
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5,
    5};
/*
Init algorithm:
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}
*/

static int stbi__parse_zlib(stbi__context * const context, stbi__zbuf * a, int parse_header) {
    int final, type;
    if (parse_header)
        if (!stbi__parse_zlib_header(a))
            return 0;
    a->num_bits = 0;
    a->code_buffer = 0;
    do {
        final = stbi__zreceive(a, 1);
        type = stbi__zreceive(a, 2);
        if (type == 0) {
            if (!stbi__parse_uncompressed_block(context, a))
                return 0;
        } else if (type == 3) {
            return 0;
        } else {
            if (type == 1) {
                // use fixed code lengths
                if (!stbi__zbuild_huffman(&a->z_length, stbi__zdefault_length, 288))
                    return 0;
                if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance, 32))
                    return 0;
            } else {
                if (!stbi__compute_huffman_codes(a))
                    return 0;
            }
            if (!stbi__parse_huffman_block(context, a))
                return 0;
        }
    } while (!final);
    return 1;
}

static int stbi__do_zlib(stbi__context * const context, stbi__zbuf * a, char * obuf, int olen, int exp, int parse_header) {
    a->zout_start = obuf;
    a->zout = obuf;
    a->zout_end = obuf + olen;
    a->z_expandable = 0;

    return stbi__parse_zlib(context, a, parse_header);
}

static char * stbi_zlib_decode_malloc_guesssize_headerflag(stbi__context * const context, const char * buffer, int len, int initial_size, int * outlen, int parse_header) {
    stbi__zbuf a;
    char * p;

    const size_t bytes_remaining = context->working_buffer_end - context->working_buffer - context->working_buffer_used;
    if ((size_t)initial_size > bytes_remaining) {
        return NULL;
    }
    p = (char *)(context->working_buffer + context->working_buffer_used);
    context->working_buffer_used += initial_size;

    a.zbuffer = (stbi_uc *)buffer;
    a.zbuffer_end = (stbi_uc *)buffer + len;
    if (stbi__do_zlib(context, &a, p, initial_size, 1, parse_header)) {
        if (outlen)
            *outlen = (int)(a.zout - a.zout_start);
        return a.zout_start;
    } else {
        return NULL;
    }
}

static int stbi_zlib_decode_buffer(stbi__context * const context, char * obuffer, int olen, char const * ibuffer, int ilen) {
    stbi__zbuf a;
    a.zbuffer = (stbi_uc *)ibuffer;
    a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
    if (stbi__do_zlib(context, &a, obuffer, olen, 0, 1))
        return (int)(a.zout - a.zout_start);
    else
        return -1;
}

static int stbi_zlib_decode_noheader_buffer(stbi__context * const context, char * obuffer, int olen, const char * ibuffer, int ilen) {
    stbi__zbuf a;
    a.zbuffer = (stbi_uc *)ibuffer;
    a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
    if (stbi__do_zlib(context, &a, obuffer, olen, 0, 0))
        return (int)(a.zout - a.zout_start);
    else
        return -1;
}

static void stbi__skip(stbi__context * s, int n) {
    if (n < 0) {
        s->img_buffer = s->img_buffer_end;
        return;
    }
    s->img_buffer += n;
}

static int stbi__getn(stbi__context * s, stbi_uc * buffer, int n) {
    if (s->img_buffer + n <= s->img_buffer_end) {
        memcpy(buffer, s->img_buffer, n);
        s->img_buffer += n;
        return 1;
    } else
        return 0;
}

stbi_inline static stbi_uc stbi__get8(stbi__context * s) {
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    return 0;
}

static int stbi__get16be(stbi__context * s) {
    int z = stbi__get8(s);
    return (z << 8) + stbi__get8(s);
}

static stbi__uint32 stbi__get32be(stbi__context * s) {
    stbi__uint32 z = stbi__get16be(s);
    return (z << 16) + stbi__get16be(s);
}

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

typedef struct
{
    stbi__uint32 length;
    stbi__uint32 type;
} stbi__pngchunk;

static stbi__pngchunk stbi__get_chunk_header(stbi__context * s) {
    stbi__pngchunk c;
    c.length = stbi__get32be(s);
    c.type = stbi__get32be(s);
    return c;
}

static int stbi__check_png_header(stbi__context * s) {
    static const stbi_uc png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    int i;
    for (i = 0; i < 8; ++i)
        if (stbi__get8(s) != png_sig[i])
            return stbi__err("bad png sig", "Not a PNG");
    return 1;
}

typedef struct
{
    stbi__context * s;
    stbi_uc *idata, *expanded, *out;
    int depth;
} stbi__png;

enum {
    STBI__F_none = 0,
    STBI__F_sub = 1,
    STBI__F_up = 2,
    STBI__F_avg = 3,
    STBI__F_paeth = 4,
    // synthetic filters used for first scanline to avoid needing a dummy row of 0s
    STBI__F_avg_first,
    STBI__F_paeth_first
};

static stbi_uc first_row_filter[5] = {
    STBI__F_none,
    STBI__F_sub,
    STBI__F_none,
    STBI__F_avg_first,
    STBI__F_paeth_first};

static int stbi__paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc)
        return a;
    if (pb <= pc)
        return b;
    return c;
}

static const stbi_uc stbi__depth_scale_table[9] = {0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 0x01};

// create the png data from post-deflated data
static int stbi__create_png_image_raw(stbi__png * a, stbi_uc * raw, stbi__uint32 raw_len, int out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color, const int interlaced, const int palette) {
    int bytes = (depth == 16 ? 2 : 1);
    stbi__context * s = a->s;
    stbi__uint32 i, j, stride = x * out_n * bytes;
    stbi__uint32 img_len, img_width_bytes;
    int k;
    int img_n = s->img_n; // copy it into a local for later

    int output_bytes = out_n * bytes;
    int filter_bytes = img_n * bytes;
    int width = x;

    STBI_ASSERT(out_n == s->img_n || out_n == s->img_n + 1);
    STBI_ASSERT(a->s->deflate_buffer);
    const int64_t required_buffer_size = x * y * output_bytes;
    const int64_t deflate_buffer_size = a->s->deflate_buffer_end - a->s->deflate_buffer;
    if (deflate_buffer_size >= required_buffer_size && !palette && !interlaced) {
        a->out = a->s->deflate_buffer;
    } else {
        const int64_t working_buffer_remaining = a->s->working_buffer_end - a->s->working_buffer - a->s->working_buffer_used;
        if (working_buffer_remaining < required_buffer_size) {
            return stbi__err("insufficient buffer", "Out of mem");
        }
        a->out = a->s->working_buffer + a->s->working_buffer_used;
        a->s->working_buffer_used += (int)required_buffer_size;
    }

    if (!stbi__mad3sizes_valid(img_n, x, depth, 7))
        return stbi__err("too large", "Corrupt PNG");
    img_width_bytes = (((img_n * x * depth) + 7) >> 3);
    img_len = (img_width_bytes + 1) * y;

    // we used to check for exact match between raw_len and img_len on non-interlaced PNGs,
    // but issue #276 reported a PNG in the wild that had extra data at the end (all zeros),
    // so just check for raw_len < img_len always.
    if (raw_len < img_len)
        return stbi__err("not enough pixels", "Corrupt PNG");

    for (j = 0; j < y; ++j) {
        stbi_uc * cur = a->out + stride * j;
        stbi_uc * prior;
        int filter = *raw++;

        if (filter > 4)
            return stbi__err("invalid filter", "Corrupt PNG");

        if (depth < 8) {
            STBI_ASSERT(img_width_bytes <= x);
            cur += x * out_n - img_width_bytes; // store output to the rightmost img_len bytes, so we can decode in place
            filter_bytes = 1;
            width = img_width_bytes;
        }
        prior = cur - stride; // bugfix: need to compute this after 'cur +=' computation above

        // if first row, use special filter that doesn't sample previous row
        if (j == 0)
            filter = first_row_filter[filter];

        // handle first byte explicitly
        for (k = 0; k < filter_bytes; ++k) {
            switch (filter) {
                case STBI__F_none:
                    cur[k] = raw[k];
                    break;
                case STBI__F_sub:
                    cur[k] = raw[k];
                    break;
                case STBI__F_up:
                    cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                    break;
                case STBI__F_avg:
                    cur[k] = STBI__BYTECAST(raw[k] + (prior[k] >> 1));
                    break;
                case STBI__F_paeth:
                    cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(0, prior[k], 0));
                    break;
                case STBI__F_avg_first:
                    cur[k] = raw[k];
                    break;
                case STBI__F_paeth_first:
                    cur[k] = raw[k];
                    break;
            }
        }

        if (depth == 8) {
            if (img_n != out_n)
                cur[img_n] = 255; // first pixel
            raw += img_n;
            cur += out_n;
            prior += out_n;
        } else if (depth == 16) {
            if (img_n != out_n) {
                cur[filter_bytes] = 255; // first pixel top byte
                cur[filter_bytes + 1] = 255; // first pixel bottom byte
            }
            raw += filter_bytes;
            cur += output_bytes;
            prior += output_bytes;
        } else {
            raw += 1;
            cur += 1;
            prior += 1;
        }

        // this is a little gross, so that we don't switch per-pixel or per-component
        if (depth < 8 || img_n == out_n) {
            int nk = (width - 1) * filter_bytes;
#define STBI__CASE(f) \
    case f:           \
        for (k = 0; k < nk; ++k)
            switch (filter) {
                    // "none" filter turns into a memcpy here; make that explicit.
                case STBI__F_none:
                    memcpy(cur, raw, nk);
                    break;
                    STBI__CASE(STBI__F_sub) {
                        cur[k] = STBI__BYTECAST(raw[k] + cur[k - filter_bytes]);
                    }
                    break;
                    STBI__CASE(STBI__F_up) {
                        cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                    }
                    break;
                    STBI__CASE(STBI__F_avg) {
                        cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1));
                    }
                    break;
                    STBI__CASE(STBI__F_paeth) {
                        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes]));
                    }
                    break;
                    STBI__CASE(STBI__F_avg_first) {
                        cur[k] = STBI__BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1));
                    }
                    break;
                    STBI__CASE(STBI__F_paeth_first) {
                        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], 0, 0));
                    }
                    break;
            }
#undef STBI__CASE
            raw += nk;
        } else {
            STBI_ASSERT(img_n + 1 == out_n);
#define STBI__CASE(f)                                                                                                          \
    case f:                                                                                                                    \
        for (i = x - 1; i >= 1; --i, cur[filter_bytes] = 255, raw += filter_bytes, cur += output_bytes, prior += output_bytes) \
            for (k = 0; k < filter_bytes; ++k)
            switch (filter) {
                STBI__CASE(STBI__F_none) {
                    cur[k] = raw[k];
                }
                break;
                STBI__CASE(STBI__F_sub) {
                    cur[k] = STBI__BYTECAST(raw[k] + cur[k - output_bytes]);
                }
                break;
                STBI__CASE(STBI__F_up) {
                    cur[k] = STBI__BYTECAST(raw[k] + prior[k]);
                }
                break;
                STBI__CASE(STBI__F_avg) {
                    cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - output_bytes]) >> 1));
                }
                break;
                STBI__CASE(STBI__F_paeth) {
                    cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - output_bytes], prior[k], prior[k - output_bytes]));
                }
                break;
                STBI__CASE(STBI__F_avg_first) {
                    cur[k] = STBI__BYTECAST(raw[k] + (cur[k - output_bytes] >> 1));
                }
                break;
                STBI__CASE(STBI__F_paeth_first) {
                    cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - output_bytes], 0, 0));
                }
                break;
            }
#undef STBI__CASE

            // the loop above sets the high byte of the pixels' alpha, but for
            // 16 bit png files we also need the low byte set. we'll do that here.
            if (depth == 16) {
                cur = a->out + stride * j; // start at the beginning of the row again
                for (i = 0; i < x; ++i, cur += output_bytes) {
                    cur[filter_bytes + 1] = 255;
                }
            }
        }
    }

    // we make a separate pass to expand bits to pixels; for performance,
    // this could run two scanlines behind the above code, so it won't
    // intefere with filtering but will still be in the cache.
    if (depth < 8) {
        for (j = 0; j < y; ++j) {
            stbi_uc * cur = a->out + stride * j;
            stbi_uc * in = a->out + stride * j + x * out_n - img_width_bytes;
            // unpack 1/2/4-bit into a 8-bit buffer. allows us to keep the common 8-bit path optimal at minimal cost for 1/2/4-bit
            // png guarante byte alignment, if width is not multiple of 8/4/2 we'll decode dummy trailing data that will be skipped in the later loop
            stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range

            // note that the final byte might overshoot and write more data than desired.
            // we can allocate enough data that this never writes out of memory, but it
            // could also overwrite the next scanline. can it overwrite non-empty data
            // on the next scanline? yes, consider 1-pixel-wide scanlines with 1-bit-per-pixel.
            // so we need to explicitly clamp the final ones

            if (depth == 4) {
                for (k = x * img_n; k >= 2; k -= 2, ++in) {
                    *cur++ = scale * ((*in >> 4));
                    *cur++ = scale * ((*in) & 0x0f);
                }
                if (k > 0)
                    *cur++ = scale * ((*in >> 4));
            } else if (depth == 2) {
                for (k = x * img_n; k >= 4; k -= 4, ++in) {
                    *cur++ = scale * ((*in >> 6));
                    *cur++ = scale * ((*in >> 4) & 0x03);
                    *cur++ = scale * ((*in >> 2) & 0x03);
                    *cur++ = scale * ((*in) & 0x03);
                }
                if (k > 0)
                    *cur++ = scale * ((*in >> 6));
                if (k > 1)
                    *cur++ = scale * ((*in >> 4) & 0x03);
                if (k > 2)
                    *cur++ = scale * ((*in >> 2) & 0x03);
            } else if (depth == 1) {
                for (k = x * img_n; k >= 8; k -= 8, ++in) {
                    *cur++ = scale * ((*in >> 7));
                    *cur++ = scale * ((*in >> 6) & 0x01);
                    *cur++ = scale * ((*in >> 5) & 0x01);
                    *cur++ = scale * ((*in >> 4) & 0x01);
                    *cur++ = scale * ((*in >> 3) & 0x01);
                    *cur++ = scale * ((*in >> 2) & 0x01);
                    *cur++ = scale * ((*in >> 1) & 0x01);
                    *cur++ = scale * ((*in) & 0x01);
                }
                if (k > 0)
                    *cur++ = scale * ((*in >> 7));
                if (k > 1)
                    *cur++ = scale * ((*in >> 6) & 0x01);
                if (k > 2)
                    *cur++ = scale * ((*in >> 5) & 0x01);
                if (k > 3)
                    *cur++ = scale * ((*in >> 4) & 0x01);
                if (k > 4)
                    *cur++ = scale * ((*in >> 3) & 0x01);
                if (k > 5)
                    *cur++ = scale * ((*in >> 2) & 0x01);
                if (k > 6)
                    *cur++ = scale * ((*in >> 1) & 0x01);
            }
            if (img_n != out_n) {
                int q;
                // insert alpha = 255
                cur = a->out + stride * j;
                if (img_n == 1) {
                    for (q = x - 1; q >= 0; --q) {
                        cur[q * 2 + 1] = 255;
                        cur[q * 2 + 0] = cur[q];
                    }
                } else {
                    STBI_ASSERT(img_n == 3);
                    for (q = x - 1; q >= 0; --q) {
                        cur[q * 4 + 3] = 255;
                        cur[q * 4 + 2] = cur[q * 3 + 2];
                        cur[q * 4 + 1] = cur[q * 3 + 1];
                        cur[q * 4 + 0] = cur[q * 3 + 0];
                    }
                }
            }
        }
    } else if (depth == 16) {
        // force the image data from big-endian to platform-native.
        // this is done in a separate pass due to the decoding relying
        // on the data being untouched, but could probably be done
        // per-line during decode if care is taken.
        stbi_uc * cur = a->out;
        stbi__uint16 * cur16 = (stbi__uint16 *)cur;

        for (i = 0; i < x * y * out_n; ++i, cur16++, cur += 2) {
            *cur16 = (cur[0] << 8) | cur[1];
        }
    }

    return 1;
}

static int stbi__create_png_image(stbi__png * a, stbi_uc * image_data, stbi__uint32 image_data_len, int out_n, int depth, int color, int interlaced, const int palette) {
    int bytes = (depth == 16 ? 2 : 1);
    int out_bytes = out_n * bytes;
    stbi_uc * final;
    int p;
    if (!interlaced)
        return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color, interlaced, palette);

    // de-interlacing
    const int64_t buffer_size = a->s->deflate_buffer_end - a->s->deflate_buffer;
    const int64_t required_size = a->s->img_x * a->s->img_y * out_bytes;
    if (buffer_size < required_size) {
        return 0;
    }
    final = a->s->deflate_buffer;
    for (p = 0; p < 7; ++p) {
        int xorig[] = {0, 4, 0, 2, 0, 1, 0};
        int yorig[] = {0, 0, 4, 0, 2, 0, 1};
        int xspc[] = {8, 8, 4, 4, 2, 2, 1};
        int yspc[] = {8, 8, 8, 4, 4, 2, 2};
        int i, j, x, y;
        // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
        x = (a->s->img_x - xorig[p] + xspc[p] - 1) / xspc[p];
        y = (a->s->img_y - yorig[p] + yspc[p] - 1) / yspc[p];
        if (x && y) {
            stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
            if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color, interlaced, palette)) {
                return 0;
            }
            for (j = 0; j < y; ++j) {
                for (i = 0; i < x; ++i) {
                    int out_y = j * yspc[p] + yorig[p];
                    int out_x = i * xspc[p] + xorig[p];
                    memcpy(final + out_y * a->s->img_x * out_bytes + out_x * out_bytes, a->out + (j * x + i) * out_bytes, out_bytes);
                }
            }
            image_data += img_len;
            image_data_len -= img_len;
        }
    }
    a->out = final;

    return 1;
}

static int stbi__compute_transparency(stbi__png * z, stbi_uc tc[3], int out_n) {
    stbi__context * s = z->s;
    stbi__uint32 i, pixel_count = s->img_x * s->img_y;
    stbi_uc * p = z->out;

    // compute color-based transparency, assuming we've
    // already got 255 as the alpha value in the output
    STBI_ASSERT(out_n == 2 || out_n == 4);

    if (out_n == 2) {
        for (i = 0; i < pixel_count; ++i) {
            p[1] = (p[0] == tc[0] ? 0 : 255);
            p += 2;
        }
    } else {
        for (i = 0; i < pixel_count; ++i) {
            if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
                p[3] = 0;
            p += 4;
        }
    }
    return 1;
}

static int stbi__compute_transparency16(stbi__png * z, stbi__uint16 tc[3], int out_n) {
    stbi__context * s = z->s;
    stbi__uint32 i, pixel_count = s->img_x * s->img_y;
    stbi__uint16 * p = (stbi__uint16 *)z->out;

    // compute color-based transparency, assuming we've
    // already got 65535 as the alpha value in the output
    STBI_ASSERT(out_n == 2 || out_n == 4);

    if (out_n == 2) {
        for (i = 0; i < pixel_count; ++i) {
            p[1] = (p[0] == tc[0] ? 0 : 65535);
            p += 2;
        }
    } else {
        for (i = 0; i < pixel_count; ++i) {
            if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
                p[3] = 0;
            p += 4;
        }
    }
    return 1;
}

static int stbi__expand_png_palette(stbi__png * a, stbi_uc * palette, int len, int pal_img_n) {
    stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
    stbi_uc *p, *temp_out, *orig = a->out;

    const size_t buffer_size = a->s->deflate_buffer_end - a->s->deflate_buffer;
    const size_t required_size = pixel_count * pal_img_n;
    if (buffer_size < required_size) {
        return 0;
    }
    p = a->s->deflate_buffer;

    // between here and free(out) below, exitting would leak
    temp_out = p;

    if (pal_img_n == 3) {
        for (i = 0; i < pixel_count; ++i) {
            int n = orig[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p += 3;
        }
    } else {
        for (i = 0; i < pixel_count; ++i) {
            int n = orig[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p[3] = palette[n + 3];
            p += 4;
        }
    }
    a->out = temp_out;

    STBI_NOTUSED(len);

    return 1;
}

static int stbi__unpremultiply_on_load = 1;
static int stbi__de_iphone_flag = 1;

static void stbi__de_iphone(stbi__png * z) {
    stbi__context * s = z->s;
    stbi__uint32 i, pixel_count = s->img_x * s->img_y;
    stbi_uc * p = z->out;

    if (s->img_out_n == 3) { // convert bgr to rgb
        for (i = 0; i < pixel_count; ++i) {
            stbi_uc t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 3;
        }
    } else {
        STBI_ASSERT(s->img_out_n == 4);
        if (stbi__unpremultiply_on_load) {
            // convert bgr to rgb and unpremultiply
            for (i = 0; i < pixel_count; ++i) {
                stbi_uc a = p[3];
                stbi_uc t = p[0];
                if (a) {
                    stbi_uc half = a / 2;
                    p[0] = (p[2] * 255 + half) / a;
                    p[1] = (p[1] * 255 + half) / a;
                    p[2] = (t * 255 + half) / a;
                } else {
                    p[0] = p[2];
                    p[2] = t;
                }
                p += 4;
            }
        } else {
            // convert bgr to rgb
            for (i = 0; i < pixel_count; ++i) {
                stbi_uc t = p[0];
                p[0] = p[2];
                p[2] = t;
                p += 4;
            }
        }
    }
}

#define STBI__PNG_TYPE(a, b, c, d) (((unsigned)(a) << 24) + ((unsigned)(b) << 16) + ((unsigned)(c) << 8) + (unsigned)(d))

static int stbi__parse_png_file(stbi__png * z, int scan, int req_comp) {
    stbi_uc palette[1024], pal_img_n = 0;
    stbi_uc has_trans = 0, tc[3] = {0};
    stbi__uint16 tc16[3] = {0};
    stbi__uint32 ioff = 0, idata_limit = 0, i, pal_len = 0;
    int first = 1, k, interlace = 0, color = 0, is_iphone = 0;
    stbi__context * s = z->s;

    z->expanded = NULL;
    z->idata = NULL;
    z->out = NULL;

    if (!stbi__check_png_header(s))
        return 0;

    if (scan == STBI__SCAN_type)
        return 1;

    for (;;) {
        stbi__pngchunk c = stbi__get_chunk_header(s);
        switch (c.type) {
            case STBI__PNG_TYPE('C', 'g', 'B', 'I'):
                is_iphone = 1;
                stbi__skip(s, c.length);
                break;
            case STBI__PNG_TYPE('I', 'H', 'D', 'R'): {
                int comp, filter;
                if (!first)
                    return stbi__err("multiple IHDR", "Corrupt PNG");
                first = 0;
                if (c.length != 13)
                    return stbi__err("bad IHDR len", "Corrupt PNG");
                s->img_x = stbi__get32be(s);
                if (s->img_x > (1 << 24))
                    return stbi__err("too large", "Very large image (corrupt?)");
                s->img_y = stbi__get32be(s);
                if (s->img_y > (1 << 24))
                    return stbi__err("too large", "Very large image (corrupt?)");
                z->depth = stbi__get8(s);
                if (z->depth != 1 && z->depth != 2 && z->depth != 4 && z->depth != 8 && z->depth != 16)
                    return stbi__err("1/2/4/8/16-bit only", "PNG not supported: 1/2/4/8/16-bit only");
                color = stbi__get8(s);
                if (color > 6)
                    return stbi__err("bad ctype", "Corrupt PNG");
                if (color == 3 && z->depth == 16)
                    return stbi__err("bad ctype", "Corrupt PNG");
                if (color == 3)
                    pal_img_n = 3;
                else if (color & 1)
                    return stbi__err("bad ctype", "Corrupt PNG");
                comp = stbi__get8(s);
                if (comp)
                    return stbi__err("bad comp method", "Corrupt PNG");
                filter = stbi__get8(s);
                if (filter)
                    return stbi__err("bad filter method", "Corrupt PNG");
                interlace = stbi__get8(s);
                if (interlace > 1)
                    return stbi__err("bad interlace method", "Corrupt PNG");
                if (!s->img_x || !s->img_y)
                    return stbi__err("0-pixel image", "Corrupt PNG");
                if (!pal_img_n) {
                    s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
                    if ((1 << 30) / s->img_x / s->img_n < s->img_y)
                        return stbi__err("too large", "Image too large to decode");
                    if (scan == STBI__SCAN_header)
                        return 1;
                } else {
                    // if paletted, then pal_n is our final components, and
                    // img_n is # components to decompress/filter.
                    s->img_n = 1;
                    if ((1 << 30) / s->img_x / 4 < s->img_y)
                        return stbi__err("too large", "Corrupt PNG");
                    // if SCAN_header, have to scan to see if we have a tRNS
                }
                break;
            }

            case STBI__PNG_TYPE('P', 'L', 'T', 'E'): {
                if (first)
                    return stbi__err("first not IHDR", "Corrupt PNG");
                if (c.length > 256 * 3)
                    return stbi__err("invalid PLTE", "Corrupt PNG");
                pal_len = c.length / 3;
                if (pal_len * 3 != c.length)
                    return stbi__err("invalid PLTE", "Corrupt PNG");
                for (i = 0; i < pal_len; ++i) {
                    palette[i * 4 + 0] = stbi__get8(s);
                    palette[i * 4 + 1] = stbi__get8(s);
                    palette[i * 4 + 2] = stbi__get8(s);
                    palette[i * 4 + 3] = 255;
                }
                break;
            }

            case STBI__PNG_TYPE('t', 'R', 'N', 'S'): {
                if (first)
                    return stbi__err("first not IHDR", "Corrupt PNG");
                if (z->idata)
                    return stbi__err("tRNS after IDAT", "Corrupt PNG");
                if (pal_img_n) {
                    if (scan == STBI__SCAN_header) {
                        s->img_n = 4;
                        return 1;
                    }
                    if (pal_len == 0)
                        return stbi__err("tRNS before PLTE", "Corrupt PNG");
                    if (c.length > pal_len)
                        return stbi__err("bad tRNS len", "Corrupt PNG");
                    pal_img_n = 4;
                    for (i = 0; i < c.length; ++i)
                        palette[i * 4 + 3] = stbi__get8(s);
                } else {
                    if (!(s->img_n & 1))
                        return stbi__err("tRNS with alpha", "Corrupt PNG");
                    if (c.length != (stbi__uint32)s->img_n * 2)
                        return stbi__err("bad tRNS len", "Corrupt PNG");
                    has_trans = 1;
                    if (z->depth == 16) {
                        for (k = 0; k < s->img_n; ++k)
                            tc16[k] = (stbi__uint16)stbi__get16be(s); // copy the values as-is
                    } else {
                        for (k = 0; k < s->img_n; ++k)
                            tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[z->depth]; // non 8-bit images will be larger
                    }
                }
                break;
            }

            case STBI__PNG_TYPE('I', 'D', 'A', 'T'): {
                if (first)
                    return stbi__err("first not IHDR", "Corrupt PNG");
                if (pal_img_n && !pal_len)
                    return stbi__err("no PLTE", "Corrupt PNG");
                if (scan == STBI__SCAN_header) {
                    s->img_n = pal_img_n;
                    return 1;
                }
                if ((int)(ioff + c.length) < (int)ioff)
                    return 0;
                if (ioff + c.length > idata_limit) {
                    stbi__uint32 idata_limit_old = idata_limit;
                    if (idata_limit == 0)
                        idata_limit = c.length > 4096 ? c.length : 4096;
                    while (ioff + c.length > idata_limit)
                        idata_limit *= 2;
                    STBI_NOTUSED(idata_limit_old);
                    if ((int64_t)idata_limit > (z->s->working_buffer_end - z->s->working_buffer)) {
                        return stbi__err("outofmem", "Out of memory");
                    } else {
                        z->idata = z->s->working_buffer;
                        z->s->working_buffer_used = idata_limit;
                    }
                }
                if (!stbi__getn(s, z->idata + ioff, c.length))
                    return stbi__err("outofdata", "Corrupt PNG");
                ioff += c.length;
                break;
            }

            case STBI__PNG_TYPE('I', 'E', 'N', 'D'): {
                stbi__uint32 raw_len, bpl;
                if (first)
                    return stbi__err("first not IHDR", "Corrupt PNG");
                if (scan != STBI__SCAN_load)
                    return 1;
                if (z->idata == NULL)
                    return stbi__err("no IDAT", "Corrupt PNG");
                // initial guess for decoded data size to avoid unnecessary reallocs
                bpl = (s->img_x * z->depth + 7) / 8; // bytes per line, per component
                raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;

                raw_len += interlace ? imagelib_interlace_huffman_max_overhead : 0;
                z->expanded = (stbi_uc *)stbi_zlib_decode_malloc_guesssize_headerflag(z->s, (char *)z->idata, ioff, raw_len, (int *)&raw_len, !is_iphone);
                if (z->expanded == NULL)
                    return 0; // zlib should set error
                if ((req_comp == s->img_n + 1 && req_comp != 3 && !pal_img_n) || has_trans)
                    s->img_out_n = s->img_n + 1;
                else
                    s->img_out_n = s->img_n;
                if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, z->depth, color, interlace, pal_img_n))
                    return 0;
                if (has_trans) {
                    if (z->depth == 16) {
                        if (!stbi__compute_transparency16(z, tc16, s->img_out_n))
                            return 0;
                    } else {
                        if (!stbi__compute_transparency(z, tc, s->img_out_n))
                            return 0;
                    }
                }
                if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
                    stbi__de_iphone(z);
                if (pal_img_n) {
                    // pal_img_n == 3 or 4
                    s->img_n = pal_img_n; // record the actual colors we had
                    s->img_out_n = pal_img_n;
                    if (req_comp >= 3)
                        s->img_out_n = req_comp;
                    if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
                        return 0;
                } else if (has_trans) {
                    // non-paletted image with tRNS -> source image has (constant) alpha
                    ++s->img_n;
                }
                return 1;
            }

            default:
                // if critical, fail
                if (first)
                    return stbi__err("first not IHDR", "Corrupt PNG");
                if ((c.type & (1 << 29)) == 0) {
#ifndef STBI_NO_FAILURE_STRINGS
                    // not threadsafe
                    static char invalid_chunk[] = "XXXX PNG chunk not known";
                    invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
                    invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
                    invalid_chunk[2] = STBI__BYTECAST(c.type >> 8);
                    invalid_chunk[3] = STBI__BYTECAST(c.type >> 0);
#endif
                    return stbi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
                }
                stbi__skip(s, c.length);
                break;
        }
        // end of PNG chunk, read and skip CRC
        stbi__get32be(s);
    }
}

static void * stbi__do_png(stbi__png * p, int * x, int * y, int * n, int req_comp, stbi__result_info * ri) {
    void * result = NULL;
    if (req_comp < 0 || req_comp > 4)
        return stbi__errpuc("bad req_comp", "Internal error");
    if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp)) {
        if (p->depth < 8)
            ri->bits_per_channel = 8;
        else
            ri->bits_per_channel = p->depth;
        result = p->out;
        p->out = NULL;
        if (req_comp && req_comp != p->s->img_out_n) {
            TRAP("imagelib_png requires that 'required_comp' be 0 so stb will auto detect as we don't do channel conversions anymore");
        }
        *x = p->s->img_x;
        *y = p->s->img_y;
        if (n)
            *n = p->s->img_n;
    }
    return result;
}

static void * stbi__png_load(stbi__context * s, int * x, int * y, int * comp, int req_comp, stbi__result_info * ri) {
    stbi__png p;
    p.s = s;
    return stbi__do_png(&p, x, y, comp, req_comp, ri);
}

static void stbi__rewind(stbi__context * s) {
    // conceptually rewind SHOULD rewind to the beginning of the stream,
    // but we just rewind to the beginning of the initial buffer, because
    // we only use it after doing 'test', which only ever looks at at most 92 bytes
    s->img_buffer = s->img_buffer_original;
    s->img_buffer_end = s->img_buffer_original_end;
}

static int stbi__png_test(stbi__context * s) {
    int r;
    r = stbi__check_png_header(s);
    stbi__rewind(s);
    return r;
}

static int stbi__png_info_raw(stbi__png * p, int * x, int * y, int * comp) {
    if (!stbi__parse_png_file(p, STBI__SCAN_header, 0)) {
        stbi__rewind(p->s);
        return 0;
    }
    if (x)
        *x = p->s->img_x;
    if (y)
        *y = p->s->img_y;
    if (comp)
        *comp = p->s->img_n;
    return 1;
}

static int stbi__png_info(stbi__context * s, int * x, int * y, int * comp) {
    stbi__png p = {0};
    p.s = s;
    return stbi__png_info_raw(&p, x, y, comp);
}

static int stbi__png_is16(stbi__context * s) {
    stbi__png p = {0};
    p.s = s;
    if (!stbi__png_info_raw(&p, NULL, NULL, NULL))
        return 0;
    if (p.depth != 16) {
        stbi__rewind(p.s);
        return 0;
    }
    return 1;
}

static void stbi__start_mem_in_line(stbi__context * s, const stbi_uc * const pixel_buffer, const int pixel_buffer_len, stbi_uc * const deflate_buffer, const int deflate_buffer_len, stbi_uc * const working_buffer, const int working_buffer_len) {
    s->img_buffer = s->img_buffer_original = (stbi_uc *)pixel_buffer;
    s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)pixel_buffer + pixel_buffer_len;
    s->deflate_buffer = deflate_buffer;
    s->deflate_buffer_end = deflate_buffer + deflate_buffer_len;
    s->working_buffer = working_buffer;
    s->working_buffer_end = working_buffer + working_buffer_len;
    s->working_buffer_used = 0;
}

static void * stbi__load_main(stbi__context * s, int * x, int * y, int * comp, int req_comp, stbi__result_info * ri, int bpc) {
    memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
    ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
    ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
    ri->num_channels = 0;

    if (stbi__png_test(s)) {
        return stbi__png_load(s, x, y, comp, req_comp, ri);
    }

    return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
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

static int big_endian_to_native_int(const int num) {
    static const int endian_constant = 0x01020304;
    if (((char *)&endian_constant)[0] == (char)0x01) {
        return num;
    } else {
        return ((num & 0xff000000) >> 24) | ((num & 0x00ff0000) >> 8) | ((num & 0x0000ff00) << 8) | (num << 24);
    }
}

static bool imagelib_png_compare_chunk_code(const uint8_t * const ptr, const uint8_t * const field_name_str) {
    // all chunk field names/codes are 4 bytes
    for (int i = 0; i < 4; ++i) {
        if (ptr[i] != field_name_str[i]) {
            return false;
        }
    }
    return true;
}

static int imagelib_get_png_palette_channels(const const_mem_region_t region) {
    const int png_header_length = 8;
    const int field_name_length = 4;
    const int crc_length = 4;

    const uint8_t iend_str[] = "IEND";
    const uint8_t trns_str[] = "tRNS";

    const uint8_t * field_header = region.byte_ptr + png_header_length;

    while (!imagelib_png_compare_chunk_code(field_header + sizeof(int), iend_str)) {
        if (imagelib_png_compare_chunk_code(field_header + sizeof(int), trns_str)) {
            return 4;
        }
        const int field_length = big_endian_to_native_int(*(int *)(field_header));
        field_header += field_length + (sizeof(int) + field_name_length + crc_length);
    }

    return 3;
}

// see: http://www.libpng.org/pub/png/spec/1.2/png-1.2.pdf page 16.
enum png_color_codes {
    png_color_code_grayscale = 0,
    png_color_code_rgb = 2,
    png_color_code_palette = 3,
    png_color_code_grayscale_alpha = 4,
    png_color_code_rgba = 6,
};

bool imagelib_read_png_header_from_memory(const const_mem_region_t png_file_data, image_t * const out_image, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size) {
    const uint8_t png_magic_number[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

    if (memcmp(png_magic_number, png_file_data.ptr, sizeof(png_magic_number)) != 0) {
        return false;
    }

    const int * const dim_start = (const int *)(png_file_data.byte_ptr + 12 + 4);
    const uint8_t * const end_of_dims = (const uint8_t *)(dim_start + 2);

    const int color_mode = *(end_of_dims + 1);
    const int depth = *end_of_dims;
    int channels;

    if (color_mode == png_color_code_grayscale) {
        channels = 1;
    } else if (color_mode == png_color_code_rgb) {
        channels = 3;
    } else if (color_mode == png_color_code_palette) {
        channels = imagelib_get_png_palette_channels(png_file_data);
    } else if (color_mode == png_color_code_grayscale_alpha) {
        channels = 2;
    } else {
        ASSERT(color_mode == png_color_code_rgba);
        channels = 4;
    }

    const bool is_interlaced = *(end_of_dims + 4);

    out_image->width = big_endian_to_native_int(*dim_start);
    out_image->height = big_endian_to_native_int(*(dim_start + 1));
    out_image->bpp = channels;

    out_image->depth = 1;
    out_image->pitch = out_image->width * out_image->bpp;
    out_image->spitch = out_image->data_len = out_image->width * out_image->height * out_image->bpp;
    out_image->encoding = image_encoding_uncompressed;

    // stb is slightly overallocated/overaligned and needs the extra height bytes at the end.
    *out_required_pixel_buffer_size = out_image->width * out_image->height * out_image->bpp + out_image->height;

    // same guess as in STB png decoding (needs to match)
    // rough logic: the maximum deflate size should be: (width * height * channels * depth), for an rgb/rgba (palette or otherwise) image, depth can only be 8bits or 16bits
    // the typical case is going to be 8bits, and the output image depth will ultimately be 8bits.
    // but in the event someone saves an image with extra bit depth we need to have additional space for decompressing the zlib stream (which can never be larger than the images original maximal dimensions)
    const int zlib_initial_decompress_size = ((out_image->width * depth + 7) / 8) * out_image->height * out_image->bpp + out_image->height;

    // STB over allocates for the stream size in factors of 4096
    // it doubles the allotted size each time it fails to allocate the zlib stream.
    // the zlib stream is the sum of all IDATA blocks
    // so a fast guess to its full size is bounded by the size of the file (as it must exist in full, in the file)
    const int zlib_stream_size = (size_t)(1 << (sizeof(uint32_t) * 8 - __builtin_clz((uint32_t)png_file_data.size)));
    const int zlib_requirement = ((zlib_stream_size > 4096) ? zlib_stream_size : 4096);

    // if an image is in 'palette' mode then the image data is an index to a 256 table of rgb/rgba
    // thus the space required for that is the (width * height * 1) (for the single byte indicating what index to use)
    const int palette_requirement = (color_mode & 1) ? (out_image->width * out_image->height) : 0;
    int interlace_requirement = 0;

    if (is_interlaced) {
        // assume worst case zlib huffman overhead per interlaced block
        // this _must_ be the same value as specified in stb
        // a huffman block's max size is 258 bytes if the image is interlaced there are 8 of them
        interlace_requirement += imagelib_interlace_huffman_max_overhead;

        // stb allocates a separate buffer for every interlace, and then copies it over into the appropriate spot for the final image..
        // interlace overhead (ripped from stb)
        // in practice this means roughly the same size as the output image
        for (int i = 0; i < 7; ++i) {
            const int xorig[] = {0, 4, 0, 2, 0, 1, 0};
            const int yorig[] = {0, 0, 4, 0, 2, 0, 1};
            const int xspc[] = {8, 8, 4, 4, 2, 2, 1};
            const int yspc[] = {8, 8, 8, 4, 4, 2, 2};

            const int x = (out_image->width - xorig[i] + xspc[i] - 1) / xspc[i];
            const int y = (out_image->height - yorig[i] + yspc[i] - 1) / yspc[i];
            if (x && y) {
                interlace_requirement += ((((out_image->bpp * x * depth) + 7) >> 3) + 1) * y;
            }
        }
    }

    *out_required_working_space_size = zlib_requirement + zlib_initial_decompress_size + palette_requirement + interlace_requirement;
    return true;
}

bool imagelib_load_png_from_memory(const const_mem_region_t png_file_data, image_t * const out_image, const mem_region_t pixel_region, const mem_region_t working_space_region) {
    int w, h, channels;
    out_image->data = stbi_load_png_from_memory_in_place(
        png_file_data.ptr,
        (int)png_file_data.size,
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
