/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
events.c

application event queue
*/

#include _PCH
#include "events.h"

#include "source/adk/steamboat/sb_thread.h"

// we will drop events if we try and queue more than this.
// don't change this to dynamic memory allocation: that is
// antithetical to determinism.

enum { max_events = 256 };

#ifndef _STB_NATIVE
enum { adk_max_gamepads = 4 };
typedef struct gamepad_state_t {
    bool connected;
    adk_gamepad_state_t state;
} gamepad_state_t;
static gamepad_state_t gamepads[adk_max_gamepads];
#endif

typedef struct event_buffer_t {
    adk_event_t events[max_events];
    int size;
} event_buffer_t;

static struct {
    sb_mutex_t * mutex;
    int buffer_index;
    event_buffer_t buffers[2];
} events;

#ifndef NDEBUG
THREAD_LOCAL bool locked;
#endif

void __init_adk_events() {
    ZEROMEM(&events);
    events.buffer_index = 0;
    events.mutex = sb_create_mutex(MALLOC_TAG);

#ifndef _STB_NATIVE
    ZEROMEM(&gamepads);
#endif
}

void __shutdown_adk_events() {
    sb_destroy_mutex(events.mutex, MALLOC_TAG);
    events.mutex = NULL;
}

void adk_lock_events() {
    sb_lock_mutex(events.mutex);
#ifndef NDEBUG
    locked = true;
#endif
}

void adk_unlock_events() {
    sb_unlock_mutex(events.mutex);
#ifndef NDEBUG
    locked = false;
#endif
}

void adk_post_event(const adk_event_t event) {
    ASSERT_MSG(locked, "adk_post_event() not locked!");

    const int i = events.buffer_index & 1;
    const int max_event_count = (event.event_data.type == adk_time_event) ? (max_events) : (max_events - 1);
    event_buffer_t * const b = &events.buffers[i];
    if (b->size < max_event_count) {
        b->events[b->size++] = event;
    }
}

#ifndef _STB_NATIVE
void adk_post_gamepad_state(const int index, const milliseconds_t time, const adk_gamepad_state_t state) {
    ASSERT(index >= 0);
    ASSERT(index < adk_max_gamepads);
    gamepad_state_t * gp = &gamepads[index];

    gp->state = state;
    if (!gp->connected) {
        gp->connected = true;
        adk_post_event((adk_event_t){
            .time = time,
            .event_data = {
                .type = adk_gamepad_event,
                .gamepad = (adk_gamepad_event_t){
                    .gamepad_index = index,
                    .event_data = {
                        .event = adk_gamepad_event_connect,
                    }}}});
    }
}

void adk_post_gamepad_disconnect(const int index, const milliseconds_t time) {
    ASSERT(index >= 0);
    ASSERT(index < adk_max_gamepads);
    gamepad_state_t * gp = &gamepads[index];

    if (gp->connected) {
        gp->connected = false;
        adk_post_event((adk_event_t){
            .time = time,
            .event_data = {
                .type = adk_gamepad_event,
                .gamepad = (adk_gamepad_event_t){
                    .gamepad_index = index,
                    .event_data = {
                        .event = adk_gamepad_event_disconnect,
                    }}}});
    }
}

const adk_gamepad_state_t * adk_get_gamepad_state(const int index) {
    ASSERT(index >= 0);
    ASSERT(index < adk_max_gamepads);
    gamepad_state_t * gp = &gamepads[index];
    if (gp->connected) {
        return &gp->state;
    }

    return NULL;
}
#endif

void adk_get_events_swap_and_unlock(const adk_event_t ** const first, const adk_event_t ** const last) {
    const int i = events.buffer_index & 1;
    ++events.buffer_index;

    adk_unlock_events();

    event_buffer_t * const b = &events.buffers[i];
    *first = &b->events[0];
    *last = &b->events[b->size];
    b->size = 0;
}
