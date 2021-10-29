/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/app_thunk/watchdog.h"

#include "source/adk/log/log.h"

#define TAG_WATCHDOG FOURCC('W', 'T', 'D', 'G')

enum {
    watchdog_tick_set = 0,
    watchdog_tick_cleared = 1
};

static int watchdog_thread_proc(void * data) {
    watchdog_t * const watchdog = data;

    milliseconds_t last_tick = adk_read_millisecond_clock();
    uint64_t time_since_last_tick = 0;

    bool warn_reported = false;

    while (watchdog->running) {
        const milliseconds_t current_time = adk_read_millisecond_clock();
        const uint32_t dt = current_time.ms - last_tick.ms;
        if (dt <= watchdog->suspend_threshold) {
            time_since_last_tick += dt;
        }

        last_tick = current_time;

        if (sb_atomic_load(&watchdog->tick_flag, memory_order_relaxed) == watchdog_tick_set) {
            sb_atomic_store(&watchdog->tick_flag, watchdog_tick_cleared, memory_order_relaxed);
            time_since_last_tick = 0;
            warn_reported = false;
        }

        if (time_since_last_tick >= watchdog->warning_delay_ms && time_since_last_tick < watchdog->fatal_delay_ms) {
            if (!warn_reported) {
                LOG_WARN(TAG_WATCHDOG, "Main thread is unresponsive for %i ms", watchdog->warning_delay_ms);
                warn_reported = true;
            }
        } else if (time_since_last_tick >= watchdog->fatal_delay_ms) {
            TRAP("Main thread didn't respond within %i ms.", watchdog->fatal_delay_ms);
        }

        sb_thread_sleep((milliseconds_t){1});
    }

    return 0;
}

void watchdog_tick(watchdog_t * const watchdog) {
    sb_atomic_store(&watchdog->tick_flag, watchdog_tick_set, memory_order_relaxed);
}

void watchdog_start(watchdog_t * const watchdog, const uint32_t suspend_threshold, const uint32_t warning_delay_ms, const uint32_t fatal_delay_ms) {
    ASSERT(warning_delay_ms != 0);
    ASSERT(fatal_delay_ms != 0);
    ASSERT(fatal_delay_ms > warning_delay_ms);
    ASSERT(watchdog->running == false);

    ZEROMEM(watchdog);

    watchdog->warning_delay_ms = warning_delay_ms;
    watchdog->fatal_delay_ms = fatal_delay_ms;
    watchdog->suspend_threshold = suspend_threshold;

    sb_thread_options_t watchdog_thread_options = sb_thread_default_options;
    watchdog_thread_options.priority = sb_thread_priority_high;

    watchdog->running = true;
    watchdog->thread = sb_create_thread("m5_watchdog", watchdog_thread_options, &watchdog_thread_proc, watchdog, MALLOC_TAG);
    sb_set_thread_priority(watchdog->thread, watchdog_thread_options.priority);
}

void watchdog_shutdown(watchdog_t * const watchdog) {
    if (watchdog->running) {
        LOG_INFO(TAG_WATCHDOG, "Terminating watchdog thread");

        watchdog->running = false;
        watchdog_tick(watchdog);
        sb_join_thread(watchdog->thread);
    }
}
