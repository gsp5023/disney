/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_thread.h

steamboat threads, atomics, condition variables, mutex
*/

#pragma once

#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================================================================
Threads
===============================================================================
*/

/// The scheduling priority of a thread
typedef enum sb_thread_priority_e {
    /// The lowest priority for a thread
    sb_thread_priority_low,
    /// The default priority for a thread
    sb_thread_priority_normal,
    /// A higher than normal priority for a thread
    sb_thread_priority_high,
    /// The highest priority for a thread
    sb_thread_priority_realtime
} sb_thread_priority_e;

/// An ID corresponding to a thread
typedef struct sb_thread_id_s * sb_thread_id_t;

/// Returns the ID of the main thread
EXT_EXPORT sb_thread_id_t sb_get_main_thread_id();

#define ASSERT_IS_MAIN_THREAD() ASSERT(sb_is_main_thread())
#define ASSERT_NOT_MAIN_THREAD() ASSERT(!sb_is_main_thread())

/// Returns the ID of the calling thread
EXT_EXPORT sb_thread_id_t sb_get_current_thread_id();

/// Config options for use with sb_create_thread
typedef struct sb_thread_options_t {
    /// The size of the threads stack in bytes
    uint32_t stack_size;
    /// The threads scheduling priority
    sb_thread_priority_e priority;
    /// if the thread is detached (i.e., not joinable)
    bool detached;
} sb_thread_options_t;

/// Default config options for thread creation
static const sb_thread_options_t sb_thread_default_options = {/*.stack_size =*/0, /*.priority =*/sb_thread_priority_normal, /*.detached =*/false};

/// Creates a new thread and returns its ID
///
/// Note: use `sb_thread_default_options` for default thread creation
///
/// * `name`: Name of the thread
/// * `options`: Thread configuration options
/// * `thread_proc(void * arg)`: The thread function
/// * `arg`: Argument for the thread function
/// * `tag`: Heap allocation tag (usually MALLOC_TAG)
///
/// Returns the ID of the created thread
EXT_EXPORT sb_thread_id_t sb_create_thread(
    const char * name,
    const sb_thread_options_t options,
    int (*const thread_proc)(void * arg),
    void * const arg,
    const char * const tag);

/// Sets the scheduling priority of thread corresponding to `id`
///
/// * `id`: ID of the thread whose priority is  being set
/// * `priority`: The new priority
///
/// Returns true on success
EXT_EXPORT bool sb_set_thread_priority(const sb_thread_id_t id, const sb_thread_priority_e priority);

/// Waits for the thread corresponding to `id` to terminate
///
/// * `id`: ID of the thread to join
///
EXT_EXPORT void sb_join_thread(const sb_thread_id_t id);

/// Suspends the execution of the calling thread until either at least the `time` specified has elapsed,
/// or a signal triggers the activation/termination of the thread
///
/// * `time`: Time to sleep in milliseconds
///
/// Note: To yield time slice sb_thread_yield should be used.
EXT_EXPORT void sb_thread_sleep(const milliseconds_t time);

/// Causes the calling thread to yield execution to another thread that's ready to run on the same processor.
EXT_EXPORT void sb_thread_yield();

/// Returns *true* if the calling thread is the main thread
static inline bool sb_is_main_thread() {
    return sb_get_current_thread_id() == sb_get_main_thread_id();
}

/*
===============================================================================
Mutex
===============================================================================
*/

/// Synchronization primitive for mutual exclusion
typedef struct sb_mutex_t sb_mutex_t;

/// Returns an initialized and unlocked mutex
///
/// * `tag`: Heap allocation tag
///
/// Returns the created mutex
EXT_EXPORT sb_mutex_t * sb_create_mutex(const char * const tag);

/// Destroys the mutex object `mutex`
///
/// * `mutex`: Mutex to destroy
/// * `tag`: Heap allocation tag
///
EXT_EXPORT void sb_destroy_mutex(sb_mutex_t * const mutex, const char * const tag);

/// Locks the `mutex`. If `mutex` is already locked, the current thread is blocked until `mutex` becomes available.
///
/// * `mutex`: Mutex to lock
///
EXT_EXPORT void sb_lock_mutex(sb_mutex_t * const mutex);

/// Releases the `mutex`, resulting in the `mutex` becoming available
///
/// * `mutex`: Mutex to unlock
///
EXT_EXPORT void sb_unlock_mutex(sb_mutex_t * const mutex);

/*
===============================================================================
Condition Variable
===============================================================================
*/

/// Synchronization primitive used to block and notify-to-resume threads
typedef struct sb_condition_variable_t sb_condition_variable_t;

/// Creates an initialized condition variable
///
/// * `tag`: Heap allocation tag
///
EXT_EXPORT sb_condition_variable_t * sb_create_condition_variable(const char * const tag);

/// Destroys the condition variable `cnd`
///
/// * `cnd`: condition variable to destroy
/// * `tag`: Heap allocation tag
///
EXT_EXPORT void sb_destroy_condition_variable(sb_condition_variable_t * const cnd, const char * const tag);

/// Unblocks at least one of the threads currently blocked on `cnd`
///
/// * `cnd`: condition variable on which to wake
///
EXT_EXPORT void sb_condition_wake_one(sb_condition_variable_t * cnd);

/// Unblocks all threads currently blocked on `cnd`
///
/// * `cnd`: condition variable on which to wake
///
EXT_EXPORT void sb_condition_wake_all(sb_condition_variable_t * cnd);

/// Value to specifify infinite wait (no timeout)
static const milliseconds_t sb_timeout_infinite = {0xFFFFFFFF};

/// Blocks the calling thread on `cnd` using `mutex` and returns true, or on timeout/error returns false
///
/// Note that `cnd` can spuriously wake up, so always check the condition!
///
/// * `cnd`: condition variable on which to wait
/// * `mutex`: Mutex locked by the calling thread
/// * `timeout`: timeout in milliseconds, use `sb_timeout_infinite` for wait with no timeout
///
EXT_EXPORT bool sb_wait_condition(sb_condition_variable_t * cnd, sb_mutex_t * mutex, const milliseconds_t timeout);

/*
===============================================================================
Atomics
===============================================================================
*/

/// Atomic integer
typedef struct sb_atomic_int32_t {
    ALIGN_4(volatile int32_t i32);
} sb_atomic_int32_t;

/// Atomic pointer
typedef struct sb_atomic_ptr_t {
#if _MACHINE_SIZE == 64
    ALIGN_8(void * volatile ptr);
#else
    ALIGN_4(void * volatile ptr);
#endif
} sb_atomic_ptr_t;

/*
x86/64 - Strong memory model

The following is guaranteed

* Loads are not reordered with other loads.
* Stores are not reordered with other stores.
* Stores are not reordered with older loads.
* In a multiprocessor system, memory ordering obeys causality (memory ordering respects transitive visibility).
* In a multiprocessor system, stores to the same location have a total order.
* In a multiprocessor system, locked instructions have a total order.
* Loads and stores are not reordered with locked instructions.

The following is not guaranteed

* Loads may be reordered with older stores to different locations

===============================================================================

SUMMARY:

In hardware:
* Stores have release semantics automatically since stores are not reordered
and loads are not reordered _after_ a store (but may move before it)
* Loads have acquire semantics automatically since loads are not reordered
with other loads, and stores are not reordered.

Example:
	STOR1
	LOAD1 -- cannot move before STOR1 because data-dependency on '1'
	STOR2 -- never reordered
	LOAD3 -- may move before STOR2
	STOR4 -- never reordered
	LOAD5 -- may move before STOR4
	STOR6 --> never reordered
	LOAD7 --> might move before STOR6, but never before LOAD5

*/

#ifdef _MSC_VER
/*
MSVC ATOMICS

We disable default MSVC 'volatile' behavior via /volatile:iso (see premake file). This also
AFAIK will disable automatic compiler barriers around volatiles?
*/

/* Win32 Interlocked* functions act as compiler barriers. */

/// Prevents the compiler from reordering loads/stores across this
/// barrier.
static inline void sb_compiler_barrier() {
    _ReadWriteBarrier();
}

/// Generates a memory fence.
///
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_thread_fence(const memory_order_e order) {
    _ReadWriteBarrier();
    if (order == memory_order_seq_cst) {
        ALIGN_4(volatile long x);
        _InterlockedExchange(&x, 0);
        _ReadWriteBarrier();
    }
}

/// Performs the operation *add* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the resulting sum
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_add(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return (int)_InterlockedExchangeAdd((long volatile *)&x->i32, (long)y);
}

/// Performs the operation *and* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_and(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return (int)_InterlockedAnd((long volatile *)&x->i32, (long)y);
}

/// Performs the operation *or* of `x` and `y`, store the result in `x`, and returns the previous value of `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_or(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return (int)_InterlockedOr((long volatile *)&x->i32, (long)y);
}

/// Returns the current value of `x`
///
/// * `x`: variable whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline int sb_atomic_load(sb_atomic_int32_t * const x, const memory_order_e order) {
    const int r = x->i32;
    switch (order) {
        case memory_order_relaxed:
            break;
        case memory_order_acquire:
        case memory_order_seq_cst:
            _ReadWriteBarrier();
            break;
        default:
            TRAP("atomic_load invalid memory order");
    }
    return r;
}

/// Returns the current value of `x`
///
/// * `x`: pointer whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline void * sb_atomic_load_ptr(sb_atomic_ptr_t * const x, const memory_order_e order) {
    void * const r = x->ptr;
    switch (order) {
        case memory_order_relaxed:
            break;
        case memory_order_acquire:
        case memory_order_seq_cst:
            _ReadWriteBarrier();
            break;
        default:
            TRAP("atomic_load invalid memory order");
    }
    return r;
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    switch (order) {
        case memory_order_release:
            _ReadWriteBarrier();
            // fall-through
        case memory_order_relaxed:
            x->i32 = y;
            break;
        case memory_order_seq_cst:
            _InterlockedExchange((long volatile *)&x->i32, y);
            break;
        default:
            TRAP("atomic_load invalid memory order");
    }
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store_ptr(sb_atomic_ptr_t * const x, void * const y, const memory_order_e order) {
    switch (order) {
        case memory_order_release:
            _ReadWriteBarrier();
            // fall-through
        case memory_order_relaxed:
            x->ptr = y;
            break;
        case memory_order_seq_cst:
            _InterlockedExchangePointer((void * volatile *)&x->ptr, y);
            break;
        default:
            TRAP("atomic_load invalid memory order");
    }
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the value to be compared and location of the results
/// * `value`: the value to be stored
/// * `compare`: the value to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline int sb_atomic_cas(sb_atomic_int32_t * const dest, const int value, const int compare, const memory_order_e order) {
    return (int)_InterlockedCompareExchange((long volatile *)&dest->i32, (long)value, (long)compare);
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the pointer to be compared and location of the results
/// * `value`: the pointer to be stored
/// * `compare`: the pointer to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline void * sb_atomic_cas_ptr(sb_atomic_ptr_t * const dest, void * const value, void * const compare, const memory_order_e order) {
    return _InterlockedCompareExchangePointer((void * volatile *)&dest->ptr, value, compare);
}

#elif defined(__GNUC__)

/// Prevents the compiler from reordering loads/stores across this
/// barrier.
static inline void sb_compiler_barrier() {
    asm volatile("" ::
                     : "memory");
}

#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7))
/// Generates a memory fence.
///
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_thread_fence(const memory_order_e order) {
    __atomic_thread_fence(order);
}

/// Performs the operation *add* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the resulting sum
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_add(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return __atomic_fetch_add(&x->i32, y, order);
}

/// Performs the operation *and* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_and(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return __atomic_fetch_and(&x->i32, y, order);
}

/// Performs the operation *or* of `x` and `y`, store the result in `x`, and returns the previous value of `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_or(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return __atomic_fetch_or(&x->i32, y, order);
}

/// Returns the current value of `x`
///
/// * `x`: variable whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline int sb_atomic_load(sb_atomic_int32_t * const x, const memory_order_e order) {
    volatile int32_t ret_val;
    __atomic_load(&x->i32, &ret_val, order);
    return ret_val;
}

/// Returns the current value of `x`
///
/// * `x`: pointer whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline void * sb_atomic_load_ptr(sb_atomic_ptr_t * const x, const memory_order_e order) {
    void * volatile ret_val;
    __atomic_load(&x->ptr, &ret_val, order);
    return ret_val;
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    __atomic_store(&x->i32, &y, order);
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store_ptr(sb_atomic_ptr_t * const x, void * const y, const memory_order_e order) {
    __atomic_store(&x->ptr, &y, order);
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the value to be compared and location of the results
/// * `value`: the value to be stored
/// * `compare`: the value to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline int sb_atomic_cas(sb_atomic_int32_t * const dest, const int value, const int compare, const memory_order_e order) {
    int expected = compare;
    if (__atomic_compare_exchange_n(&dest->i32, &expected, value, false, order, order == memory_order_release ? memory_order_relaxed : order)) {
        return compare;
    }
    return expected;
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the pointer to be compared and location of the results
/// * `value`: the pointer to be stored
/// * `compare`: the pointer to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline void * sb_atomic_cas_ptr(sb_atomic_ptr_t * const dest, void * const value, void * const compare, const memory_order_e order) {
    void * expected = compare;
    if (__atomic_compare_exchange_n(&dest->ptr, &expected, value, false, order, order == memory_order_release ? memory_order_relaxed : order)) {
        return compare;
    }
    return expected;
}

#else
/// Establishes memory synchronization ordering as defined by `order`
///
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_thread_fence(const memory_order_e order) {
    ((void)order);
    __sync_synchronize();
}

/// Performs the operation *add* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the resulting sum
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_add(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    ((void)order);
    // full barrier
    return __sync_fetch_and_add(&x->i32, y);
}

/// Performs the operation *and* of `x` and `y`, store the result in `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_and(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    ((void)order);
    // full barrier
    return __sync_fetch_and_and(&x->i32, y);
}

/// Performs the operation *or* of `x` and `y`, store the result in `x`, and returns the previous value of `x`
///
/// * `x`: first operand and location of the results
/// * `y`: second operand
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the previous value of `x`
static inline int sb_atomic_fetch_or(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    ((void)order);
    // full barrier
    return __sync_fetch_and_or(&x->i32, y);
}

/// Returns the current value of `x`
///
/// * `x`: variable whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline int sb_atomic_load(sb_atomic_int32_t * const x, const memory_order_e order) {
    ASSERT(order != memory_order_release);
    // full barrier
    return __sync_fetch_and_add(&x->i32, 0);
}

/// Returns the current value of `x`
///
/// * `x`: pointer whose value is the be returned
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns the current value of `x`
static inline void * sb_atomic_load_ptr(sb_atomic_ptr_t * const x, const memory_order_e order) {
    ASSERT(order != memory_order_release);
    // full barrier
    return __sync_fetch_and_add(&x->ptr, 0);
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    ASSERT(order != memory_order_acquire);
    // acquire barrier
    __sync_lock_test_and_set(&x->i32, y);
    if (order != memory_order_relaxed) {
        __sync_synchronize();
    }
}

/// Stores the value of `y` in `x`
///
/// * `x`: location to store the value of `y`
/// * `y`: the value to be stored
/// * `order`: 	The memory synchronization ordering for this operation
///
static inline void sb_atomic_store_ptr(sb_atomic_ptr_t * const x, void * const y, const memory_order_e order) {
    ASSERT(order != memory_order_acquire);
    // acquire barrier
    ((void)__sync_lock_test_and_set(&x->ptr, y));
    if (order != memory_order_relaxed) {
        __sync_synchronize();
    }
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the value to be compared and location of the results
/// * `value`: the value to be stored
/// * `compare`: the value to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline int sb_atomic_cas(sb_atomic_int32_t * const dest, const int value, const int compare, const memory_order_e order) {
    // full barrier
    return __sync_val_compare_and_swap(&dest->i32, compare, value);
}

/// Performs an atomic 'compare-and-swap' operation: if `dest` equals `compare`, then `dest` is updated to `value` and `compare` is returned. Otherwise, the current value of `dest` is returned.
///
/// * `dest`: the pointer to be compared and location of the results
/// * `value`: the pointer to be stored
/// * `compare`: the pointer to be compared
/// * `order`: 	The memory synchronization ordering for this operation
///
/// Returns `compare` if `dest` equals `compare`, else `dest`
static inline void * sb_atomic_cas_ptr(sb_atomic_ptr_t * const dest, void * const value, void * const compare, const memory_order_e order) {
    // full barrier
    return __sync_val_compare_and_swap(&dest->ptr, compare, value);
}

#endif
#else
#error "unsupported platform"
#endif

#ifdef __cplusplus
}
#endif
