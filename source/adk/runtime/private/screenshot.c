/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
screenshot.c

support for capturing, comparing, saving, and loading screenshots
*/

#include _PCH

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(x) VERIFY(x)
#include "source/adk/runtime/screenshot.h"

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/log/log.h"
#include "source/adk/renderer/renderer.h"
#include "stb/stb_image_write.h"

#define SCREENSHOT_TAG FOURCC('S', 'C', 'R', 'N')

typedef struct stb_image_write_mem_user_t {
    mem_region_t region;
    size_t offset;
} stb_image_write_mem_user_t;

static void stb_image_write_mem(void * void_user, void * data, int size) {
    stb_image_write_mem_user_t * const user = void_user;
    VERIFY(user->offset + (size_t)size <= user->region.size);
    memcpy(user->region.byte_ptr + user->offset, data, size);
    user->offset += (size_t)size;
}

static size_t image_save_as_type_to_region(const image_t * const src, const mem_region_t region, const image_save_file_type_e file_type) {
    stb_image_write_mem_user_t write_mem_user = {.region = region, .offset = 0};

    switch (file_type) {
        case image_save_tga:
            stbi_write_tga_to_func(stb_image_write_mem, &write_mem_user, src->width, src->height, src->bpp, src->data);
            break;
        case image_save_png:
            // passing zero for the stride will get stb to calculate the appropriate stride.
            stbi_write_png_to_func(stb_image_write_mem, &write_mem_user, src->width, src->height, src->bpp, src->data, 0);
            break;
        default:
            TRAP("Unimplemented mem region save type [%i]", file_type);
    }
    return write_mem_user.offset;
}

static void stb_image_write_func(void * file_pointer, void * data, int size) {
    sb_fwrite(data, size, 1, (sb_file_t *)file_pointer);
}

static bool image_save_as_type(const sb_file_directory_e directory, const char * const path, const image_t * const src, const image_save_file_type_e file_type) {
    ASSERT(src->encoding == image_encoding_uncompressed);
    sb_file_t * const fp = sb_fopen(directory, path, "wb");
    ASSERT_MSG(fp, "Could not open file at:\n%s\nnote: directory: [%i]", path, directory);
    bool status = false;
    switch (file_type) {
        case image_save_tga:
            status = stbi_write_tga_to_func(stb_image_write_func, fp, src->width, src->height, src->bpp, src->data) != 0;
            break;
        case image_save_png:
            // passing zero for the stride will get stb to calculate the appropriate stride.
            status = stbi_write_png_to_func(stb_image_write_func, fp, src->width, src->height, src->bpp, src->data, 0);
            break;
        default:
            TRAP("Unimplemented file save type [%i]", file_type);
    }

    sb_fclose(fp);
    return status;
}

size_t adk_get_screenshot_required_memory() {
    sb_enumerate_display_modes_result_t display_mode_result = {0};
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);
    return (size_t)(display_mode_result.display_mode.width * display_mode_result.display_mode.height * 4);
}

void adk_take_screenshot(image_t * const out_screenshot, const mem_region_t screenshot_mem_region) {
    ASSERT(screenshot_mem_region.size >= (size_t)adk_get_screenshot_required_memory());

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_screenshot,
        out_screenshot,
        screenshot_mem_region,
        MALLOC_TAG);
}

void adk_take_screenshot_flush(image_t * const out_screenshot, const mem_region_t screenshot_mem_region) {
    adk_take_screenshot(out_screenshot, screenshot_mem_region);

    render_flush_cmd_stream(&the_app.render_device->default_cmd_stream, render_wait);
}

size_t adk_write_screenshot_mem_user_by_type(
    const image_t * const screenshot,
    const mem_region_t region,
    const image_save_file_type_e file_type) {
    const microseconds_t start = adk_read_microsecond_clock();
    const size_t bytes_written = image_save_as_type_to_region(screenshot, region, file_type);
    const microseconds_t end = adk_read_microsecond_clock();
    LOG_ALWAYS(SCREENSHOT_TAG, "Compress screenshot into memory took: [%f]ms bytes: [%i] bytes after compression: [%i] dims: {width: %i, height: %i, bpp: %i}", (double)(end.us - start.us) / 1000.0, screenshot->data_len, bytes_written, screenshot->width, screenshot->height, screenshot->bpp);

    return bytes_written;
}

void adk_save_screenshot(
    const image_t * const screenshot,
    const image_save_file_type_e file_type,
    const sb_file_directory_e directory,
    const char * const filename) {
    image_save_as_type(directory, filename, screenshot, file_type);
}

void adk_load_screenshot(
    image_t * const out_screenshot,
    const const_mem_region_t screenshot_file_region,
    const mem_region_t screenshot_pixel_region,
    const mem_region_t screenshot_working_space_region) {
    ZEROMEM(out_screenshot);

    size_t required_pixel_buffer_size, required_working_space_size;

    if (imagelib_read_tga_header_from_memory(screenshot_file_region, out_screenshot, &required_pixel_buffer_size, &required_working_space_size)) {
        VERIFY((screenshot_pixel_region.size >= required_pixel_buffer_size) && (screenshot_working_space_region.size >= required_working_space_size));
        VERIFY(imagelib_load_tga_from_memory(screenshot_file_region, out_screenshot, screenshot_pixel_region, screenshot_working_space_region));
    } else if (imagelib_read_png_header_from_memory(screenshot_file_region, out_screenshot, &required_pixel_buffer_size, &required_working_space_size)) {
        VERIFY((screenshot_pixel_region.size >= required_pixel_buffer_size) && (screenshot_working_space_region.size >= required_working_space_size));
        VERIFY(imagelib_load_png_from_memory(screenshot_file_region, out_screenshot, screenshot_pixel_region, screenshot_working_space_region));
    }
}

void adk_get_screenshot_file_required_memory(
    const const_mem_region_t screenshot_file_region,
    image_t * const screenshot,
    size_t * const out_required_pixel_buffer_size,
    size_t * const out_required_working_space_size) {
    if (imagelib_read_png_header_from_memory(
            screenshot_file_region,
            screenshot,
            out_required_pixel_buffer_size,
            out_required_working_space_size)
        || imagelib_read_tga_header_from_memory(
            screenshot_file_region,
            screenshot,
            out_required_pixel_buffer_size,
            out_required_working_space_size)) {
        return;
    }
    *out_required_working_space_size = *out_required_pixel_buffer_size = 0;
}

static void * ffi_screenshot_alloc(const size_t size) {
    return malloc(size);
}

static void ffi_screenshot_free(void * const ptr) {
    free(ptr);
}

adk_screenshot_t * ffi_capture_screenshot() {
    const microseconds_t start = adk_read_microsecond_clock();
    const size_t screenshot_required_mem = adk_get_screenshot_required_memory();

    adk_screenshot_t * const screenshot = ffi_screenshot_alloc(screenshot_required_mem + sizeof(adk_screenshot_t));
    ZEROMEM(screenshot);
    screenshot->region = MEM_REGION(.ptr = screenshot + 1, .size = screenshot_required_mem);

    adk_take_screenshot_flush(&screenshot->image, screenshot->region);

    const microseconds_t end = adk_read_microsecond_clock();
    LOG_ALWAYS(SCREENSHOT_TAG, "Capturing screenshot took: [%f]ms bytes: [%i] dims: {width: %i, height: %i, bpp: %i}", (double)(end.us - start.us) / 1000.0, screenshot->image.data_len, screenshot->image.width, screenshot->image.height, screenshot->image.bpp);

    return screenshot;
}

void ffi_release_screenshot(adk_screenshot_t * const screenshot) {
    ffi_screenshot_free(screenshot);
}

void ffi_save_screenshot(const adk_screenshot_t * const screenshot, const image_save_file_type_e file_type, const sb_file_directory_e directory, const char * const filename) {
    const microseconds_t start = adk_read_microsecond_clock();
    adk_save_screenshot(&screenshot->image, file_type, directory, filename);
    const microseconds_t end = adk_read_microsecond_clock();
    LOG_ALWAYS(SCREENSHOT_TAG, "Saving screenshot took: [%f]ms bytes: [%i] dims: {width: %i, height: %i, bpp: %i}", (double)(end.us - start.us) / 1000.0, screenshot->image.data_len, screenshot->image.width, screenshot->image.height, screenshot->image.bpp);
}

adk_screenshot_t * ffi_load_screenshot(const sb_file_directory_e directory, const char * const filename) {
    const microseconds_t start = adk_read_microsecond_clock();
    sb_file_t * const screenshot_file = sb_fopen(directory, filename, "rb");
    VERIFY(screenshot_file);
    sb_fseek(screenshot_file, 0, sb_seek_end);
    const size_t screenshot_file_size = sb_ftell(screenshot_file);
    sb_fseek(screenshot_file, 0, sb_seek_set);

    const const_mem_region_t screenshot_bytes = CONST_MEM_REGION(.ptr = ffi_screenshot_alloc(screenshot_file_size), .size = screenshot_file_size);
    sb_fread((void *)screenshot_bytes.ptr, 1, screenshot_bytes.size, screenshot_file);
    sb_fclose(screenshot_file);

    image_t screenshot_image;
    size_t required_pixel_buffer, required_working_space_buffer;
    adk_get_screenshot_file_required_memory(screenshot_bytes, &screenshot_image, &required_pixel_buffer, &required_working_space_buffer);

    adk_screenshot_t * const screenshot = ffi_screenshot_alloc(required_pixel_buffer + sizeof(adk_screenshot_t));
    screenshot->region = MEM_REGION(.ptr = (screenshot + 1), .size = required_pixel_buffer);

    const mem_region_t working_space_region = MEM_REGION(.ptr = ffi_screenshot_alloc(required_working_space_buffer), .size = required_working_space_buffer);

    adk_load_screenshot(&screenshot->image, screenshot_bytes, screenshot->region, working_space_region);

    ffi_screenshot_free((void *)screenshot_bytes.ptr);
    ffi_screenshot_free(working_space_region.ptr);

    const microseconds_t end = adk_read_microsecond_clock();
    LOG_ALWAYS(SCREENSHOT_TAG, "Loading screenshot took: [%f]ms bytes: [%i] dims: {width: %i, height: %i, bpp: %i}", (double)(end.us - start.us) / 1000.0, screenshot->image.data_len, screenshot->image.width, screenshot->image.height, screenshot->image.bpp);

    return screenshot;
}

bool adk_screenshot_compare(const adk_screenshot_t * const testcase_screenshot, const adk_screenshot_t * const baseline_screenshot, const int image_tolerance) {
    const image_t * const testcase = &testcase_screenshot->image;
    const image_t * const baseline = &baseline_screenshot->image;
    VERIFY((testcase->width == baseline->width) && (testcase->height == baseline->height) && (testcase->depth == baseline->depth) && (testcase->bpp == baseline->bpp));

    for (int y = 0; y < testcase->height; ++y) {
        for (int x = 0; x < testcase->width; ++x) {
            for (int channel = 0; channel < testcase->bpp; ++channel) {
                const int ind = channel + (x * testcase->bpp) + (y * testcase->width * testcase->bpp);
                const int base_texel = ((uint8_t *)baseline->data)[ind];
                const int test_texel = ((uint8_t *)testcase->data)[ind];

                if (abs(base_texel - test_texel) > image_tolerance) {
                    LOG_WARN(SCREENSHOT_TAG, "Image tolerance exceeded when comparing test case and baseline\ndelta: %i\n x: %i, y: %i, channel: %i", abs(base_texel - test_texel), x, y, channel);

                    return false;
                }
            }
        }
    }
    return true;
}

void adk_screenshot_dump_deltas(adk_screenshot_t * const testcase_screenshot, adk_screenshot_t * const baseline_screenshot, const int image_tolerance, const image_save_file_type_e file_type, const sb_file_directory_e directory, const char * const filename_prefix) {
    const image_t * const testcase = &testcase_screenshot->image;
    const image_t * const baseline = &baseline_screenshot->image;
    static const char * const image_extensions[] = {[image_save_tga] = ".tga", [image_save_png] = ".png"};
    static const uint8_t threshold_fail_color[] = {0xff, 0xff, 0xff, 0xff};

    bool texel_breaches_threshold = false;
    for (int y = 0; y < testcase->height; ++y) {
        for (int x = 0; x < testcase->width; ++x) {
            texel_breaches_threshold = false;
            for (int channel = 0; channel < testcase->bpp; ++channel) {
                const int index = channel + (x * testcase->bpp) + (y * testcase->width * testcase->bpp);
                const int base_texel = ((uint8_t *)baseline->data)[index];
                const int test_texel = ((uint8_t *)testcase->data)[index];

                const uint8_t delta = (uint8_t)abs(base_texel - test_texel);
                ((uint8_t *)testcase->data)[index] = delta;
                ((uint8_t *)baseline->data)[index] = delta;

                if (delta > image_tolerance) {
                    texel_breaches_threshold = true;
                }
            }

            if (texel_breaches_threshold) {
                const int index = (x + y * testcase->width) * testcase->bpp;
                for (int channel = 0; channel < testcase->bpp; ++channel) {
                    ((uint8_t *)testcase->data)[index + channel] = threshold_fail_color[channel];
                }
            }
        }
    }

    char failure_filename[1024];
    char delta_filename[1024];

    sprintf_s(failure_filename, ARRAY_SIZE(failure_filename), "%s_failing_pixels%s", filename_prefix, image_extensions[file_type]);

    sprintf_s(delta_filename, ARRAY_SIZE(delta_filename), "%s_deltas%s", filename_prefix, image_extensions[file_type]);

    image_save_as_type(directory, failure_filename, testcase, file_type);
    image_save_as_type(directory, delta_filename, baseline, file_type);
}
