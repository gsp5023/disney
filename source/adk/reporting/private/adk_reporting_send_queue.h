/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
    ADK reporting send queue - A queue used to hold events waiting to be sent
*/

#pragma once

#include "extern/cjson/cJSON.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/reporting/adk_reporting_sentry_options.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adk_reporting_send_node_t {
    cJSON * event;
    struct adk_reporting_send_node_t * next;
} adk_reporting_send_node_t;

typedef struct adk_reporting_send_queue_t {
    sb_mutex_t * mutex;
    // maximum number of events allowed in the queue
    uint32_t max_events;
    adk_reporting_send_node_t * head;
    adk_reporting_send_node_t * tail;
    uint32_t length;
    uint32_t pause_until_seconds_since_epoch;
} adk_reporting_send_queue_t;

typedef enum flush_send_queue_options_e {
    regard_pause,
    disregard_pause
} flush_send_queue_options_e;

// Creates a send queue with memory from the given lockable mutex
adk_reporting_send_queue_t * adk_reporting_create_send_queue(heap_t * const heap, sb_mutex_t * const heap_mutex, const uint32_t max_size);

// Add a JSON event to the send queue
void adk_reporting_enqueue_to_send(adk_reporting_instance_t * const instance, cJSON * event);

// Returns the content of the send queue as a linked list and resets the queue to empty if
//    A) the queue is not paused do to a temporarily down server
// or B) pause is overwritten by setting flush_send_queue_option = disregard_pause
adk_reporting_send_node_t * adk_reporting_flush_send_queue(adk_reporting_instance_t * const instance, const flush_send_queue_options_e flush_send_queue_option);

// Frees the given node using the heap in the given instance, including the event JSON object
void adk_reporting_free_send_node(adk_reporting_instance_t * const instance, adk_reporting_send_node_t * node);

// returns true if the send queue contains no events
bool adk_reporting_is_send_queue_empty(adk_reporting_instance_t * const instance);

// Blocks flushing the queue in the instance for `delay` seconds unless overwritten by flush command
void adk_reporting_pause_sending_queue(adk_reporting_instance_t * const instance, const uint32_t delay);

#ifdef __cplusplus
}
#endif
