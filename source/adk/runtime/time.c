/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#include "source/adk/runtime/time.h"

#include "source/adk/steamboat/sb_platform.h"

microseconds_t adk_read_microsecond_clock() {
    return (microseconds_t){.us = sb_read_nanosecond_clock().ns / 1000ULL};
}

milliseconds_t adk_read_millisecond_clock() {
    return (milliseconds_t){.ms = (uint32_t)(adk_read_microsecond_clock().us / 1000ULL)};
}

uint64_t adk_get_milliseconds_since_epoch() {
    const sb_time_since_epoch_t time_since_epoch = sb_get_time_since_epoch();
    return ((uint64_t)time_since_epoch.seconds * 1000) + ((uint64_t)time_since_epoch.microseconds / 1000);
}
