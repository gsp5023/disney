/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adk_coredump_data_t {
    struct adk_coredump_data_t * next;
    const char * name;
    const char * value;
} adk_coredump_data_t;

bool adk_coredump_init(int coredump_stack_size);

void adk_coredump_shutdown();

void adk_crash_handler();

FFI_EXPORT void adk_coredump_add_data(FFI_PTR_WASM const char * name, FFI_PTR_WASM const char * value);

FFI_EXPORT void adk_coredump_add_data_public(FFI_PTR_WASM const char * name, FFI_PTR_WASM const char * value);

void adk_coredump_get_data(adk_coredump_data_t ** data);

#ifdef __cplusplus
}
#endif
