/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
cncbus.h

Command & Control Bus

Unified message routing and delivery.
*/

#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef GUARD_PAGE_SUPPORT
#include "source/adk/runtime/memory.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================================================================
CNC bus overview

An asynchronous message queue, dispatch, and delivery system with minimal
dependencies. The bus component can be used with only runtime being initialized.

The bus routes messages to receivers by address. Message delivery and order is
guaranteed. Bus addressing supports one-to-many (broadcast) and many-to-one.

A connected receiver will receive messages serially so the receiver itself
will not ever be invoked from multiple threads simultaneously. However no
guarantee is made about the thread that a receiver dispatch will be executed on.
===============================================================================
*/

#ifndef _SHIP
// enabling this causes severe performance penalty
//#define _CNCBUS_DEBUG_GUARDS
#endif

#define CNCBUS_BROADCAST_ADDRESS \
    (cncbus_address_t) {         \
        .u32 = 0                 \
    }
#define CNCBUS_MAKE_ADDRESS(_a, _b, _c, _d) \
    (cncbus_address_t) {                    \
        .u32 = FOURCC(_d, _c, _b, _a)       \
    }
#define CNCBUS_INVALID_ADDRESS \
    (cncbus_address_t) {       \
        .u32 = UINT32_MAX      \
    }

enum {
    cncbus_max_receivers = 10
};

typedef enum cncbus_dispatch_mode_e {
    cncbus_dispatch_single_message,
    cncbus_dispatch_flush
} cncbus_dispatch_mode_e;

typedef enum cncbus_dispatch_result_e {
    cncbus_dispatch_no_messages, // no pending messages
    cncbus_dispatch_busy, // messages pending but all receivers are busy
    cncbus_dispatch_ok // one or more messages were dispatched
} cncbus_dispatch_result_e;

// see cncbus_msg_seek/cncbus_msg_tell
typedef enum cncbus_msg_seek_e {
    cncbus_msg_seek_set,
    cncbus_msg_seek_cur,
    cncbus_msg_seek_end
} cncbus_msg_seek_e;

typedef struct cncbus_msg_data_t cncbus_msg_data_t;
typedef struct cncbus_msg_frag_t cncbus_msg_frag_t;

// contains msg cursor and data
typedef struct cncbus_msg_t {
    struct {
#ifdef _CNCBUS_DEBUG_GUARDS
        uint8_t guard0[4];
#endif
        cncbus_msg_data_t * msg_data;
        cncbus_msg_frag_t * frag;
        int cursor;
        int frag_ofs;
#ifdef _CNCBUS_DEBUG_GUARDS
        uint8_t guard1[4];
#endif
    } internal;
} cncbus_msg_t;

// Bus addresses are modeled after IP addresses
// for routing flexibility. They are, however,
// NOT IP addresses and have nothing to do with
// sending data over a network.
typedef union cncbus_address_t {
    uint8_t u8[4];
    uint32_t u32;
} cncbus_address_t;

// All bus messages container this header.
typedef struct cncbus_msg_header_t {
    uint32_t msg_type;
    uint32_t msg_size;
    uint32_t msg_time;
    cncbus_address_t msg_reply_address;
    cncbus_address_t msg_dest_address;
    cncbus_address_t msg_dest_subnet;
} cncbus_msg_header_t;

// A receiver needs to provide the location of
// a cncbus_receiver_t that it owns, as well
// as a vtable for callbacks.
struct cncbus_receiver_t;
typedef struct cncbus_receiver_vtable_t {
    // non-zero return codes are errors
    int (*on_msg_recv)(struct cncbus_receiver_t * const self, const cncbus_msg_header_t header, cncbus_msg_t * const msg);
} cncbus_receiver_vtable_t;

// internal use by bus.
typedef struct cncbus_receiver_msg_chain_t {
    struct cncbus_receiver_msg_chain_t * next;
    cncbus_msg_data_t * msg_data;
#ifdef GUARD_PAGE_SUPPORT
    debug_sys_page_block_t guard_pages;
#endif
} cncbus_receiver_msg_chain_t;

// an instance of a receiver, managed by receiver author.
typedef struct cncbus_receiver_t {
    struct {
        cncbus_receiver_msg_chain_t *chain_head, *chain_tail;
        const cncbus_receiver_vtable_t * vtable;
        sb_atomic_int32_t hazard; // set when being accessed
#ifdef _CNCBUS_DEBUG_GUARDS
        sb_atomic_int32_t access_count;
#endif
        cncbus_address_t address;
    } internal;
} cncbus_receiver_t;

// bus signal
typedef struct cncbus_signal_t {
    sb_mutex_t * mutex;
    sb_condition_variable_t * condition;
    int signaled;
} cncbus_signal_t;

// cncbus object
typedef struct cncbus_t {
    struct {
        cncbus_msg_frag_t * free_msg_frag_chain;
        cncbus_receiver_msg_chain_t * free_receiver_msg_chain;
        sb_atomic_int32_t free_msg_chain_hazard;
        sb_atomic_int32_t free_receiver_msg_chain_hazard;
        sb_atomic_int32_t receivers_hazard;
        sb_atomic_int32_t read_hazard_count;
        sb_atomic_int32_t next_dispatch_slot;
#ifdef _CNCBUS_DEBUG_GUARDS
        // thread condition checks
        sb_atomic_int32_t reader_guard;
        sb_atomic_int32_t writer_guard;
#endif
        // sorted by address
        cncbus_receiver_t * receivers[cncbus_max_receivers];
        cncbus_address_t receiver_addresses[cncbus_max_receivers];
        int num_receivers;
        int alloc_msg_frags_count;
#ifdef GUARD_PAGE_SUPPORT
        bool guard_pages;
#endif
    } internal;
} cncbus_t;

/*
===============================================================================
CNC bus API
===============================================================================
*/

// initialize the bus object using the specified memory region for state.
EXT_EXPORT void cncbus_init(cncbus_t * const bus, const mem_region_t region, const system_guard_page_mode_e guard_pages);
// destroy is not thread safe: bus cannot be being accessed from any other thread
// during destroy.
EXT_EXPORT void cncbus_destroy(cncbus_t * const bus);

// dispatches messages to receivers, can be called on multiple threads simultaneously.
EXT_EXPORT cncbus_dispatch_result_e cncbus_dispatch(cncbus_t * const bus, const cncbus_dispatch_mode_e mode);

// initialize a receiver and bind it to a specific address.
EXT_EXPORT void cncbus_init_receiver(cncbus_receiver_t * const receiver, const struct cncbus_receiver_vtable_t * const vtable, const cncbus_address_t address);

// bus connect/disconnect only supports connecting or disconnect one receiver
// at a time, and is not thread-safe.
// the exception is message sending and dispatch can occur on other threads
// during connect/disconnect
EXT_EXPORT void cncbus_connect(cncbus_t * const bus, cncbus_receiver_t * const receiver);
EXT_EXPORT void cncbus_disconnect(cncbus_t * const bus, cncbus_receiver_t * const receiver);

// unchecked bus message functions can fail, allowing caller to gracefully handle
// bus saturation

// may return NULL if bus is saturated and out of message buffers.
EXT_EXPORT cncbus_msg_t * cncbus_msg_begin_unchecked(cncbus_t * const bus, const uint32_t msg_type);

// reserves space of "size" bytes in the message payload.
// this is not required, as cncbus_msg_write will grow the message if necessary.
// only valid during message construction.
EXT_EXPORT bool cncbus_msg_reserve_unchecked(cncbus_msg_t * const msg, const int size);

// writes data into the message from the messages current cursor position.
// advances the msg cursor.
// only valid during message construction.
// may return false if there are no available message fragment buffers
EXT_EXPORT bool cncbus_msg_write_unchecked(cncbus_msg_t * const msg, const void * const src, const int size);

// sets the size of the message, if necessary message buffer grows
// to accommodate. this is only valid during message construction.
// may return false if there are no available message fragment buffers
EXT_EXPORT bool cncbus_msg_set_size_unchecked(cncbus_msg_t * const msg, int size);

// sets the size of the message, if necessary message buffer grows
// to accommodate. this is only valid during message construction.
static void cncbus_msg_set_size_checked(cncbus_msg_t * const msg, int size) {
    VERIFY(cncbus_msg_set_size_unchecked(msg, size));
}

// constructs a new message. if a new message cannot be allocated a program error is generated
static cncbus_msg_t * cncbus_msg_begin_checked(cncbus_t * const bus, const uint32_t msg_type) {
    cncbus_msg_t * const msg = cncbus_msg_begin_unchecked(bus, msg_type);
    VERIFY_MSG(msg, "no free bus messages");
    return msg;
}

// cancels message construction
EXT_EXPORT void cncbus_msg_cancel(cncbus_msg_t * const msg);

// reserves space of "size" bytes in the message payload.
// this is not required, as cncbus_msg_write will grow the message if necessary.
// only valid during message construction.
static void cncbus_msg_reserve_checked(cncbus_msg_t * const msg, const int size) {
    VERIFY(cncbus_msg_reserve_unchecked(msg, size));
}

// writes data into the message from the messages current cursor position.
// advances the msg cursor.
// only valid during message construction.
static void cncbus_msg_write_checked(cncbus_msg_t * const msg, const void * const src, const int size) {
    VERIFY(cncbus_msg_write_unchecked(msg, src, size));
}

// reads data from the message from the messages current cursor position.
// advances the cursor.
// may return less than the requested size if end of message.
EXT_EXPORT int cncbus_msg_read(cncbus_msg_t * const msg, void * const dst, const int size);

// get the size of the message payload.
int cncbus_msg_get_size(cncbus_msg_t * const msg);

// returns the messages cursor position.
int cncbus_msg_tell(cncbus_msg_t * const msg);

// moves the messages cursor position.
int cncbus_msg_seek(cncbus_msg_t * const msg, const int ofs, const cncbus_msg_seek_e seek_mode);

// send a message to the specified destination address and subnet.
// message will be delivered at some point in the future.
EXT_EXPORT void cncbus_send_async(cncbus_msg_t * const msg, const cncbus_address_t source_address, const cncbus_address_t dest_address, const cncbus_address_t subnet_mask, cncbus_signal_t * const signal);

#ifdef __cplusplus
}
#endif
