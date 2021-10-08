/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "impl_tracking.h"
#include "source/adk/steamboat/sb_file.h"

sb_file_t * sb_fopen(
    const sb_file_directory_e directory,
    const char * const path,
    const char * const mode) {
    UNUSED(directory);
    UNUSED(path);
    UNUSED(mode);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

bool sb_fclose(sb_file_t * const file) {
    UNUSED(file);

    NOT_IMPLEMENTED_EX;

    return false;
}

bool sb_delete_file(const sb_file_directory_e directory, const char * const filename) {
    UNUSED(directory);
    UNUSED(filename);

    NOT_IMPLEMENTED_EX;

    return false;
}

sb_directory_delete_error_e sb_delete_directory(
    const sb_file_directory_e directory,
    const char * const subpath) {
    UNUSED(directory);
    UNUSED(subpath);

    NOT_IMPLEMENTED_EX;

    return (sb_directory_delete_error_e)-1;
}

bool sb_create_directory_path(
    const sb_file_directory_e directory,
    const char * const input_path) {
    UNUSED(directory);
    UNUSED(input_path);

    NOT_IMPLEMENTED_EX;

    return false;
}

long sb_ftell(sb_file_t * const file) {
    UNUSED(file);

    NOT_IMPLEMENTED_EX;

    return 0;
}

bool sb_fseek(sb_file_t * const file, const long offset, const sb_seek_mode_e origin) {
    UNUSED(file);
    UNUSED(offset);
    UNUSED(origin);

    NOT_IMPLEMENTED_EX;

    return false;
}

size_t sb_fread(
    void * const buffer,
    const size_t elem_size,
    const size_t elem_count,
    sb_file_t * const file) {
    UNUSED(buffer);
    UNUSED(elem_size);
    UNUSED(elem_count);
    UNUSED(file);

    NOT_IMPLEMENTED_EX;

    return 0;
}

size_t sb_fwrite(
    const void * const buffer,
    const size_t elem_size,
    const size_t elem_count,
    sb_file_t * const file) {
    UNUSED(buffer);
    UNUSED(elem_size);
    UNUSED(elem_count);
    UNUSED(file);

    NOT_IMPLEMENTED_EX;

    return 0;
}

bool sb_feof(sb_file_t * const file) {
    UNUSED(file);

    NOT_IMPLEMENTED_EX;

    return false;
}

sb_directory_t * sb_open_directory(const sb_file_directory_e mount_point, const char * const subpath) {
    UNUSED(mount_point);
    UNUSED(subpath);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

void sb_close_directory(sb_directory_t * const directory) {
    UNUSED(directory);

    NOT_IMPLEMENTED_EX;
}

sb_read_directory_result_t sb_read_directory(sb_directory_t * const directory) {
    sb_read_directory_result_t retval = {0};

    UNUSED(directory);

    NOT_IMPLEMENTED_EX;

    return retval;
}

const char * sb_get_directory_entry_name(const sb_directory_entry_t * const directory_entry) {
    UNUSED(directory_entry);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

sb_stat_result_t sb_stat(const sb_file_directory_e mount_point, const char * const subpath) {
    sb_stat_result_t retval = {0};

    UNUSED(mount_point);
    UNUSED(subpath);

    NOT_IMPLEMENTED_EX;

    return retval;
}

bool sb_rename(const sb_file_directory_e mount_point, const char * const current_path, const char * const new_path) {
    UNUSED(mount_point);
    UNUSED(current_path);
    UNUSED(new_path);

    NOT_IMPLEMENTED_EX;

    return false;
}
