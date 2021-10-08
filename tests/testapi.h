/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
testapi.h

API for unit tests
*/

#include "source/adk/runtime/runtime.h"

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "extern/cmocka/include/cmocka.h"

enum {
    unit_test_guard_page_mode = system_guard_page_mode_enabled,
    fragment_size = 4 * 1024
};

int test_findarg(const char * arg);
const char * test_getargarg(const char * arg);

static float rand_float() {
    return rand() / (float)RAND_MAX;
}

static int rand_int(const int min, const int max) {
    return min + (int)((float)(max - min) * rand_float());
}

#ifdef __cplusplus
}
#endif