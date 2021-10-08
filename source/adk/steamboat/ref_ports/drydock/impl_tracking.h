/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

typedef enum drydock_api_avail_e {
    steam_api_schrodinger = 0,
    steam_api_implemented,
    steam_api_not_implemented
} drydock_api_avail_e;

typedef enum drydock_test_result_e {
    pass = 1,
    fail
} drydock_test_result_e;

typedef struct drydock_test_report_t {
    drydock_test_result_e result;
    drydock_api_avail_e api_avail;
    double timing;
} drydock_test_report_t;

#ifdef _DRYDOCK

#include <stdio.h>
#include <string.h>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef struct drydock_api_entry_t {
    char name[64];
    char file_name[256];
    int line_num;
    drydock_api_avail_e api_avail;
} drydock_api_entry_t;

extern drydock_api_entry_t g_sb_impl_table[];

int index_of(const char * fname);
void dump_table(void);

#define DUMP_TABLE dump_table()

#define IMPLEMENTED(fname) g_sb_impl_table[index_of(#fname)].api_avail = steam_api_implemented
#define NOT_IMPLEMENTED(fname) g_sb_impl_table[index_of(#fname)].api_avail = steam_api_not_implemented
#define NOT_IMPLEMENTED_EX                                              \
    drydock_api_entry_t * entry = &g_sb_impl_table[index_of(__func__)]; \
    entry->api_avail = steam_api_not_implemented;                       \
    strcpy(entry->file_name, __FILE__);                                 \
    entry->line_num = __LINE__

#define IMPLEMENTED_EX                                                  \
    drydock_api_entry_t * entry = &g_sb_impl_table[index_of(__func__)]; \
    entry->api_avail = steam_api_implemented;                           \
    strcpy(entry->file_name, __FILE__);                                 \
    entry->line_num = __LINE__

#define IS_IMPLEMENTED(fname) g_sb_impl_table[index_of(#fname)].api_avail == steam_api_implemented
#define IS_NOT_IMPLEMENTED(fname) g_sb_impl_table[index_of(#fname)].api_avail == steam_api_not_implemented
#define IS_SCHRODINGER(fname) g_sb_impl_table[index_of(#fname)].api_avail == steam_api_schrodinger

#define TEST_REPORT_UPDATE(tr_p, fname, res, duration)             \
    tr_p->result = res;                                            \
    tr_p->api_avail = g_sb_impl_table[index_of(#fname)].api_avail; \
    tr_p->timing = duration;

// clang-format off
#define TEST_RUN(t, tr)                                                              \
    t(&tr);                                                                          \
    if (tr.api_avail == steam_api_implemented) {                                     \
        printf("Test [%s]: api is implemented and [%s] taking %.0f microseconds.\n", \
            #t, (tr.result == pass) ? "passed" : "failed", tr.timing);               \
    } else {                                                                         \
        printf("Test [%s]: api is NOT implemented and [%s].\n",                      \
            #t, (tr.result == pass) ? "passed" : "failed");                          \
    }
// clang-format off

#else

#define DUMP_TABLE
#define IMPLEMENTED(fname)
#define NOT_IMPLEMENTED(fname)
#define NOT_IMPLEMENTED_EX
#define IMPLEMENTED_EX
#define IS_IMPLEMENTED(fname) 0
#define IS_NOT_IMPLEMENTED(fname) 0
#define IS_SCHRODINGER(fname) 1
#define TEST_REPORT_UPDATE(tr_p, fname, res, duration)
#define TEST_RUN(t, tr)

#endif /* _DRYDOCK */
