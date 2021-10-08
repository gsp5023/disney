/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 sb_file_posix.c

 sb_file posix support
 */

#include _PCH

#include "source/adk/log/log.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/steamboat/sb_file.h"

// needed for nftw
#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif

#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef _DIRENT_HAVE_D_TYPE
#error "dirent must have d_type"
#endif

#define FILE_TAG FOURCC('F', 'I', 'L', 'E')

enum {
    max_file_descriptors = 10,
};

static int ftw_callback(const char * const filepath, const struct stat * const sb, const int type_flag, struct FTW * const walk) {
    if (type_flag == FTW_F) {
        unlink(filepath);
        if ((remove(filepath) != 0) && (errno != ENOENT)) {
            return -1;
        }
    } else {
        ASSERT((type_flag == FTW_D) || (type_flag == FTW_DP));

        // since we can't pass in a user variable to nftw, make sure that the directory we're deleting isn't part of the app's root directories
        for (sb_file_directory_e dir = sb_app_root_directory; dir <= sb_app_cache_directory; ++dir) {
            if (strcmp(adk_get_file_directory_path(dir), filepath) == 0) {
                return 0;
            }
        }

        if (rmdir(filepath) != 0) {
            return -1;
        }
    }
    return 0;
}

sb_directory_delete_error_e sb_delete_directory(const sb_file_directory_e directory, const char * const subpath) {
    if ((directory == sb_app_root_directory) || (subpath && (strstr(subpath, "..") != NULL))) {
        return sb_directory_delete_invalid_input;
    }

    char path[sb_max_path_length];
    const char * const directory_path = adk_get_file_directory_path(directory);
    if (subpath && subpath[0]) {
        sprintf_s(path, ARRAY_SIZE(path), "%s/%s", directory_path, subpath);
    } else {
        sprintf_s(path, ARRAY_SIZE(path), "%s", directory_path);
    }

    {
        struct stat stat_buf;
        if (stat(path, &stat_buf) != 0) {
            return errno == ENOENT ? sb_directory_delete_does_not_exist : sb_directory_delete_error;
        }
    }

    const int status = nftw(path, ftw_callback, max_file_descriptors, FTW_DEPTH);
    return status == 0 ? sb_directory_delete_success : sb_directory_delete_error;
}

sb_directory_t * sb_open_directory(const sb_file_directory_e mount_point, const char * const subpath) {
    char path[sb_max_path_length];
    if (subpath && *subpath) {
        VERIFY(strstr(subpath, "..") == NULL);
        sprintf_s(path, ARRAY_SIZE(path), "%s/%s", adk_get_file_directory_path(mount_point), subpath);
    } else {
        sprintf_s(path, ARRAY_SIZE(path), "%s", adk_get_file_directory_path(mount_point));
    }
    DIR * const directory = opendir(path);
    if (!directory) {
        LOG_WARN(FILE_TAG, "could not open directory: [%s] errno: [%i]", path, errno);
    }
    return (sb_directory_t *)directory;
}

void sb_close_directory(sb_directory_t * const directory) {
    if (directory) {
        VERIFY_MSG(closedir((DIR *)directory) == 0, "invalid directory stream descriptor: [%p] (EBADF)", directory);
    }
}

static sb_directory_entry_type_e sys_dir_type_to_adk(const int file_type) {
    switch (file_type) {
        case DT_LNK:
            return sb_directory_entry_sym_link;
        case DT_REG:
            return sb_directory_entry_file;
        case DT_DIR:
            return sb_directory_entry_directory;
        default:
            return sb_directory_entry_unknown;
    }
}

sb_read_directory_result_t sb_read_directory(sb_directory_t * const directory) {
    const struct dirent * const entry = readdir((DIR *)directory);
    return (sb_read_directory_result_t){.entry = (sb_directory_entry_t *)entry, .entry_type = entry ? sys_dir_type_to_adk(entry->d_type) : sb_directory_entry_null};
}

const char * sb_get_directory_entry_name(const sb_directory_entry_t * const directory_entry) {
    return ((struct dirent *)directory_entry)->d_name;
}

static sb_stat_error_e sys_stat_err_to_sb(const int32_t err) {
    switch (err) {
        case 0:
            return sb_stat_success;
        case ENOENT:
            return sb_stat_error_no_entry;
        default:
            return sb_stat_error_unknown;
    }
}

static sb_file_mode_bits_e sys_stat_mode_to_sb(const int32_t sys_mode) {
    sb_file_mode_bits_e sb_mode = sb_file_mode_none;

    sb_mode |= (sys_mode & S_IRUSR) ? sb_file_mode_readable : 0;
    sb_mode |= (sys_mode & S_IWUSR) ? sb_file_mode_writable : 0;
    sb_mode |= (sys_mode & S_IXUSR) ? sb_file_mode_executable : 0;
    sb_mode |= S_ISDIR(sys_mode) ? sb_file_mode_directory : 0;
    sb_mode |= S_ISREG(sys_mode) ? sb_file_mode_regular_file : 0;
    sb_mode |= S_ISLNK(sys_mode) ? sb_file_mode_sym_link : 0;

    return sb_mode;
}

static uint64_t sys_stat_timestamp_to_sb(const time_t timestamp) {
    return timestamp;
}

sb_stat_result_t sb_stat(const sb_file_directory_e mount_point, const char * const subpath) {
    VERIFY(strstr(subpath, "..") == NULL);
    char path[sb_max_path_length];
    sprintf_s(path, ARRAY_SIZE(path), "%s/%s", adk_get_file_directory_path(mount_point), subpath);

    struct stat stat_buf;
    sb_stat_result_t result = {.error = sys_stat_err_to_sb(stat(path, &stat_buf))};
    if (result.error == sb_stat_success) {
        result.stat = (sb_stat_t){
            .mode = sys_stat_mode_to_sb(stat_buf.st_mode),

            .access_time_s = sys_stat_timestamp_to_sb(stat_buf.st_atime),
            .modification_time_s = sys_stat_timestamp_to_sb(stat_buf.st_mtime),
            .create_time_s = sys_stat_timestamp_to_sb(stat_buf.st_ctime),

            .hard_links = stat_buf.st_nlink,
            .size = stat_buf.st_size,
            .user_id = stat_buf.st_uid,
            .group_id = stat_buf.st_gid};
    }

    return result;
}
