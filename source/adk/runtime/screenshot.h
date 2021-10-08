/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
screenshot.h

support for capturing, comparing, saving, and loading screenshots
*/

#pragma once

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT
FFI_TYPE_MODULE(image)
FFI_ENUM_TRIM_START_NAMES(image_save_)
typedef enum image_save_file_type_e {
    image_save_png = 0,
    image_save_tga = 1
} image_save_file_type_e;

EXT_EXPORT size_t adk_get_screenshot_required_memory();

EXT_EXPORT void adk_take_screenshot_flush(image_t * const out_screenshot, const mem_region_t screenshot_mem_region);
void adk_take_screenshot(image_t * const out_screenshot, const mem_region_t screenshot_mem_region);

void adk_save_screenshot(const image_t * const screenshot, const image_save_file_type_e file_type, const sb_file_directory_e directory, const char * const filename);

void adk_get_screenshot_file_required_memory(const const_mem_region_t screenshot_file_region, image_t * const screenshot, size_t * const out_required_pixel_buffer_size, size_t * const out_required_working_space_size);
void adk_load_screenshot(image_t * const out_screenshot, const const_mem_region_t screenshot_file_region, const mem_region_t screenshot_pixel_region, const mem_region_t screenshot_working_space_region);

EXT_EXPORT size_t adk_write_screenshot_mem_user_by_type(const image_t * const screenshot, const mem_region_t region, const image_save_file_type_e file_type);

FFI_NAME(adk_screenshot_t)
FFI_DROP(ffi_release_screenshot)
typedef struct adk_screenshot_t {
    image_t image;
    mem_region_t region;
} adk_screenshot_t;

FFI_EXPORT FFI_NAME(adk_capture_screenshot) FFI_PTR_NATIVE adk_screenshot_t * ffi_capture_screenshot();
FFI_EXPORT FFI_NAME(adk_release_screenshot) void ffi_release_screenshot(FFI_PTR_NATIVE adk_screenshot_t * const screenshot);
FFI_EXPORT FFI_NAME(adk_save_screenshot) void ffi_save_screenshot(FFI_PTR_NATIVE const adk_screenshot_t * const screenshot, const image_save_file_type_e file_type, const sb_file_directory_e directory, FFI_PTR_WASM const char * const filename);
FFI_EXPORT FFI_NAME(adk_load_screenshot) FFI_PTR_NATIVE adk_screenshot_t * ffi_load_screenshot(const sb_file_directory_e directory, FFI_PTR_WASM const char * const filename);

FFI_EXPORT bool adk_screenshot_compare(FFI_PTR_NATIVE const adk_screenshot_t * const testcase_screenshot, FFI_PTR_NATIVE const adk_screenshot_t * const baseline_screenshot, const int32_t image_tolerance);
FFI_EXPORT void adk_screenshot_dump_deltas(FFI_PTR_NATIVE adk_screenshot_t * const testcase_screenshot, FFI_PTR_NATIVE adk_screenshot_t * const baseline_screenshot, const int32_t image_tolerance, const image_save_file_type_e file_type, const sb_file_directory_e directory, FFI_PTR_WASM const char * const filename_prefix);

#ifdef __cplusplus
}
#endif
