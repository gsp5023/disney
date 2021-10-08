/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 bundle.h

 bundle file system for accessing compressed application bundles
 */

#pragma once

#include "source/adk/steamboat/sb_file.h"

enum {
    max_open_bundle_files = 256 // affects bundle fs heap size
};

/// Handle to an open bundle
struct bundle_t;
typedef struct bundle_t bundle_t;

/// Handle to a directory within a bundle
struct bundle_directory_t;
typedef struct bundle_directory_t bundle_directory_t;

/// Handle to a file within a bundle
struct bundle_file_t;
typedef struct bundle_file_t bundle_file_t;

/// Initializes bundle library, must be called before any other bundle APIs
void bundle_init();

/// Shuts down bundle library, bundle APIs must not be called after this
void bundle_shutdown();

/// Opens a bundle for reading
EXT_EXPORT bundle_t * bundle_open(const sb_file_directory_e directory, const char * const path);

/// Opens a bundle on an existing file for reading, passing ownership of file to bundle
bundle_t * bundle_open_fp(sb_file_t * const file, const size_t initial_offset);

/// Closes the bundle handle. Returns false if the bundle is currently mounted.
EXT_EXPORT bool bundle_close(bundle_t * const bundle);

/// Opens the file in 'bundle' whose name is the string pointed to by `subpath`
///
/// * `subpath`: The path within the bundle to the file
///
/// Returns a handle to the opened file
bundle_file_t * bundle_fopen(bundle_t * const bundle, const char * const subpath);

/// Closes `file` within bundle
///
/// * `file`: handle of file to be closed
///
/// Returns true if file was successfully closed
bool bundle_fclose(bundle_file_t * const file);

/// Performs a unix-like stat call for a file within a bundle, returning the converted results and the error status
sb_stat_result_t bundle_stat(bundle_t * const bundle, const char * const subpath);

/// Reads `elem_count` elements, each of size `elem_size` bytes, from `file` into `buffer`
///
/// * `buffer`: Memory block in which to store read data
/// * `elem_size`: The size (in bytes) of each element to be read
/// * `elem_count`: The number of elements to read
/// * `file`: The file from which to read
///
/// Returns the number of elements successfully read
size_t bundle_fread(void * const buffer, const size_t elem_size, const size_t elem_count, bundle_file_t * const file);

/// Returns *true* if the end-of-file indicator associated with `file` is set (*false* otherwise)
bool bundle_feof(bundle_file_t * const file);
