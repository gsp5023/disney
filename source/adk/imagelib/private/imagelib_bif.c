/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
imagelib_bif.c

Incorporation of nanojpeg for decoding jpeg files in bif containers.

BIF (Base Index Frames) File Archive Support.
The BIF archive is used to encapsulate a set of still images for supporting
video trick modes such as (FF/REW). All values stores as little endian.
*/

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGELIB_BIF_FOURCC_TAG FOURCC('I', 'B', 'I', 'F')

static unsigned char bif_magic_number[8] = {0x89, 0x42, 0x49, 0x46, 0x0d, 0x0a, 0x1a, 0x0a};

enum {
    bif_file_version = 0
};

/* UJ_NODECODE_BLOCK_SIZE: if #defined, this specifies the amount of bytes
 * to load from disk if ujDecodeFile() is used after ujDisableDecoding().
 * This will speed up checking of large files, because not the whole file has
 * to be read, but just a small portion of it. On the other hand, it will also
 * break checking of files where the actual image data starts after this point
 * (though it's questionable if any real-world file would ever trigger that). */
#define UJ_NODECODE_BLOCK_SIZE (256 * 1024)

typedef PACK(struct bif_header_t {
    uint8_t magic_number[8]; // .BIF
    uint32_t version;
    uint32_t num_images;
    // Real timestamp is the multiplier x timestamp entry in BIF index
    // If 0 this is zero, then use 1000.
    uint32_t timestamp_multiplier;
    uint8_t reserved[44];
}) bif_header_t;

typedef PACK(struct bif_index_t {
    uint32_t timestamp;
    uint32_t frame_offset;
}) bif_index_t;

STATIC_ASSERT(sizeof(bif_header_t) == 64);
STATIC_ASSERT(sizeof(bif_index_t) == 8);

typedef struct _uj_code {
    unsigned char bits, code;
} ujVLCCode;

typedef struct _uj_cmp {
    int width, height;
    int stride;
    unsigned char * pixels;
    unsigned char * pixels_buffer;
    int pixels_length;
    int cid;
    int ssx, ssy;
    int qtsel;
    int actabsel, dctabsel;
    int dcpred;
} ujComponent;

typedef struct _uj_ctx {
    const unsigned char * pos;
    int valid, decoded;
    int no_decode;
    int fast_chroma;
    int size;
    int length;
    int width, height;
    int mbwidth, mbheight;
    int mbsizex, mbsizey;
    int ncomp;
    ujComponent comp[3];
    int qtused, qtavail;
    unsigned char qtab[4][64];
    ujVLCCode vlctab[4][65536];
    int buf, bufbits;
    int block[64];
    int rstinterval;
    unsigned char * rgb;
    int exif_le;
    int co_sited_chroma;
    // added to support in-place decoding
    int calc_mem_requirements;
    size_t offset_to_pixels_buffer;
    size_t offset_to_final_working_buffer;
    unsigned char * working_buffer;
    int working_buffer_length;
} ujContext;

// result codes for ujDecode()
typedef enum _uj_result {
    UJ_OK = 0, // no error, decoding successful
    UJ_NO_CONTEXT = 1, // called uj* function without image handle
    UJ_NOT_DECODED = 2, // image has not yet been decoded
    UJ_INVALID_ARG = 3, // invalid argument
    UJ_IO_ERROR = 4, // file I/O error
    UJ_OUT_OF_MEM = 5, // out of memory
    UJ_NO_JPEG = 6, // not a JPEG file
    UJ_UNSUPPORTED = 7, // unsupported format
    UJ_SYNTAX_ERROR = 8, // syntax error
    UJ_INTERNAL_ERR = 9, // internal error
    __UJ_FINISHED // used internally, will never be reported
} ujResult;

// plane (color component) structure
typedef struct _uj_plane {
    int width; // visible width
    int height; // visible height
    int stride; // line size in bytes
    unsigned char * pixels; // pixel data
} ujPlane;

// data type for uJPEG image handles
typedef void * ujImage;

// tell the context whether which chroma upsampling mode to use
#define UJ_CHROMA_MODE_FAST 1 // low-quality pixel repetition (fast)
#define UJ_CHROMA_MODE_ACCURATE 0 // accurate bicubic upsampling (slower)
#define UJ_CHROMA_MODE_DEFAULT 0 // default mode: accurate

static ujResult ujError = UJ_OK;

static const char ujZZ[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

static inline unsigned char ujClip(const int x) {
    return (x < 0) ? 0 : ((x > 0xFF) ? 0xFF : (unsigned char)x);
}

///////////////////////////////////////////////////////////////////////////////

#define W1 2841
#define W2 2676
#define W3 2408
#define W5 1609
#define W6 1108
#define W7 565

static inline void ujRowIDCT(int * blk) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blk[4] << 11)
          | (x2 = blk[6])
          | (x3 = blk[2])
          | (x4 = blk[1])
          | (x5 = blk[7])
          | (x6 = blk[5])
          | (x7 = blk[3]))) {
        blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
        return;
    }
    x0 = (blk[0] << 11) + 128;
    x8 = W7 * (x4 + x5);
    x4 = x8 + (W1 - W7) * x4;
    x5 = x8 - (W1 + W7) * x5;
    x8 = W3 * (x6 + x7);
    x6 = x8 - (W3 - W5) * x6;
    x7 = x8 - (W3 + W5) * x7;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2);
    x2 = x1 - (W2 + W6) * x2;
    x3 = x1 + (W2 - W6) * x3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;
    blk[0] = (x7 + x1) >> 8;
    blk[1] = (x3 + x2) >> 8;
    blk[2] = (x0 + x4) >> 8;
    blk[3] = (x8 + x6) >> 8;
    blk[4] = (x8 - x6) >> 8;
    blk[5] = (x0 - x4) >> 8;
    blk[6] = (x3 - x2) >> 8;
    blk[7] = (x7 - x1) >> 8;
}

static inline void ujColIDCT(const int * blk, unsigned char * out, int stride) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blk[8 * 4] << 8)
          | (x2 = blk[8 * 6])
          | (x3 = blk[8 * 2])
          | (x4 = blk[8 * 1])
          | (x5 = blk[8 * 7])
          | (x6 = blk[8 * 5])
          | (x7 = blk[8 * 3]))) {
        x1 = ujClip(((blk[0] + 32) >> 6) + 128);
        for (x0 = 8; x0; --x0) {
            *out = (unsigned char)x1;
            out += stride;
        }
        return;
    }
    x0 = (blk[0] << 8) + 8192;
    x8 = W7 * (x4 + x5) + 4;
    x4 = (x8 + (W1 - W7) * x4) >> 3;
    x5 = (x8 - (W1 + W7) * x5) >> 3;
    x8 = W3 * (x6 + x7) + 4;
    x6 = (x8 - (W3 - W5) * x6) >> 3;
    x7 = (x8 - (W3 + W5) * x7) >> 3;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2) + 4;
    x2 = (x1 - (W2 + W6) * x2) >> 3;
    x3 = (x1 + (W2 - W6) * x3) >> 3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;
    *out = ujClip(((x7 + x1) >> 14) + 128);
    out += stride;
    *out = ujClip(((x3 + x2) >> 14) + 128);
    out += stride;
    *out = ujClip(((x0 + x4) >> 14) + 128);
    out += stride;
    *out = ujClip(((x8 + x6) >> 14) + 128);
    out += stride;
    *out = ujClip(((x8 - x6) >> 14) + 128);
    out += stride;
    *out = ujClip(((x0 - x4) >> 14) + 128);
    out += stride;
    *out = ujClip(((x3 - x2) >> 14) + 128);
    out += stride;
    *out = ujClip(((x7 - x1) >> 14) + 128);
}

///////////////////////////////////////////////////////////////////////////////

#define ujThrow(e)   \
    do {             \
        ujError = e; \
        return;      \
    } while (0)
#define ujCheckError() \
    do {               \
        if (ujError)   \
            return;    \
    } while (0)

static int ujShowBits(ujContext * uj, int bits) {
    unsigned char newbyte;
    if (!bits)
        return 0;
    while (uj->bufbits < bits) {
        if (uj->size <= 0) {
            uj->buf = (uj->buf << 8) | 0xFF;
            uj->bufbits += 8;
            continue;
        }
        newbyte = *uj->pos++;
        uj->size--;
        uj->bufbits += 8;
        uj->buf = (uj->buf << 8) | newbyte;
        if (newbyte == 0xFF) {
            if (uj->size) {
                unsigned char marker = *uj->pos++;
                uj->size--;
                switch (marker) {
                    case 0x00:
                    case 0xFF:
                        break;
                    case 0xD9:
                        uj->size = 0;
                        break;
                    default:
                        if ((marker & 0xF8) != 0xD0)
                            ujError = UJ_SYNTAX_ERROR;
                        else {
                            uj->buf = (uj->buf << 8) | marker;
                            uj->bufbits += 8;
                        }
                }
            } else
                ujError = UJ_SYNTAX_ERROR;
        }
    }
    return (uj->buf >> (uj->bufbits - bits)) & ((1 << bits) - 1);
}

static inline void ujSkipBits(ujContext * uj, int bits) {
    if (uj->bufbits < bits)
        (void)ujShowBits(uj, bits);
    uj->bufbits -= bits;
}

static inline int ujGetBits(ujContext * uj, int bits) {
    int res = ujShowBits(uj, bits);
    ujSkipBits(uj, bits);
    return res;
}

static inline void ujByteAlign(ujContext * uj) {
    uj->bufbits &= 0xF8;
}

static void ujSkip(ujContext * uj, int count) {
    uj->pos += count;
    uj->size -= count;
    uj->length -= count;
    if (uj->size < 0)
        ujError = UJ_SYNTAX_ERROR;
}

static inline unsigned short ujDecode16(const unsigned char * pos) {
    return (pos[0] << 8) | pos[1];
}

static void ujDecodeLength(ujContext * uj) {
    if (uj->size < 2)
        ujThrow(UJ_SYNTAX_ERROR);
    uj->length = ujDecode16(uj->pos);
    if (uj->length > uj->size)
        ujThrow(UJ_SYNTAX_ERROR);
    ujSkip(uj, 2);
}

static inline void ujSkipMarker(ujContext * uj) {
    ujDecodeLength(uj);
    ujSkip(uj, uj->length);
}

static inline void ujDecodeSOF(ujContext * uj) {
    int i, ssxmax = 0, ssymax = 0;
    ujComponent * c;
    ujDecodeLength(uj);
    ujCheckError();
    if (uj->length < 9)
        ujThrow(UJ_SYNTAX_ERROR);
    if (uj->pos[0] != 8)
        ujThrow(UJ_UNSUPPORTED);
    uj->height = ujDecode16(uj->pos + 1);
    uj->width = ujDecode16(uj->pos + 3);
    if (!uj->width || !uj->height)
        ujThrow(UJ_SYNTAX_ERROR);
    uj->ncomp = uj->pos[5];
    ujSkip(uj, 6);
    switch (uj->ncomp) {
        case 1:
        case 3:
            break;
        default:
            ujThrow(UJ_UNSUPPORTED);
    }
    if (uj->length < (uj->ncomp * 3))
        ujThrow(UJ_SYNTAX_ERROR);
    for (i = 0, c = uj->comp; i < uj->ncomp; ++i, ++c) {
        c->cid = uj->pos[0];

        c->ssx = uj->pos[1] >> 4;
        if (!(c->ssx)) {
            ujThrow(UJ_SYNTAX_ERROR);
        }

        if (c->ssx & (c->ssx - 1))
            ujThrow(UJ_UNSUPPORTED); // non-power of two

        c->ssy = uj->pos[1] & 15;
        if (!(c->ssy))
            ujThrow(UJ_SYNTAX_ERROR);

        if (c->ssy & (c->ssy - 1))
            ujThrow(UJ_UNSUPPORTED); // non-power of two

        c->qtsel = uj->pos[2];
        if ((c->qtsel) & 0xFC)
            ujThrow(UJ_SYNTAX_ERROR);

        ujSkip(uj, 3);
        uj->qtused |= 1 << c->qtsel;
        if (c->ssx > ssxmax)
            ssxmax = c->ssx;
        if (c->ssy > ssymax)
            ssymax = c->ssy;
    }
    if (uj->ncomp == 1) {
        c = uj->comp;
        c->ssx = c->ssy = ssxmax = ssymax = 1;
    }
    uj->mbsizex = ssxmax << 3;
    uj->mbsizey = ssymax << 3;
    uj->mbwidth = (uj->width + uj->mbsizex - 1) / uj->mbsizex;
    uj->mbheight = (uj->height + uj->mbsizey - 1) / uj->mbsizey;
    for (i = 0, c = uj->comp; i < uj->ncomp; ++i, ++c) {
        c->width = (uj->width * c->ssx + ssxmax - 1) / ssxmax;
        c->height = (uj->height * c->ssy + ssymax - 1) / ssymax;
        c->stride = uj->mbwidth * c->ssx << 3;
        if (((c->width < 3) && (c->ssx != ssxmax)) || ((c->height < 3) && (c->ssy != ssymax)))
            ujThrow(UJ_UNSUPPORTED);
        if (!uj->no_decode) {
            // The precise buffer size is calculated as follows, however, to standardize
            // for bif, we use the maximum assumed buffer size which is set prior to
            // this call and stored in c->pixels_length
            // Which should be width * height * << 1
            //size = c->stride * uj->mbheight * c->ssy << 3;
            // c->pixels_length = size;
            if (!uj->calc_mem_requirements) {
                memset(c->pixels, 0x80, c->pixels_length);
            }
        }
    }
    ujSkip(uj, uj->length);
}

static inline void ujDecodeDHT(ujContext * uj) {
    int codelen, currcnt, remain, spread, i, j;
    ujVLCCode * vlc;
    static unsigned char counts[16];
    ujDecodeLength(uj);
    ujCheckError();
    while (uj->length >= 17) {
        i = uj->pos[0];
        if (i & 0xEC)
            ujThrow(UJ_SYNTAX_ERROR);
        if (i & 0x02)
            ujThrow(UJ_UNSUPPORTED);
        i = (i | (i >> 3)) & 3; // combined DC/AC + tableid value
        for (codelen = 1; codelen <= 16; ++codelen)
            counts[codelen - 1] = uj->pos[codelen];
        ujSkip(uj, 17);
        vlc = &uj->vlctab[i][0];
        remain = spread = 65536;
        for (codelen = 1; codelen <= 16; ++codelen) {
            spread >>= 1;
            currcnt = counts[codelen - 1];
            if (!currcnt)
                continue;
            if (uj->length < currcnt)
                ujThrow(UJ_SYNTAX_ERROR);
            remain -= currcnt << (16 - codelen);
            if (remain < 0)
                ujThrow(UJ_SYNTAX_ERROR);
            for (i = 0; i < currcnt; ++i) {
                register unsigned char code = uj->pos[i];
                for (j = spread; j; --j) {
                    vlc->bits = (unsigned char)codelen;
                    vlc->code = code;
                    ++vlc;
                }
            }
            ujSkip(uj, currcnt);
        }
        while (remain--) {
            vlc->bits = 0;
            ++vlc;
        }
    }
    if (uj->length)
        ujThrow(UJ_SYNTAX_ERROR);
}

static inline void ujDecodeDQT(ujContext * uj) {
    int i;
    unsigned char * t;
    ujDecodeLength(uj);
    ujCheckError();
    while (uj->length >= 65) {
        i = uj->pos[0];
        if (i & 0xFC)
            ujThrow(UJ_SYNTAX_ERROR);
        uj->qtavail |= 1 << i;
        t = &uj->qtab[i][0];
        for (i = 0; i < 64; ++i)
            t[i] = uj->pos[i + 1];
        ujSkip(uj, 65);
    }
    if (uj->length)
        ujThrow(UJ_SYNTAX_ERROR);
}

static inline void ujDecodeDRI(ujContext * uj) {
    ujDecodeLength(uj);
    ujCheckError();
    if (uj->length < 2)
        ujThrow(UJ_SYNTAX_ERROR);
    uj->rstinterval = ujDecode16(uj->pos);
    ujSkip(uj, uj->length);
}

static int ujGetVLC(ujContext * uj, ujVLCCode * vlc, unsigned char * code) {
    int value = ujShowBits(uj, 16);
    int bits = vlc[value].bits;
    if (!bits) {
        ujError = UJ_SYNTAX_ERROR;
        return 0;
    }
    ujSkipBits(uj, bits);
    value = vlc[value].code;
    if (code)
        *code = (unsigned char)value;
    bits = value & 15;
    if (!bits)
        return 0;
    value = ujGetBits(uj, bits);
    if (value < (1 << (bits - 1))) {
        //value += ((-1) << bits) + 1;
        value += (-1 * (1 << bits)) + 1;
    }

    return value;
}

static inline void ujDecodeBlock(ujContext * uj, ujComponent * c, unsigned char * out) {
    unsigned char code = 0;
    int value, coef = 0;
    memset(uj->block, 0, sizeof(uj->block));
    c->dcpred += ujGetVLC(uj, &uj->vlctab[c->dctabsel][0], NULL);
    uj->block[0] = (c->dcpred) * uj->qtab[c->qtsel][0];
    do {
        value = ujGetVLC(uj, &uj->vlctab[c->actabsel][0], &code);
        if (!code)
            break; // EOB
        if (!(code & 0x0F) && (code != 0xF0))
            ujThrow(UJ_SYNTAX_ERROR);
        coef += (code >> 4) + 1;
        if (coef > 63)
            ujThrow(UJ_SYNTAX_ERROR);
        uj->block[(int)ujZZ[coef]] = value * uj->qtab[c->qtsel][coef];
    } while (coef < 63);
    for (coef = 0; coef < 64; coef += 8)
        ujRowIDCT(&uj->block[coef]);
    for (coef = 0; coef < 8; ++coef)
        ujColIDCT(&uj->block[coef], &out[coef], c->stride);
}

static inline void ujDecodeScan(ujContext * uj) {
    int i, mbx, mby, sbx, sby;
    int rstcount = uj->rstinterval, nextrst = 0;
    ujComponent * c;
    ujDecodeLength(uj);
    ujCheckError();
    if (uj->length < (4 + 2 * uj->ncomp))
        ujThrow(UJ_SYNTAX_ERROR);
    if (uj->pos[0] != uj->ncomp)
        ujThrow(UJ_UNSUPPORTED);
    ujSkip(uj, 1);
    for (i = 0, c = uj->comp; i < uj->ncomp; ++i, ++c) {
        if (uj->pos[0] != c->cid)
            ujThrow(UJ_SYNTAX_ERROR);
        if (uj->pos[1] & 0xEE)
            ujThrow(UJ_SYNTAX_ERROR);
        c->dctabsel = uj->pos[1] >> 4;
        c->actabsel = (uj->pos[1] & 1) | 2;
        ujSkip(uj, 2);
    }
    if (uj->pos[0] || (uj->pos[1] != 63) || uj->pos[2])
        ujThrow(UJ_UNSUPPORTED);
    ujSkip(uj, uj->length);
    uj->valid = 1;
    if (uj->no_decode) {
        ujError = __UJ_FINISHED;
        return;
    }
    uj->decoded = 1; // mark the image as decoded now -- every subsequent error
        // just means that the image hasn't been decoded
        // completely
    for (mbx = mby = 0;;) {
        for (i = 0, c = uj->comp; i < uj->ncomp; ++i, ++c)
            for (sby = 0; sby < c->ssy; ++sby)
                for (sbx = 0; sbx < c->ssx; ++sbx) {
                    ujDecodeBlock(uj, c, &c->pixels[((mby * c->ssy + sby) * c->stride + mbx * c->ssx + sbx) << 3]);
                    ujCheckError();
                }
        if (++mbx >= uj->mbwidth) {
            mbx = 0;
            if (++mby >= uj->mbheight)
                break;
        }
        if (uj->rstinterval && !(--rstcount)) {
            ujByteAlign(uj);
            i = ujGetBits(uj, 16);
            if (((i & 0xFFF8) != 0xFFD0) || ((i & 7) != nextrst))
                ujThrow(UJ_SYNTAX_ERROR);
            nextrst = (nextrst + 1) & 7;
            rstcount = uj->rstinterval;
            for (i = 0; i < 3; ++i)
                uj->comp[i].dcpred = 0;
        }
    }
    ujError = __UJ_FINISHED;
}

///////////////////////////////////////////////////////////////////////////////

#define CF4A (-9)
#define CF4B (111)
#define CF4C (29)
#define CF4D (-3)
#define CF3A (28)
#define CF3B (109)
#define CF3C (-9)
#define CF3X (104)
#define CF3Y (27)
#define CF3Z (-3)
#define CF2A (139)
#define CF2B (-11)
#define CF(x) ujClip(((x) + 64) >> 7)

static inline void ujUpsampleHCentered(ujComponent * c) {
    const int xmax = c->width - 3;
    unsigned char *lin, *lout;
    int x, y;

    lin = c->pixels;
    lout = c->pixels_buffer;
    for (y = c->height; y; --y) {
        lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
        lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
        lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
        for (x = 0; x < xmax; ++x) {
            lout[(x << 1) + 3] = CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
            lout[(x << 1) + 4] = CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
        }
        lin += c->stride;
        lout += c->width << 1;
        lout[-3] = CF(CF3A * lin[-1] + CF3B * lin[-2] + CF3C * lin[-3]);
        lout[-2] = CF(CF3X * lin[-1] + CF3Y * lin[-2] + CF3Z * lin[-3]);
        lout[-1] = CF(CF2A * lin[-1] + CF2B * lin[-2]);
    }
    c->width <<= 1;
    c->stride = c->width;

    memcpy(c->pixels, c->pixels_buffer, c->pixels_length);
}

static inline void ujUpsampleVCentered(ujComponent * c) {
    const int w = c->width, s1 = c->stride, s2 = s1 + s1;
    unsigned char *out, *cin, *cout;
    int x, y;

    out = c->pixels_buffer;

    for (x = 0; x < w; ++x) {
        cin = &c->pixels[x];
        cout = &out[x];
        *cout = CF(CF2A * cin[0] + CF2B * cin[s1]);
        cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[s1] + CF3Z * cin[s2]);
        cout += w;
        *cout = CF(CF3A * cin[0] + CF3B * cin[s1] + CF3C * cin[s2]);
        cout += w;
        cin += s1;
        for (y = c->height - 3; y; --y) {
            *cout = CF(CF4A * cin[-s1] + CF4B * cin[0] + CF4C * cin[s1] + CF4D * cin[s2]);
            cout += w;
            *cout = CF(CF4D * cin[-s1] + CF4C * cin[0] + CF4B * cin[s1] + CF4A * cin[s2]);
            cout += w;
            cin += s1;
        }
        cin += s1;
        *cout = CF(CF3A * cin[0] + CF3B * cin[-s1] + CF3C * cin[-s2]);
        cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[-s1] + CF3Z * cin[-s2]);
        cout += w;
        *cout = CF(CF2A * cin[0] + CF2B * cin[-s1]);
    }
    c->height <<= 1;
    c->stride = c->width;
    memcpy(c->pixels, c->pixels_buffer, c->pixels_length);
}

#define SF(x) ujClip(((x) + 8) >> 4)

static inline void ujUpsampleHCoSited(ujComponent * c) {
    const int xmax = c->width - 1;
    unsigned char *lin, *lout;
    int x, y;

    lin = c->pixels;
    lout = c->pixels_buffer;
    for (y = c->height; y; --y) {
        lout[0] = lin[0];
        lout[1] = SF((lin[0] << 3) + 9 * lin[1] - lin[2]);
        lout[2] = lin[1];
        for (x = 2; x < xmax; ++x) {
            lout[(x << 1) - 1] = SF(9 * (lin[x - 1] + lin[x]) - (lin[x - 2] + lin[x + 1]));
            lout[x << 1] = lin[x];
        }
        lin += c->stride;
        lout += c->width << 1;
        lout[-3] = SF((lin[-1] << 3) + 9 * lin[-2] - lin[-3]);
        lout[-2] = lin[-1];
        lout[-1] = SF(17 * lin[-1] - lin[-2]);
    }
    c->width <<= 1;
    c->stride = c->width;
    memcpy(c->pixels, c->pixels_buffer, c->pixels_length);
}

static inline void ujUpsampleVCoSited(ujComponent * c) {
    const int w = c->width, s1 = c->stride, s2 = s1 + s1;
    unsigned char *out, *cin, *cout;
    int x, y;

    out = c->pixels_buffer;
    for (x = 0; x < w; ++x) {
        cin = &c->pixels[x];
        cout = &out[x];
        *cout = cin[0];
        cout += w;
        *cout = SF((cin[0] << 3) + 9 * cin[s1] - cin[s2]);
        cout += w;
        *cout = cin[s1];
        cout += w;
        cin += s1;
        for (y = c->height - 3; y; --y) {
            *cout = SF(9 * (cin[0] + cin[s1]) - (cin[-s1] + cin[s2]));
            cout += w;
            *cout = cin[s1];
            cout += w;
            cin += s1;
        }
        *cout = SF((cin[s1] << 3) + 9 * cin[0] - cin[-s1]);
        cout += w;
        *cout = cin[-s1];
        cout += w;
        *cout = SF(17 * cin[s1] - cin[0]);
    }
    c->height <<= 1;
    c->stride = c->width;
    memcpy(c->pixels, c->pixels_buffer, c->pixels_length);
}

static inline void ujUpsampleFast(ujContext * uj, ujComponent * c) {
    int x, y, xshift = 0, yshift = 0;
    unsigned char *out, *lin, *lout;
    while (c->width < uj->width) {
        c->width <<= 1;
        ++xshift;
    }
    while (c->height < uj->height) {
        c->height <<= 1;
        ++yshift;
    }
    if (!xshift && !yshift)
        return;

    out = c->pixels_buffer;
    lin = c->pixels;
    lout = out;
    for (y = 0; y < c->height; ++y) {
        lin = &c->pixels[(y >> yshift) * c->stride];
        for (x = 0; x < c->width; ++x)
            lout[x] = lin[x >> xshift];
        lout += c->width;
    }
    c->stride = c->width;
    memcpy(c->pixels, c->pixels_buffer, c->pixels_length);
}

static inline void ujConvert(ujContext * uj, unsigned char * pout) {
    int i;
    ujComponent * c;
    for (i = 0, c = uj->comp; i < uj->ncomp; ++i, ++c) {
        if (uj->fast_chroma) {
            ujUpsampleFast(uj, c);
            ujCheckError();
        } else {
            while ((c->width < uj->width) || (c->height < uj->height)) {
                if (c->width < uj->width) {
                    if (uj->co_sited_chroma)
                        ujUpsampleHCoSited(c);
                    else
                        ujUpsampleHCentered(c);
                }
                ujCheckError();
                if (c->height < uj->height) {
                    if (uj->co_sited_chroma)
                        ujUpsampleVCoSited(c);
                    else
                        ujUpsampleVCentered(c);
                }
                ujCheckError();
            }
        }
        if ((c->width < uj->width) || (c->height < uj->height))
            ujThrow(UJ_INTERNAL_ERR);
    }
    if (uj->ncomp == 3) {
        // convert to RGB
        int x, yy;
        const unsigned char * py = uj->comp[0].pixels;
        const unsigned char * pcb = uj->comp[1].pixels;
        const unsigned char * pcr = uj->comp[2].pixels;
        for (yy = uj->height; yy; --yy) {
            for (x = 0; x < uj->width; ++x) {
                register int y = py[x] << 8;
                register int cb = pcb[x] - 128;
                register int cr = pcr[x] - 128;
                *pout++ = ujClip((y + 359 * cr + 128) >> 8);
                *pout++ = ujClip((y - 88 * cb - 183 * cr + 128) >> 8);
                *pout++ = ujClip((y + 454 * cb + 128) >> 8);
            }
            py += uj->comp[0].stride;
            pcb += uj->comp[1].stride;
            pcr += uj->comp[2].stride;
        }
    } else {
        // grayscale -> only remove stride
        unsigned char * pin = &uj->comp[0].pixels[uj->comp[0].stride];
        int y;
        for (y = uj->height - 1; y; --y) {
            memcpy(pout, pin, uj->width);
            pin += uj->comp[0].stride;
            pout += uj->width;
        }
    }
}

static void ujInit(ujContext * uj) {
    int save_no_decode = uj->no_decode;
    int save_fast_chroma = uj->fast_chroma;

    uj->no_decode = save_no_decode;
    uj->fast_chroma = save_fast_chroma;
}

///////////////////////////////////////////////////////////////////////////////

static unsigned short ujGetExif16(ujContext * uj, const unsigned char * p) {
    if (uj->exif_le)
        return p[0] + (p[1] << 8);
    else
        return (p[0] << 8) + p[1];
}

static int ujGetExif32(ujContext * uj, const unsigned char * p) {
    if (uj->exif_le)
        return p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);
    else
        return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

static void ujDecodeExif(ujContext * uj) {
    const unsigned char * ptr;
    int size, count, i;
    if (uj->no_decode || uj->fast_chroma) {
        ujSkipMarker(uj);
        return;
    }
    ujDecodeLength(uj);
    ujCheckError();
    ptr = uj->pos;
    size = uj->length;
    ujSkip(uj, uj->length);
    if (size < 18)
        return;
    if (!memcmp(ptr, "Exif\0\0II*\0", 10))
        uj->exif_le = 1;
    else if (!memcmp(ptr, "Exif\0\0MM\0*", 10))
        uj->exif_le = 0;
    else
        return; // invalid Exif header
    i = ujGetExif32(uj, ptr + 10) + 6;
    if ((i < 14) || (i > (size - 2)))
        return;
    ptr += i;
    size -= i;
    count = ujGetExif16(uj, ptr);
    i = (size - 2) / 12;
    if (count > i)
        return;
    ptr += 2;
    while (count--) {
        if ((ujGetExif16(uj, ptr) == 0x0213) // tag = YCbCrPositioning
            && (ujGetExif16(uj, ptr + 2) == 3) // type = SHORT
            && (ujGetExif32(uj, ptr + 4) == 1) // length = 1
        ) {
            uj->co_sited_chroma = (ujGetExif16(uj, ptr + 8) == 2);
            return;
        }
        ptr += 12;
    }
}

///////////////////////////////////////////////////////////////////////////////

static void ujDisableDecoding(ujImage img) {
    ujContext * uj = (ujContext *)img;
    if (uj) {
        uj->no_decode = 1;
        ujError = UJ_OK;
    } else
        ujError = UJ_NO_CONTEXT;
}

static void ujSetChromaMode(ujImage img, int mode) {
    ujContext * uj = (ujContext *)img;
    if (uj) {
        uj->fast_chroma = mode;
        ujError = UJ_OK;
    } else
        ujError = UJ_NO_CONTEXT;
}

static ujImage ujDecode(ujImage img, const void * jpeg, const int size) {
    ujContext * uj = (ujContext *)img;

    ujError = UJ_OK;

    uj->pos = (const unsigned char *)jpeg;
    uj->size = size & 0x7FFFFFFF;
    if (uj->size < 2) {
        ujError = UJ_NO_JPEG;
        return (ujImage)uj;
    }
    if ((uj->pos[0] ^ 0xFF) | (uj->pos[1] ^ 0xD8)) {
        ujError = UJ_NO_JPEG;
        return (ujImage)uj;
    }
    ujSkip(uj, 2);
    while (!ujError) {
        if ((uj->size < 2) || (uj->pos[0] != 0xFF)) {
            ujError = UJ_SYNTAX_ERROR;
            return (ujImage)uj;
        }
        ujSkip(uj, 2);
        switch (uj->pos[-1]) {
            case 0xC0:
                ujDecodeSOF(uj);
                break;
            case 0xC4:
                ujDecodeDHT(uj);

                break;
            case 0xDB:

                ujDecodeDQT(uj);

                break;
            case 0xDD:

                ujDecodeDRI(uj);

                break;
            case 0xDA:
                ujDecodeScan(uj);
                break;
            case 0xFE:

                ujSkipMarker(uj);

                break;
            case 0xE1:

                ujDecodeExif(uj);

                break;
            default:
                if ((uj->pos[-1] & 0xF0) == 0xE0)
                    ujSkipMarker(uj);
                else {
                    ujError = UJ_UNSUPPORTED;
                    return (ujImage)uj;
                }
        }
    }
    if (ujError == __UJ_FINISHED)
        ujError = UJ_OK;

    return (ujImage)uj;
}

static ujResult ujGetError(void) {
    return ujError;
}

static int ujIsValid(ujImage img) {
    ujContext * uj = (ujContext *)img;
    if (!uj) {
        ujError = UJ_NO_CONTEXT;
        return 0;
    }
    return uj->valid;
}

static int ujGetWidth(ujImage img) {
    ujContext * uj = (ujContext *)img;
    ujError = !uj ? UJ_NO_CONTEXT : (uj->valid ? UJ_OK : UJ_NOT_DECODED);
    return ujError ? 0 : uj->width;
}

static int ujGetHeight(ujImage img) {
    ujContext * uj = (ujContext *)img;
    ujError = !uj ? UJ_NO_CONTEXT : (uj->valid ? UJ_OK : UJ_NOT_DECODED);
    return ujError ? 0 : uj->height;
}

static int ujIsColor(ujImage img) {
    ujContext * uj = (ujContext *)img;
    ujError = !uj ? UJ_NO_CONTEXT : (uj->valid ? UJ_OK : UJ_NOT_DECODED);
    return ujError ? 0 : (uj->ncomp != 1);
}

static int ujGetImageSize(ujImage img) {
    ujContext * uj = (ujContext *)img;
    ujError = !uj ? UJ_NO_CONTEXT : (uj->valid ? UJ_OK : UJ_NOT_DECODED);
    return ujError ? 0 : (uj->width * uj->height * uj->ncomp);
}

static ujPlane * ujGetPlane(ujImage img, int num) {
    ujContext * uj = (ujContext *)img;
    ujError = !uj ? UJ_NO_CONTEXT : (uj->decoded ? UJ_OK : UJ_NOT_DECODED);
    if (!ujError && (num >= uj->ncomp))
        ujError = UJ_INVALID_ARG;
    return ujError ? NULL : ((ujPlane *)&uj->comp[num]);
}

// return the number of jpeg frames
static int imagelib_bif_get_num_frames(const const_mem_region_t bif_image) {
    ASSERT(bif_image.ptr);

    const struct bif_header_t * const bif_header = (struct bif_header_t *)(bif_image.byte_ptr);

    if (0 != memcmp(bif_header->magic_number, bif_magic_number, sizeof(bif_magic_number))
        || bif_header->version != bif_file_version || bif_header->num_images <= 0) {
        return 0;
    }

    return bif_header->num_images;
}

// JPEG functions
static bool imagelib_get_jpeg_size(const const_mem_region_t jpeg_region, int * const out_width, int * const out_height, int * const hmax, int * vmax, int * const ncomp) {
    // Check for valid JPEG image
    const uint8_t soi_header[] = {0xff, 0xd8, 0xff};
    const uint8_t block_indicator = jpeg_region.byte_ptr[ARRAY_SIZE(soi_header)];
    if ((memcmp(jpeg_region.byte_ptr, soi_header, ARRAY_SIZE(soi_header)) != 0) || (((block_indicator < 0xE0) || (block_indicator > 0xEF)) && (block_indicator != 0xFE))) {
        return false;
    }

    size_t ind = ARRAY_SIZE(soi_header) + 1; // Keeps track of the position within the file

    // Retrieve the block length of the first block since the first block will not contain the size of file
    unsigned short block_length = jpeg_region.byte_ptr[ind] * 256 + jpeg_region.byte_ptr[ind + 1];

    while (ind < jpeg_region.size) {
        // Increase the file index to get to the next block
        ind += block_length;

        // end loop if past the end of the data
        if (ind >= jpeg_region.size) {
            return false;
        }

        // Check that we are truly at the start of another block
        if (jpeg_region.byte_ptr[ind] != 0xFF) {
            return false;
        }

        if ((jpeg_region.byte_ptr[ind + 1] == 0xC0) || (jpeg_region.byte_ptr[ind + 1] == 0xC1) || (jpeg_region.byte_ptr[ind + 1] == 0xC2)) {
            // 0xFFC0 is the "Start of frame" marker which contains the file size
            // The structure of the 0xFFC0 block is [0xFFC0][ushort length][uchar precision][ushort x][ushort y]
            *out_height = (int)(jpeg_region.byte_ptr[ind + 5] * 256 + jpeg_region.byte_ptr[ind + 6]);
            *out_width = (int)(jpeg_region.byte_ptr[ind + 7] * 256 + jpeg_region.byte_ptr[ind + 8]);

            *hmax = 0;
            *vmax = 0;
            int h = 0;
            int v = 0;
            *ncomp = jpeg_region.byte_ptr[ind + 9];
            const size_t offset = ind + 10;
            for (int comp = 0; comp < *ncomp; comp++) {
                // <byte> number of components
                // <byte> component 1
                // <byte> sampling factor (left most = horiz, right most = vert)
                h = jpeg_region.byte_ptr[offset + (comp * 3) + 1] >> 4;
                v = jpeg_region.byte_ptr[offset + (comp * 3) + 1] & 15;

                if (h > *hmax) {
                    *hmax = h;
                }
                if (v > *vmax) {
                    *vmax = v;
                }
            }

            return true;
        } else {
            ind += 2; // Skip the block marker
            block_length = jpeg_region.byte_ptr[ind] * 256 + jpeg_region.byte_ptr[ind + 1]; // Go to the next block
        }
    }

    return false; // If this point is reached then no size was found
}

static int imagelib_bif_get_frame_size_and_offset(const const_mem_region_t bif_image, image_t * const header, const unsigned int frame_num, int * const out_hmax, int * const out_vmax, int * const out_ncomp) {
    // The last frame has timestamp = 0xFFFFFFFF and does not have data
    ASSERT(bif_image.byte_ptr);

    const bif_index_t * bif_index_ptr = (bif_index_t *)(bif_image.byte_ptr + sizeof(struct bif_header_t));
    const unsigned int framesize = bif_index_ptr[frame_num + 1].frame_offset - bif_index_ptr[frame_num].frame_offset;

    // Get width and height of image
    if (!imagelib_get_jpeg_size(CONST_MEM_REGION(.ptr = bif_image.byte_ptr + bif_index_ptr[frame_num].frame_offset, .size = framesize), &header->width, &header->height, out_hmax, out_vmax, out_ncomp)) {
        return -1;
    }

    return framesize;
}

static bool imagelib_calculate_buffer_size(
    const const_mem_region_t bif_file_data,
    image_t * const out_image,
    const int frame_num,
    size_t * const required_pixel_buffer_size,
    size_t * const required_working_buffer_size,
    size_t * const pixel_length) {
    int hmax; // max horiz sampling factor
    int vmax; // max vert  sampling factor
    int ncomp; // number of components

    // Get size data from first frame
    if (imagelib_bif_get_frame_size_and_offset(bif_file_data, out_image, frame_num, &hmax, &vmax, &ncomp) <= 0) {
        LOG_ERROR(IMAGELIB_BIF_FOURCC_TAG, "Unsupported file format");
        return false;
    }

    *pixel_length = out_image->width * out_image->height << 1;
    // One set of 3 buffers for the pixels and a second set for the pixel buffer
    *required_working_buffer_size = *pixel_length * ncomp * 2;

    *required_pixel_buffer_size = out_image->width * out_image->height * ncomp;

    return true;
}

bool imagelib_load_bif_jpg_frame_from_memory(const const_mem_region_t bif_file_data, image_t * const out_image, int frame_number, const mem_region_t pixel_region, const mem_region_t working_space_region) {
#if defined(_VADER) || defined(_LEIA)
    // lazy hack of getting our image channels, pitch, etc back to what a .jpg expects (we will be passing in things with channels = 4, and this need to be reverted inside this call)
    uint32_t ignored_frames;
    size_t ignored_working_space;
    size_t ignored_pixel_space;
    imagelib_read_bif_header_from_memory(bif_file_data, out_image, &ignored_frames, &ignored_pixel_space, &ignored_working_space);
#endif

    if (bif_file_data.size <= sizeof(struct bif_header_t)) {
        LOG_ERROR(IMAGELIB_BIF_FOURCC_TAG, "Invalid bif file data");
        return false;
    }

    const struct bif_header_t * const bif_header = (struct bif_header_t *)(bif_file_data.byte_ptr);

    if (0 != memcmp(bif_header->magic_number, bif_magic_number, sizeof(bif_magic_number))) {
        LOG_ERROR(IMAGELIB_BIF_FOURCC_TAG, "Header does not match BIF format");

        return false;
    }

    if (bif_header->version != bif_file_version) {
        LOG_ERROR(IMAGELIB_BIF_FOURCC_TAG, "Expected version %d, but found %d", bif_file_version, bif_header->version);

        return false;
    }

    ASSERT(bif_header->num_images > 0);
    ASSERT((frame_number >= 0) && (frame_number < (int)bif_header->num_images));

    size_t ignored_required_pixel_buffer_size = 0;
    size_t ignored_required_working_buffer_size = 0;

    size_t pixel_length;
    if (!imagelib_calculate_buffer_size(bif_file_data, out_image, frame_number, &ignored_required_pixel_buffer_size, &ignored_required_working_buffer_size, &pixel_length)) {
        return false;
    }

    ujContext context = {0};
    context.ncomp = out_image->bpp;
    context.comp[0].pixels = working_space_region.byte_ptr;
    context.comp[0].pixels_length = (int)pixel_length;
    context.offset_to_pixels_buffer = pixel_length;
    // Setup memory for pixel data
    for (int i = 1; i < context.ncomp; i++) {
        // Note - this size is potentially different for each comp.

        context.comp[i].pixels = context.comp[i - 1].pixels + context.comp[i - 1].pixels_length;
        context.comp[i].pixels_length = (int)pixel_length;
        context.offset_to_pixels_buffer += pixel_length;
    }

    // There are 2 buffers - one for the encoded pixel data per component and a companion scratch buffer.
    // The offset to the pixels buffer indicates how much memory the pixel data for  all 3 components
    // takes up.  Since we have a 2nd buffer of equal size to this, we multiply by 2 to get the
    // full buffer size and then sanity check against the required_working_buffer_size calculated.
    ASSERT((context.offset_to_pixels_buffer * 2) <= working_space_region.size);

    // setup memory for pixel_buffer data
    context.comp[0].pixels_buffer = working_space_region.byte_ptr + context.offset_to_pixels_buffer;

    for (int i = 1; i < context.ncomp; i++) {
        // The pixels_buffer will be the same size as the pixels data area
        context.comp[i].pixels_buffer = context.comp[i - 1].pixels_buffer + context.comp[i - 1].pixels_length;
    }

    const bif_index_t * const bif_index_ptr = (bif_index_t *)(bif_file_data.byte_ptr + sizeof(struct bif_header_t));
    const unsigned int framesize = bif_index_ptr[frame_number + 1].frame_offset - bif_index_ptr[frame_number].frame_offset;

    ujContext * const img = ujDecode(&context, (unsigned char *)bif_file_data.byte_ptr + bif_index_ptr[frame_number].frame_offset, framesize);

    ASSERT((unsigned int)(img->width * img->height * context.ncomp) <= pixel_region.size);

    ASSERT(img->width == out_image->width);
    ASSERT(img->height == out_image->height);

    ujConvert(img, (unsigned char *)pixel_region.byte_ptr);
    if (UJ_OK != ujError) {
        return false;
    }
    out_image->data = pixel_region.byte_ptr;

    return true;
}

bool imagelib_read_bif_header_from_memory(const const_mem_region_t bif_file_data, image_t * const out_image, unsigned int * const num_frames, size_t * const required_pixel_buffer_size, size_t * const required_working_buffer_size) {
    ASSERT(bif_file_data.ptr);
    ZEROMEM(out_image);

    const struct bif_header_t * const bif_header = (struct bif_header_t *)(bif_file_data.byte_ptr);

    if (0 != memcmp(bif_header->magic_number, bif_magic_number, sizeof(bif_magic_number))) {
        return false;
    }

    if (bif_header->version != bif_file_version) {
        return false;
    }

    ASSERT(bif_header->num_images > 0);

    // pixel_length not used here; Use the first frame for buffer calculation.
    size_t pixel_length;
    if (!imagelib_calculate_buffer_size(bif_file_data, out_image, 1, required_pixel_buffer_size, required_working_buffer_size, &pixel_length)) {
        return false;
    }

    out_image->x = 0;
    out_image->y = 0;
    out_image->depth = 1;
    out_image->bpp = 3;
    out_image->pitch = out_image->width * 3;
    out_image->data_len = out_image->spitch = out_image->width * out_image->height * 3;
    out_image->data = NULL;

    *num_frames = bif_header->num_images;

    return true;
}

bool imagelib_read_jpg_header_from_memory(
    const const_mem_region_t jpg_file_data,
    image_t * const out_image,
    size_t * const out_required_pixel_buffer_size,
    size_t * const out_required_working_space_size) {
    ZEROMEM(out_image);
    int32_t hmax_ignored, vmax_ignored;
    if (!imagelib_get_jpeg_size(jpg_file_data, &out_image->width, &out_image->height, &hmax_ignored, &vmax_ignored, &out_image->bpp)) {
        return false;
    }

    out_image->x = 0;
    out_image->y = 0;
    out_image->depth = 1;
    out_image->bpp = 3;
    out_image->pitch = out_image->width * 3;
    out_image->data_len = out_image->spitch = out_image->width * out_image->height * 3;
    out_image->data = NULL;

    *out_required_working_space_size = (out_image->width * out_image->height << 1) * out_image->bpp * 2;
    *out_required_pixel_buffer_size = out_image->width * out_image->height * out_image->bpp;
    return true;
}

bool imagelib_load_jpg_from_memory(
    const const_mem_region_t jpg_file_data,
    image_t * const out_image,
    const mem_region_t pixel_region,
    const mem_region_t working_space_region) {
    ujContext context = {0};

    const int pixel_length = (int)(out_image->width * out_image->height << 1);

    context.ncomp = out_image->bpp;
    context.comp[0].pixels = working_space_region.byte_ptr;
    context.offset_to_pixels_buffer = context.comp[0].pixels_length = (int)pixel_length;
    // Setup memory for pixel data
    for (int i = 1; i < context.ncomp; i++) {
        // Note - this size is potentially different for each comp.

        context.comp[i].pixels = context.comp[i - 1].pixels + context.comp[i - 1].pixels_length;
        context.comp[i].pixels_length = (int)pixel_length;
        context.offset_to_pixels_buffer += pixel_length;
    }

    // There are 2 buffers - one for the encoded pixel data per component and a companion scratch buffer.
    // The offset to the pixels buffer indicates how much memory the pixel data for  all 3 components
    // takes up.  Since we have a 2nd buffer of equal size to this, we multiply by 2 to get the
    // full buffer size and then sanity check against the required_working_buffer_size calculated.
    ASSERT((context.offset_to_pixels_buffer * 2) <= working_space_region.size);

    // setup memory for pixel_buffer data
    context.comp[0].pixels_buffer = working_space_region.byte_ptr + context.offset_to_pixels_buffer;

    for (int i = 1; i < context.ncomp; i++) {
        // The pixels_buffer will be the same size as the pixels data area
        context.comp[i].pixels_buffer = context.comp[i - 1].pixels_buffer + context.comp[i - 1].pixels_length;
    }

    ujContext * const img = ujDecode(&context, jpg_file_data.ptr, (int)jpg_file_data.size);

    ASSERT((unsigned int)(img->width * img->height * context.ncomp) <= pixel_region.size);

    ASSERT(img->width == out_image->width);
    ASSERT(img->height == out_image->height);

    ujConvert(img, (unsigned char *)pixel_region.byte_ptr);
    if (UJ_OK != ujError) {
        return false;
    }
    out_image->data = pixel_region.byte_ptr;

    return true;
}
