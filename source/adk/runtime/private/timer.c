/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "source/adk/runtime/timer.h"

#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_thread.h"

static struct statics {
    adk_timer_t * timer;
    milliseconds_t last_clock;
    bool running;

} statics;

extern void * sb_runtime_heap_alloc(const size_t size, const char * const tag);
extern void sb_runtime_heap_free(void * const p, const char * const tag);

void adk_process_timers() {
    if (!statics.timer)
        return;

    adk_timer_t * timer = statics.timer;

    const milliseconds_t current_clock = adk_read_millisecond_clock();

    const milliseconds_t delta = {.ms = current_clock.ms - statics.last_clock.ms};
    timer->curr_time.ms += delta.ms;

    if (timer->curr_time.ms >= timer->repeat_interval.ms) {
        timer->curr_time.ms = 0;
        timer->callback_func(timer->callback_args);
    }
    statics.last_clock.ms = current_clock.ms;
}

void adk_add_timer_to_queue(adk_timer_t * timer) {
    statics.timer = timer;
}

void adk_timers_init(const uint32_t num_threads) {
    statics.running = false;
    statics.timer = NULL;
}

void adk_timers_shutdown() {
    // TODO: M5-1815
    // come up with a more logically accurate way of handling timers being shutdown during sb_shutdown.
    // they are currently only init during sb_init which is not called in unit tests.
    if (statics.timer) {
        sb_runtime_heap_free(statics.timer, MALLOC_TAG);
    }
    statics.timer = NULL;
}

adk_timer_t * adk_timer_create(const milliseconds_t timer_duration, bool repeat, timer_callback callback_func, void * callback_args) {
    adk_timer_t * new_timer = (adk_timer_t *)sb_runtime_heap_alloc(sizeof(adk_timer_t), MALLOC_TAG);
    new_timer->repeat_interval.ms = repeat ? timer_duration.ms : 0;
    new_timer->curr_time.ms = 0;
    new_timer->callback_func = callback_func;
    new_timer->callback_args = callback_args;
    adk_add_timer_to_queue(new_timer);
    return new_timer;
}

void adk_timer_destroy(adk_timer_t * timer) {
    sb_runtime_heap_free(timer, MALLOC_TAG);
    statics.timer = NULL;
}
