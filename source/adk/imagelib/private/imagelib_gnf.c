/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "imagelib_gnf.h"

#include "source/adk/imagelib/imagelib.h"

int gnf_read_register(const uint32_t gnf_register, const int mask, const int shift) {
    return (((gnf_register) & (mask)) >> (shift));
}

bool imagelib_load_gnf_from_memory(const const_mem_region_t gnf_file_data, image_t * const out_image) {
    ZEROMEM(out_image);

    if (gnf_file_data.size < sizeof(gnf_header_t) + sizeof(gnf_tex_desc_t) + gnf_constant_table_size) {
        return false;
    }

    const gnf_header_t * header = (const gnf_header_t *)(gnf_file_data.byte_ptr);
    const gnf_tex_desc_t * t0 = (const gnf_tex_desc_t *)(gnf_file_data.byte_ptr + sizeof(gnf_header_t) + gnf_constant_table_size);

    if (header->magic != FOURCC('G', 'N', 'F', ' ')) {
        return false;
    }

    out_image->encoding = image_encoding_gnf1;
    out_image->data = (void *)gnf_file_data.ptr;
    out_image->data_len = (int)gnf_file_data.size;

    //SCE code adds one to all of these dimensions.  It is reflected in their API, but no explanation why
    out_image->width = gnf_read_register(t0->regs[2], gnf_width_mask, gnf_width_shift) + 1;
    out_image->height = gnf_read_register(t0->regs[2], gnf_height_mask, gnf_height_shift) + 1;
    out_image->depth = gnf_read_register(t0->regs[4], gnf_depth_mask, gnf_depth_shift) + 1;
    out_image->pitch = gnf_read_register(t0->regs[4], gnf_pitch_mask, gnf_pitch_shift) + 1;

    return true;
}
