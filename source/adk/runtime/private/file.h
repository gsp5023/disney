/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
private/file.h

private file system api
*/
#pragma once

#include "source/adk/steamboat/sb_file.h"

#ifdef __cplusplus
extern "C" {
#endif

void adk_set_directory_path(const sb_file_directory_e directory, const char * const dir);
EXT_EXPORT const char * adk_get_file_directory_path(const sb_file_directory_e directory);
bool adk_is_valid_sub_path(const char * const input_path);

// Copies sb_fopen() mode string, removing non-posix 't' char
bool adk_posix_mode(char * const posix_mode, size_t const posix_mode_size, const char * const mode);

#ifdef __cplusplus
}
#endif
