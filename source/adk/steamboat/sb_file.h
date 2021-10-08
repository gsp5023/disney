/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_file.h

steamboat file system
*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif
/// Constant integer value(s)
enum {
    /// Maximum path length (in bytes) including the directory and file name
    sb_max_path_length = 1024
};

/// Steamboat file handle
FFI_NAME(adk_file_handle_t)
FFI_DROP(adk_fclose)
typedef struct sb_file_t sb_file_t;

/// List of specialized directories for the application
FFI_EXPORT
FFI_NAME(adk_file_directory_e)
FFI_TYPE_MODULE(file)
FFI_FIELD_NAME(sb_app_root_directory, adk_app_root_directory)
FFI_FIELD_NAME(sb_app_config_directory, adk_app_config_directory)
FFI_PRIVATE(sb_app_cache_directory)
typedef enum sb_file_directory_e {
    /// The top-level (root) directory of the application (read-only)
    sb_app_root_directory,
    /// The directory for configuration files (writable)
    sb_app_config_directory,
    /// This directory is for caching internal information. This directory is not exposed through the ADK. (writable)
    sb_app_cache_directory
} sb_file_directory_e;

/// The mode of offset when setting (seeking) the position indicator of a file
FFI_EXPORT
FFI_NAME(adk_seek_mode_e)
FFI_TYPE_MODULE(file)
FFI_FIELD_NAME(sb_seek_set, adk_seek_set)
FFI_FIELD_NAME(sb_seek_end, adk_seek_end)
FFI_FIELD_NAME(sb_seek_cur, adk_seek_cur)
typedef enum sb_seek_mode_e {
    /// Seek offset relative to start of file
    sb_seek_set,
    /// Seek offset relative to end of file
    sb_seek_end,
    /// Seek offset relative to current position indicator
    sb_seek_cur
} sb_seek_mode_e;

/// Possible results of deleting a directory
FFI_EXPORT FFI_NAME(adk_directory_delete_error_e) FFI_TYPE_MODULE(file) FFI_ENUM_CLEAN_NAMES typedef enum sb_directory_delete_error_e {
    /// Success deletion
    sb_directory_delete_success = 0,
    /// An error occurred during deletion
    sb_directory_delete_error = -1,
    /// The specified directory to delete does not exist
    sb_directory_delete_does_not_exist = -2,
    /// The operation cannot be performed due to invalid input
    /// - attempting to delete root directory
    /// - relative sub-path (e.g. ../dir)
    sb_directory_delete_invalid_input = -3,
} sb_directory_delete_error_e;

/// Opens the file whose name is the string pointed to by `path` in `directory`
///
/// * `directory`: The directory in which the file is located
/// * `path`: The path within the directory to the file
/// * `mode`: Concatenated string of file access modes
///     * "r" = Opens an existing file for reading
///     * "w" = Created a file for writing
///     * "a" = Opens or creates a file for writing to to the end
///     * "r+" = Opens an existing file for both reading and writing
///     * "w+" = Creates an empty file for both reading and writing.
///     * "a+" = Opens or creates a file for reading and appending.
///
/// Returns a handle to the opened file
EXT_EXPORT sb_file_t * sb_fopen(const sb_file_directory_e directory, const char * const path, const char * const mode);

/// Closes `file`
///
/// * `file`: handle of file to be closed
///
/// Returns true if file was successfully closed
EXT_EXPORT bool sb_fclose(sb_file_t * const file);

/// Deletes the file specified by `filename` in `directory`
///
/// * `directory`: The directory in which the file is located
/// * `filename`: Path to the file to be deleted
///
/// Returns true if file was successfully deleted
EXT_EXPORT bool sb_delete_file(const sb_file_directory_e directory, const char * const filename);

/// Deletes the directory at the path `subpath`
///
/// In the case of a NULL `subpath`, the specified `directory` will be deleted (if that directory is not the root directory).
/// To delete only the content of a directory, delete the directory then re-create it.
///
/// * `directory`: The directory in which the file is located
/// * `subpath`: path to the directory to be deleted
///
/// Returns the results of the delete action
EXT_EXPORT sb_directory_delete_error_e sb_delete_directory(const sb_file_directory_e directory, const char * const subpath);

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
EXT_EXPORT bool sb_create_directory_path(const sb_file_directory_e directory, const char * const input_path);

typedef struct sb_directory_t sb_directory_t;

typedef struct sb_directory_entry_t sb_directory_entry_t;

/// Possible file entry types
typedef enum sb_directory_entry_type_e {
    /// This file entry type is not understood by the implementation
    sb_directory_entry_unknown = -2,
    /// Indicates that the entry is a nullptr/there are no more entries in the directory
    sb_directory_entry_null = -1,
    /// This entry is a directory
    sb_directory_entry_directory = 0,
    /// This entry is a file
    sb_directory_entry_file = 1,
    /// This entry is a sym link
    sb_directory_entry_sym_link = 2,
} sb_directory_entry_type_e;

/// Return type from sb_read_directory
typedef struct sb_read_directory_result_t {
    ///  contains the applicable entry if it exists (otherwise NULL)
    sb_directory_entry_t * entry;
    /// The type of file entry
    sb_directory_entry_type_e entry_type;
} sb_read_directory_result_t;

/// Opens a directory for walking the contents with subsequent sb_read_directory calls. May return NULL if the path is invalid, or opening fails.
EXT_EXPORT sb_directory_t * sb_open_directory(const sb_file_directory_e mount_point, const char * const subpath);

/// Closes the directory
EXT_EXPORT void sb_close_directory(sb_directory_t * const directory);

/// Read the next available directory entry (if no entries have been read, this returns the first entry in the directory)
EXT_EXPORT sb_read_directory_result_t sb_read_directory(sb_directory_t * const directory);

/// Gets the name of the specified directory entry
EXT_EXPORT const char * sb_get_directory_entry_name(const sb_directory_entry_t * const directory_entry);

/// Bit flags of the specific mode of a statable entry
FFI_EXPORT FFI_NAME(adk_file_mode_bits_e) FFI_TYPE_MODULE(file) FFI_ENUM_BITFLAGS
    FFI_ENUM_TRIM_START_NAMES(sb_file_mode_) typedef enum sb_file_mode_bits_e {
        sb_file_mode_none = 0x0,
        sb_file_mode_readable = 0x1,
        sb_file_mode_writable = 0x2,
        sb_file_mode_executable = 0x4,
        sb_file_mode_directory = 0x8,
        sb_file_mode_regular_file = 0x10,
        sb_file_mode_sym_link = 0x20,
        FORCE_ENUM_INT32(sb_file_mode_bits_e)
    } sb_file_mode_bits_e;

/// Return type of sb_stat
FFI_EXPORT FFI_NAME(adk_stat_t) FFI_TYPE_MODULE(file) typedef struct sb_stat_t {
    /// The entries file mode bits. If the entry does not exist this must be `sb_file_mode_none`
    sb_file_mode_bits_e mode;

    /// Access time since unix epoch in seconds
    uint64_t access_time_s;
    /// Modification time since unix epoch in seconds
    uint64_t modification_time_s;
    /// Create time since unix epoch in seconds
    uint64_t create_time_s;

    /// Num unix style hard links
    uint32_t hard_links;
    /// The size of the entry (0 typically for a directory, otherwise file size)
    uint64_t size;
    /// Owning user id if available
    uint32_t user_id;
    /// Owning group id if available
    uint32_t group_id;
} sb_stat_t;

/// Error states of sb_stat call
FFI_EXPORT FFI_NAME(adk_stat_error_e) FFI_TYPE_MODULE(file) FFI_ENUM_TRIM_START_NAMES(sb_stat_) typedef enum sb_stat_error_e {
    sb_stat_error_unknown = -2,
    sb_stat_error_no_entry = -1,
    sb_stat_success = 0,
    FORCE_ENUM_INT32(sb_stat_error_e)
} sb_stat_error_e;

/// Return type from sb_stat
FFI_EXPORT FFI_NAME(adk_stat_result_t) FFI_TYPE_MODULE(file) typedef struct sb_stat_result_t {
    /// The stat object
    sb_stat_t stat;
    /// Error status
    sb_stat_error_e error;
} sb_stat_result_t;

/// Performs a unix-like stat call returning the converted results and the error status
EXT_EXPORT sb_stat_result_t sb_stat(const sb_file_directory_e mount_point, const char * const subpath);

/// Obtains and returns the current value of the file position indicator of `file`
///
/// * `file`: File whose position will be returned
///
/// Returns the value position indicator
EXT_EXPORT long sb_ftell(sb_file_t * const file);

/// Sets the file position indicator of `file` to an `offset` from the specified `origin`
///
/// * `file`: File whose position will be returned
/// * `offset`: The number of bytes from the origin at which to set the position indicator
/// * `origin`: The position from whence the offset is measured
///
/// Returns true if successful
EXT_EXPORT bool sb_fseek(sb_file_t * const file, const long offset, const sb_seek_mode_e origin);

/// Reads `elem_count` elements, each of size `elem_size` bytes, from `file` into `buffer`
///
/// * `buffer`: Memory block in which to store read data
/// * `elem_size`: The size (in bytes) of each element to be read.
/// * `elem_count`: The number of elements to read
/// * `file`: The file from which to read
///
/// Returns the number or elements successfully read
EXT_EXPORT size_t sb_fread(void * const buffer, const size_t elem_size, const size_t elem_count, sb_file_t * const file);

/// Writes `elem_count` elements, each of size `elem_size` bytes, to `file` from `buffer`
///
/// * `buffer`: data to be written
/// * `elem_size`: The size (in bytes) of each element to be written.
/// * `elem_count`: The number of elements to written
/// * `file`: The file into which the data is written
///
/// Returns the number of elements successfully written
EXT_EXPORT size_t sb_fwrite(const void * const buffer, const size_t elem_size, const size_t elem_count, sb_file_t * const file);

/// Returns *true* if the end-of-file indicator associated with `file` is set (*false* otherwise)
EXT_EXPORT bool sb_feof(sb_file_t * const file);

/// Changes the name of the file at the `current_path` to `new_path`, returning true if the operation was successful and false otherwise.
EXT_EXPORT bool sb_rename(const sb_file_directory_e mount_point, const char * const current_path, const char * const new_path);

#ifdef __cplusplus
}
#endif
