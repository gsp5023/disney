/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cncbus.c

 Command & Control Bus

 Unified message routing and delivery.
 */

#include "cncbus.h"

#include "source/adk/log/log.h"

typedef cncbus_receiver_t * cncbus_receiver_ptr_t;
#define ALGO_STATIC cncbus_receiver_ptr_t

static bool compare_less_cncbus_receiver_ptr_t(const cncbus_receiver_ptr_t a, const cncbus_receiver_ptr_t b) {
    // descending sort
    return a->internal.address.u32 > b->internal.address.u32;
}

#include "source/adk/runtime/algorithm.h"

#define TAG_CNC_BUS FOURCC('C', 'C', 'B', 'S')

/*
===============================================================================
CNC bus API

The overall contention design is to minimize dispatch contention: a dispatch can
fail with busy if there is an active connect or disconnect. The only contention
for sending and dispatch is appending messages to update the linked list head/tail,
and the duration here is very short, once a message is dequeued the hazard is
released and the receiver is invoked.

A connect/disconnect (cdc) hazard could wait quite a while before it can get in
there (i.e. hazard is set which will cause any new dispatch attempts to return
busy) but the currently executing dispatches would have to complete.

A CDC operation is quite short but it does potentially block a sender. It won't
ever block a dispatch, the dispatch will either complete before the CDC can execute
or it will return busy.

During the design it was preferred to have the connect/disconnect operations stall
longer since they are much less frequent (usually a cdc happens during app init
or shutdown but not often during normal operation).

Further complexity around CDC could eliminate all waits.
===============================================================================
*/

enum {
    message_fragment_struct_size = 1024,
    max_pending_messages_per_receiver_soft_limit = 4,

#ifdef GUARD_PAGE_SUPPORT
#ifdef _CNCBUS_DEBUG_GUARDS
#if _MACHINE_SIZE == 64
    message_fragment_size = message_fragment_struct_size - sizeof(void *) - sizeof(debug_sys_page_block_t) - sizeof(sb_atomic_int32_t) - sizeof(int)
#else
    message_fragment_size = message_fragment_struct_size - sizeof(void *) - sizeof(debug_sys_page_block_t) - sizeof(sb_atomic_int32_t)
#endif
#else
#if _MACHINE_SIZE == 64
    message_fragment_size = message_fragment_struct_size - sizeof(void *) - sizeof(debug_sys_page_block_t) - sizeof(sb_atomic_int32_t)
#else
    message_fragment_size = message_fragment_struct_size - sizeof(void *) - sizeof(debug_sys_page_block_t) - sizeof(sb_atomic_int32_t) + sizeof(int)
#endif
#endif
#else
    message_fragment_size = message_fragment_struct_size - sizeof(void *)
#endif
};

struct cncbus_msg_frag_t {
    struct cncbus_msg_frag_t * next;
#ifdef _CNCBUS_DEBUG_GUARDS
    sb_atomic_int32_t refcount;
#if _MACHINE_SIZE == 64
    int padd;
#endif
#endif
#ifdef GUARD_PAGE_SUPPORT
    debug_sys_page_block_t guard_pages;
    uint8_t data[message_fragment_size];
#else
    uint8_t data[message_fragment_size];
#endif
};

STATIC_ASSERT(sizeof(cncbus_msg_frag_t) == message_fragment_struct_size);

struct cncbus_msg_data_t {
    cncbus_t * bus;
    cncbus_signal_t * signal;
    cncbus_msg_frag_t * frag_head;
    cncbus_msg_frag_t * frag_tail;
    cncbus_msg_header_t header;
    cncbus_msg_t cursor;
    // reserved size
    sb_atomic_int32_t ref_count;
    int reserved;
};

enum {
    first_msg_frag_ofs = ALIGN_INT(sizeof(cncbus_msg_data_t), 8),
    first_msg_frag_size = message_fragment_size - first_msg_frag_ofs
};

STATIC_ASSERT(first_msg_frag_size > 0);
STATIC_ASSERT(message_fragment_size > sizeof(cncbus_msg_data_t));
STATIC_ASSERT(IS_ALIGNED(sizeof(cncbus_msg_data_t), 8));

static void set_msg_guard(cncbus_msg_t * const msg) {
#ifdef _CNCBUS_DEBUG_GUARDS
    msg->internal.guard0[0] = 'g';
    msg->internal.guard0[1] = 'a';
    msg->internal.guard0[2] = 'r';
    msg->internal.guard0[3] = 'd';
    msg->internal.guard1[0] = 'g';
    msg->internal.guard1[1] = 'a';
    msg->internal.guard1[2] = 'r';
    msg->internal.guard1[3] = 'd';
#endif
}

static void check_msg_guard(cncbus_msg_t * const msg) {
#ifdef _CNCBUS_DEBUG_GUARDS
    VERIFY(msg->internal.guard0[0] == 'g');
    VERIFY(msg->internal.guard0[1] == 'a');
    VERIFY(msg->internal.guard0[2] == 'r');
    VERIFY(msg->internal.guard0[3] == 'd');
    VERIFY(msg->internal.guard1[0] == 'g');
    VERIFY(msg->internal.guard1[1] == 'a');
    VERIFY(msg->internal.guard1[2] == 'r');
    VERIFY(msg->internal.guard1[3] == 'd');
#endif
}

void cncbus_init(cncbus_t * const bus, const mem_region_t region, const system_guard_page_mode_e guard_pages) {
    ZEROMEM(bus);

    const int num_receiver_chains = max_pending_messages_per_receiver_soft_limit * cncbus_max_receivers;
    const int receiver_chain_block_size = ALIGN_INT(sizeof(cncbus_receiver_msg_chain_t) * num_receiver_chains, 8);

    VERIFY(receiver_chain_block_size < (int)region.size);

    // setup receiver message free chain
    {
        cncbus_receiver_msg_chain_t * last;
#ifdef GUARD_PAGE_SUPPORT
        if (guard_pages) {
            const debug_sys_page_block_t block = debug_sys_map_page_block(sizeof(cncbus_receiver_msg_chain_t), system_page_protect_read_write, guard_pages, MALLOC_TAG);
            TRAP_OUT_OF_MEMORY(block.region.ptr);
            last = (cncbus_receiver_msg_chain_t *)block.region.ptr;
            last->guard_pages = block;
            bus->internal.guard_pages = true;
        } else
#endif
        {
            last = (cncbus_receiver_msg_chain_t *)region.ptr;
            ZEROMEM(last);
        }

        bus->internal.free_receiver_msg_chain = last;
        {
            for (int i = 1; i < num_receiver_chains; ++i) {
                cncbus_receiver_msg_chain_t * next;
#ifdef GUARD_PAGE_SUPPORT
                if (guard_pages) {
                    const debug_sys_page_block_t block = debug_sys_map_page_block(sizeof(cncbus_receiver_msg_chain_t), system_page_protect_read_write, guard_pages, MALLOC_TAG);
                    TRAP_OUT_OF_MEMORY(block.region.ptr);
                    next = (cncbus_receiver_msg_chain_t *)block.region.ptr;
                    next->guard_pages = block;
                } else
#endif
                {
                    next = last + 1;
                    ZEROMEM(next);
                }

                last->next = next;
                last = next;
            }

            last->next = NULL;
        }
    }

    // setup message fragment chain
    {
        const mem_region_t msg_frag_region = MEM_REGION(
                .adr = region.adr + receiver_chain_block_size,
                .size = region.size - receiver_chain_block_size);

        VERIFY(msg_frag_region.size >= sizeof(cncbus_msg_frag_t));
        const int num_frags = (int)(msg_frag_region.size / sizeof(cncbus_msg_frag_t));
        bus->internal.alloc_msg_frags_count = num_frags;

        cncbus_msg_frag_t * last;
#ifdef GUARD_PAGE_SUPPORT
        if (guard_pages) {
            const debug_sys_page_block_t block = debug_sys_map_page_block(sizeof(cncbus_msg_frag_t), system_page_protect_read_write, guard_pages, MALLOC_TAG);
            TRAP_OUT_OF_MEMORY(block.region.ptr);
            last = (cncbus_msg_frag_t *)block.region.ptr;
            last->guard_pages = block;
        } else
#endif
        {
            last = (cncbus_msg_frag_t *)msg_frag_region.ptr;
            ZEROMEM(last);
        }

        bus->internal.free_msg_frag_chain = last;
        {
            for (int i = 1; i < num_frags; ++i) {
                cncbus_msg_frag_t * next;
#ifdef GUARD_PAGE_SUPPORT
                if (guard_pages) {
                    const debug_sys_page_block_t block = debug_sys_map_page_block(sizeof(cncbus_msg_frag_t), system_page_protect_read_write, guard_pages, MALLOC_TAG);
                    TRAP_OUT_OF_MEMORY(block.region.ptr);
                    next = (cncbus_msg_frag_t *)block.region.ptr;
                    next->guard_pages = block;
                } else
#endif
                {
                    next = last + 1;
                    ZEROMEM(next);
                }

                last->next = next;
                last = next;
            }

            last->next = NULL;
        }
    }
}

void cncbus_destroy(cncbus_t * const bus) {
#if defined(GUARD_PAGE_SUPPORT) || !defined(_SHIP)
#ifdef GUARD_PAGE_SUPPORT
    const bool guard_pages = bus->internal.guard_pages;
#endif
    {
        int iter = 0;
        cncbus_receiver_msg_chain_t * next;
        for (cncbus_receiver_msg_chain_t * chain = bus->internal.free_receiver_msg_chain; chain; chain = next, ++iter) {
            next = chain->next;
#ifdef GUARD_PAGE_SUPPORT
            if (guard_pages) {
                debug_sys_unmap_page_block(chain->guard_pages, MALLOC_TAG);
            }
#endif
        }
        const int num_leaked = (max_pending_messages_per_receiver_soft_limit * cncbus_max_receivers) - iter;
        if (num_leaked > 0) {
            LOG_DEBUG(TAG_CNC_BUS, "cncbus_destroy: leaked %d message chains", num_leaked);
        }
    }
    {
        int iter = 0;
        cncbus_msg_frag_t * next;
        for (cncbus_msg_frag_t * frag = bus->internal.free_msg_frag_chain; frag; frag = next, ++iter) {
            next = frag->next;
#ifdef GUARD_PAGE_SUPPORT
            if (guard_pages) {
                debug_sys_unmap_page_block(frag->guard_pages, MALLOC_TAG);
            }
#endif
        }
        const int num_leaked = bus->internal.alloc_msg_frags_count - iter;
        if (num_leaked > 0) {
            LOG_DEBUG(TAG_CNC_BUS, "cncbus_destroy: leaked %d message fragments", num_leaked);
        }
    }
#endif
}

typedef enum bus_hazard_e {
    bus_hazard_cdc = -1,
    bus_hazard_none = 0
} bus_hazard_e;

typedef enum receiver_hazard_e {
    receiver_hazard_none = 0,
    receiver_hazard_access_msg_chain = 1,
    receiver_hazard_dispatch = 2,
    receiver_hazard_destroyed = 4
} receiver_hazard_e;

// returns true if hazard was successfully set.
// this hazard guarantees that the receivers array won't be modified,
// however individual receivers can still be disconnected.

static bool bus_try_enter_read_hazard(cncbus_t * const bus) {
    // set read hazard flag
    sb_atomic_fetch_add(&bus->internal.read_hazard_count, 1, memory_order_relaxed);
    if (sb_atomic_load(&bus->internal.receivers_hazard, memory_order_acquire) == bus_hazard_none) {
#ifdef _CNCBUS_DEBUG_GUARDS
        sb_atomic_fetch_add(&bus->internal.reader_guard, 1, memory_order_relaxed);
        VERIFY_MSG(sb_atomic_load(&bus->internal.writer_guard, memory_order_relaxed) == 0, "bus read hazard during cdc");
#endif
        return true;
    }

    sb_atomic_fetch_add(&bus->internal.read_hazard_count, -1, memory_order_relaxed);
    return false;
}

static void bus_exit_read_hazard(cncbus_t * const bus) {
#ifdef _CNCBUS_DEBUG_GUARDS
    VERIFY_MSG(sb_atomic_load(&bus->internal.writer_guard, memory_order_relaxed) == 0, "bus read hazard during cdc");
    sb_atomic_fetch_add(&bus->internal.reader_guard, -1, memory_order_relaxed);
#endif
    sb_atomic_fetch_add(&bus->internal.read_hazard_count, -1, memory_order_release);
}

static bool atomic_test_and_set(sb_atomic_int32_t * const x, const int set, const int test, const memory_order_e order) {
    return (sb_atomic_fetch_or(x, set, order) & (set | test)) == 0;
}

static int atomic_clear(sb_atomic_int32_t * const x, const int y, const memory_order_e order) {
    return sb_atomic_fetch_and(x, ~y, order);
}

// sets a connect/disconnect hazard (meaning the receivers array is changing).
// this will not return until the hazard is set and there are no read_hazards.
static void bus_wait_enter_cdc_hazard(cncbus_t * const bus) {
    sb_atomic_store(&bus->internal.receivers_hazard, bus_hazard_cdc, memory_order_relaxed);
    while (sb_atomic_load(&bus->internal.read_hazard_count, memory_order_acquire) != 0) {
    }
#ifdef _CNCBUS_DEBUG_GUARDS
    VERIFY_MSG(sb_atomic_load(&bus->internal.reader_guard, memory_order_relaxed) == 0, "bus cdc hazard during read");
    sb_atomic_fetch_add(&bus->internal.writer_guard, 1, memory_order_relaxed);
#endif
}

static void bus_exit_cdc_hazard(cncbus_t * const bus) {
#ifdef _CNCBUS_DEBUG_GUARDS
    VERIFY_MSG(sb_atomic_load(&bus->internal.reader_guard, memory_order_relaxed) == 0, "bus cdc hazard during read");
    sb_atomic_fetch_add(&bus->internal.writer_guard, -1, memory_order_relaxed);
#endif
    sb_atomic_store(&bus->internal.receivers_hazard, 0, memory_order_release);
}

static cncbus_msg_frag_t * bus_get_free_msg_frag(cncbus_t * const bus) {
    // grab head element hazard
    while (sb_atomic_cas(&bus->internal.free_msg_chain_hazard, 1, 0, memory_order_acquire) != 0) {
    }

    cncbus_msg_frag_t * const frag = bus->internal.free_msg_frag_chain;

    if (frag) {
        bus->internal.free_msg_frag_chain = frag->next;
    }

    sb_atomic_store(&bus->internal.free_msg_chain_hazard, 0, memory_order_release);

    return frag;
}

static void bus_release_msg_frag_chain(cncbus_t * const bus, cncbus_msg_frag_t * const head, cncbus_msg_frag_t * const tail) {
    // grab head element hazard
    while (sb_atomic_cas(&bus->internal.free_msg_chain_hazard, 1, 0, memory_order_acquire) != 0) {
    }

    tail->next = bus->internal.free_msg_frag_chain;
    bus->internal.free_msg_frag_chain = head;

    sb_atomic_store(&bus->internal.free_msg_chain_hazard, 0, memory_order_release);
}

static cncbus_receiver_msg_chain_t * bus_get_free_msg_chain(cncbus_t * const bus) {
    while (sb_atomic_cas(&bus->internal.free_receiver_msg_chain_hazard, 1, 0, memory_order_acquire) != 0) {
    }

    cncbus_receiver_msg_chain_t * const chain = bus->internal.free_receiver_msg_chain;

    if (chain) {
        // store next
        bus->internal.free_receiver_msg_chain = chain->next;
    }

    sb_atomic_store(&bus->internal.free_receiver_msg_chain_hazard, 0, memory_order_release);

    return chain;
}

static void bus_release_msg_chain(cncbus_t * const bus, cncbus_receiver_msg_chain_t * msg) {
    cncbus_receiver_msg_chain_t * next;
    for (; msg; msg = next) {
        next = msg->next;

        check_msg_guard(&msg->msg_data->cursor);

        if (sb_atomic_fetch_add(&msg->msg_data->ref_count, -1, memory_order_relaxed) == 1) {
            // signal requested
            if (msg->msg_data->signal) {
                sb_lock_mutex(msg->msg_data->signal->mutex);
                msg->msg_data->signal->signaled = 1;
                // ideally this wake would be outside the mutex to
                // prevent a wake-sleep event occuring, but that risks
                // the signal data or condition being destroyed
                // when we try and access it outside the mutex
                sb_condition_wake_all(msg->msg_data->signal->condition);
                sb_unlock_mutex(msg->msg_data->signal->mutex);
            }

            bus_release_msg_frag_chain(bus, msg->msg_data->frag_head, msg->msg_data->frag_tail);
        }

        // grab head element hazard
        while (sb_atomic_cas(&bus->internal.free_receiver_msg_chain_hazard, 1, 0, memory_order_acquire) != 0) {
        }

        msg->next = bus->internal.free_receiver_msg_chain;
        bus->internal.free_receiver_msg_chain = msg;

        sb_atomic_store(&bus->internal.free_receiver_msg_chain_hazard, 0, memory_order_release);
    }
}

cncbus_dispatch_result_e cncbus_dispatch(cncbus_t * const bus, const cncbus_dispatch_mode_e mode) {
    if (!bus_try_enter_read_hazard(bus)) {
        return cncbus_dispatch_busy;
    }

    cncbus_dispatch_result_e result = cncbus_dispatch_no_messages;

    int num_receivers = bus->internal.num_receivers;
    bool all_busy = num_receivers > 0;
    bool read_hazard = true;

    for (int i = 0; i < num_receivers; ++i) {
        const int idx = sb_atomic_fetch_add(&bus->internal.next_dispatch_slot, 1, memory_order_relaxed) % num_receivers;
        cncbus_receiver_t * const receiver = bus->internal.receivers[idx];

        // try and get dispatch access to the receiver
        if (atomic_test_and_set(&receiver->internal.hazard, receiver_hazard_dispatch, 0, memory_order_relaxed)) {
            all_busy = false;

            // safe for other threads to modify receivers[]
            bus_exit_read_hazard(bus);
            read_hazard = false;

            // acquire message chain hazard
            while (!atomic_test_and_set(&receiver->internal.hazard, receiver_hazard_access_msg_chain, 0, memory_order_acquire)) {
            }

#ifdef _CNCBUS_DEBUG_GUARDS
            VERIFY(sb_atomic_fetch_add(&receiver->internal.access_count, 1, memory_order_relaxed) == 0);
#endif

            cncbus_receiver_msg_chain_t * const msg = receiver->internal.chain_head;
            if (msg) {
                if (mode == cncbus_dispatch_flush) {
                    // unhook all pending messages
                    receiver->internal.chain_head = receiver->internal.chain_tail = NULL;
                } else {
                    receiver->internal.chain_head = msg->next;
                    if (!msg->next) {
                        receiver->internal.chain_tail = NULL;
                    }
                    msg->next = NULL;
                }

                // clear message chain access hazard, we won't access it further.

#ifdef _CNCBUS_DEBUG_GUARDS
                VERIFY(sb_atomic_fetch_add(&receiver->internal.access_count, -1, memory_order_relaxed) == 1);
#endif
                atomic_clear(&receiver->internal.hazard, receiver_hazard_access_msg_chain, memory_order_release);

                // dispatch each pending message serially to the receiver
                cncbus_receiver_msg_chain_t * next;
                for (cncbus_receiver_msg_chain_t * chain = msg; chain; chain = next) {
                    next = chain->next;
                    chain->next = NULL;

                    cncbus_msg_t cursor;
                    ZEROMEM(&cursor);
                    cursor.internal.msg_data = chain->msg_data;
                    cursor.internal.frag_ofs = first_msg_frag_ofs;
                    cursor.internal.frag = chain->msg_data->frag_head;

                    check_msg_guard(&chain->msg_data->cursor);
                    receiver->internal.vtable->on_msg_recv(receiver, chain->msg_data->header, &cursor);
                    bus_release_msg_chain(bus, chain);
                }

                result = cncbus_dispatch_ok;

                // release all hazards
                atomic_clear(&receiver->internal.hazard, receiver_hazard_dispatch, memory_order_relaxed);
            } else {
#ifdef _CNCBUS_DEBUG_GUARDS
                VERIFY(sb_atomic_fetch_add(&receiver->internal.access_count, -1, memory_order_relaxed) == 1);
#endif
                // release all hazards
                atomic_clear(&receiver->internal.hazard, receiver_hazard_dispatch | receiver_hazard_access_msg_chain, memory_order_release);
            }

            if (msg && (mode == cncbus_dispatch_single_message)) {
                break;
            }

            if (!bus_try_enter_read_hazard(bus)) {
                return cncbus_dispatch_busy;
            }

            num_receivers = bus->internal.num_receivers;
            read_hazard = true;
        }
    }

    if (read_hazard) {
        bus_exit_read_hazard(bus);
    }

    return all_busy ? cncbus_dispatch_busy : result;
}

void cncbus_init_receiver(cncbus_receiver_t * const receiver, const cncbus_receiver_vtable_t * const vtable, const cncbus_address_t address) {
    ZEROMEM(receiver);
    receiver->internal.vtable = vtable;
    receiver->internal.address = address;
}

void cncbus_connect(cncbus_t * const bus, cncbus_receiver_t * const receiver) {
#ifndef NDEBUG
    for (int i = 0; i < bus->internal.num_receivers; ++i) {
        ASSERT_MSG(bus->internal.receivers[i] != receiver, "bus receiver connected multiple times");
    }
#endif

    VERIFY(bus->internal.num_receivers < cncbus_max_receivers);

    receiver->internal.hazard.i32 = 0;
    receiver->internal.chain_head = NULL;
    receiver->internal.chain_tail = NULL;

#ifdef _CNCBUS_DEBUG_GUARDS
    receiver->internal.access_count.i32 = 0;
#endif

    cncbus_receiver_t ** const start = &bus->internal.receivers[0];
    cncbus_receiver_t ** const end = &bus->internal.receivers[bus->internal.num_receivers];
    cncbus_receiver_t ** const lb = lower_bound_cncbus_receiver_ptr_t(start, end, receiver);
    ASSERT(lb >= start);
    ASSERT(lb <= end);

    const int ofs = (int)(lb - start);

    bus_wait_enter_cdc_hazard(bus);

    // shift forward
    if (lb != end) {
        const int num_to_move = (int)(end - lb);
        memmove(lb + 1, lb, sizeof(cncbus_receiver_t **) * num_to_move);
        memmove(&bus->internal.receiver_addresses[ofs + 1], &bus->internal.receiver_addresses[ofs], sizeof(cncbus_address_t) * num_to_move);
    }

    *lb = receiver;
    bus->internal.receiver_addresses[ofs] = receiver->internal.address;
    ++bus->internal.num_receivers;

    bus_exit_cdc_hazard(bus);
}

void cncbus_disconnect(cncbus_t * const bus, cncbus_receiver_t * const receiver) {
    sb_atomic_fetch_or(&receiver->internal.hazard, receiver_hazard_destroyed, memory_order_relaxed);

    cncbus_receiver_t ** const start = &bus->internal.receivers[0];
    cncbus_receiver_t ** const end = &bus->internal.receivers[bus->internal.num_receivers];
    cncbus_receiver_t ** lb = lower_bound_cncbus_receiver_ptr_t(start, end, receiver);
    ASSERT(lb >= start);
    ASSERT(lb < end);

    while (*lb != receiver) {
        ++lb;
    }

    ASSERT(lb >= start);
    const int ofs = (int)(lb - start);

    bus_wait_enter_cdc_hazard(bus);

    if (lb + 1 < end) {
        const int num_to_move = (int)(end - lb);
        memmove(lb, lb + 1, sizeof(cncbus_receiver_t **) * num_to_move);
        memmove(&bus->internal.receiver_addresses[ofs], &bus->internal.receiver_addresses[ofs + 1], sizeof(cncbus_address_t) * num_to_move);
    }

    --bus->internal.num_receivers;

    bus_exit_cdc_hazard(bus);

    // wait for read hazards to clear
    while (sb_atomic_cas(&receiver->internal.hazard, receiver_hazard_destroyed, receiver_hazard_destroyed, memory_order_acquire) != receiver_hazard_destroyed) {
    }

    // read hazards are clear, this receiver is now disconnected from the bus
    // return any unprocessed messages fragments

    if (receiver->internal.chain_head) {
        bus_release_msg_chain(bus, receiver->internal.chain_head);
    }
}

cncbus_msg_t * cncbus_msg_begin_unchecked(cncbus_t * const bus, const uint32_t msg_type) {
    cncbus_msg_frag_t * const frag = bus_get_free_msg_frag(bus);
    if (!frag) {
        return NULL;
    }

    frag->next = NULL;

    cncbus_msg_data_t * const msg_data = (cncbus_msg_data_t *)frag->data;
    ASSERT(IS_ALIGNED(msg_data, ALIGN_OF(cncbus_msg_data_t)));
    ZEROMEM(msg_data);

    set_msg_guard(&msg_data->cursor);
    msg_data->bus = bus;
    msg_data->signal = NULL;
    msg_data->frag_head = msg_data->frag_tail = frag;
    msg_data->reserved = first_msg_frag_size;
    msg_data->cursor.internal.msg_data = msg_data;
    msg_data->cursor.internal.frag = frag;
    msg_data->cursor.internal.frag_ofs = first_msg_frag_ofs;
    msg_data->header.msg_type = msg_type;

    check_msg_guard(&msg_data->cursor);

    return &msg_data->cursor;
}

void cncbus_msg_cancel(cncbus_msg_t * const msg) {
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
    check_msg_guard(msg);
    ASSERT_MSG(msg == &msg_data->cursor, "attempt to modify immutable message data!");
    bus_release_msg_frag_chain(msg_data->bus, msg_data->frag_head, msg_data->frag_tail);
}

bool cncbus_msg_reserve_unchecked(cncbus_msg_t * const msg, const int size) {
    check_msg_guard(msg);

    // allocate fragments to cover this reserved size
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
    ASSERT_MSG(msg == &msg_data->cursor, "attempt to modify immutable message data!");

    while (msg_data->reserved < size) {
        cncbus_msg_frag_t * frag = bus_get_free_msg_frag(msg_data->bus);
        if (!frag) {
            return false;
        }
        frag->next = NULL;
        check_msg_guard(msg);
        if (msg_data->frag_tail) {
            ASSERT(msg_data->frag_head);
            msg_data->frag_tail->next = frag;
        } else {
            ASSERT(!msg_data->frag_head);
            ASSERT(!msg_data->frag_tail);
            msg_data->frag_head = frag;
        }
        msg_data->frag_tail = frag;
        msg_data->reserved += message_fragment_size;
    }

    return true;
}

bool cncbus_msg_write_unchecked(cncbus_msg_t * const msg, const void * const src, const int size) {
    check_msg_guard(msg);
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
    ASSERT_MSG(msg == &msg_data->cursor, "attempt to modify immutable message data!");

    const uint8_t * const src_bytes = src;

    ASSERT(msg->internal.cursor >= 0);
    if (!cncbus_msg_reserve_unchecked(msg, msg->internal.cursor + size)) {
        return false;
    }

    int written = 0;
    while (written < size) {
        ASSERT(msg->internal.frag);
        const int frag_bytes_left = message_fragment_size - msg->internal.frag_ofs;
        ASSERT(frag_bytes_left >= 0);

        if (frag_bytes_left > 0) {
            const int bytes_to_write = min_int(frag_bytes_left, size - written);
            ASSERT(bytes_to_write > 0);
            memcpy(&msg->internal.frag->data[msg->internal.frag_ofs], src_bytes + written, bytes_to_write);
            written += bytes_to_write;
            msg->internal.frag_ofs += bytes_to_write;
        } else {
            msg->internal.frag_ofs = 0;
            msg->internal.frag = msg->internal.frag->next;
            ASSERT(msg->internal.frag);
        }
    }

    ASSERT(written == size);
    msg->internal.cursor += written;
    msg_data->header.msg_size = max_uint32_t(msg_data->header.msg_size, (uint32_t)msg->internal.cursor);
    return true;
}

int cncbus_msg_read(cncbus_msg_t * const msg, void * const dst, const int size) {
    check_msg_guard(msg);
    uint8_t * const dst_bytes = dst;

    const int bytes_to_read = min_int(size, msg->internal.msg_data->header.msg_size - msg->internal.cursor);

    int bytes_read = 0;
    while (bytes_read < bytes_to_read) {
        ASSERT(msg->internal.frag);
        const int frag_bytes_left = message_fragment_size - msg->internal.frag_ofs;
        ASSERT(frag_bytes_left >= 0);

        if (frag_bytes_left > 0) {
            const int read_size = min_int(frag_bytes_left, bytes_to_read - bytes_read);
            ASSERT(read_size > 0);
            memcpy(dst_bytes + bytes_read, &msg->internal.frag->data[msg->internal.frag_ofs], read_size);
            msg->internal.frag_ofs += read_size;
            bytes_read += read_size;
        } else {
            // advance to next fragment
            msg->internal.frag_ofs = 0;
            msg->internal.frag = msg->internal.frag->next;
            if (!msg->internal.frag) {
                break;
            }
        }
    }

    ASSERT(bytes_read <= bytes_to_read);

    msg->internal.cursor += bytes_read;
    return bytes_read;
}

int cncbus_msg_get_size(cncbus_msg_t * const msg) {
    check_msg_guard(msg);
    return msg->internal.msg_data->header.msg_size;
}

bool cncbus_msg_set_size_unchecked(cncbus_msg_t * const msg, int size) {
    check_msg_guard(msg);
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
    ASSERT_MSG(msg == &msg_data->cursor, "attempt to send message that wasn't created from cncbus_msg_begin()!");
    ASSERT(size >= 0);
    if (!cncbus_msg_reserve_unchecked(msg, size)) {
        return false;
    }
    msg_data->header.msg_size = size;
    return true;
}

int cncbus_msg_tell(cncbus_msg_t * const msg) {
    check_msg_guard(msg);
    return msg->internal.cursor;
}

int cncbus_msg_seek(cncbus_msg_t * const msg, const int ofs, const cncbus_msg_seek_e seek_mode) {
    check_msg_guard(msg);
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
    const int msg_size = (int)msg_data->header.msg_size;

    int cursor;

    switch (seek_mode) {
        case cncbus_msg_seek_cur:
            cursor = msg->internal.cursor + ofs;
            break;
        case cncbus_msg_seek_set:
            cursor = ofs;
            break;
        default:
            VERIFY(seek_mode == cncbus_msg_seek_end);
            ASSERT(ofs <= 0);
            cursor = msg_size + ofs;
            break;
    }

    if ((cursor < 0) || (cursor > msg_size)) {
        return -1;
    }

    // fast cursor seek (within same fragment)
    const int cursor_delta = cursor - msg->internal.cursor;
    const int frag_ofs = msg->internal.frag_ofs + cursor_delta;
    if (msg->internal.frag == msg->internal.msg_data->frag_head) { // first frag is smaller
        ASSERT(frag_ofs >= first_msg_frag_ofs);
        if ((frag_ofs < first_msg_frag_size) || ((frag_ofs == first_msg_frag_size) && (cursor == msg_size))) {
            msg->internal.frag_ofs = frag_ofs;
            msg->internal.cursor = cursor;
            return cursor;
        }
    } else if ((frag_ofs >= 0) && ((frag_ofs < message_fragment_size) || ((frag_ofs == message_fragment_size) && (cursor == msg_size)))) {
        msg->internal.frag_ofs = frag_ofs;
        msg->internal.cursor = cursor;
        return cursor;
    }

    // seek off current fragment

    msg->internal.cursor = 0;
    msg->internal.frag = msg->internal.msg_data->frag_head;
    msg->internal.frag_ofs = ALIGN_INT(sizeof(cncbus_msg_t), 8);

    int fragment_size = first_msg_frag_size;

    while ((msg->internal.cursor + fragment_size) < cursor) {
        msg->internal.frag = msg->internal.frag->next;
        msg->internal.frag_ofs = 0;
        msg->internal.cursor += fragment_size;
        ASSERT(msg->internal.frag);
        fragment_size = message_fragment_size;
    }

    const int cursor_ofs = cursor - msg->internal.cursor;
    ASSERT(cursor_ofs >= 0);
    msg->internal.frag_ofs += cursor_ofs;
    ASSERT((msg->internal.frag_ofs < fragment_size) || ((msg->internal.frag_ofs == fragment_size) && (cursor == msg_size)));
    msg->internal.cursor = cursor;
    return cursor;
}

// must be done inside a read hazard
static int collect_receivers(
    cncbus_receiver_t * const receiver_table[cncbus_max_receivers],
    const cncbus_address_t receiver_address_table[cncbus_max_receivers],
    const int num_receivers,
    cncbus_receiver_t * out_matched_receiver_table[cncbus_max_receivers],
    const cncbus_address_t address,
    const cncbus_address_t subnet_mask) {
    if (address.u32) {
        const uint32_t masked_address = address.u32 & subnet_mask.u32;

        int num_matched = 0;
        for (int i = 0; i < num_receivers; ++i) {
            const cncbus_address_t receiver_address = receiver_address_table[i];

            // because this array is sorted the array can't match
            // any further receivers.

            if (subnet_mask.u32 < receiver_address.u32) {
                break;
            }

            if ((receiver_address.u32 & subnet_mask.u32) == masked_address) {
                cncbus_receiver_t * const receiver = receiver_table[i];
                out_matched_receiver_table[num_matched++] = receiver;
            }
        }

        return num_matched;
    }

    // broadcast address
    memcpy(out_matched_receiver_table, receiver_table, sizeof(cncbus_receiver_t *) * num_receivers);
    return num_receivers;
}

void cncbus_send_async(cncbus_msg_t * const msg, const cncbus_address_t source_address, const cncbus_address_t dest_address, const cncbus_address_t subnet_mask, cncbus_signal_t * const signal) {
    check_msg_guard(msg);
    ASSERT_MSG(msg == &msg->internal.msg_data->cursor, "attempt to send message that wasn't created from cncbus_msg_begin()!");
    cncbus_msg_data_t * const msg_data = msg->internal.msg_data;
#ifdef _CNCBUS_DEBUG_GUARDS
    msg_data->cursor.internal.frag = NULL;
    msg_data->cursor.internal.msg_data = NULL;
    msg_data->cursor.internal.cursor = 0;
    msg_data->cursor.internal.frag_ofs = 0;
#else
    ZEROMEM(&msg_data->cursor);
#endif
    cncbus_t * const bus = msg_data->bus;

    if (signal) {
        signal->signaled = 0;
    }

    msg_data->signal = signal;
    msg_data->header.msg_reply_address = source_address;
    msg_data->header.msg_dest_address = dest_address;
    msg_data->header.msg_dest_subnet = subnet_mask;
    msg_data->header.msg_time = adk_read_millisecond_clock().ms;

    while (!bus_try_enter_read_hazard(bus)) {
    }

    cncbus_receiver_t * pending[cncbus_max_receivers];

    int num_pending = collect_receivers(bus->internal.receivers, bus->internal.receiver_addresses, bus->internal.num_receivers, pending, dest_address, subnet_mask);

    if (num_pending < 1) {
        bus_exit_read_hazard(bus);
        bus_release_msg_frag_chain(bus, msg_data->frag_head, msg_data->frag_tail);
        return;
    }

    msg_data->ref_count.i32 = 1;

    cncbus_receiver_msg_chain_t * chain = NULL;

    while (num_pending > 0) {
        // make a pass through the pending list
        // if the receiver was latched enqueue the message and remove it from pending.
        // if the receiver was destroyed remove it from pending.

        for (int i = 0; i < num_pending;) {
            bool remove = false;

            cncbus_receiver_t * const receiver = pending[i];

            // grab another message link, do this outside of the msg_chain access hazard
            if (!chain) {
                chain = bus_get_free_msg_chain(bus);
                if (chain == NULL) {
                    // there are no more free message chains, run dispatch pump
                    // to try and free one up.
                    do {
                        cncbus_dispatch(bus, cncbus_dispatch_single_message);
                        chain = bus_get_free_msg_chain(bus);
                    } while (chain == NULL);
                }
                chain->next = NULL;
                chain->msg_data = msg_data;
                sb_atomic_fetch_add(&msg_data->ref_count, 1, memory_order_relaxed);
            }

            const int flags = sb_atomic_fetch_or(&receiver->internal.hazard, receiver_hazard_access_msg_chain, memory_order_acquire);
            const bool locked = !(flags & receiver_hazard_access_msg_chain);

#ifdef _CNCBUS_DEBUG_GUARDS
            if (locked) {
                VERIFY(sb_atomic_fetch_add(&receiver->internal.access_count, 1, memory_order_relaxed) == 0);
            }
#endif

            if (flags & receiver_hazard_destroyed) {
                remove = true;
            } else if (locked) {
                remove = true;

                if (receiver->internal.chain_tail) {
                    receiver->internal.chain_tail->next = chain;
                } else {
                    ASSERT(!receiver->internal.chain_head);
                    receiver->internal.chain_head = chain;
                }

                receiver->internal.chain_tail = chain;
                chain = NULL;
            }

            if (remove) {
                if (locked) {
#ifdef _CNCBUS_DEBUG_GUARDS
                    VERIFY(sb_atomic_fetch_add(&receiver->internal.access_count, -1, memory_order_relaxed) == 1);
#endif
                    // clear access to allow destroy to proceed
                    sb_atomic_fetch_and(&receiver->internal.hazard, ~receiver_hazard_access_msg_chain, (flags & receiver_hazard_destroyed) ? memory_order_relaxed : memory_order_release);
                }

                // compact
                if (i < num_pending - 1) {
                    memmove(&pending[i], &pending[i + 1], sizeof(cncbus_receiver_t *) * (num_pending - i));
                }

                --num_pending;
            } else {
                ++i;
            }
        }
    }

    bus_exit_read_hazard(bus);

    if (chain) {
        // free dangling reference
        bus_release_msg_chain(bus, chain);
    }

    if (sb_atomic_fetch_add(&msg_data->ref_count, -1, memory_order_relaxed) == 1) {
        bus_release_msg_frag_chain(bus, msg_data->frag_head, msg_data->frag_tail);
    }
}
