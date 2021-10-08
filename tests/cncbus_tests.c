/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cncbus_tests.c

 Command & Control Bus test fixture
 */

#include "source/adk/cncbus/cncbus.h"
#include "testapi.h"

enum {
    bus_test_mem_size = 1024 * 1024 * 128,
    bus_test_mem_alignment = 8
};

typedef struct bus_test_receiver_t {
    cncbus_receiver_t base;
    sb_atomic_int32_t recv_guard;
    uint32_t recv_count;
    uint32_t recv_bytes;
} bus_test_receiver_t;

typedef struct bus_test_data_t {
    cncbus_t bus;
    bus_test_receiver_t receivers[cncbus_max_receivers];
    void * mem;
    void * aligned;
} bus_test_data_t;

static int on_msg_recv(cncbus_receiver_t * const _self, const cncbus_msg_header_t header, cncbus_msg_t * const msg) {
    bus_test_receiver_t * const self = (bus_test_receiver_t *)_self;
    //VERIFY_MSG(atomic_fetch_add(&self->recv_guard, 1, memory_order_relaxed) == 0, "Bus dispatch serialization violation: receiver dispatched message on multiple threads");

    ++self->recv_count;
    self->recv_bytes += header.msg_size;

    //bus_test_receiver_t * const self = (bus_test_receiver_t*)_self;
    //print_message("%i.%i.%i.%i: received [%d] bytes, time [%u], reply [%i.%i.%i.%i]\n", self->base.internal.address.u8[3], self->base.internal.address.u8[2], self->base.internal.address.u8[1], self->base.internal.address.u8[0], header.msg_size, header.msg_time, header.msg_reply_address.u8[3], header.msg_reply_address.u8[2], header.msg_reply_address.u8[1], header.msg_reply_address.u8[0]);

    //VERIFY_MSG(atomic_fetch_add(&self->recv_guard, -1, memory_order_relaxed) == 1, "Bus dispatch serialization violation: receiver dispatched message on multiple threads");
    return 0;
}

static cncbus_receiver_vtable_t test_receiver_vtable = {
    .on_msg_recv = on_msg_recv};

static const char lipsum[] = "On the other hand, we denounce with righteous indignation and dislike men who are so beguiled and demoralized by the charms of pleasure of the moment, so blinded by desire, that they cannot foresee the pain and trouble that are bound to ensue; and equal blame belongs to those who fail in their duty through weakness of will, which is the same as saying through shrinking from toil and pain.These cases are perfectly simple and easy to distinguish.In a free hour, when our power of choice is untrammelled and when nothing prevents our being able to do what we like best, every pleasure is to be welcomed and every pain avoided.But in certain circumstances and owing to the claims of duty or the obligations of business it will frequently occur that pleasures have to be repudiated and annoyances accepted.The wise man therefore always holds in these matters to this principle of selection : he rejects pleasures to secure other greater pleasures, or else he endures pains to avoid worse pains.";
static const int LIPSUM_SIZE = ARRAY_SIZE(lipsum);

static cncbus_msg_t * make_bus_msg_size_unchecked(cncbus_t * const bus, const int size) {
    cncbus_msg_t * const msg = cncbus_msg_begin_unchecked(bus, 0);
    if (!msg) {
        return NULL;
    }

    int size_left = size;
    if (size_left > 0) {
        for (;;) {
            if (size_left > LIPSUM_SIZE) {
                if (!cncbus_msg_write_unchecked(msg, lipsum, LIPSUM_SIZE)) {
                    cncbus_msg_cancel(msg);
                    return NULL;
                }
                size_left -= LIPSUM_SIZE;
            } else {
                const int rand_max = LIPSUM_SIZE - size_left;
                const int start = rand_int(0, rand_max);
                if (!cncbus_msg_write_unchecked(msg, &lipsum[start], size_left)) {
                    cncbus_msg_cancel(msg);
                    return NULL;
                }
                break;
            }
        }
    }

    return msg;
}

static cncbus_msg_t * make_bus_random_msg_unchecked(cncbus_t * const bus) {
    cncbus_msg_t * const msg = make_bus_msg_size_unchecked(bus, 32);
    return msg;
}

static cncbus_msg_t * make_bus_random_msg_checked(cncbus_t * const bus) {
    cncbus_msg_t * const msg = make_bus_msg_size_unchecked(bus, 32);
    VERIFY(msg);
    return msg;
}

static int bus_test_setup(void ** state) {
    bus_test_data_t * const test_data = malloc(sizeof(bus_test_data_t));

    if (!test_data) {
        print_message("cncbus_test_setup, out of memory (1)!");
        return -1;
    }

    ZEROMEM(test_data);

    test_data->mem = malloc(bus_test_mem_size + bus_test_mem_alignment - 1);

    if (!test_data->mem) {
        print_message("cncbus_test_setup, out of memory (2)!");
        return -1;
    }

    test_data->aligned = (void *)ALIGN_PTR(test_data->mem, bus_test_mem_alignment);

    cncbus_init(&test_data->bus, MEM_REGION(.ptr = test_data->aligned, .size = bus_test_mem_size), system_guard_page_mode_disabled);

    for (int i = 0; i < ARRAY_SIZE(test_data->receivers); ++i) {
        cncbus_init_receiver(&test_data->receivers[i].base, &test_receiver_vtable, CNCBUS_MAKE_ADDRESS(10, 10, 1, 1 + i));
    }

    *state = test_data;
    return 0;
}

static int bus_test_teardown(void ** state) {
    bus_test_data_t * const test_data = *state;

    cncbus_destroy(&test_data->bus);

    free(test_data->mem);
    free(test_data);
    return 0;
}

static void bus_connect(bus_test_data_t * const test_data, const int count) {
    ASSERT(count <= ARRAY_SIZE(test_data->receivers));

    for (int i = 0; i < ARRAY_SIZE(test_data->receivers); ++i) {
        test_data->receivers[i].recv_bytes = 0;
        test_data->receivers[i].recv_count = 0;
        test_data->receivers[i].recv_guard.i32 = 0;

        cncbus_connect(&test_data->bus, &test_data->receivers[i].base);
    }
}

static void bus_disconnect(bus_test_data_t * const test_data, const int count) {
    ASSERT(count <= ARRAY_SIZE(test_data->receivers));

    for (int i = 0; i < ARRAY_SIZE(test_data->receivers); ++i) {
        cncbus_disconnect(&test_data->bus, &test_data->receivers[i].base);
        VERIFY_MSG(sb_atomic_load(&test_data->receivers[i].recv_guard, memory_order_relaxed) == 0, "receiver disconnected during receive!");
    }
}

static void bus_connect_all(bus_test_data_t * const test_data) {
    bus_connect(test_data, ARRAY_SIZE(test_data->receivers));
}

static void bus_disconnect_all(bus_test_data_t * const test_data) {
    bus_disconnect(test_data, ARRAY_SIZE(test_data->receivers));
}

static void bus_test_connect_disconnect(void ** state) {
    bus_test_data_t * const test_data = *state;

    bus_connect_all(test_data);
    bus_disconnect_all(test_data);
}

static void bus_send_random_message_checked(cncbus_t * const bus, const cncbus_address_t dest_address, const cncbus_address_t subnet_mask, cncbus_signal_t * const signal) {
    cncbus_send_async(
        make_bus_random_msg_checked(bus),
        CNCBUS_INVALID_ADDRESS,
        dest_address,
        subnet_mask,
        signal);
}

static int bus_send_random_message_unchecked(cncbus_t * const bus, const cncbus_address_t dest_address, const cncbus_address_t subnet_mask, cncbus_signal_t * const signal) {
    cncbus_msg_t * const msg = make_bus_random_msg_unchecked(bus);
    int size = 0;

    if (msg) {
        size = cncbus_msg_get_size(msg);

        cncbus_send_async(
            msg,
            CNCBUS_INVALID_ADDRESS,
            dest_address,
            subnet_mask,
            signal);
    }

    return size;
}

static void bus_test_simple_pump(void ** state) {
    enum { message_count = 200000 };
    bus_test_data_t * const test_data = *state;
    cncbus_t * const bus = &test_data->bus;

    bus_connect_all(test_data);

    const microseconds_t start = adk_read_microsecond_clock();

    for (int i = 0; i < message_count; ++i) {
        // fix this
        bus_send_random_message_checked(
            bus,
            CNCBUS_MAKE_ADDRESS(10, 10, 1, 1 + (i % cncbus_max_receivers)),
            CNCBUS_MAKE_ADDRESS(255, 255, 255, 255),
            NULL);

        cncbus_dispatch(bus, cncbus_dispatch_flush);
    }

    const microseconds_t end = adk_read_microsecond_clock();

    print_message("bandwidth = %0.2f msgs per second\n", (float)((double)1000000 / (double)(end.us - start.us)) * (float)message_count);

    bus_disconnect_all(test_data);
}

static void bus_test_send_no_receivers(void ** state) {
    bus_test_data_t * const test_data = *state;
    cncbus_t * const bus = &test_data->bus;

    bus_send_random_message_checked(
        bus,
        CNCBUS_MAKE_ADDRESS(10, 10, 1, 1),
        CNCBUS_MAKE_ADDRESS(255, 255, 255, 255),
        NULL);
}

typedef enum bus_thread_worker_mode_e {
    bus_thread_worker_dispatch,
    bus_thread_worker_send,
    bus_thread_worker_flipflop
} bus_thread_worker_mode_e;

typedef struct bus_thread_worker_t {
    cncbus_t * bus;
    sb_thread_id_t thread_id;
    bus_thread_worker_mode_e mode;
    milliseconds_t tick_rate;
    int message_count;
    int send_count;
    int send_size;
    volatile bool run;
} bus_thread_worker_t;

static int bus_thread_proc(void * const arg) {
    bus_thread_worker_t * const worker = arg;

    switch (worker->mode) {
        case bus_thread_worker_dispatch:
            while (worker->run) {
                if (cncbus_dispatch(worker->bus, cncbus_dispatch_flush) != cncbus_dispatch_ok) {
                    // tick rate throttle if we didn't have anything to do
                    sb_thread_sleep(worker->tick_rate);
                }
            }
            break;
        case bus_thread_worker_send:
            while (worker->run && (worker->send_count < worker->message_count)) {
                const int size = bus_send_random_message_unchecked(worker->bus, CNCBUS_MAKE_ADDRESS(10, 10, 1, rand_int(1, cncbus_max_receivers)), CNCBUS_MAKE_ADDRESS(255, 255, 255, 255), NULL);
                if (size) {
                    ++worker->send_count;
                    worker->send_size += size;
                } else {
                    sb_thread_sleep(worker->tick_rate);
                }
            }
            VERIFY(!worker->run || (worker->message_count == worker->send_count));
            break;
        default: {
            ASSERT(worker->mode == bus_thread_worker_flipflop);

            int i = 0;
            while (worker->run) {
                bool rate_limit;

                if (i & 1) {
                    const int size = bus_send_random_message_unchecked(worker->bus, CNCBUS_MAKE_ADDRESS(10, 10, 1, rand_int(1, cncbus_max_receivers)), CNCBUS_MAKE_ADDRESS(255, 255, 255, 255), NULL);
                    if (size) {
                        ++worker->send_count;
                        worker->send_size += size;
                    }
                    rate_limit = true;
                } else {
                    rate_limit = cncbus_dispatch(worker->bus, cncbus_dispatch_flush) != cncbus_dispatch_ok;
                }

                if (rate_limit) {
                    sb_thread_sleep(worker->tick_rate);
                }

                ++i;
            }
        } break;
    }

    return 0;
}

static void start_worker_thread(cncbus_t * const bus, bus_thread_worker_t * const worker, const char * const name, const bus_thread_worker_mode_e mode, const milliseconds_t tick_rate, const int message_count) {
    ZEROMEM(worker);
    worker->mode = mode;
    worker->tick_rate = tick_rate;
    worker->message_count = message_count;
    worker->run = true;
    worker->bus = bus;

    worker->thread_id = sb_create_thread(name, sb_thread_default_options, bus_thread_proc, worker, MALLOC_TAG);
}

static void stop_and_join_worker_thread(bus_thread_worker_t * const worker) {
    worker->run = false;
    sb_join_thread(worker->thread_id);
}

static void bus_test_threaded_dispatch(void ** state) {
    enum { message_count = cncbus_max_receivers * 100000 };

    bus_test_data_t * const test_data = *state;
    cncbus_t * const bus = &test_data->bus;

    bus_connect_all(test_data);

    bus_thread_worker_t workers[4];

    for (int i = 0; i < ARRAY_SIZE(workers); ++i) {
        start_worker_thread(bus, &workers[i], "bus_dispatch", bus_thread_worker_dispatch, (milliseconds_t){0}, 0);
    }

    const microseconds_t start = adk_read_microsecond_clock();

    int num_sent = 0;
    int sent_bytes = 0;
    while (num_sent < message_count) {
        const int size = bus_send_random_message_unchecked(
            bus,
            CNCBUS_MAKE_ADDRESS(10, 10, 1, 1 + (num_sent % cncbus_max_receivers)),
            CNCBUS_MAKE_ADDRESS(255, 255, 255, 255),
            NULL);

        sent_bytes += size;

        if (size > 0) {
            ++num_sent;
        } else {
            cncbus_dispatch(bus, cncbus_dispatch_single_message);
        }
    }

    for (int i = 0; i < ARRAY_SIZE(workers); ++i) {
        stop_and_join_worker_thread(&workers[i]);
    }

    while (cncbus_dispatch(bus, cncbus_dispatch_flush) != cncbus_dispatch_no_messages) {
    }

    const microseconds_t end = adk_read_microsecond_clock();

    print_message("bandwidth = %0.2f msgs per second\n", (float)((double)1000000 / (double)(end.us - start.us)) * (float)message_count);

    int recv_bytes = 0;
    int recv_count = 0;
    for (int i = 0; i < ARRAY_SIZE(test_data->receivers); ++i) {
        recv_bytes += test_data->receivers[i].recv_bytes;
        recv_count += test_data->receivers[i].recv_count;
    }

    VERIFY(recv_count == message_count);
    VERIFY(recv_bytes == sent_bytes);

    bus_disconnect_all(test_data);
}

// returns messages per-second
static float bus_run_threaded_send_and_dispatch(bus_test_data_t * const test_data, const int num_send_threads, const int num_dispatch_threads, const int message_count_per_thread) {
    cncbus_t * const bus = &test_data->bus;

    bus_connect_all(test_data);

    bus_thread_worker_t * const dispatch_workers = ALLOCA(sizeof(bus_thread_worker_t) * num_dispatch_threads);

    for (int i = 0; i < num_dispatch_threads; ++i) {
        start_worker_thread(bus, &dispatch_workers[i], "bus_dispatch", bus_thread_worker_dispatch, (milliseconds_t){0}, 0);
    }

    bus_thread_worker_t * const send_workers = ALLOCA(sizeof(bus_thread_worker_t) * num_send_threads);

    const microseconds_t start = adk_read_microsecond_clock();

    for (int i = 0; i < num_send_threads; ++i) {
        start_worker_thread(bus, &send_workers[i], "bus_send", bus_thread_worker_send, (milliseconds_t){0}, message_count_per_thread);
    }

    // send threads will exit after sending all their messages
    for (int i = 0; i < num_send_threads; ++i) {
        sb_join_thread(send_workers[i].thread_id);
    }

    while (cncbus_dispatch(bus, cncbus_dispatch_flush) != cncbus_dispatch_no_messages) {
    }

    const microseconds_t end = adk_read_microsecond_clock();

    for (int i = 0; i < num_dispatch_threads; ++i) {
        stop_and_join_worker_thread(&dispatch_workers[i]);
    }

    int sent_message_count = 0;
    int sent_byte_count = 0;
    for (int i = 0; i < num_send_threads; ++i) {
        sent_message_count += send_workers[i].send_count;
        sent_byte_count += send_workers[i].send_size;
    }

    VERIFY(sent_message_count == (message_count_per_thread * num_send_threads));

    int recv_message_count = 0;
    int recv_byte_count = 0;

    for (int i = 0; i < ARRAY_SIZE(test_data->receivers); ++i) {
        recv_message_count += test_data->receivers[i].recv_count;
        recv_byte_count += test_data->receivers[i].recv_bytes;
    }

    VERIFY(sent_message_count == recv_message_count);
    VERIFY(sent_byte_count == recv_byte_count);

    bus_disconnect_all(test_data);

    return (float)((double)1000000 / (double)(end.us - start.us)) * (float)sent_message_count;
}

static void bus_test_threaded_send_and_dispatch(void ** state) {
    bus_test_data_t * const test_data = *state;
    const float messages_per_second = bus_run_threaded_send_and_dispatch(test_data, 2, 1, 200000);
    print_message("bandwidth = %0.2f msgs per second\n", messages_per_second);
}

int test_cncbus() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(bus_test_connect_disconnect, NULL, NULL),
        cmocka_unit_test_setup_teardown(bus_test_send_no_receivers, NULL, NULL),
        cmocka_unit_test_setup_teardown(bus_test_simple_pump, NULL, NULL),
        cmocka_unit_test_setup_teardown(bus_test_threaded_dispatch, NULL, NULL),
        cmocka_unit_test_setup_teardown(bus_test_threaded_send_and_dispatch, NULL, NULL)

    };

    return cmocka_run_group_tests(tests, bus_test_setup, bus_test_teardown);
}