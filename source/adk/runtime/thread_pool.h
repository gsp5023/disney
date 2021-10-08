/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
 thread_pool.h

 multi consumer, multi producer work stealing thread pool with basic job timing information
 */

#include "runtime.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    // limit is due to posix thread's name limit of 16 -3 characters for converting the total threads (255) to chars.
    thread_pool_name_length = 16 - 3,
#if defined(_VADER) || defined(_LEIA)
    thread_pool_max_threads = 12,
#else
    thread_pool_max_threads = 2,
#endif
};

typedef struct job_t job_t;

typedef struct thread_pool_t {
    char name[thread_pool_name_length];
    sb_thread_id_t threads[thread_pool_max_threads];
    uint8_t thread_count;
    int job_id_counter;
    int max_pending_jobs;
    int busy_counter;

    job_t * free_head;
    job_t * free_tail;
    job_t * queued_head;
    job_t * queued_tail;
    job_t * completion_call_head;
    job_t * completion_call_tail;

    sb_mutex_t * mutex;
    sb_condition_variable_t * cv;

    bool should_quit;
#ifdef GUARD_PAGE_SUPPORT
    mem_region_t guard_mem;
#endif
} thread_pool_t;

// Standard job and completion callback
typedef void (*thread_pool_call_fn_t)(void * user, thread_pool_t * const pool);

thread_pool_t * thread_pool_emplace_init(const mem_region_t region, const uint8_t num_threads, const char * const pool_name, const char * const tag);
void thread_pool_init(thread_pool_t * const pool, const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const uint8_t num_threads, const char * const pool_name, const char * const tag);
void thread_pool_shutdown(thread_pool_t * const pool, const char * const tag);

void thread_pool_enqueue(thread_pool_t * const pool, const thread_pool_call_fn_t new_job, const thread_pool_call_fn_t completion_call, void * const user);
void thread_pool_enqueue_front(thread_pool_t * const pool, const thread_pool_call_fn_t new_job, const thread_pool_call_fn_t completion_call, void * const user);

// Extended job support.  Facilitates jobs that need to control the order of thread pool operations (e.g., to avoid race conditions).  In
// particular, an extended job may schedule a completion call on itself and then enqueue a continuation job.  This ensures that the
// completion calls run in the same order as the work-thread jobs.
//

struct job_token_t; // opaque
typedef struct job_token_t job_token_t;

// Extended job callback.  It may do (a limited amount of) work after enqueuing a completion callback
typedef void (*thread_pool_ext_call_fn_t)(void * user, thread_pool_t * const pool, job_token_t * const one_time_tok);

// Enqueues an extended job
void thread_pool_enqueue_ext_job(thread_pool_t * const pool, const thread_pool_ext_call_fn_t new_job, void * const user);

// Enqueues a completion callback for an extended job.  The callback may run immediately in another thread, even before this function
// returns.  This function must only be called, once, from an extended job callback.  After calling this function, the one-time token
// will be invalid and cannot be reused.
void thread_pool_enqueue_completion(thread_pool_t * const pool, const thread_pool_call_fn_t completion_call, job_token_t * const one_time_tok);

typedef enum thread_pool_status_e {
    thread_pool_idle,
    thread_pool_busy
} thread_pool_status_e;

thread_pool_status_e thread_pool_run_completion_callbacks(thread_pool_t * const pool);
void thread_pool_drain(thread_pool_t * const pool);

#ifdef __cplusplus
}
#endif
