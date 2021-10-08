/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/time.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <time.h>

typedef void (*timer_callback)(void *);

typedef struct adk_timer_t {
    milliseconds_t repeat_interval;
    milliseconds_t curr_time;

    timer_callback callback_func;
    void * callback_args;
} adk_timer_t;

void adk_process_timers();

void adk_timers_init(const uint32_t num_threads);

void adk_timers_shutdown();

adk_timer_t * adk_timer_create(const milliseconds_t timer_duration, bool repeat, timer_callback callback_func, void * callback_args);

void adk_timer_destroy(adk_timer_t * timer);
