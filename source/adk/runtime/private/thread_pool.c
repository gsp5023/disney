/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 thread_pool.c

 thread pool implementation details
 */

#include _PCH

#include "source/adk/runtime/thread_pool.h"

#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"

#define THREAD_POOL_TAG FOURCC('T', 'P', 'O', 'L')

enum {
    posix_thread_name_max_length = 16,
};

typedef struct job_timings_t {
    microseconds_t enqueue_timestamp;
    microseconds_t start_timestamp;
    microseconds_t end_timestamp;
} job_timings_t;

typedef union job_callback_t {
    thread_pool_call_fn_t std;
    thread_pool_ext_call_fn_t ext;
} job_callback_t;

struct job_t {
    job_callback_t job;
    thread_pool_call_fn_t completion_call;
    void * user;
    struct job_t * next;
    job_timings_t job_timings;
    int id;
};

static void thread_pool_enqueue_impl(thread_pool_t * const pool, const job_callback_t new_job, const thread_pool_call_fn_t completion_call, void * const user) {
    const microseconds_t enque_timestamp = adk_read_microsecond_clock();

    sb_lock_mutex(pool->mutex);
    job_t * const job = pool->free_head;
    if (!job) {
        sb_unlock_mutex(pool->mutex);
        TRAP("Thread pool exhausted max queued jobs limit: [%i]", pool->max_pending_jobs);
    }

    pool->free_head = pool->free_head->next;
    if (pool->free_head == NULL) {
        pool->free_tail = NULL;
    }
    job->next = NULL;

    if (pool->queued_tail == NULL) {
        pool->queued_head = pool->queued_tail = job;
    } else {
        pool->queued_tail->next = job;
        pool->queued_tail = job;
    }

    job->job_timings.enqueue_timestamp = enque_timestamp;
    job->job = new_job;
    job->completion_call = completion_call;
    job->user = user;
    job->id = pool->job_id_counter++;

    sb_unlock_mutex(pool->mutex);
    sb_condition_wake_one(pool->cv);
}

void thread_pool_enqueue(thread_pool_t * const pool, const thread_pool_call_fn_t new_job, const thread_pool_call_fn_t completion_call, void * const user) {
    job_callback_t job_callback = {.std = new_job};
    thread_pool_enqueue_impl(pool, job_callback, completion_call, user);
}

void thread_pool_enqueue_front(thread_pool_t * const pool, const thread_pool_call_fn_t new_job, const thread_pool_call_fn_t completion_call, void * const user) {
    const microseconds_t enque_timestamp = adk_read_microsecond_clock();

    sb_lock_mutex(pool->mutex);
    job_t * const job = pool->free_head;
    if (!job) {
        sb_unlock_mutex(pool->mutex);
        TRAP("Thread pool exhausted max queued jobs limit: [%i]", pool->max_pending_jobs);
    }

    pool->free_head = pool->free_head->next;
    if (pool->free_head == NULL) {
        pool->free_tail = NULL;
    }

    job->next = pool->queued_head;
    pool->queued_head = job;

    if (pool->queued_tail == NULL) {
        pool->queued_tail = job;
    }

    job->job_timings.enqueue_timestamp = enque_timestamp;
    job->job.std = new_job;
    job->completion_call = completion_call;
    job->user = user;
    job->id = pool->job_id_counter++;

    sb_unlock_mutex(pool->mutex);
    sb_condition_wake_one(pool->cv);
}

// No-op completion call to signify an extended job
static void ext_completion_placeholder(void * const user, thread_pool_t * const pool) {
    (void)user;
    (void)pool;
}

void thread_pool_enqueue_ext_job(thread_pool_t * const pool, const thread_pool_ext_call_fn_t new_job, void * const user) {
    job_callback_t new_callback = {.ext = new_job};
    thread_pool_enqueue_impl(pool, new_callback, ext_completion_placeholder, user);
}

// The opaque one-time job token points to the currently executing job
struct job_token_t {
    job_t * job;
};

void thread_pool_enqueue_completion(thread_pool_t * const pool, const thread_pool_call_fn_t completion_call, job_token_t * const one_time_tok) {
    VERIFY_MSG(one_time_tok->job != NULL, "attempt to reuse one-time token");
    job_t * const job = one_time_tok->job;
    one_time_tok->job = NULL; // clear one-time token

    job->completion_call = completion_call;
    job->job_timings.end_timestamp = adk_read_microsecond_clock();

    // Enqueue the completion call
    sb_lock_mutex(pool->mutex);

    job->next = NULL;
    if (pool->completion_call_tail == NULL) {
        pool->completion_call_head = pool->completion_call_tail = job;
    } else {
        pool->completion_call_tail->next = job;
        pool->completion_call_tail = job;
    }

    // Note that we can't touch the job after unlocking as it may run immediately in another thread and be returned to free list
    sb_unlock_mutex(pool->mutex);
}

static int thread_pool_proc(void * const args) {
    thread_pool_t * const owning_pool = args;
    sb_lock_mutex(owning_pool->mutex);
    while (!owning_pool->should_quit) {
        job_t * const job = owning_pool->queued_head;
        if (job) {
            owning_pool->queued_head = job->next;
            if (owning_pool->queued_head == NULL) {
                owning_pool->queued_tail = NULL;
            }
            ++owning_pool->busy_counter;
            sb_unlock_mutex(owning_pool->mutex);

            job->job_timings.start_timestamp = adk_read_microsecond_clock();
            if (job->completion_call == ext_completion_placeholder) {
                // Call extended job callback
                job->completion_call = NULL;
                job_token_t one_time_tok = {job};
                job->job.ext(job->user, owning_pool, &one_time_tok);
                if (one_time_tok.job == NULL) {
                    // Completion call has been scheduled, 'job' is no longer valid
                    sb_lock_mutex(owning_pool->mutex);
                    --owning_pool->busy_counter;
                    continue;
                }
            } else {
                // Call standard job callback
                job->job.std(job->user, owning_pool);
                if (job->completion_call) {
                    job->job_timings.end_timestamp = adk_read_microsecond_clock();
                    sb_lock_mutex(owning_pool->mutex);
                    --owning_pool->busy_counter;
                    job->next = NULL;
                    if (owning_pool->completion_call_tail == NULL) {
                        owning_pool->completion_call_head = owning_pool->completion_call_tail = job;
                    } else {
                        owning_pool->completion_call_tail->next = job;
                        owning_pool->completion_call_tail = job;
                    }
                    continue;
                }
            }

            // Job is done, no completion call, return to free list
            ZEROMEM(job);
            sb_lock_mutex(owning_pool->mutex);
            --owning_pool->busy_counter;
            if (owning_pool->free_tail == NULL) {
                owning_pool->free_head = owning_pool->free_tail = job;
            } else {
                owning_pool->free_tail->next = job;
                owning_pool->free_tail = job;
            }
        } else {
            sb_wait_condition(owning_pool->cv, owning_pool->mutex, sb_timeout_infinite);
        }
    }
    sb_unlock_mutex(owning_pool->mutex);
    return 0;
}

void thread_pool_init(thread_pool_t * const pool, const mem_region_t _region, const system_guard_page_mode_e guard_page_mode, const uint8_t num_threads, const char * const pool_name, const char * const tag) {
    ASSERT(pool_name);
    ASSERT((num_threads > 0) && num_threads <= thread_pool_max_threads);
    ASSERT(strlen(pool_name) < ARRAY_SIZE(pool->name));

    LOG_DEBUG(THREAD_POOL_TAG, "Initializing thread pool with %u thread(s)", num_threads);

    ZEROMEM(pool);

#ifdef GUARD_PAGE_SUPPORT
    mem_region_t region;
    if (guard_page_mode == system_guard_page_mode_enabled) {
        region = debug_sys_map_pages(_region.size, system_page_protect_read_write, MALLOC_TAG);
        pool->guard_mem = region;
    } else {
        region = _region;
    }
#else
    const mem_region_t region = _region;
#endif

    ASSERT(region.ptr);
    ASSERT(region.size > 0);
    ASSERT(IS_ALIGNED(region.ptr, ALIGN_OF(job_t)));

    strcpy_s(pool->name, ARRAY_SIZE(pool->name), pool_name);
    pool->mutex = sb_create_mutex(tag);
    pool->cv = sb_create_condition_variable(tag);
    pool->thread_count = num_threads;

    char thread_name[posix_thread_name_max_length];
    for (uint8_t i = 0; i < pool->thread_count; ++i) {
        sprintf_s(thread_name, ARRAY_SIZE(thread_name), "%s%i", pool_name, i);
        pool->threads[i] = sb_create_thread(thread_name, sb_thread_default_options, thread_pool_proc, pool, tag);
    }

    const size_t job_aligned_size = ALIGN_INT(sizeof(job_t), ALIGN_OF(job_t));
    pool->max_pending_jobs = (int)(region.size / job_aligned_size);
    job_t * cur = pool->free_head = pool->free_tail = region.ptr;
    for (int i = 0; i < pool->max_pending_jobs - 1; ++i) {
        pool->free_tail->next = cur;
        pool->free_tail = cur;
        cur = (job_t *)((char *)cur + job_aligned_size);
    }

    if (pool->free_tail != NULL) {
        pool->free_tail->next = NULL;
    }
}

thread_pool_t * thread_pool_emplace_init(const mem_region_t region, const uint8_t num_threads, const char * const pool_name, const char * const tag) {
    ASSERT(region.ptr);
    const size_t thread_pool_reserved = ALIGN_INT(sizeof(thread_pool_t), 8);
    ASSERT(region.size > thread_pool_reserved);

    mem_region_t pool_region = region;
    thread_pool_t * const pool = pool_region.ptr;

    pool_region.byte_ptr += thread_pool_reserved;
    pool_region.size -= thread_pool_reserved;

    const size_t alignment_offset = ALIGN_INT(pool_region.adr, (uintptr_t)ALIGN_OF(job_t)) - pool_region.adr;
    pool_region.byte_ptr += alignment_offset;
    pool_region.size -= alignment_offset;

    thread_pool_init(pool, pool_region, system_guard_page_mode_disabled, num_threads, pool_name, tag);

    return pool;
}

void thread_pool_shutdown(thread_pool_t * const pool, const char * const tag) {
    pool->should_quit = true;
    sb_compiler_barrier();
    sb_condition_wake_all(pool->cv);
    for (int i = 0; i < pool->thread_count; ++i) {
        sb_join_thread(pool->threads[i]);
    }

    sb_destroy_condition_variable(pool->cv, tag);
    sb_destroy_mutex(pool->mutex, tag);

    if (pool->queued_head != NULL) {
        LOG_WARN(THREAD_POOL_TAG, "thread pool [%s] shutdown with jobs still in queue", pool->name);
        ASSERT(false); // this should be handled appropriately in a debug build, and never occur in the wild..
    }

#ifdef GUARD_PAGE_SUPPORT
    if (pool->guard_mem.ptr) {
        debug_sys_unmap_pages(pool->guard_mem, MALLOC_TAG);
    }
#endif

#ifndef NDEBUG
    ZEROMEM(pool);
#endif
}

thread_pool_status_e thread_pool_run_completion_callbacks(thread_pool_t * const pool) {
    ASSERT_IS_MAIN_THREAD();

    sb_lock_mutex(pool->mutex);
    job_t * const completion_head = pool->completion_call_head;
    job_t * const completion_tail = pool->completion_call_tail;
    pool->completion_call_tail = pool->completion_call_head = NULL;
    sb_unlock_mutex(pool->mutex);

    job_t * curr_call = completion_head;
    while (curr_call) {
        curr_call->completion_call(curr_call->user, pool);
        curr_call = curr_call->next;
    }
    if (completion_head != NULL) {
        sb_lock_mutex(pool->mutex);
        if (pool->free_tail) {
            pool->free_tail->next = completion_head;
            pool->free_tail = completion_tail;
        } else {
            pool->free_head = completion_head;
            pool->free_tail = completion_tail;
        }
        sb_unlock_mutex(pool->mutex);
    }

    sb_lock_mutex(pool->mutex);
    const bool busy = pool->queued_head || pool->busy_counter;
    sb_unlock_mutex(pool->mutex);

    return busy ? thread_pool_busy : thread_pool_idle;
}

void thread_pool_drain(thread_pool_t * const pool) {
    while (thread_pool_run_completion_callbacks(pool) != thread_pool_idle) {
        sb_thread_sleep((milliseconds_t){1});
    }
}
