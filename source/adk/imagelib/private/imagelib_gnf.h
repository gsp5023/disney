/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
imagelib_gnf.h

Local definition of SCE structs for handling GNF both in and out of cpp.
Documentation for the GNF format can be found here
https://ps4.siedev.net/resources/documents/SDK/8.000/TextureTool-Overview/0002.html

*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

//the following mask and shift constants come from <ORBIS>/include_common/gnm/regsinfo_public.h
enum {
    gnf_chan_x_mask = 0x00000007L,
    gnf_chan_x_shift = 0,
    gnf_chan_y_mask = 0x00000038L,
    gnf_chan_y_shift = 3,
    gnf_chan_z_mask = 0x000001c0L,
    gnf_chan_z_shift = 6,
    gnf_chan_w_mask = 0x00000e00L,
    gnf_chan_w_shift = 9,
    gnf_constant_table_size = 8,
    gnf_data_fmt_mask = 0x03f00000L,
    gnf_data_fmt_shift = 20,
    gnf_depth_mask = 0x00001fffL,
    gnf_depth_shift = 0,
    gnf_height_mask = 0x0fffc000L,
    gnf_height_shift = 14,
    gnf_num_fmt_mask = 0x3c000000L,
    gnf_num_fmt_shift = 26,
    gnf_pitch_mask = 0x07ffe000L,
    gnf_pitch_shift = 13,
    gnf_tile_mask = 0x01f00000L,
    gnf_tile_shift = 20,
    gnf_type_mask = 0xf0000000L,
    gnf_type_shift = 28,
    gnf_width_mask = 0x00003fffL,
    gnf_width_shift = 0

}; //The GNF Content Table is 4 *  + 1 * 32bit

typedef struct gnf_tex_desc_t {
    uint32_t regs[8];
} gnf_tex_desc_t;

typedef struct gnf_header_t {
    uint32_t magic; //0x20464E47 ("GNF " in ASCII)
    uint32_t content_size; //size of content table for T# 0 - T# n
} gnf_header_t;

#ifdef __cplusplus
}
#endif
