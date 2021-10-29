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

/// Time in microseconds
FFI_EXPORT typedef struct microseconds_t {
    uint64_t us;
} microseconds_t;

/// Time in milliseconds
FFI_EXPORT typedef struct milliseconds_t {
    uint32_t ms;
} milliseconds_t;

EXT_EXPORT FFI_EXPORT microseconds_t adk_read_microsecond_clock();
EXT_EXPORT FFI_EXPORT milliseconds_t adk_read_millisecond_clock();

FFI_EXPORT uint64_t adk_get_milliseconds_since_epoch();

typedef struct seconds_t {
    uint64_t seconds;
} seconds_t;
    
#ifdef __cplusplus
}
#endif
