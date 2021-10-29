/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"

/*
watchdog.h

Watchdog thread API to monitor the application's heartbeat
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct watchdog_t {
    sb_thread_id_t thread;

    uint32_t warning_delay_ms;
    uint32_t fatal_delay_ms;

    uint32_t suspend_threshold;

    sb_atomic_int32_t tick_flag;

    bool running;
} watchdog_t;

void watchdog_start(watchdog_t * const watchdog, const uint32_t suspend_threshold, const uint32_t warning_delay_ms, const uint32_t fatal_delay_ms);

void watchdog_tick(watchdog_t * const watchdog);

void watchdog_shutdown(watchdog_t * const watchdog);

#ifdef __cplusplus
}
#endif
