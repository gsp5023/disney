/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "extern/libzip/lib/zip.h"
#include "source/adk/steamboat/sb_file.h"

// Creates a new read-only zipfile source on the named file
zip_source_t * bundle_zip_source_new(const sb_file_directory_e directory, const char * const fname, zip_error_t * const error);

// Creates a new read-only zipfile source on an open file
zip_source_t * bundle_zip_source_new_from_fp(sb_file_t * const file, size_t initial_offset, zip_error_t * const error);
