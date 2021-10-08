/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
file_common.c

file system routines common to all platforms
*/

#include _PCH

#include "file.h"

#include <stdio.h>
#include <string.h>

static struct
{
    char root_dir_path[sb_max_path_length];
    char config_dir_path[sb_max_path_length];
    char cache_dir_path[sb_max_path_length];
} statics;

static char * get_mutable_file_directory(const sb_file_directory_e directory) {
    switch (directory) {
        case sb_app_config_directory: {
            return statics.config_dir_path;
        }
        case sb_app_cache_directory: {
            return statics.cache_dir_path;
        }
        default: {
            ASSERT(directory == sb_app_root_directory);
            return statics.root_dir_path;
        }
    }
}

bool adk_is_valid_sub_path(const char * const input_path) {
    if ((*input_path == '/') || (strstr(input_path, "..") != NULL)) {
        return false;
    }

    static const char invalid_chars[] = "\\:<>;\"|?*";
    const char * curr = input_path;

    while (*curr != 0) {
        // windows truncates preciding white space on folders, so we can't actually take them.
        if ((*curr == '/') && (*(curr + 1) == ' ')) {
            return false;
        }

        if ((*curr < ' ') || (*curr > '~')) {
            return false;
        }

        const char cb[] = {*curr, '\0'};
        if (strstr(invalid_chars, cb)) {
            return false;
        }

        ++curr;
    }
    return ((curr - input_path) < sb_max_path_length);
}

const char * adk_get_file_directory_path(const sb_file_directory_e directory) {
    return get_mutable_file_directory(directory);
}

void adk_set_directory_path(const sb_file_directory_e directory, const char * const dir) {
    ASSERT(strlen(dir) < sb_max_path_length);
    ASSERT(dir && *dir);

    strcpy_s(get_mutable_file_directory(directory), sb_max_path_length, dir);
}

// adk/sb_fopen() require either 'b' or 't', but 't' is non-posix (its the default), so remove it to be safe
bool adk_posix_mode(char * const posix_mode, size_t const posix_mode_size, const char * const mode) {
    int i, o;
    const int end = (int)posix_mode_size - 1;
    for (i = 0, o = 0; (o < end) && mode[i]; ++i) {
        if (mode[i] != 't') {
            posix_mode[o++] = mode[i];
        }
    }
    posix_mode[o] = '\0';
    return mode[i] == '\0';
}
