/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_file_stdio.c

steamboat stdio file system backend
*/
#include _PCH

#include "source/adk/log/log.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/sb_file.h"

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// shut up windows
static FILE * unwhiny_fopen(const char * const path, const char * const mode) {
    FILE * fp = NULL;
    if (fopen_s(&fp, path, mode)) {
        return NULL;
    }

    return fp;
}

#define fopen unwhiny_fopen
#endif

#ifdef _WIN32
#include <direct.h>
#else
#include <errno.h>
#include <sys/stat.h>
typedef enum {
    // linux read/write perms. assumption is only our user is meant to work on the files we need.
    default_file_perms = S_IRWXU,
    default_root_perms = S_IRWXU,
    default_config_perms = S_IRWXU,
    default_download_perms = S_IRWXU
} default_file_perms_e;

static int get_file_perms(const sb_file_directory_e directory) {
    switch (directory) {
        case sb_app_config_directory: {
            return default_config_perms;
        }
        case sb_app_cache_directory: {
            return default_download_perms;
        }
        default: {
            ASSERT(directory == sb_app_root_directory);
            return default_root_perms;
        }
    }
}
#endif

#define TAG_SBIO FOURCC('S', 'B', 'I', 'O')

static void combine_directory_and_file_path(const sb_file_directory_e directory, const char * const filename, char file_path[sb_max_path_length]) {
    ASSERT(adk_is_valid_sub_path(filename));

    const char * const root_directory = adk_get_file_directory_path(directory);
    ASSERT(root_directory);
    ASSERT_MSG((strlen(root_directory) + strlen(filename)) < (sb_max_path_length - 1), "Root directory and filename/path too long >= max_path_length");

    sprintf_s(file_path, sb_max_path_length, "%s/%s", root_directory, filename);
}

bool sb_create_directory_path(const sb_file_directory_e directory, const char * const input_path) {
    LOG_DEBUG(TAG_SBIO, "%s %s %d", __FILE__, __func__, __LINE__);
    if ((directory == sb_app_root_directory)) {
        LOG_DEBUG(TAG_SBIO, "%s %s %d", __FILE__, __func__, __LINE__);
        return false;
    }
    // this function will traverse a re-created filepath to either where you want to store a file.
    // eg.. C:/some_path/with arbitrary_chars/data.dat
    // or the above minus the last file. either or.
    // to do so we peel off characters after each folder level and re-assign them into our buffer
    // thus we don't need more than 1 buffer to work on.. it just needs to be mutable.
    char path[sb_max_path_length];

    // can't assert due to unit tests needing to tell if this fails properly.....

#ifndef _SHIP
LOG_DEBUG(TAG_SBIO, "%s %s %d", __FILE__, __func__, __LINE__);
    VERIFY(adk_is_valid_sub_path(input_path));
    LOG_DEBUG(TAG_SBIO, "%s %s %d", __FILE__, __func__, __LINE__);
#else
    if (!adk_is_valid_sub_path(input_path)) {
         LOG_DEBUG(TAG_SBIO, "%s %s %d", __FILE__, __func__, __LINE__);
        return false;
    }
#endif

    combine_directory_and_file_path(directory, input_path, path);

    int node_count = 1;
    for (char * curr_ptr = path + 1; (curr_ptr = strchr(curr_ptr, '/')) != NULL; ++curr_ptr, ++node_count) {
        // verify for windows like systems that we skip trying to create a drive letter folder.
        // e.g. c:/some_path/other/more
        // we can't make "c:/" so skip it out right.
        // also skip the immediate first slash on linux incase it's causing issues
        const ptrdiff_t head_dist = curr_ptr - path;
        if ((head_dist == 2) && (curr_ptr[-1] == ':')) {
            continue;
        }

        const char curr_char = curr_ptr[1];
        curr_ptr[1] = '\0';

#ifdef _WIN32
        if ((_mkdir(path) != 0) && (errno != EEXIST)) {
            return false;
        }
#else
        if ((mkdir(path, get_file_perms(directory)) != 0) && (errno != EEXIST)) {
            LOG_ERROR(TAG_SBIO, "Can not make directory %s", path);
            return false;
        }
#endif
        curr_ptr[1] = curr_char;
    }
    LOG_DEBUG(TAG_SBIO, "%s %s %d path %s", __FILE__, __func__, __LINE__, path);
    return true;
}

sb_file_t * sb_fopen(const sb_file_directory_e directory, const char * const filename, const char * const mode) {
    if ((directory == sb_app_root_directory) && (strpbrk(mode, "aw+") != NULL)) {
        return NULL; // attempt to open writable file on root directory
    }
    if (strpbrk(mode, "bt") == NULL) {
        return NULL; // missing required eol translation mode
    }

#ifndef _SHIP
    VERIFY_MSG(adk_is_valid_sub_path(filename), "invalid sub path [%s]", filename);
#else
    if (!adk_is_valid_sub_path(filename)) {
        return NULL;
    }
#endif

    char file_path[sb_max_path_length];

    combine_directory_and_file_path(directory, filename, file_path);

#ifdef _WIN32
    return (sb_file_t *)fopen(file_path, mode);
#else
    char posix_mode[8] = {0}; // larger than needed but let fopen() handle extra/invalid mode chars
    if (!adk_posix_mode(posix_mode, ARRAY_SIZE(posix_mode), mode)) {
        return NULL; // truncated mode chars
    }
    return (sb_file_t *)fopen(file_path, posix_mode);
#endif
}

bool sb_fclose(sb_file_t * const file) {
    if (file == NULL) {
        return false;
    }
    return fclose((FILE *)file) == 0;
}

bool sb_delete_file(const sb_file_directory_e directory, const char * const filename) {
    if ((directory == sb_app_root_directory)) {
        return false;
    }
#ifndef _SHIP
    VERIFY(adk_is_valid_sub_path(filename));
#else
    if (!adk_is_valid_sub_path(filename)) {
        return false;
    }
#endif

    char file_path[sb_max_path_length];

    combine_directory_and_file_path(directory, filename, file_path);

    return remove(file_path) == 0;
}

long sb_ftell(sb_file_t * const file) {
    return ftell((FILE *)file);
}

bool sb_fseek(sb_file_t * const file, const long offset, const sb_seek_mode_e origin) {
    int stdio_origin = 0;
    switch (origin) {
        case sb_seek_cur:
            stdio_origin = SEEK_CUR;
            break;
        case sb_seek_set:
            stdio_origin = SEEK_SET;
            break;
        case sb_seek_end:
            stdio_origin = SEEK_END;
            break;
        default:
            TRAP("Invalid adk_seek_mode_e");
    }
    return fseek((FILE *)file, offset, stdio_origin) == 0;
}

size_t sb_fread(void * buffer, const size_t elem_size, const size_t elem_count, sb_file_t * const file) {
    return fread(buffer, elem_size, elem_count, (FILE *)file);
}

size_t sb_fwrite(void const * const buffer, const size_t elem_size, const size_t elem_count, sb_file_t * const file) {
    return fwrite(buffer, elem_size, elem_count, (FILE *)file);
}

bool sb_feof(sb_file_t * const file) {
    return feof((FILE *)file) != 0;
}

bool sb_rename(const sb_file_directory_e mount_point, const char * const current_path, const char * const new_path) {
    char prefixed_current_path[sb_max_path_length];
    sprintf_s(
        prefixed_current_path,
        ARRAY_SIZE(prefixed_current_path),
        "%s/%s",
        adk_get_file_directory_path(mount_point),
        current_path);

    char prefixed_new_path[sb_max_path_length];
    sprintf_s(
        prefixed_new_path,
        ARRAY_SIZE(prefixed_new_path),
        "%s/%s",
        adk_get_file_directory_path(mount_point),
        new_path);

    return rename(prefixed_current_path, prefixed_new_path) == 0;
}
