/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/bundle/bundle.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Mounts bundle on 'sb_app_root_directory', returns false if a bundle is already mounted
///
/// * `bundle`: Bundle to be mounted
EXT_EXPORT bool adk_mount_bundle(bundle_t * const bundle);

/// Returns true if given bundle is currently mounted
///
/// * `bundle`: Bundle to be queried, can be NULL
bool adk_is_mounted_bundle(bundle_t * const bundle);

/// Unmounts current bundle from 'sb_app_root_directory'
///
/// * `bundle`: Bundle to be unmounted, must be the currently mounted bundle
///
/// Returns true if the bundle was unmounted, false if bundle files are open
EXT_EXPORT bool adk_unmount_bundle(bundle_t * const bundle);

/// Opens the file whose name is the string pointed to by `path` in `directory`
///
/// * `directory`: The directory in which the file is located
/// * `path`: The path within the directory to the file
/// * `mode`: Concatenated string of file access modes plus required end-of-line translation mode
///     * "r" = Opens an existing file for reading
///     * "w" = Created a file for writing
///     * "a" = Opens or creates a file for writing to to the end
///     * "r+" = Opens an existing file for both reading and writing
///     * "w+" = Creates an empty file for both reading and writing
///     * "a+" = Opens or creates a file for reading and appending
///     * "b" = Binary mode, no end-of-line translation
///     * "t" = Text mode, end-of-line translation if needed
///
/// Returns a handle to the opened file
EXT_EXPORT
FFI_EXPORT
FFI_PTR_NATIVE FFI_CAN_BE_NULL sb_file_t * adk_fopen(
    const sb_file_directory_e directory,
    FFI_PTR_WASM const char * const path,
    FFI_PTR_WASM const char * const mode);

/// Closes `file`
///
/// * `file`: handle of file to be closed
///
/// Returns true if file was successfully closed
EXT_EXPORT FFI_EXPORT bool adk_fclose(FFI_PTR_NATIVE sb_file_t * const file);

/// Deletes the file specified by `filename` in `directory`
///
/// * `directory`: The directory in which the file is located
/// * `filename`: Path to the file to be deleted
///
/// Returns true if file was successfully deleted
FFI_EXPORT bool adk_delete_file(const sb_file_directory_e directory, FFI_PTR_WASM const char * const filename);

/// Deletes the directory at the path `subpath`
///
/// In the case of a NULL `subpath`, the specified `directory` will be deleted (if that directory is not the root directory).
/// To delete only the content of a directory, delete the directory then re-create it.
///
/// * `directory`: The directory in which the file is located
/// * `subpath`: path to the directory to be deleted
///
/// Returns the results of the delete action
FFI_EXPORT
sb_directory_delete_error_e adk_delete_directory(const sb_file_directory_e directory, FFI_PTR_WASM const char * const subpath);

/// Creates a directory at some path `input_path`
///
/// * `directory`: The directory in which the directory is to be created
/// * `input_path`:  Path to the directory to be created
///
/// Returns true if the directory now exists, false if the directory is impossible to create
///
/// # Notes:
/// * a trailing folder deliminator (e.g. "some_path/") is expected. "some_path/path" is interpreted as a path to a file named "path".
/// * `true` shall be returned in the case where the directory already exists
FFI_EXPORT bool adk_create_directory_path(
    const sb_file_directory_e directory,
    FFI_PTR_WASM const char * const input_path);

/// Performs a unix-like stat call returning the converted results and the error status
EXT_EXPORT FFI_EXPORT sb_stat_result_t adk_stat(const sb_file_directory_e mount_point, FFI_PTR_WASM const char * const subpath);

/// Reads `elem_count` elements, each of size `elem_size` bytes, from `file` into `buffer`
///
/// * `buffer`: Memory block in which to store read data
/// * `elem_size`: The size (in bytes) of each element to be read.
/// * `elem_count`: The number of elements to read
/// * `file`: The file from which to read
///
/// Returns the number or elements successfully read
EXT_EXPORT
FFI_EXPORT
FFI_TYPE_OVERRIDE(int32_t)
size_t adk_fread(
    FFI_PTR_WASM FFI_SLICE void * const buffer,
    FFI_ELEMENT_SIZE_OF(buffer) FFI_TYPE_OVERRIDE(int32_t) const size_t elem_size,
    FFI_TYPE_OVERRIDE(int32_t) const size_t elem_count,
    FFI_PTR_NATIVE sb_file_t * const file);

/// Returns *true* if the end-of-file indicator associated with `file` is set (*false* otherwise)
FFI_EXPORT bool adk_feof(FFI_PTR_NATIVE sb_file_t * const file);

/// Resulting error status of `sb_fwrite` operation
FFI_EXPORT FFI_TYPE_MODULE(file)
    FFI_ENUM_TRIM_START_NAMES(adk_fwrite_) typedef enum adk_fwrite_error_e {
        /// Element size is impossible to write due to current rate-limiting of the device.
        adk_fwrite_error_element_too_large_for_rate_limit = -3,
        /// File is read-only
        adk_fwrite_error_read_only = -2,
        /// Write operation failed due to rate-limiting of device
        adk_fwrite_error_rate_limited = -1,
        /// Operation was successful (no errors)
        adk_fwrite_success = 0,
        FORCE_ENUM_INT32(adk_fwrite_error_e)
    } adk_fwrite_error_e;

/// Result of an `adk_fwrite` operation (containing error status)
FFI_EXPORT FFI_TYPE_MODULE(file) typedef struct adk_fwrite_result_t {
    /// Number of elements written to file _not_ the number of bytes (bytes is calculated based on elements_written*element_size)
    uint32_t elements_written;
    /// Status of associated `adk_fwrite` operation
    adk_fwrite_error_e error;
} adk_fwrite_result_t;

/// Writes `elem_count` elements, each of size `elem_size` bytes, to `file` from `buffer`
///
/// * `buffer`: data to be written
/// * `elem_size`: The size (in bytes) of each element to be written.
/// * `elem_count`: The number of elements to written
/// * `file`: The file into which the data is written
///
/// Returns `adk_fwrite_result_t` based on result
FFI_EXPORT
adk_fwrite_result_t adk_fwrite(
    FFI_PTR_WASM FFI_SLICE const void * const buffer,
    FFI_ELEMENT_SIZE_OF(buffer) FFI_TYPE_OVERRIDE(int32_t) const size_t elem_size,
    FFI_TYPE_OVERRIDE(int32_t) const size_t elem_count,
    FFI_PTR_NATIVE sb_file_t * const file);

// specify a non zero value to enable rate limiting, specify zero to disable rate limiting
void adk_file_set_write_limit(const int32_t persistent_storage_max_write_bytes_per_second);

void adk_file_write_limit_drain(const int32_t bytes_to_drain);

// Helper utility functions

int get_file_size(sb_file_t * const file);
int get_artifact_size(const sb_file_directory_e directory, const char * const filename);
bool load_artifact_data(
    const sb_file_directory_e directory,
    const mem_region_t buffer,
    const char * const filename,
    const int ofs);

#ifdef __cplusplus
}
#endif
