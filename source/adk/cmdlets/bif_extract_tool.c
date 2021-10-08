/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/runtime/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    cmdlet_bif_success = 0,
    cmdlet_bif_incorrect_args = 1,
    cmdlet_bif_failure = 2
};

// needs to be packed
typedef PACK(struct bif_header_t {
    uint8_t magic_number[8]; // .BIF
    uint32_t version; // bytes 8,9,10,11
    uint32_t num_images; // bytes 12,13,14,15
    // Real timestamp is the multiplier x timestamp entry in BIF index
    // If 0 this is zero, then use 1000.
    uint32_t timestamp_multiplier; // bytes 16,17,18,19
    uint8_t reserved[44]; // reserved
}) bif_header_t;

typedef PACK(struct bif_index_t {
    uint32_t timestamp;
    uint32_t frame_offset;
}) bif_index_t;

STATIC_ASSERT(sizeof(bif_header_t) == 64);
STATIC_ASSERT(sizeof(bif_index_t) == 8);

bool write_jpg_file(FILE * out_file, image_t * image_header);
bool extract_single_bif_image(const const_mem_region_t bif_image_region, image_t * image_header, const char * out_filename, int frame_number);

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static FILE * unwhiny_fopen(const char * const path, const char * const mode) {
    FILE * fp = NULL;
    if (fopen_s(&fp, path, mode)) {
        return NULL;
    }

    return fp;
}

#define fopen unwhiny_fopen
#endif

int cmdlet_bif_extract_tool(const int argc, const char * const * const argv) {
    if (argc < 4) {
        debug_write_line("Usage:\n  bif_extract [frame-number || * ] [input-file] [output-file-prefix]");
        return cmdlet_bif_incorrect_args;
    }

    int frame_number = 0;

    if (strcasecmp(argv[1], "*") == 0) {
        // Allow extraction of all images
        frame_number = -1;
    } else {
        frame_number = atoi(argv[1]);
    }

    FILE * in_file;
    in_file = fopen(argv[2], "rb");
    if (in_file == NULL) {
        printf("Failed to open input file");
        return cmdlet_bif_failure;
    }
    ASSERT_MSG(in_file != NULL, "Failed to open input file");

    fseek(in_file, 0L, SEEK_END);
    unsigned long in_file_size = ftell(in_file);
    rewind(in_file);

    const mem_region_t bif_image_region = {{{{.ptr = malloc(in_file_size)}, .size = in_file_size}}};
    TRAP_OUT_OF_MEMORY(bif_image_region.ptr);

    const size_t bytes_read = fread(bif_image_region.byte_ptr, sizeof(char), in_file_size, in_file);
    if (bytes_read != in_file_size) {
        free(bif_image_region.byte_ptr);
        fclose(in_file);
        printf("Error reading file into memory");
        return cmdlet_bif_failure;
    }

    // Open output file
    image_t image_header;
    unsigned int num_frames = 0;

    size_t pixel_buff_size;
    size_t working_buff_size;

    // Get the bif header and number of frames
    if (!imagelib_read_bif_header_from_memory(
            bif_image_region.consted,
            &image_header,
            &num_frames,
            &pixel_buff_size,
            &working_buff_size)) {
        return cmdlet_bif_failure;
    }

    if ((frame_number < 0) && (num_frames > 0)) {
        for (int i = 0; i < (int)num_frames; i++) {
            extract_single_bif_image(bif_image_region.consted, &image_header, argv[3], i);
        }
    } else if ((frame_number >= 0) && (frame_number < (int)num_frames)) {
        extract_single_bif_image(bif_image_region.consted, &image_header, argv[3], frame_number);
    } else {
        free(bif_image_region.ptr);
        fclose(in_file);
        return cmdlet_bif_failure;
    }

    // done with input file
    fclose(in_file);
    free(bif_image_region.ptr);

    return cmdlet_bif_success;
}

bool extract_single_bif_image(const const_mem_region_t bif_image_region, image_t * image_header, const char * out_filename, int frame_number) {
    size_t required_pixel_buffer_size = 0;
    size_t required_working_buffer_size = 0;
    unsigned int num_frames;

    // Get the bif header and number of frames
    if (!imagelib_read_bif_header_from_memory(
            bif_image_region,
            image_header,
            &num_frames,
            &required_pixel_buffer_size,
            &required_working_buffer_size)) {
        return cmdlet_bif_failure;
    }

    const bif_index_t * bif_index_ptr = (bif_index_t *)(bif_image_region.byte_ptr + sizeof(struct bif_header_t));
    const unsigned int framesize = bif_index_ptr[frame_number + 1].frame_offset - bif_index_ptr[frame_number].frame_offset;

    image_header->data = (unsigned char *)bif_image_region.byte_ptr + bif_index_ptr[frame_number].frame_offset;
    image_header->data_len = framesize;

    // jpg data
    char indexed_filename[256] = {'\0'};
    char frame_str[5];
    snprintf(frame_str, ARRAY_SIZE(frame_str), "%d", frame_number);

    VERIFY(0 == strcpy_s(indexed_filename, ARRAY_SIZE(indexed_filename), out_filename));
    VERIFY(0 == strcat_s(indexed_filename, ARRAY_SIZE(indexed_filename), frame_str));
    VERIFY(0 == strcat_s(indexed_filename, ARRAY_SIZE(indexed_filename), ".jpg"));

    FILE * const out_file = fopen(indexed_filename, "wb+");
    if (out_file == NULL) {
        printf("Failed to open output file");
        return false;
    }

    write_jpg_file(out_file, image_header);
    fclose(out_file);

    return cmdlet_bif_success;
}

bool write_jpg_file(FILE * out_file, image_t * image_header) {
    const size_t bytes_written = fwrite(image_header->data, 1, image_header->data_len, out_file);
    if ((int)bytes_written != image_header->data_len) {
        printf("Failed to write image data");
        return false;
    }
    return true;
}
