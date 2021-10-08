/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/steamboat/sb_file.h"

void json_deflate_write_string(FILE * const f, const char * const str) {
    fwrite(str, sizeof(char), strlen(str), f);
}

void json_deflate_write_int(FILE * const f, const int num) {
    char text[64] = {0};
    sprintf_s(text, sizeof(text), "%d", num);
    json_deflate_write_string(f, text);
}

void json_deflate_write_uint(FILE * const f, const unsigned int num) {
    char text[64] = {0};
    sprintf_s(text, sizeof(text), "%u", num);
    json_deflate_write_string(f, text);
}

void json_deflate_write_hex(FILE * const f, const unsigned int num) {
    char text[64] = {0};
    sprintf_s(text, sizeof(text), "%x", num);
    json_deflate_write_string(f, text);
}

#ifndef _WIN32
#if defined(_VADER) || defined(_LEIA)
char * vader_strok_s(char * str, const char * delim, char ** saveptr) {
    ASSERT(delim != NULL);
    ASSERT(saveptr != NULL);

    return strtok_r(str, delim, saveptr);
}
#else
char * strtok_s(char * str, const char * delim, char ** saveptr) {
    ASSERT(delim != NULL);
    ASSERT(saveptr != NULL);

    return strtok_r(str, delim, saveptr);
}
#endif
#endif