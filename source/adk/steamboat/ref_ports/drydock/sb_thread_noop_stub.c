/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "impl_tracking.h"
#include "source/adk/steamboat/sb_thread.h"

// A thread-related global variable relied-upon by m5
sb_thread_id_t sb_main_thread_id;

sb_thread_id_t sb_get_current_thread_id(void) {
    NOT_IMPLEMENTED_EX;

    return (sb_thread_id_t)0;
}

sb_thread_id_t sb_create_thread(
    const char * name,
    const sb_thread_options_t options,
    int (*const thread_proc)(void * arg),
    void * const arg,
    const char * const tag) {
    UNUSED(name);
    UNUSED(options);
    UNUSED(thread_proc);
    UNUSED(arg);
    UNUSED(tag);

    NOT_IMPLEMENTED_EX;

    return (sb_thread_id_t)0;
}

bool sb_set_thread_priority(
    const sb_thread_id_t id,
    const sb_thread_priority_e priority) {
    UNUSED(id);
    UNUSED(priority);

    NOT_IMPLEMENTED_EX;

    return false;
}

void sb_join_thread(const sb_thread_id_t id) {
    UNUSED(id);

    NOT_IMPLEMENTED_EX;
}

void sb_thread_sleep(const milliseconds_t time) {
    UNUSED(time);

    NOT_IMPLEMENTED_EX;
}

sb_mutex_t * sb_create_mutex(const char * const tag) {
    UNUSED(tag);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

void sb_destroy_mutex(
    sb_mutex_t * const mutex,
    const char * const tag) {
    UNUSED(mutex);
    UNUSED(tag);

    NOT_IMPLEMENTED_EX;
}

void sb_lock_mutex(sb_mutex_t * const mutex) {
    UNUSED(mutex);

    NOT_IMPLEMENTED_EX;
}

void sb_unlock_mutex(sb_mutex_t * const mutex) {
    UNUSED(mutex);

    NOT_IMPLEMENTED_EX;
}

sb_condition_variable_t * sb_create_condition_variable(const char * const tag) {
    UNUSED(tag);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

void sb_destroy_condition_variable(
    sb_condition_variable_t * const cnd,
    const char * const tag) {
    UNUSED(cnd);
    UNUSED(tag);

    NOT_IMPLEMENTED_EX;
}

void sb_condition_wake_one(sb_condition_variable_t * cnd) {
    UNUSED(cnd);

    NOT_IMPLEMENTED_EX;
}

void sb_condition_wake_all(sb_condition_variable_t * cnd) {
    UNUSED(cnd);

    NOT_IMPLEMENTED_EX;
}

bool sb_wait_condition(
    sb_condition_variable_t * cnd,
    sb_mutex_t * mutex,
    const milliseconds_t timeout) {
    UNUSED(cnd);
    UNUSED(mutex);
    UNUSED(timeout);

    NOT_IMPLEMENTED_EX;

    return false;
}

void sb_thread_yield() {
    NOT_IMPLEMENTED_EX;
}
