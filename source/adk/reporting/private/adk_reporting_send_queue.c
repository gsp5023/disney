/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 adk_reporting_send_queue.c

Implementation of the queue used to hold events waiting to be sent
*/

#include "source/adk/reporting/private/adk_reporting_send_queue.h"

#include "adk_reporting_sentry.h"

adk_reporting_send_queue_t * adk_reporting_create_send_queue(heap_t * const heap, sb_mutex_t * const heap_mutex, const uint32_t max_size) {
    ASSERT(heap != NULL);
    ASSERT(max_size > 0);

    sb_lock_mutex(heap_mutex);
    adk_reporting_send_queue_t * queue = heap_alloc(heap, sizeof(adk_reporting_send_queue_t), MALLOC_TAG);
    sb_unlock_mutex(heap_mutex);

    ZEROMEM(queue);
    queue->max_events = max_size;
    queue->mutex = sb_create_mutex(MALLOC_TAG);

    return queue;
}

void adk_reporting_enqueue_to_send(adk_reporting_instance_t * const instance, cJSON * event) {
    ASSERT(instance != NULL);
    ASSERT(instance->send_queue != NULL);
    ASSERT(instance->send_queue->mutex != NULL);
    ASSERT(event != NULL);

    adk_reporting_send_node_t * new_event = adk_reporting_malloc(instance, sizeof(adk_reporting_send_node_t), MALLOC_TAG);
    new_event->event = event;
    new_event->next = NULL;

    adk_reporting_send_node_t * stale_event = NULL;

    sb_lock_mutex(instance->send_queue->mutex);
    if (instance->send_queue->length == instance->send_queue->max_events) {
        // Dequeue oldest
        stale_event = instance->send_queue->head;
        instance->send_queue->head = instance->send_queue->head->next;
        if (instance->send_queue->tail == stale_event) { // for queues of size 1
            instance->send_queue->tail = instance->send_queue->head;
        }
        instance->send_queue->length--;
    }
    // Enqueue new
    if (instance->send_queue->length == 0) {
        ASSERT(instance->send_queue->head == NULL);
        ASSERT(instance->send_queue->tail == NULL);
        instance->send_queue->head = new_event;
        instance->send_queue->tail = new_event;
    } else {
        ASSERT(instance->send_queue->tail != NULL);
        instance->send_queue->tail->next = new_event;
        instance->send_queue->tail = new_event;
    }
    instance->send_queue->length++;
    sb_unlock_mutex(instance->send_queue->mutex);
    if (stale_event) {
        adk_reporting_free_send_node(instance, stale_event);
    }
}

adk_reporting_send_node_t * adk_reporting_flush_send_queue(adk_reporting_instance_t * const instance, const flush_send_queue_options_e flush_send_queue_option) {
    ASSERT(instance != NULL);
    ASSERT(instance->send_queue != NULL);
    ASSERT(instance->send_queue->mutex != NULL);

    adk_reporting_send_node_t * head = NULL;
    sb_lock_mutex(instance->send_queue->mutex);
    if ((flush_send_queue_option == disregard_pause) || (sb_get_time_since_epoch().seconds > instance->send_queue->pause_until_seconds_since_epoch)) {
        head = instance->send_queue->head;
        instance->send_queue->head = NULL;
        instance->send_queue->tail = NULL;
        instance->send_queue->length = 0;
    }
    sb_unlock_mutex(instance->send_queue->mutex);

    return head;
}

void adk_reporting_free_send_node(adk_reporting_instance_t * const instance, adk_reporting_send_node_t * node) {
    ASSERT(instance != NULL);
    ASSERT(node != NULL);
    if (node->event) {
        cJSON_Delete(&instance->json_ctx, node->event);
    }
    adk_reporting_free(instance, node, MALLOC_TAG);
}

bool adk_reporting_is_send_queue_empty(adk_reporting_instance_t * const instance) {
    ASSERT(instance != NULL);
    ASSERT(instance->send_queue != NULL);
    ASSERT(instance->send_queue->mutex != NULL);

    sb_lock_mutex(instance->send_queue->mutex);
    bool is_empty = (instance->send_queue->length == 0);
    sb_unlock_mutex(instance->send_queue->mutex);
    return is_empty;
}

void adk_reporting_pause_sending_queue(adk_reporting_instance_t * const instance, const uint32_t delay) {
    ASSERT(instance != NULL);
    ASSERT(instance->send_queue != NULL);
    ASSERT(instance->send_queue->mutex != NULL);

    sb_lock_mutex(instance->send_queue->mutex);
    instance->send_queue->pause_until_seconds_since_epoch = max_int(instance->send_queue->pause_until_seconds_since_epoch, (sb_get_time_since_epoch().seconds + delay));
    sb_unlock_mutex(instance->send_queue->mutex);
}
