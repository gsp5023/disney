/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
imagelib.h

support for decoding various images and animations
*/

#pragma once

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum image_encoding_e {
    image_encoding_uncompressed,
    image_encoding_dxt1,
    image_encoding_dxt5,
    image_encoding_etc1,
    image_encoding_gnf1,
} image_encoding_e;

typedef struct image_t {
    image_encoding_e encoding;
    int x, y;
    int width, height, depth;
    int bpp;
    int pitch;
    int spitch;
    int data_len;
    void * data;
} image_t;

enum { image_max_mipmaps = 14 };

typedef struct image_mips_t {
    image_t levels[image_max_mipmaps];
    int first_level;
    int num_levels;
} image_mips_t;

typedef enum imagelib_gif_restart_mode_e {
    imagelib_gif_force_restart = 2,
    imagelib_gif_continue = 3,
} imagelib_gif_restart_mode_e;

// imagelib calls that take a `pixel_region` and/or `working_space_region` expect buffers of at least size `requied_pixel_buffer_size` and `required_working_space_size` for the respective regions
// `working_space_region` is a buffer needed for any internal operations and size is calculated as the absolute high water mark
// `pixel_region` is a buffer of sufficient size to store the final image texels into. by convention anything that has this region and returns pixel data will return the first byte to this region.

bool imagelib_read_png_header_from_memory(const const_mem_region_t png_file_data, image_t * const out_image, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size);
bool imagelib_load_png_from_memory(const const_mem_region_t png_file_data, image_t * const out_image, const mem_region_t pixel_region, const mem_region_t working_space_region);

bool imagelib_read_tga_header_from_memory(const const_mem_region_t tga_file_data, image_t * const out_image, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size);
bool imagelib_load_tga_from_memory(const const_mem_region_t tga_file_data, image_t * const out_image, const mem_region_t pixel_region, const mem_region_t working_space_region);

bool imagelib_read_gif_header_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size);
bool imagelib_gif_load_first_frame_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, milliseconds_t * const out_image_delay_in_ms, const mem_region_t pixel_region, const mem_region_t working_space_region);
bool imagelib_gif_load_next_frame_from_memory(const const_mem_region_t gif_file_data, image_t * const out_image, milliseconds_t * const out_image_delay_in_ms, const mem_region_t pixel_region, const mem_region_t working_space_region, const imagelib_gif_restart_mode_e restart_mode);

bool imagelib_load_pvr_from_memory(const const_mem_region_t pvr_file_data, image_t * const out_image);
bool imagelib_load_gnf_from_memory(const const_mem_region_t gnf_file_data, image_t * const out_image);

// BIF and JPEG
bool imagelib_read_bif_header_from_memory(const const_mem_region_t bif_file_data, image_t * const out_image, unsigned int * const num_frames, size_t * const required_pixel_buffer_size, size_t * const required_working_buffer_size);
bool imagelib_load_bif_jpg_frame_from_memory(const const_mem_region_t bif_file_data, image_t * const out_image, int frame_number, const mem_region_t pixel_region, const mem_region_t working_space_region);

bool imagelib_read_jpg_header_from_memory(
    const const_mem_region_t jpg_file_data,
    image_t * const out_image,
    size_t * const out_required_pixel_buffer_size,
    size_t * const out_required_working_space_size);

bool imagelib_load_jpg_from_memory(
    const const_mem_region_t jpg_file_data,
    image_t * const out_image,
    const mem_region_t pixel_region,
    const mem_region_t working_space_region);

#ifdef __cplusplus
}
#endif
