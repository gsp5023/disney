/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
file_stbi.c

stbi io support
*/
#include _PCH

#include "source/adk/runtime/file_stbi.h"

#include "file.h"

int adk_stbi_fread(void * const user, char * data, const int size) {
    return (int)sb_fread(data, 1, size, (sb_file_t * const)user);
}

void adk_stbi_fwrite(void * const user, void * data, const int size) {
    sb_fwrite(data, sizeof(char), size, (sb_file_t * const)(user));
}

void adk_stbi_fseek(void * const user, const int n) {
    sb_fseek((sb_file_t * const)user, n, sb_seek_cur);
}

int adk_stbi_feof(void * const user) {
    // feof will return true for being end of file.. so mimic that.
    return sb_feof((sb_file_t * const)user) ? 1 : 0;
}
