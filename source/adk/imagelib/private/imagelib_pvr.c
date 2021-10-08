/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "memory.h"
#include "source/adk/imagelib/imagelib.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"

typedef PACK(struct pvr_header_t {
    uint32_t version;
    uint32_t flag;
    uint64_t pixel_format;
    uint32_t color_space;
    uint32_t channel_type;
    uint32_t height;
    uint32_t width;
    uint32_t depth;
    uint32_t num_surfaces;
    uint32_t num_faces;
    uint32_t mipmap_count;
    uint32_t metadata_size;
}) pvr_header_t;

STATIC_ASSERT(sizeof(pvr_header_t) == 52);

typedef enum pvr_pixel_format_code_e {
    pvr_pixel_format_etc = 6
} pvr_pixel_format_code_e;

static inline uint32_t swap_uint32(const uint32_t n) {
    return ((n >> 24) & 0xff) | ((n >> 8) & 0xff00) | ((n << 8) & 0xff0000) | ((n << 24) & 0xff000000);
}

static inline uint64_t swap_uint64(const uint64_t n) {
    return ((n >> 56) & 0xff) | ((n >> 40) & 0xff00) | ((n >> 24) & 0xff0000) | ((n >> 8) & 0xff000000) | ((n << 8) & 0xff00000000) | ((n << 24) & 0xff0000000000) | ((n << 40) & 0xff000000000000) | ((n << 56) & 0xff00000000000000);
}

// Swap the endianess if the PVR has an endian mismatch
static void conditional_byteswap_pvr_header(pvr_header_t * const header) {
    if (header->version == FOURCC_SWAPPED('P', 'V', 'R', 3)) {
        header->version = swap_uint32(header->version);
        header->flag = swap_uint32(header->flag);
        header->pixel_format = swap_uint64(header->pixel_format);
        header->color_space = swap_uint32(header->color_space);
        header->channel_type = swap_uint32(header->channel_type);
        header->height = swap_uint32(header->height);
        header->width = swap_uint32(header->width);
        header->depth = swap_uint32(header->depth);
        header->num_surfaces = swap_uint32(header->num_surfaces);
        header->num_faces = swap_uint32(header->num_faces);
        header->mipmap_count = swap_uint32(header->mipmap_count);
        header->metadata_size = swap_uint32(header->metadata_size);
    }
}

/* This function extracts an ETC1 image from a PVR file.  This is done by 
 * reading metadata from the 52-byte header of the PVR format.  This header 
 * is defined by http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf 
 * as follows:
 * [4-Bytes] Version - Identifies the format as PVR and shows endian match
 * [4-Bytes] Flags
 * [8-Bytes] Pixel Format
 * [4-Bytes] Color Space
 * [4-Bytes] Channel Type
 * [4-Bytes] Height
 * [4-Bytes] Width
 * [4-Bytes] Depth
 * [4-Bytes] Number of Surfaces
 * [4-Bytes] Number of Faces
 * [4-Bytes] MIP-Map Count
 * [4-Bytes] Metadata Size
 * [?-Bytes] Metadata of a side defined above
 * [?-Bytes] Payload (Image data) 
 *
 * All numerical constants in this function are based on this format.
 */
bool imagelib_load_pvr_from_memory(const const_mem_region_t pvr_file_data, image_t * const out_image) {
    ZEROMEM(out_image);

    if (pvr_file_data.size < sizeof(pvr_header_t)) {
        return false;
    }

    pvr_header_t header = *(const pvr_header_t *)pvr_file_data.ptr;
    conditional_byteswap_pvr_header(&header);

    if (header.version != FOURCC('P', 'V', 'R', 3)) {
        return false;
    }

    if (header.pixel_format != pvr_pixel_format_etc) {
        return false;
    }

    // etc1 must have a depth of 1 or it is an error
    if (header.depth != 1) {
        return false;
    }

    out_image->encoding = image_encoding_etc1;
    out_image->height = header.height;
    out_image->width = header.width;
    out_image->depth = header.depth;

    // NOTE: bpp, pitch, and spitch do not  apply to ETC1
    const uint32_t bytes_per_block = 8;
    const uint32_t header_size = sizeof(pvr_header_t) + header.metadata_size;

    out_image->data_len = ((header.width + 3) / 4) * ((header.height + 3) / 4) * bytes_per_block;
    out_image->data = (uint8_t *)pvr_file_data.adr + header_size;

    return true;
}
