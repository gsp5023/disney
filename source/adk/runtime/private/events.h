/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
events.h

Private ADK events api
*/

#include "source/adk/runtime/app/events.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================================================================
adk_lock_events

Locks event system for event queueing
===============================================================================
*/

EXT_EXPORT void adk_lock_events();

/*
===============================================================================
adk_unlock_events

Unlocks event system
===============================================================================
*/

EXT_EXPORT void adk_unlock_events();

/*
===============================================================================
adk_post_event

Post an event to the application event queue.

Only valid in between adk_lock_events(), adk_unlock_events()
===============================================================================
*/

EXT_EXPORT void adk_post_event(const adk_event_t event);

#ifndef _STB_NATIVE
/*
===============================================================================
adk_post_gamepad_state

Post the most recent gamepade state to the application event queue. If gamepad
was previously disconnected a connect event is generated.

Only valid in between adk_lock_events(), adk_unlock_events()
===============================================================================
*/

void adk_post_gamepad_state(const int index, const milliseconds_t time, const adk_gamepad_state_t state);

/*
===============================================================================
adk_post_gamepad_disconnect

Post a disconnect event to the application event queue.

Only valid in between adk_lock_events(), adk_unlock_events()
===============================================================================
*/

void adk_post_gamepad_disconnect(const int index, const milliseconds_t time);

#endif

/*
===============================================================================
adk_post_event_async

Post an event asynchronously to the application event queue, can be called
from any thread
===============================================================================
*/

static inline void adk_post_event_async(const adk_event_t event) {
    adk_lock_events();
    adk_post_event(event);
    adk_unlock_events();
}

/*
===============================================================================
adk_get_events_swap_and_unlock

Gets the current set of queued events, swaps the event buffers and unlocks
the event mutex.

Only valid after adk_lock_events()
===============================================================================
*/

void adk_get_events_swap_and_unlock(const adk_event_t ** const first, const adk_event_t ** const last);

#ifdef __cplusplus
}
#endif
