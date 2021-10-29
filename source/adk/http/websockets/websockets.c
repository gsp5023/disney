/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/websockets/websockets.h"

#include "extern/curl/curl/include/curl/curl.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/sha1.h"
#include "source/adk/http/private/adk_curl_common.h"
#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/http/websockets/base64_encode.h"
#include "source/adk/http/websockets/private/websocket_constants.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/rand_gen.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/telemetry/telemetry.h"

#include <ctype.h>

#define _WS_SHIM_SUPPORT

#define WS_TAG FOURCC('W', 'S', 'C', 'K')

typedef enum ws_op_code_e {
    ws_op_continuation = 0x0,
    ws_op_text = 0x1,
    ws_op_binary = 0x2,
    ws_op_close = 0x8,
    ws_op_ping = 0x9,
    ws_op_pong = 0xA,
} ws_op_code_e;

typedef struct websocket_received_message_t {
    struct websocket_received_message_t * next;
    struct websocket_received_message_t * prev;

    mem_region_t region;
    websocket_message_type_e type;
    bool complete;
} websocket_received_message_t;

typedef struct websocket_send_message_t {
    struct websocket_send_message_t * next;
    struct websocket_send_message_t * prev;
    mem_region_t region;
    ws_op_code_e op_code;
    size_t total_sent;
} websocket_send_message_t;

typedef struct ws_frame_t {
    uint32_t fin : 1;
    uint32_t rsv1 : 1;
    uint32_t rsv2 : 1;
    uint32_t rsv3 : 1;
    uint32_t op_code : 4;
    uint32_t masked : 1;
    uint64_t payload_len;
    uint32_t mask;
} ws_frame_t;

typedef struct packet_t {
    mem_region_t region;
    size_t amount_received;
    bool incomplete;
} packet_t;

typedef struct http_headers_t {
    mem_region_t region;
    size_t header_bytes_sent;
    size_t end_of_headers;
    int32_t num_headers;
} http_headers_t;

typedef enum connection_io_error_e {
    connection_io_success = 0,
    connection_io_would_block = -1,
    connection_io_unrecoverable_error = -2,
} connection_io_error_e;

typedef enum handshake_headers_status_e {
    handshake_headers_init,
    handshake_headers_sent,
    handshake_headers_received,
} handshake_headers_status_e;

struct websocket_client_t {
    xoroshiro256plusplus_t rand_gen;

    curl_common_certs_t certs;
    adk_curl_context_t * curl_ctx;
    CURLM * curl_multi;

    heap_t * heap;

    websocket_t * ws_list_head;
    websocket_t * ws_list_tail;
};

typedef enum curl_connect_only_status_e {
    curl_status_pending,
    curl_status_ready
} curl_connect_only_status_e;

typedef enum curl_connect_only_result_e {
    curl_result_ok,
    curl_result_timeout,
    curl_result_error,
} curl_connect_only_result_e;

typedef struct curl_connect_only_handle_t {
    curl_connect_only_status_e status;
    curl_connect_only_result_e result;
    CURL * curl_handle;
} curl_connect_only_handle_t;

struct websocket_t {
    struct websocket_t * prev;
    struct websocket_t * next;

    websocket_received_message_t * recv_msg_head;
    websocket_received_message_t * recv_msg_tail;

    websocket_send_message_t * send_msg_head;
    websocket_send_message_t * send_msg_tail;

    websocket_client_t * client;

    uint64_t id;
    struct connection_t {
        curl_connect_only_handle_t * connect_only_handle;
        char * url;
        sb_socket_t socket;
        milliseconds_t connection_start_timestamp;
        milliseconds_t max_handshake_timeout;
        bool socket_init;
    } connection;

    mem_region_t receive_buffer;
    struct send_buffer_t {
        // we may not be able to write all our desired data in a single send, so check that amount_sent is equal to message_size about before reusing.
        mem_region_t buffer;
        size_t amount_sent;
        size_t message_size;
    } send_buffer;

    handshake_headers_status_e headers_status;
    websocket_status_e status;
    websocket_error_e error;

    // the unix epoch millisecond timestamp since we last received any response
    uint64_t last_response_timestamp_epoch_ms;
    // if we have not received any activity in this time frame, then we should send a ping.
    milliseconds_t no_activity_wait_period;
    // if we have not received any activity, or response in (no_activity_wait_period + ping_timeout) we should close the connection->
    milliseconds_t ping_timeout;

    uint32_t max_receivable_message_size;

    bool received_ping;
    bool waiting_on_pong;

    // stored values are valid for the lifetime of a single websocket packet.
    // values inside receive are for persisting state across socket read operations where we may not have a completed packet ready for processing.
    struct receive_tmps_t {
        // an intermediary buffer that we can dump working data into when doing reads.
        // do not consider this permanent (make a new buffer if things must persist)
        packet_t packet;
        struct frame_t {
            ws_frame_t ws_frame;
            bool complete;
            uint8_t buff[ws_frame_header_max_len];
            size_t end;
        } frame;
    } receive_tmps;

    struct http_handshake_t {
        // header values (besides region) _must_ be zeroed after the client successfully sends its headers.
        http_headers_t headers;
        const char * accepted_protocols; // this field lives inside the headers' region.
        websocket_http_header_list_t * optional_user_headers;
        char * optional_user_protocols;
        mem_region_t requested_protocols;
        // 30 bytes for encoding a 20 byte buffer into base64 + the encoders inserted newline character + nul
        unsigned char sec_ws_accept_sha1_base64[30];
        http_status_code_e http_status;
        uint32_t remaining_redirects_allowed;
    } http_handshake;

    struct close_message_t {
        http_status_code_e status_code;
        mem_region_t utf8_reason;
        bool close_sent;
    } close_message;

#ifdef _WS_SHIM_SUPPORT
    struct ws_shim_support_t {
        websocket_message_t last_message;
        curl_common_certs_t certs;
    } ws_shim_support;
#endif

    curl_common_ssl_ctx_data_t ssl_ctx_data;

    const char * tag;
};

static void * ws_curl_malloc(void * const void_client, const size_t size, const char * const tag) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    websocket_client_t * const client = void_client;
    void * const ptr = heap_unchecked_alloc(client->heap, size, tag);
    if (!ptr) {
        LOG_ERROR(WS_TAG, "%s: Allocation failed!\nSize: [%" PRIu64 "]\n", tag, (uint64_t)size);
    }
    WEBSOCKET_FULL_TRACE_POP();
    return ptr;
}

static void * ws_curl_calloc(void * const void_client, const size_t size, const char * const tag) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    websocket_client_t * const client = void_client;
    void * const ptr = heap_unchecked_calloc(client->heap, size, tag);
    if (!ptr) {
        LOG_ERROR(WS_TAG, "%s: Allocation failed!\nSize: [%" PRIu64 "]\n", tag, (uint64_t)size);
    }
    WEBSOCKET_FULL_TRACE_POP();
    return ptr;
}

static void ws_curl_free(void * const void_client, void * const ptr, const char * const tag) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    websocket_client_t * const client = void_client;
    if (!ptr) {
        WEBSOCKET_FULL_TRACE_POP();
        return;
    }
    heap_free(client->heap, ptr, tag);
    WEBSOCKET_FULL_TRACE_POP();
}

static void * ws_curl_realloc(void * const void_client, void * const ptr, const size_t size, const char * const tag) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    websocket_client_t * const client = void_client;
    void * const new_ptr = heap_unchecked_realloc(client->heap, ptr, size, tag);
    if (!new_ptr) {
        LOG_ERROR(WS_TAG, "%s: Allocation failed!\nSize: [%" PRIu64 "]\n", tag, (uint64_t)size);
    }
    WEBSOCKET_FULL_TRACE_POP();
    return new_ptr;
}

websocket_client_t * websocket_client_emplace_init_custom_certs(
    const mem_region_t region,
    const system_guard_page_mode_e guard_page_mode,
    websocket_client_additional_cert_t * const additional_cert_array,
    const size_t additional_cert_array_size,
    const websocket_client_default_cert_usage_e default_cert_usage,
    const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    heap_t * heap;
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode != system_guard_page_mode_disabled) {
        heap = debug_heap_emplace_init(region.size, 8, 0, "websocket_heap", guard_page_mode, tag);
    } else
#endif
    {
        heap = heap_emplace_init_with_region(region, 8, 0, "websocket_heap");
    }

    websocket_client_t * websocket_client = heap_alloc(heap, sizeof(websocket_client_t), MALLOC_TAG);
    ZEROMEM(websocket_client);
    websocket_client->heap = heap;
    {
        splitmix64_t seed = splitmix64_init(rand());
        websocket_client->rand_gen = xoroshiro256plusplus_init(&seed);
    }

    websocket_client->curl_ctx = adk_curl_context_create(
        (void *)websocket_client,
        (adk_curl_context_callbacks_t){
            .malloc = (adk_curl_context_malloc_t)ws_curl_malloc,
            .calloc = (adk_curl_context_calloc_t)ws_curl_calloc,
            .free = (adk_curl_context_free_t)ws_curl_free,
            .realloc = (adk_curl_context_realloc_t)ws_curl_realloc,
        });
    if (default_cert_usage == websocket_client_default_certs_load) {
        websocket_client->certs.default_certs = curl_common_load_default_certs_into_memory(websocket_client->heap, NULL, MALLOC_TAG);
    } else {
        ASSERT(default_cert_usage == websocket_client_default_certs_skip);
    }
    if (additional_cert_array && additional_cert_array_size) {
        for (size_t i = 0; i < additional_cert_array_size; ++i) {
            if (additional_cert_array[i].type == websocket_client_additional_cert_type_file) {
                curl_common_load_cert_into_memory(websocket_client->heap, NULL, &websocket_client->certs.custom_certs, additional_cert_array[i].filepath, MALLOC_TAG);
            } else {
                ASSERT(additional_cert_array[i].type == websocket_client_additional_cert_type_mem_region);
                curl_common_load_cert_from_memory(websocket_client->heap, NULL, &websocket_client->certs.custom_certs, additional_cert_array[i].cert_region, additional_cert_array[i].cert_name, MALLOC_TAG);
            }
        }
    }
    ASSERT_MSG(websocket_client->certs.custom_certs.head || websocket_client->certs.default_certs.head, "WS: no certs were loaded!");

    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(websocket_client->curl_ctx);
    CURLM * const curl_multi = curl_multi_init();
    VERIFY(curl_multi);
    websocket_client->curl_multi = curl_multi;
    adk_curl_set_context(ctx_old);

    WEBSOCKET_MINIMAL_TRACE_POP();
    return websocket_client;
}

websocket_client_t * websocket_client_emplace_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    websocket_client_t * const ws_client = websocket_client_emplace_init_custom_certs(region, guard_page_mode, NULL, 0, websocket_client_default_certs_load, tag);
    WEBSOCKET_MINIMAL_TRACE_POP();
    return ws_client;
}

void websocket_client_dump_heap_usage(const websocket_client_t * const websocket_client) {
    heap_dump_usage(websocket_client->heap);
}

heap_metrics_t websocket_client_get_heap_metrics(const websocket_client_t * const websocket_client) {
    return heap_get_metrics(websocket_client->heap);
}

static void ws_free(websocket_t * const ws);

void websocket_client_shutdown(websocket_client_t * const websocket_client, const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    heap_t * heap = websocket_client->heap;

    websocket_t * curr_ws = websocket_client->ws_list_head;
    while (curr_ws) {
        websocket_t * const next_ws = curr_ws->next;
        LL_REMOVE(curr_ws, prev, next, websocket_client->ws_list_head, websocket_client->ws_list_tail);
        ws_free(curr_ws);
        curr_ws = next_ws;
    }

    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(websocket_client->curl_ctx);
    curl_multi_cleanup(websocket_client->curl_multi);
    adk_curl_set_context(ctx_old);

    curl_common_free_certs(websocket_client->heap, NULL, &websocket_client->certs.default_certs, MALLOC_TAG);
    curl_common_free_certs(websocket_client->heap, NULL, &websocket_client->certs.custom_certs, MALLOC_TAG);
    adk_curl_context_destroy(websocket_client->curl_ctx);

    heap_free(heap, websocket_client, MALLOC_TAG);
#ifndef NDEBUG
    heap_debug_print_leaks(heap);
#endif
    heap_destroy(heap, tag);
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static uint64_t ws_get_epoch_timestamp_u64() {
    const sb_time_since_epoch_t time_since_epoch = sb_get_time_since_epoch();
    return ((uint64_t)time_since_epoch.seconds * 1000) + ((uint64_t)time_since_epoch.microseconds / 1000);
}

static connection_io_error_e ws_send_bytes(websocket_t * const ws, const_mem_region_t message, size_t * const num_bytes_sent) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    const CURLcode send_error = curl_easy_send(ws->connection.connect_only_handle->curl_handle, message.ptr, message.size, num_bytes_sent);
    WEBSOCKET_FULL_TRACE_POP();
    switch (send_error) {
        case CURLE_OK:
            return connection_io_success;
        case CURLE_AGAIN:
            return connection_io_would_block;
        default:
            return connection_io_unrecoverable_error;
    }
}

static connection_io_error_e ws_read_bytes(websocket_t * const ws, mem_region_t message, size_t * const num_bytes_read) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    const CURLcode send_error = curl_easy_recv(ws->connection.connect_only_handle->curl_handle, message.ptr, message.size, num_bytes_read);
    WEBSOCKET_FULL_TRACE_POP();
    switch (send_error) {
        case CURLE_OK:
            if (*num_bytes_read == 0) {
                return connection_io_unrecoverable_error;
            } else {
                return connection_io_success;
            }
        case CURLE_AGAIN:
            return connection_io_would_block;
        default:
            return connection_io_unrecoverable_error;
    }
}

// the provided mask should be difficult/impossible to guess (spec suggests cryptographically secure)
// https://tools.ietf.org/html/rfc6455#section-10.3
// passed in mask must be in big endian order
static void ws_mask_payload(const mem_region_t region, const uint32_t mask) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    uint8_t bitmask[sizeof(mask)];
    memcpy(bitmask, &mask, sizeof(mask));
    for (size_t i = 0; i < region.size; ++i) {
        (region.byte_ptr)[i] ^= bitmask[i % sizeof(bitmask)];
    }
    WEBSOCKET_FULL_TRACE_POP();
}

static void ws_construct_frame(const mem_region_t frame_mem, const ws_frame_t frame, size_t * const out_frame_end) {
    // Framing protocol: https://tools.ietf.org/html/rfc6455#section-5.2
    //
    //  0               1               2               3
    //  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
    // +-+-+-+-+-------+-+-------------+-------------------------------+
    // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    // | |1|2|3|       |K|             |                               |
    // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    // |     Extended payload length continued, if payload len == 127  |
    // + - - - - - - - - - - - - - - - +-------------------------------+
    // |                               |Masking-key, if MASK set to 1  |
    // +-------------------------------+-------------------------------+
    // | Masking-key (continued)       |          Payload Data         |
    // +-------------------------------- - - - - - - - - - - - - - - - +
    // :                     Payload Data continued ...                :
    // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    // |                     Payload Data continued ...                |
    // +---------------------------------------------------------------+

    WEBSOCKET_FULL_TRACE_PUSH_FN();
    memset(frame_mem.ptr, 0, ws_frame_header_max_len);
    *out_frame_end = 0;

    const uint8_t fin_and_op = (uint8_t)((frame.fin << 7) | frame.op_code);

    const uint8_t payload_head = (frame.payload_len <= ws_byte_max_len) ? (uint8_t)frame.payload_len : (uint8_t)(frame.payload_len < short_max_val ? ws_uint16_code : ws_uint64_code);
    const uint8_t mask_and_payload_head = ((uint8_t)frame.masked << 7) | payload_head;
    frame_mem.byte_ptr[(*out_frame_end)++] = fin_and_op;
    frame_mem.byte_ptr[(*out_frame_end)++] = mask_and_payload_head;

    if (frame.payload_len > ws_byte_max_len && frame.payload_len < short_max_val) {
        const uint16_t payload_len = __builtin_bswap16((uint16_t)frame.payload_len);
        memcpy(frame_mem.byte_ptr + *out_frame_end, &payload_len, sizeof(payload_len));
        *out_frame_end += sizeof(payload_len);
    } else if (frame.payload_len > short_max_val) {
        const uint64_t payload_len = __builtin_bswap64((uint64_t)frame.payload_len);
        memcpy(frame_mem.byte_ptr + *out_frame_end, &payload_len, sizeof(payload_len));
        *out_frame_end += sizeof(payload_len);
    }
    if (frame.masked) {
        memcpy(frame_mem.byte_ptr + *out_frame_end, &frame.mask, sizeof(frame.mask));
        *out_frame_end += sizeof(frame.mask);
    }
    WEBSOCKET_FULL_TRACE_POP();
}

// returns true if the frame header can be fully read with the provided memory region `frame_mem`.
static bool ws_read_frame(const const_mem_region_t frame_mem, size_t * const out_frame_end, ws_frame_t * const out_frame) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    *out_frame_end = 0;
    ZEROMEM(out_frame);
    if (frame_mem.size < 2) {
        WEBSOCKET_FULL_TRACE_POP();
        return false;
    }

    const uint8_t fin_rsv_opcode = frame_mem.byte_ptr[(*out_frame_end)++];
    out_frame->fin = (fin_rsv_opcode & 0x80) >> 7;
    out_frame->rsv1 = (fin_rsv_opcode & 0x40) >> 6;
    out_frame->rsv2 = (fin_rsv_opcode & 0x20) >> 5;
    out_frame->rsv3 = (fin_rsv_opcode & 0x10) >> 4;
    out_frame->op_code = fin_rsv_opcode & 0xf;

    ASSERT(
        (out_frame->op_code == ws_op_continuation) || (out_frame->op_code == ws_op_text) || (out_frame->op_code == ws_op_binary) || (out_frame->op_code == ws_op_close) || (out_frame->op_code == ws_op_ping) || (out_frame->op_code == ws_op_pong));

    const uint8_t masked_and_payload_len = frame_mem.byte_ptr[(*out_frame_end)++];
    out_frame->masked = (masked_and_payload_len & 0x80) >> 7;

    const size_t payload_len_key = masked_and_payload_len & ~0x80;
    if (payload_len_key <= ws_byte_max_len) {
        out_frame->payload_len = payload_len_key;
    } else if (payload_len_key == ws_uint16_code) {
        uint16_t payload_len;
        if (frame_mem.size < *out_frame_end + sizeof(payload_len)) {
            WEBSOCKET_FULL_TRACE_POP();
            return false;
        }

        memcpy(&payload_len, frame_mem.byte_ptr + *out_frame_end, sizeof(payload_len));
        out_frame->payload_len = __builtin_bswap16(payload_len);
        *out_frame_end += sizeof(payload_len);
    } else {
        uint64_t payload_len;
        if (frame_mem.size < (*out_frame_end) + sizeof(payload_len)) {
            WEBSOCKET_FULL_TRACE_POP();
            return false;
        }

        memcpy(&payload_len, frame_mem.byte_ptr + (*out_frame_end), sizeof(payload_len));
        out_frame->payload_len = __builtin_bswap64(payload_len);
        *out_frame_end += sizeof(payload_len);
    }
    if (out_frame->masked) {
        uint32_t mask;
        if (frame_mem.size < *out_frame_end + sizeof(mask)) {
            WEBSOCKET_FULL_TRACE_POP();
            return false;
        }

        memcpy(&mask, frame_mem.byte_ptr + *out_frame_end, sizeof(mask));
        out_frame->mask = __builtin_bswap32(mask);
        *out_frame_end += sizeof(mask);
    }
    ASSERT(*out_frame_end <= ws_frame_header_max_len);
    WEBSOCKET_FULL_TRACE_POP();
    return true;
}

static uint32_t ws_mask_gen(websocket_t * const ws) {
    return __builtin_bswap32(xoroshiro256plusplus_next(&ws->client->rand_gen) & UINT32_MAX);
}

static websocket_status_e ws_to_close_status(websocket_status_e status) {
    ASSERT(status != websocket_status_user_initiated_closed);
    return status == websocket_status_user_initiated_closing ? websocket_status_user_initiated_closed : websocket_status_closed;
}

static void ws_close(websocket_t * const ws, uint16_t reason, const char * const msg) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    ws->close_message.close_sent = true;
    const size_t msg_len = msg ? strlen(msg) : 0;
    VERIFY((msg_len + ws_frame_header_max_len + sizeof(reason)) <= ws->send_buffer.buffer.size);

    const ws_frame_t close_frame = {
        .fin = 1,
        .op_code = ws_op_close,
        .masked = 1,
        .mask = ws_mask_gen(ws),
        .payload_len = sizeof(reason) + msg_len,
    };

    size_t frame_end;
    ws_construct_frame(ws->send_buffer.buffer, close_frame, &frame_end);
    const size_t msg_size = frame_end + msg_len + sizeof(reason);

    reason = __builtin_bswap16(reason);
    memcpy(ws->send_buffer.buffer.byte_ptr + frame_end, &reason, sizeof(reason));

    if (msg_len) {
        memcpy(ws->send_buffer.buffer.byte_ptr + frame_end + sizeof(reason), msg, msg_len);
    }
    ws_mask_payload(MEM_REGION(.ptr = ws->send_buffer.buffer.byte_ptr + frame_end, .size = close_frame.payload_len), close_frame.mask);
    size_t bytes_sent;
    if (ws_send_bytes(ws, CONST_MEM_REGION(.ptr = ws->send_buffer.buffer.ptr, .size = msg_size), &bytes_sent) == connection_io_unrecoverable_error) {
        ws->error = websocket_error_socket_send_failure;
        ws->status = ws_to_close_status(ws->status);
    } else if (bytes_sent != msg_size) {
        ws->send_buffer.message_size = msg_size;
        ws->send_buffer.amount_sent = bytes_sent;
        ASSERT(ws->send_buffer.message_size >= ws->send_buffer.amount_sent);
    } else if (bytes_sent == msg_size) {
        ws->status = ws_to_close_status(ws->status);
    }
    WEBSOCKET_FULL_TRACE_POP();
}

static void ws_ping_pong(websocket_t * const ws, const ws_op_code_e op_code) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    const ws_frame_t ping_frame = {
        .fin = 1,
        .op_code = op_code,
        .masked = 1,
        .mask = ws_mask_gen(ws),
    };
    size_t frame_end;
    ws_construct_frame(ws->send_buffer.buffer, ping_frame, &frame_end);
    size_t bytes_sent;
    if (ws_send_bytes(ws, CONST_MEM_REGION(.ptr = ws->send_buffer.buffer.ptr, .size = frame_end), &bytes_sent) == connection_io_unrecoverable_error) {
        ws->error = websocket_error_socket_send_failure;
        ws->status = ws_to_close_status(ws->status);
    } else if (bytes_sent != frame_end) {
        ws->send_buffer.message_size = frame_end;
        ws->send_buffer.amount_sent = bytes_sent;
        ASSERT(ws->send_buffer.message_size >= ws->send_buffer.amount_sent);
    }
    WEBSOCKET_FULL_TRACE_POP();
}

typedef enum ws_finalize_received_payload_status_e {
    ws_finalize_received_payload_unknown_op = -2,
    ws_finalize_received_payload_error = -1,
    ws_finalize_received_payload_success,
} ws_finalize_received_payload_status_e;

static ws_finalize_received_payload_status_e ws_finalize_received_payload(websocket_t * const ws, const ws_frame_t received_frame) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if ((received_frame.op_code == ws_op_binary) || (received_frame.op_code == ws_op_text)) {
        websocket_received_message_t * const message = ws->recv_msg_tail;
        if (received_frame.masked) {
            ws_mask_payload(ws->receive_tmps.packet.region, received_frame.mask);
        }
        message->type = (received_frame.op_code == ws_op_text) ? websocket_message_text : websocket_message_binary;
        message->complete = received_frame.fin == 1;
    } else if (received_frame.op_code == ws_op_continuation) {
        if (received_frame.masked) {
            ws_mask_payload(ws->receive_tmps.packet.region, received_frame.mask);
        }
        ws->recv_msg_tail->complete = received_frame.fin == 1;
    } else if (received_frame.op_code == ws_op_ping) {
        ws->received_ping = true;
    } else if (received_frame.op_code == ws_op_pong) {
        ws->waiting_on_pong = false;
    } else if (received_frame.op_code == ws_op_close) {
        ws->status = (ws->status == websocket_status_user_initiated_closing) ? websocket_status_user_initiated_closed : websocket_status_closing;
        if (received_frame.payload_len < 2) {
            LOG_ERROR(WS_TAG, "[%s] Protocol error when closing frame size must be at minumum 2, frame size was: [%i]", ws->connection.url, received_frame.payload_len);
            ws->error = websocket_error_protocol;
            WEBSOCKET_MINIMAL_TRACE_POP();
            return ws_finalize_received_payload_error;
        }
        ws->close_message.status_code = __builtin_bswap16(*(uint16_t *)(ws->receive_tmps.packet.region.byte_ptr));
        ws->close_message.utf8_reason = MEM_REGION(.ptr = ws->receive_tmps.packet.region.byte_ptr + sizeof(uint16_t), .size = received_frame.payload_len - sizeof(uint16_t));
    } else {
        LOG_ERROR(WS_TAG, "[%s] Received an unknown op code [%i]", ws->connection.url, received_frame.op_code);
        ws->error = websocket_error_protocol;
        ws->status = ws_to_close_status(ws->status);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws_finalize_received_payload_unknown_op;
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
    return ws_finalize_received_payload_success;
}

static connection_io_error_e ws_read_sock(websocket_t * const ws, packet_t * const packet) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    size_t amount_received = 0;
    ASSERT(packet->region.size >= packet->amount_received);
    const connection_io_error_e io_error = ws_read_bytes(ws, MEM_REGION(.ptr = packet->region.byte_ptr + packet->amount_received, .size = packet->region.size - packet->amount_received), &amount_received);

    if (io_error == connection_io_unrecoverable_error) {
        if (ws->status == websocket_status_user_initiated_closing) {
            ws->status = websocket_status_user_initiated_closed;
        } else {
            if (amount_received == 0) {
                LOG_ERROR(WS_TAG, "[%s] Received a zero sized packet", ws->connection.url);
            } else {
                LOG_ERROR(WS_TAG, "[%s] Encountered an error while trying to fetch ws content", ws->connection.url);
            }
            ws->status = ws_to_close_status(ws->status);
        }
        ws->error = websocket_error_socket_read_failure;
        WEBSOCKET_FULL_TRACE_POP();
        return io_error;
    }

    packet->amount_received += amount_received;
    WEBSOCKET_FULL_TRACE_POP();
    return io_error;
}

static void ws_signal_allocation_failure(websocket_t * const ws, const size_t alloc_size, const char * const tag) {
    ws->status = websocket_status_closed;
    ws->error = websocket_error_allocation_failure;
    LOG_ERROR(WS_TAG, "%s: Allocation failed!\nSize: [%" PRIu64 "]\n", tag, (uint64_t)alloc_size);
#if defined(_CONSOLE_NATIVE) || !defined(_SHIP)
    heap_dump_usage(ws->client->heap);
#endif
}

static bool ws_alloc_recv_buffer(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    const size_t payload_len = ws->receive_tmps.frame.ws_frame.payload_len;
    const ws_op_code_e op_code = ws->receive_tmps.frame.ws_frame.op_code;
    const bool is_control_frame = (op_code != ws_op_text) && (op_code != ws_op_binary) && (op_code != ws_op_continuation);

    // filter out messages that are either too large, or close/ping/pong frames that would be too large to fit inside the receive buffer.
    if ((payload_len > ws->max_receivable_message_size) || (is_control_frame && (payload_len > ws->receive_buffer.size))) {
        ws->error = websocket_error_payload_too_large;
        LOG_ERROR(WS_TAG, "[%s] Exceeded maximum configured allowed receivable message size of [%" PRIu32 "]. Requested size: [%" PRIu64 "]", ws->connection.url, ws->max_receivable_message_size, (uint64_t)payload_len);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return false;
    } else if (is_control_frame) {
        // close, ping, pong frames can live inside of the receive buffer.
        ws->receive_tmps.packet.region = MEM_REGION(.ptr = ws->receive_buffer.ptr, .size = payload_len);
    } else {
        // if its a continuation verify preconditions are not violated, make sure the sum total of the new buffer would be within our limits
        if (op_code == ws_op_continuation) {
            websocket_received_message_t * const continuation_msg = ws->recv_msg_tail;
            const size_t required_mem = continuation_msg->region.size + payload_len;
            if (!continuation_msg || continuation_msg->complete) {
                LOG_ERROR(WS_TAG, "[%s] Cannot receive a continuation frame if we don't have a frame, or the other frame was marked as complete.", ws->connection.url);
                ws->error = websocket_error_protocol;
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            } else if (required_mem > ws->max_receivable_message_size) {
                ws->error = websocket_error_payload_too_large;
                LOG_ERROR(WS_TAG, "[%s] Exceeded maximum configured allowed receivable message size of [%" PRIu32 "]. Requested size: [%" PRIu64 "]", ws->connection.url, ws->max_receivable_message_size, (uint64_t)payload_len + (uint64_t)continuation_msg->region.size);
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
            // grow our buffer for the message and assign the packets region..
            const size_t msg_old_size = continuation_msg->region.size;
            const mem_region_t new_region = MEM_REGION(.ptr = heap_unchecked_realloc(ws->client->heap, continuation_msg->region.ptr, required_mem, MALLOC_TAG), .size = required_mem);
            if (!new_region.ptr) {
                LL_REMOVE(continuation_msg, prev, next, ws->recv_msg_head, ws->recv_msg_tail);
                heap_free(ws->client->heap, continuation_msg->region.ptr, MALLOC_TAG);
                heap_free(ws->client->heap, continuation_msg, MALLOC_TAG);
                ws_signal_allocation_failure(ws, required_mem, MALLOC_TAG);
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
            continuation_msg->region = new_region;
            ws->receive_tmps.packet.region = MEM_REGION(.ptr = continuation_msg->region.byte_ptr + msg_old_size, .size = payload_len);
        } else {
            // the packet is a text or binary frame, verify preconditions and alloc resources required...
            if (ws->recv_msg_tail && !ws->recv_msg_tail->complete) {
                LOG_ERROR(WS_TAG,
                          "[%s] Received a new message while waiting on completing a different message.\n"
                          "(this functionally shouldn't happen with conforming ws impls, but it is plausible for custom rolled)",
                          ws->connection.url);
                ws->error = websocket_error_protocol;
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
            websocket_received_message_t * const msg = heap_unchecked_alloc(ws->client->heap, sizeof(websocket_received_message_t), MALLOC_TAG);
            if (!msg) {
                ws_signal_allocation_failure(ws, sizeof(websocket_received_message_t), MALLOC_TAG);
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
            ZEROMEM(msg);
            msg->region = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, payload_len, MALLOC_TAG), .size = payload_len);
            if (!msg->region.ptr) {
                heap_free(ws->client->heap, msg, MALLOC_TAG);
                ws_signal_allocation_failure(ws, payload_len, MALLOC_TAG);
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
            ws->receive_tmps.packet.region = msg->region;
            LL_ADD(msg, prev, next, ws->recv_msg_head, ws->recv_msg_tail);
        }
    }

    // now that we have a valid packet/location to store our message copy any bytes that wonud up in the frame buffer into our destination (packet) buffer so they aren't lost..
    const size_t frame_buff_msg_bytes = min_size_t(ws->receive_tmps.packet.amount_received, min_size_t(ws->receive_tmps.packet.amount_received - ws->receive_tmps.frame.end, payload_len));
    if (frame_buff_msg_bytes > 0) {
        memcpy(ws->receive_tmps.packet.region.ptr, ws->receive_tmps.frame.buff + ws->receive_tmps.frame.end, frame_buff_msg_bytes);
        ws->receive_tmps.packet.amount_received = frame_buff_msg_bytes;
    } else {
        ws->receive_tmps.packet.amount_received = 0;
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
    return true;
}

static void ws_read_from_sock(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    // read off the socket, persisting data until we have completed a step for the current websocket packet.
    milliseconds_t receive_timeout = {0};
    while ((sb_socket_select(ws->connection.socket, &receive_timeout) & sb_socket_select_flag_readable) && (ws->status != websocket_status_closing)) {
        ws->last_response_timestamp_epoch_ms = ws_get_epoch_timestamp_u64();
        if (!ws->receive_tmps.packet.incomplete) {
            // check for a completed frame, if not read into a local buffer.
            if (!ws->receive_tmps.frame.complete) {
                if (ws->receive_tmps.packet.region.size == 0) {
                    ws->receive_tmps.packet.region = MEM_REGION(.ptr = ws->receive_tmps.frame.buff, .size = ARRAY_SIZE(ws->receive_tmps.frame.buff));
                }
                const connection_io_error_e io_error = ws_read_sock(ws, &ws->receive_tmps.packet);
                if (io_error == connection_io_unrecoverable_error) {
                    break;
                } else if (io_error == connection_io_would_block) {
                    continue;
                }
                ws->receive_tmps.frame.complete = ws_read_frame(ws->receive_tmps.packet.region.consted, &ws->receive_tmps.frame.end, &ws->receive_tmps.frame.ws_frame);
                ASSERT(ws->receive_tmps.frame.end <= ws_frame_header_max_len);
                if (ws->receive_tmps.frame.complete) {
                    // manually null out most fields, except amount_received because we could read less data than the size of the frame buffer (14 bytes)
                    // so we have to persist .amount_received if we have read any message data, into our local buffer.
                    // otherwise we should set the amount_received to zero to indicate there is no further data that needs to be read or allocated.
                    ws->receive_tmps.packet.region = (mem_region_t){0};
                    ws->receive_tmps.packet.incomplete = false;
                    if (ws->receive_tmps.packet.amount_received <= ws->receive_tmps.frame.end) {
                        ws->receive_tmps.packet.amount_received = 0;
                    }
                } else {
                    continue;
                }
            }
            // if we need to allocate a buffer do so
            // control ops should be placed into receive_buffer
            // anything that the user could consider a message (binary, text, continuation) should be placed into the message queue
            // note: this step can push new messages onto the recv queue. but we will defer finalizing until all the data has been fully read for the packet.
            if ((ws->receive_tmps.packet.region.size == 0) && (ws->receive_tmps.frame.ws_frame.payload_len != 0)) {
                if (!ws_alloc_recv_buffer(ws)) {
                    break;
                }
            }
        }
        if ((ws->receive_tmps.packet.region.size != ws->receive_tmps.packet.amount_received)) {
            const connection_io_error_e io_error = ws_read_sock(ws, &ws->receive_tmps.packet);
            if (io_error == connection_io_unrecoverable_error) {
                break;
            } else if ((io_error == connection_io_would_block) || (ws->receive_tmps.packet.amount_received != ws->receive_tmps.packet.region.size)) {
                ws->receive_tmps.packet.incomplete = true;
                continue;
            }
        }

        if ((ws_finalize_received_payload(ws, ws->receive_tmps.frame.ws_frame) != ws_finalize_received_payload_success) || (ws->status == websocket_status_user_initiated_closed)) {
            break;
        }

        // cleanup persisted receive temporaries..
        ZEROMEM(&ws->receive_tmps);
        ws->last_response_timestamp_epoch_ms = ws_get_epoch_timestamp_u64();
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_send_message(websocket_t * const ws, const ws_op_code_e message_type, const const_mem_region_t message, const bool is_last_fragment) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    uint8_t frame_header[ws_frame_header_max_len] = {0};
    ws_frame_t send_frame = {
        .fin = is_last_fragment,
        .op_code = message_type,
        .masked = 1,
        .mask = ws_mask_gen(ws),
        .payload_len = message.size};

    size_t frame_end;
    ws_construct_frame(MEM_REGION(.ptr = frame_header, .size = sizeof(frame_header)), send_frame, &frame_end);

    memcpy(ws->send_buffer.buffer.ptr, frame_header, frame_end);
    memcpy(ws->send_buffer.buffer.byte_ptr + frame_end, message.ptr, message.size);

    ws_mask_payload(MEM_REGION(.ptr = ws->send_buffer.buffer.byte_ptr + frame_end, .size = send_frame.payload_len), send_frame.mask);
    const size_t msg_size = message.size + frame_end;
    size_t bytes_sent;
    if (ws_send_bytes(ws, CONST_MEM_REGION(.ptr = ws->send_buffer.buffer.ptr, .size = msg_size), &bytes_sent) == connection_io_unrecoverable_error) {
        ws->error = websocket_error_socket_send_failure;
        ws->status = ws_to_close_status(ws->status);
    } else if (bytes_sent != msg_size) {
        ws->send_buffer.message_size = msg_size;
        ws->send_buffer.amount_sent = bytes_sent;
        ASSERT(ws->send_buffer.message_size >= ws->send_buffer.amount_sent);
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_flush_last_message(websocket_t * const ws) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    size_t bytes_sent;
    size_t remaining_message_size = ws->send_buffer.message_size - ws->send_buffer.amount_sent;
    if (ws_send_bytes(ws, CONST_MEM_REGION(ws->send_buffer.buffer.byte_ptr + ws->send_buffer.amount_sent, remaining_message_size), &bytes_sent) == connection_io_unrecoverable_error) {
        ws->error = websocket_error_socket_send_failure;
        ws->status = ws_to_close_status(ws->status);
    } else if (bytes_sent != remaining_message_size) {
        ws->send_buffer.amount_sent += bytes_sent;
    } else {
        ws->send_buffer.amount_sent = 0;
        ws->send_buffer.message_size = 0;
    }
    ASSERT(ws->send_buffer.message_size >= ws->send_buffer.amount_sent);
    WEBSOCKET_FULL_TRACE_POP();
}

static http_status_code_e ws_errors_to_http_status(const websocket_error_e error) {
    switch (error) {
        case websocket_error_payload_too_large:
            return http_status_websocket_message_too_big;
        case websocket_error_timeout:
            return http_status_websocket_no_status_received;
        case websocket_error_protocol:
            return http_status_websocket_protocol_error;
        default:
            TRAP("Unexpected websocket error [%i]", error);
    }
    return http_status_websocket_going_away;
}

static void ws_send_to_sock(websocket_t * const ws, const uint64_t epoch_ms_timestamp) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    milliseconds_t send_timeout = {0};
    const uint64_t time_since_last_msg = ws->last_response_timestamp_epoch_ms < epoch_ms_timestamp ? epoch_ms_timestamp - ws->last_response_timestamp_epoch_ms : 0;
    if (time_since_last_msg > (ws->ping_timeout.ms + ws->no_activity_wait_period.ms)) {
        // if we can send the reason, send the reason, regardless we're marking the connection as dead on our end.
        ws->status = ws_to_close_status(ws->status);
        ws->error = websocket_error_timeout;
        if ((sb_socket_select(ws->connection.socket, &send_timeout) & sb_socket_select_flag_writable) && !ws->close_message.close_sent) {
            LOG_INFO(WS_TAG, "[%s] Connection closed. Ping timeout exceeded.", ws->connection.url);
            ws_close(ws, ws_errors_to_http_status(ws->error), "Ping timeout exceeded");
        }
        WEBSOCKET_MINIMAL_TRACE_POP();
        return;
    }
    while ((sb_socket_select(ws->connection.socket, &send_timeout) & sb_socket_select_flag_writable) && (ws->status != websocket_status_closed)) {
        if (ws->send_buffer.amount_sent != ws->send_buffer.message_size) {
            ws_flush_last_message(ws);
            continue;
        } else if ((ws->error != websocket_error_none) && !ws->close_message.close_sent) {
            // since we're in a failure case we can't reliably wait for the other end to send us an acknoweledgement of closure, so we forcefully mark us as closed.
            ws_close(ws, ws_errors_to_http_status(ws->error), NULL);
            ws->status = websocket_status_closed;
            break;

        } else if (!ws->close_message.close_sent && ((ws->status == websocket_status_closing) || (ws->status == websocket_status_user_initiated_closing)) && (!ws->send_msg_head || (ws->send_msg_head->total_sent == 0))) {
            // if we have received a close, and we don't have any inflight messages go ahead and close the connection down.
            // per spec, resend just the close reason no utf8 reason required.
            ws_close(ws, ws->close_message.status_code, NULL);
            // if we are just responding to the server requesting us to close, we should mark the websockt as closed.
            // the socket could have errored out via the server closing us, or some other error before we can get to sending the close frame.
            if ((ws->status != websocket_status_user_initiated_closing) && (ws->status != websocket_status_user_initiated_closed)) {
                ws->status = websocket_status_closed;
            }

        } else if (ws->received_ping) {
            ws_ping_pong(ws, ws_op_pong);
            ws->received_ping = false;

        } else if (time_since_last_msg >= ws->no_activity_wait_period.ms && !ws->waiting_on_pong) {
            ws_ping_pong(ws, ws_op_ping);
            ws->waiting_on_pong = true;

        } else if ((time_since_last_msg < ws->no_activity_wait_period.ms) && ws->send_msg_head) {
            // actually send our data..
            websocket_send_message_t * const msg = ws->send_msg_head;
            const size_t msg_remaining_size = msg->region.size - msg->total_sent;
            const ws_op_code_e op_code = msg->total_sent != 0 ? ws_op_continuation : msg->op_code;
            const bool is_last_fragment = msg_remaining_size <= ws->send_buffer.buffer.size - ws_frame_header_max_len;
            const size_t amount_to_send = is_last_fragment ? msg_remaining_size : ws->send_buffer.buffer.size - ws_frame_header_max_len;
            ws_send_message(ws, op_code, CONST_MEM_REGION(.ptr = msg->region.byte_ptr + msg->total_sent, .size = amount_to_send), is_last_fragment);
            msg->total_sent += amount_to_send;

            if (msg->total_sent == msg->region.size) {
                LL_REMOVE(msg, prev, next, ws->send_msg_head, ws->send_msg_tail);
                heap_free(ws->client->heap, msg->region.ptr, MALLOC_TAG);
                heap_free(ws->client->heap, msg, MALLOC_TAG);
            }
        }
        if (!ws->send_msg_head) {
            break;
        }
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

void ws_check_sock_error(websocket_t * const ws) {
    milliseconds_t send_timeout = {0};
    if (sb_socket_select(ws->connection.socket, &send_timeout) & sb_socket_select_flag_exception) {
        ws->error = websocket_error_socket_exception;
        ws->status = ws_to_close_status(ws->status);
    }
}

typedef enum ws_http_header_value_case_e {
    ws_header_value_case_unchanged,
    ws_header_value_case_lower,
} ws_http_header_value_case_e;

char * ws_strcasestr(const char * searched, const char * substr) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    // mild modifications from (MIT): https://android.googlesource.com/platform/bionic/+/ics-mr0/libc/string/strcasestr.c
    char c, sc;
    size_t len;
    if ((c = *substr++) != 0) {
        c = (char)tolower((unsigned char)c);
        len = strlen(substr);
        do {
            do {
                if ((sc = *searched++) == 0) {
                    WEBSOCKET_FULL_TRACE_POP();
                    return NULL;
                }
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(searched, substr, len) != 0);
        searched--;
    }
    WEBSOCKET_FULL_TRACE_POP();
    return (char *)searched;
}

static const char * ws_http_header_read(const http_headers_t * const headers, const char * const name, const size_t name_len, char * const out_value, const size_t value_len, const ws_http_header_value_case_e casing) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    *out_value = '\0';
    const char * const http_name_loc = ws_strcasestr((const char * const)headers->region.byte_ptr, name);
    if (!http_name_loc) {
        WEBSOCKET_FULL_TRACE_POP();
        return NULL;
    }
    const char * value_start = http_name_loc + name_len;
    while (*value_start == ' ') {
        ++value_start;
    }
    const char * const value_end = strstr(value_start, "\r\n");
    if ((size_t)(value_end - value_start) >= value_len) {
        WEBSOCKET_FULL_TRACE_POP();
        return NULL;
    }
    char * buff = out_value;
    if (casing == ws_header_value_case_lower) {
        while (value_start != value_end) {
            *(buff++) = (char)tolower(*(value_start++));
        }
        *(buff++) = '\0';
    } else {
        sprintf_s(out_value, value_len, "%.*s", (int)(value_end - value_start), value_start);
    }
    WEBSOCKET_FULL_TRACE_POP();
    return out_value;
}

typedef struct ws_http_header_list_node_t {
    unsigned char * name;
    unsigned char * value;
    struct ws_http_header_list_node_t * next;
    struct ws_http_header_list_node_t * prev;
} ws_http_header_list_node_t;

struct websocket_http_header_list_t {
    ws_http_header_list_node_t * head;
    ws_http_header_list_node_t * tail;
};

static bool ws_add_header(http_headers_t * const headers, char * const fmt, ...) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    const char crlf[] = "\r\n";
    const size_t crlf_size = ARRAY_SIZE(crlf) - 1;

    va_list args;
    va_start(args, fmt);
    headers->end_of_headers += vsnprintf((char *)headers->region.byte_ptr + headers->end_of_headers, headers->region.size - headers->end_of_headers, fmt, args);
    va_end(args);

    ++headers->num_headers;
    if (headers->end_of_headers + crlf_size > headers->region.size) {
        WEBSOCKET_FULL_TRACE_POP();
        return false;
    }

    memcpy(headers->region.byte_ptr + headers->end_of_headers, crlf, crlf_size);
    headers->end_of_headers += crlf_size;
    WEBSOCKET_FULL_TRACE_POP();
    return true;
}

static void ws_create_http_handshake(websocket_t * const ws, const char * const host, const char * const optional_path) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Host
    // the host field can optionally have a port in it so we don't have to parse it out as part of our steps.
    if (optional_path) {
        VERIFY(ws_add_header(&ws->http_handshake.headers, "GET %s HTTP/1.1", optional_path));
        VERIFY(ws_add_header(&ws->http_handshake.headers, "Host: %.*s", (int)(optional_path - host), host));
    } else {
        VERIFY(ws_add_header(&ws->http_handshake.headers, "GET / HTTP/1.1"));
        VERIFY(ws_add_header(&ws->http_handshake.headers, "Host: %s", host));
    }
    VERIFY(ws_add_header(&ws->http_handshake.headers, "Upgrade: websocket"));
    VERIFY(ws_add_header(&ws->http_handshake.headers, "Connection: Upgrade"));

    uint16_t websocket_key_bytes[8];
    uint8_t websocket_key[(sizeof(websocket_key_bytes) * 4 / 3 + 4) + 1];
    STATIC_ASSERT(sizeof(websocket_key_bytes) == 16); // spec indicates that the websocket key must be 16 bytes.
    for (int i = 0; i < ARRAY_SIZE(websocket_key_bytes); ++i) {
        websocket_key_bytes[i] = (uint16_t)rand();
    }
    size_t websocket_key_64encode_len;
    VERIFY(base64_encode((uint8_t *)&websocket_key_bytes[0], sizeof(websocket_key_bytes), websocket_key, &websocket_key_64encode_len));
    VERIFY(websocket_key_64encode_len == ARRAY_SIZE(websocket_key) - 1);
    // remove the base64's newline being inserted.
    websocket_key[ARRAY_SIZE(websocket_key) - 2] = '\0';

    // calculate the expected `Sec-WebSocket-Accept` value we will receive in the handshake.
    {
        static const unsigned char sec_ws_accept_str[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        char sec_ws_concat_buff[ARRAY_SIZE(sec_ws_accept_str) + ARRAY_SIZE(websocket_key) - 1];
        const int sec_ws_concat_buff_len = sprintf_s(sec_ws_concat_buff, ARRAY_SIZE(sec_ws_concat_buff), "%s%s", websocket_key, sec_ws_accept_str);
        unsigned char sec_ws_accept_sha1_buff[20];
        mbedtls_sha1((unsigned char *)sec_ws_concat_buff, sec_ws_concat_buff_len, sec_ws_accept_sha1_buff);
        size_t ws_sec_accept_sha1_base64_len;
        VERIFY(base64_encode(sec_ws_accept_sha1_buff, ARRAY_SIZE(sec_ws_accept_sha1_buff), ws->http_handshake.sec_ws_accept_sha1_base64, &ws_sec_accept_sha1_base64_len));
        VERIFY(ws_sec_accept_sha1_base64_len == ARRAY_SIZE(ws->http_handshake.sec_ws_accept_sha1_base64) - 1);
        // remove the base64's newline being inserted.
        ws->http_handshake.sec_ws_accept_sha1_base64[ARRAY_SIZE(ws->http_handshake.sec_ws_accept_sha1_base64) - 2] = '\0';
    }

    VERIFY(ws_add_header(&ws->http_handshake.headers, "Sec-Websocket-Key: %s", websocket_key));
    if (ws->http_handshake.optional_user_protocols) {
        VERIFY(ws_add_header(&ws->http_handshake.headers, "Sec-WebSocket-Protocol: %s", ws->http_handshake.optional_user_protocols));
    }
    VERIFY(ws_add_header(&ws->http_handshake.headers, "Sec-WebSocket-Version: 13"));

    if (ws->http_handshake.optional_user_headers) {
        ws_http_header_list_node_t * node = ws->http_handshake.optional_user_headers->head;
        while (node) {
            VERIFY_MSG(ws_add_header(&ws->http_handshake.headers, "%s%s", node->name, node->value), "Failed to add header, out of space. [%" PRIu64 "] of [%" PRIu64 "] used.", (uint64_t)ws->http_handshake.headers.end_of_headers, (uint64_t)ws->http_handshake.headers.region.size);
            node = node->next;
        }
    }
    // finally, add the trailing clrf
    VERIFY_MSG(ws_add_header(&ws->http_handshake.headers, ""), "Failed to add header terminator, out of space. [%" PRIu64 "] of [%" PRIu64 "] used.", (uint64_t)ws->http_handshake.headers.end_of_headers, (uint64_t)ws->http_handshake.headers.region.size);
    ws->headers_status = handshake_headers_init;
    WEBSOCKET_MINIMAL_TRACE_POP();
}

typedef struct ws_url_fragments_t {
    const char * host;
    const char * path;
} ws_url_fragments_t;

static bool ws_alloc_url_and_parse_fragments(websocket_t * const ws, const char * const src_url, ws_url_fragments_t * const out_url_fragments) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();

    const char https_str[] = "https";
    const char http_str[] = "http";
    const char wss_str[] = "wss";
    const char ws_str[] = "ws";
    const char host_delim[] = "://";
    ASSERT_MSG(strstr(src_url, host_delim), "Websocket urls should have either wss://, ws://, http://, or https:// at the start of the url.\nurl provided: [%s]", src_url);
    const size_t ws_to_http_req_buf_size = ARRAY_SIZE(https_str) - ARRAY_SIZE(wss_str);

    const size_t url_buf_size = strlen(src_url) + 1 + ((memcmp(src_url, http_str, ARRAY_SIZE(http_str) - 1) != 0) ? ws_to_http_req_buf_size : 0);
    ws->connection.url = heap_unchecked_alloc(ws->client->heap, url_buf_size, MALLOC_TAG);
    if (!ws->connection.url) {
        ws_signal_allocation_failure(ws, url_buf_size, MALLOC_TAG);
        WEBSOCKET_FULL_TRACE_POP();
        return false;
    }
    if (memcmp(src_url, wss_str, ARRAY_SIZE(wss_str) - 1) == 0) {
        sprintf_s(ws->connection.url, url_buf_size, "%s%s", https_str, src_url + ARRAY_SIZE(wss_str) - 1);
    } else if (memcmp(src_url, ws_str, ARRAY_SIZE(ws_str) - 1) == 0) {
        sprintf_s(ws->connection.url, url_buf_size, "%s%s", http_str, src_url + ARRAY_SIZE(ws_str) - 1);
    } else {
        memcpy(ws->connection.url, src_url, url_buf_size);
    }
    out_url_fragments->host = strstr(ws->connection.url, host_delim) + ARRAY_SIZE(host_delim) - 1;
    out_url_fragments->path = strchr(out_url_fragments->host, '/');
    WEBSOCKET_FULL_TRACE_POP();
    return true;
}

static bool ws_read_redirect(websocket_t * const ws, const http_headers_t * const headers, ws_url_fragments_t * const out_url_fragments) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    const http_status_code_e redirect_codes[] = {
        http_status_moved_permanently,
        http_status_temporary_redirect,
        http_status_permanent_redirect};
    bool remote_signaled_redirect = false;
    for (size_t i = 0; i < ARRAY_SIZE(redirect_codes); ++i) {
        if (redirect_codes[i] == ws->http_handshake.http_status) {
            remote_signaled_redirect = true;
            break;
        }
    }
    if (!remote_signaled_redirect) {
        WEBSOCKET_FULL_TRACE_POP();
        return false;
    }
    heap_free(ws->client->heap, ws->connection.url, MALLOC_TAG);
    ws->connection.url = NULL;
    char header_value_buff[1024];
    static const char location[] = "Location:";
    const char * const new_url = ws_http_header_read(headers, location, ARRAY_SIZE(location) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_unchanged);
    if (!new_url) {
        WEBSOCKET_FULL_TRACE_POP();
        return false;
    }
    const bool status = ws_alloc_url_and_parse_fragments(ws, new_url, out_url_fragments);
    WEBSOCKET_FULL_TRACE_POP();
    return status;
}

static bool ws_check_upgrade_response(websocket_t * const ws, const http_headers_t * const headers) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    // verify expected responses... https://tools.ietf.org/html/rfc6455#page-19
    char header_value_buff[1024];
    {
        static const char http11_version[] = "HTTP/1.1";
        static const char http10_version[] = "HTTP/1.0";
        const char * http_status = ws_http_header_read(headers, http11_version, ARRAY_SIZE(http11_version) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
        if (!http_status) {
            http_status = ws_http_header_read(headers, http10_version, ARRAY_SIZE(http10_version) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
            if (!http_status) {
                WEBSOCKET_MINIMAL_TRACE_POP();
                return false;
            }
        }
        ws->http_handshake.http_status = atoi(http_status);
        if (ws->http_handshake.http_status != http_status_switching_protocol) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
    }
    {
        static const char upgrade[] = "Upgrade:";
        const char * const upgrade_value = ws_http_header_read(headers, upgrade, ARRAY_SIZE(upgrade) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
        if (!upgrade_value) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
        const char * const expected_upgrade_value = "websocket";
        if (strcmp(upgrade_value, expected_upgrade_value) != 0) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
    }
    {
        static const char connection[] = "Connection:";
        const char * const connection_value = ws_http_header_read(headers, connection, ARRAY_SIZE(connection) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
        if (!connection_value) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
        const char * const expected_connection_value = "upgrade";
        if (strcmp(connection_value, expected_connection_value) != 0) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
    }
    {
        static const char sec_ws_accept[] = "Sec-WebSocket-Accept:";
        const char * const sec_ws_accept_value = ws_http_header_read(headers, sec_ws_accept, ARRAY_SIZE(sec_ws_accept) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_unchanged);
        if (!sec_ws_accept_value) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }

        // verify expected sha1 length..
        const size_t expected_sha1_len = 28;
        const size_t sha1_len = strlen(sec_ws_accept_value);
        if ((sha1_len != expected_sha1_len) || (memcmp(sec_ws_accept_value, ws->http_handshake.sec_ws_accept_sha1_base64, expected_sha1_len) != 0 || ((sec_ws_accept_value[expected_sha1_len] != '\0') && (sec_ws_accept_value[expected_sha1_len] != ' ')))) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
    }
    {
        // we are not using extensions, so if the header exists we must fail.
        static const char sec_ws_ext[] = "Sec-WebSocket-Extensions:";
        const char * const sec_ws_ext_value = ws_http_header_read(headers, sec_ws_ext, ARRAY_SIZE(sec_ws_ext) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
        if (sec_ws_ext_value) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return false;
        }
    }
    {
        // let the user determine if they want to care about accepted sub protocols or not.
        // there's an implicit contract that if you care about protocols you should not attempt to send any messages on a websocket
        // until the socket is connected, and you have verified the protocls you want/expect.
        static const char sec_ws_prot[] = "Sec-WebSocket-Protocol:";
        ws->http_handshake.accepted_protocols = ws_http_header_read(headers, sec_ws_prot, ARRAY_SIZE(sec_ws_prot) - 1, header_value_buff, ARRAY_SIZE(header_value_buff), ws_header_value_case_lower);
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
    return true;
}

static void ws_read_http_connection(websocket_t * const ws) {
    if (ws->connection.connect_only_handle->status == curl_status_ready) {
        if (ws->connection.connect_only_handle->result == curl_result_ok) {
            curl_socket_t curl_sockfd;
            curl_easy_getinfo(ws->connection.connect_only_handle->curl_handle, CURLINFO_ACTIVESOCKET, &curl_sockfd);
            ws->connection.socket = (sb_socket_t){(uint64_t)curl_sockfd};
            ws->connection.socket_init = true;
        } else {
            ws->status = websocket_status_closed;
            ws->error = websocket_error_connection;
        }
    }
}

static void ws_free_connect_only_handle(websocket_client_t * const client, curl_connect_only_handle_t * const handle) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(client->curl_ctx);
    curl_multi_remove_handle(client->curl_multi, handle->curl_handle);
    curl_easy_cleanup(handle->curl_handle);
    adk_curl_set_context(ctx_old);

    heap_free(client->heap, handle, MALLOC_TAG);
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static curl_connect_only_handle_t * ws_create_connect_only(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    curl_connect_only_handle_t * const handle = heap_unchecked_alloc(ws->client->heap, sizeof(curl_connect_only_handle_t), MALLOC_TAG);
    if (!handle) {
        ws_signal_allocation_failure(ws, sizeof(curl_connect_only_handle_t), MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return NULL;
    }

    handle->status = curl_status_pending;

    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(ws->client->curl_ctx);

    handle->curl_handle = curl_easy_init();

    curl_easy_setopt(handle->curl_handle, CURLOPT_URL, ws->connection.url);
    curl_easy_setopt(handle->curl_handle, CURLOPT_CONNECT_ONLY, 1L);
    curl_easy_setopt(handle->curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    curl_common_set_socket_callbacks(handle->curl_handle, ws->client->curl_ctx);
#ifdef _WS_SHIM_SUPPORT
    if (ws->ws_shim_support.certs.custom_certs.head) {
        ws->ssl_ctx_data.certs = &ws->ws_shim_support.certs;
        ws->ssl_ctx_data.heap = ws->client->heap;
        curl_common_set_ssl_ctx(&ws->ssl_ctx_data, handle->curl_handle, ws->client->curl_ctx);
    } else
#endif
    {
        ws->ssl_ctx_data.certs = &ws->client->certs;
        ws->ssl_ctx_data.heap = ws->client->heap;
        curl_common_set_ssl_ctx(&ws->ssl_ctx_data, handle->curl_handle, ws->client->curl_ctx);
    }
    curl_easy_setopt(handle->curl_handle, CURLOPT_PRIVATE, handle);

    VERIFY(curl_multi_add_handle(ws->client->curl_multi, handle->curl_handle) == CURLM_OK);
    adk_curl_set_context(ctx_old);

    WEBSOCKET_MINIMAL_TRACE_POP();
    return handle;
}

static void ws_reset_connect_only_handle(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ws->connection.socket_init = false;
    ws_free_connect_only_handle(ws->client, ws->connection.connect_only_handle);
    ws->connection.connect_only_handle = ws_create_connect_only(ws);
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_reset_http_handshake(websocket_t * const ws) {
    WEBSOCKET_FULL_TRACE_PUSH_FN();
    http_headers_t * const headers = &ws->http_handshake.headers;
    const mem_region_t header_region = headers->region;
    ZEROMEM(headers);
    headers->region = header_region;
    memset(ws->http_handshake.headers.region.ptr, 0, ws->http_handshake.headers.region.size);
    WEBSOCKET_FULL_TRACE_POP();
}

static void ws_send_http_handshake(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ASSERT(ws->headers_status == handshake_headers_init);

    milliseconds_t timeout = {0};
    while ((sb_socket_select(ws->connection.socket, &timeout) & sb_socket_select_flag_writable) && (ws->status == websocket_status_connecting)) {
        size_t amount_sent;
        http_headers_t * const headers = &ws->http_handshake.headers;
        const connection_io_error_e io_error = ws_send_bytes(ws, CONST_MEM_REGION(.ptr = headers->region.byte_ptr + headers->header_bytes_sent, .size = headers->end_of_headers - headers->header_bytes_sent), &amount_sent);
        if (io_error == connection_io_unrecoverable_error) {
            ws->status = websocket_status_closed;
            ws->error = websocket_error_upgrade_failed_send_failure;
            break;
        }
        headers->header_bytes_sent += amount_sent;
        if (headers->header_bytes_sent == headers->end_of_headers) {
            ws_reset_http_handshake(ws);
            ws->headers_status = handshake_headers_sent;
            break;
        }
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_free_header_list(websocket_client_t * const websocket_client, websocket_http_header_list_t * const list, const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if (list) {
        ws_http_header_list_node_t * node = list->head;
        while (node) {
            ws_http_header_list_node_t * const next = node->next;
            heap_free(websocket_client->heap, node->name, MALLOC_TAG);
            heap_free(websocket_client->heap, node->value, MALLOC_TAG);
            heap_free(websocket_client->heap, node, MALLOC_TAG);
            node = next;
        }
        heap_free(websocket_client->heap, list, MALLOC_TAG);
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_free_temporary_handshake_data(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if (ws->connection.url) {
        heap_free(ws->client->heap, ws->connection.url, MALLOC_TAG);
        ws->connection.url = NULL;
    }
    if (ws->http_handshake.optional_user_headers) {
        ws_free_header_list(ws->client, ws->http_handshake.optional_user_headers, MALLOC_TAG);
        ws->http_handshake.optional_user_headers = NULL;
    }
    if (ws->http_handshake.optional_user_protocols) {
        heap_free(ws->client->heap, ws->http_handshake.optional_user_protocols, MALLOC_TAG);
        ws->http_handshake.optional_user_protocols = NULL;
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

static void ws_read_http_handshake(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ASSERT(ws->headers_status == handshake_headers_sent);

    milliseconds_t timeout = {0};
    while ((sb_socket_select(ws->connection.socket, &timeout) & sb_socket_select_flag_readable) && (ws->status == websocket_status_connecting)) {
        size_t amount_received;
        http_headers_t * const headers = &ws->http_handshake.headers;
        const connection_io_error_e io_error = ws_read_bytes(ws, MEM_REGION(.ptr = headers->region.byte_ptr + headers->end_of_headers, .size = headers->region.size - headers->end_of_headers), &amount_received);

        if (io_error == connection_io_unrecoverable_error) {
            ws->status = websocket_status_closed;
            ws->error = websocket_error_upgrade_failed_read_failure;
            break;
        }

        // check to see if we have received all of the headers..
        headers->end_of_headers += amount_received;
        static const char header_terminator[] = "\r\n\r\n";
        const char * const header_terminator_pos = strstr((const char *)headers->region.ptr, header_terminator);
        if (!header_terminator_pos) {
            continue;
        }
        headers->end_of_headers = header_terminator_pos - (const char *)headers->region.ptr + (ARRAY_SIZE(header_terminator) - 1);
        // at this point we've received all the headers and can process the response.
        ws_url_fragments_t url_fragments;
        if (ws_check_upgrade_response(ws, headers)) {
            ws_free_temporary_handshake_data(ws);
            ws->status = websocket_status_connected;
            ws->headers_status = handshake_headers_received;
            ws->last_response_timestamp_epoch_ms = ws_get_epoch_timestamp_u64();
        } else if (ws_read_redirect(ws, headers, &url_fragments) && ws->http_handshake.remaining_redirects_allowed > 0) {
            ws_reset_http_handshake(ws);
            --ws->http_handshake.remaining_redirects_allowed;
            ws_create_http_handshake(ws, url_fragments.host, url_fragments.path);
            ws_reset_connect_only_handle(ws);
            ws->headers_status = handshake_headers_init;
        } else {
            ws->status = websocket_status_closed;
            ws->error = websocket_error_upgrade_failed_handshake_failure;
        }
        break;
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

websocket_http_header_list_t * websocket_client_append_header_list(websocket_client_t * const websocket_client, websocket_http_header_list_t * const list, const char * const name, const char * const value, const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    const size_t name_length = strlen(name) + 1;
    const size_t value_length = strlen(value) + 1;
    websocket_http_header_list_t * header_list = list;
    if (!header_list) {
        header_list = heap_alloc(websocket_client->heap, sizeof(websocket_http_header_list_t), tag);
        ZEROMEM(header_list);
    }
    ws_http_header_list_node_t * const new_node = heap_alloc(websocket_client->heap, sizeof(ws_http_header_list_node_t), tag);
    ZEROMEM(new_node);
    LL_ADD(new_node, prev, next, header_list->head, header_list->tail);

    new_node->name = heap_alloc(websocket_client->heap, name_length, tag);
    new_node->value = heap_alloc(websocket_client->heap, value_length, tag);

    memcpy(new_node->name, name, name_length);
    memcpy(new_node->value, value, value_length);

    WEBSOCKET_MINIMAL_TRACE_POP();
    return header_list;
}

websocket_t * websocket_create(
    websocket_client_t * const websocket_client,
    const char * const url,
    const char * const optional_protocols,
    websocket_http_header_list_t * const optional_additional_headers,
    const websocket_config_t config,
    const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ASSERT((config.ping_timeout.ms != 0) && (config.no_activity_wait_period.ms != 0) && (config.max_receivable_message_size != 0) && (config.receive_buffer_size != 0) && (config.send_buffer_size != 0) && (config.header_buffer_size != 0));

    websocket_t * const ws = heap_calloc(websocket_client->heap, sizeof(struct websocket_t), tag);
    ws->tag = tag;
    ws->client = websocket_client;
    ws->http_handshake.optional_user_headers = optional_additional_headers;
    ws->http_handshake.remaining_redirects_allowed = config.maximum_redirects;
    ws->ssl_ctx_data.custom_certs = NULL;

    ws_url_fragments_t url_fragments;
    if (!ws_alloc_url_and_parse_fragments(ws, url, &url_fragments)) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    if (optional_protocols) {
        const size_t protocols_len = strlen(optional_protocols) + 1;
        ws->http_handshake.optional_user_protocols = heap_unchecked_alloc(ws->client->heap, protocols_len, MALLOC_TAG);
        if (!ws->http_handshake.optional_user_protocols) {
            ws_signal_allocation_failure(ws, protocols_len, MALLOC_TAG);
            WEBSOCKET_MINIMAL_TRACE_POP();
            return ws;
        }
        memcpy(ws->http_handshake.optional_user_protocols, optional_protocols, protocols_len);
    }

    ws->connection.connect_only_handle = ws_create_connect_only(ws);
    ws->connection.max_handshake_timeout = config.max_handshake_timeout;
    ws->connection.connection_start_timestamp = adk_read_millisecond_clock();
    if (!ws->connection.connect_only_handle) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }

    ws->ping_timeout = config.ping_timeout;
    ws->no_activity_wait_period = config.no_activity_wait_period;

    ws->max_receivable_message_size = config.max_receivable_message_size;
    ws->receive_buffer = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.receive_buffer_size + ws_frame_header_max_len, MALLOC_TAG), .size = config.receive_buffer_size + ws_frame_header_max_len);
    if (!ws->receive_buffer.ptr) {
        ws_signal_allocation_failure(ws, config.receive_buffer_size, MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    ws->send_buffer.buffer = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.send_buffer_size + ws_frame_header_max_len, MALLOC_TAG), .size = config.send_buffer_size + ws_frame_header_max_len);
    if (!ws->send_buffer.buffer.ptr) {
        ws_signal_allocation_failure(ws, config.send_buffer_size, MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    ws->http_handshake.headers.region = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.header_buffer_size, MALLOC_TAG), config.header_buffer_size);
    if (!ws->http_handshake.headers.region.ptr) {
        ws_signal_allocation_failure(ws, config.header_buffer_size, MALLOC_TAG);
    }
    ws_create_http_handshake(ws, url_fragments.host, url_fragments.path);

    // Push websocket onto static list (for processing in tick)
    LL_ADD(ws, prev, next, ws->client->ws_list_head, ws->client->ws_list_tail);

    WEBSOCKET_MINIMAL_TRACE_POP();
    return ws;
}

#ifdef _WS_SHIM_SUPPORT
websocket_t * websocket_create_with_ssl_ctx(
    websocket_client_t * const websocket_client,
    const char * const url,
    const char * const optional_protocols,
    websocket_http_header_list_t * const optional_additional_headers,
    const websocket_config_t config,
    const mem_region_t ssl_ca,
    const mem_region_t ssl_client,
    const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ASSERT((config.ping_timeout.ms != 0) && (config.no_activity_wait_period.ms != 0) && (config.max_receivable_message_size != 0) && (config.receive_buffer_size != 0) && (config.send_buffer_size != 0) && (config.header_buffer_size != 0));

    websocket_t * const ws = heap_calloc(websocket_client->heap, sizeof(struct websocket_t), tag);
    ws->tag = tag;
    ws->client = websocket_client;
    ws->http_handshake.optional_user_headers = optional_additional_headers;
    ws->http_handshake.remaining_redirects_allowed = config.maximum_redirects;
    ws->ssl_ctx_data.custom_certs = NULL;

    curl_common_load_cert_from_memory(ws->client->heap, NULL, &ws->ws_shim_support.certs.custom_certs, ssl_client.consted, "ssl_client", MALLOC_TAG);
    curl_common_load_cert_from_memory(ws->client->heap, NULL, &ws->ws_shim_support.certs.custom_certs, ssl_ca.consted, "ssl_ca", MALLOC_TAG);
    ws->ws_shim_support.certs.default_certs = websocket_client->certs.default_certs;

    ws_url_fragments_t url_fragments;
    if (!ws_alloc_url_and_parse_fragments(ws, url, &url_fragments)) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    if (optional_protocols) {
        const size_t protocols_len = strlen(optional_protocols) + 1;
        ws->http_handshake.optional_user_protocols = heap_unchecked_alloc(ws->client->heap, protocols_len, MALLOC_TAG);
        if (!ws->http_handshake.optional_user_protocols) {
            ws_signal_allocation_failure(ws, protocols_len, MALLOC_TAG);
            WEBSOCKET_MINIMAL_TRACE_POP();
            return ws;
        }
        memcpy(ws->http_handshake.optional_user_protocols, optional_protocols, protocols_len);
    }

    ws->connection.connect_only_handle = ws_create_connect_only(ws);
    ws->connection.max_handshake_timeout = config.max_handshake_timeout;
    ws->connection.connection_start_timestamp = adk_read_millisecond_clock();
    if (!ws->connection.connect_only_handle) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }

    ws->ping_timeout = config.ping_timeout;
    ws->no_activity_wait_period = config.no_activity_wait_period;

    ws->max_receivable_message_size = config.max_receivable_message_size;
    ws->receive_buffer = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.receive_buffer_size + ws_frame_header_max_len, MALLOC_TAG), .size = config.receive_buffer_size + ws_frame_header_max_len);
    if (!ws->receive_buffer.ptr) {
        ws_signal_allocation_failure(ws, config.receive_buffer_size, MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    ws->send_buffer.buffer = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.send_buffer_size + ws_frame_header_max_len, MALLOC_TAG), .size = config.send_buffer_size + ws_frame_header_max_len);
    if (!ws->send_buffer.buffer.ptr) {
        ws_signal_allocation_failure(ws, config.send_buffer_size, MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return ws;
    }
    ws->http_handshake.headers.region = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, config.header_buffer_size, MALLOC_TAG), config.header_buffer_size);
    if (!ws->http_handshake.headers.region.ptr) {
        ws_signal_allocation_failure(ws, config.header_buffer_size, MALLOC_TAG);
    }
    ws_create_http_handshake(ws, url_fragments.host, url_fragments.path);

    // Push websocket onto static list (for processing in tick)
    LL_ADD(ws, prev, next, ws->client->ws_list_head, ws->client->ws_list_tail);

    WEBSOCKET_MINIMAL_TRACE_POP();
    return ws;
}
#endif

websocket_error_e websocket_send(websocket_t * const ws, const websocket_message_type_e message_type, const const_mem_region_t message) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if ((ws->status != websocket_status_connecting) && (ws->status != websocket_status_connected)) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return (ws->error != websocket_error_none) ? ws->error : websocket_error_not_connected;
    }
    ASSERT((message_type == websocket_message_text) || (message_type == websocket_message_binary));
    websocket_send_message_t * const msg = heap_unchecked_alloc(ws->client->heap, sizeof(websocket_send_message_t), MALLOC_TAG);
    if (!msg) {
        WEBSOCKET_MINIMAL_TRACE_POP();
        return websocket_error_allocation_failure;
    }
    msg->region = MEM_REGION(.ptr = heap_unchecked_alloc(ws->client->heap, message.size, MALLOC_TAG), .size = message.size);
    if (!msg->region.ptr) {
        heap_free(ws->client->heap, msg, MALLOC_TAG);
        WEBSOCKET_MINIMAL_TRACE_POP();
        return websocket_error_allocation_failure;
    }
    memcpy(msg->region.ptr, message.ptr, message.size);
    msg->op_code = (message_type == websocket_message_text) ? ws_op_text : ws_op_binary;
    msg->total_sent = 0;
    LL_ADD(msg, prev, next, ws->send_msg_head, ws->send_msg_tail);
    WEBSOCKET_MINIMAL_TRACE_POP();
    return websocket_error_none;
}

websocket_error_e websocket_read_message(websocket_t * const ws, websocket_message_t * const out_message) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if (!ws->recv_msg_head || !ws->recv_msg_head->complete) {
        if (ws->error != websocket_error_none) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return ws->error;
        } else if (ws->status == websocket_status_closed) {
            WEBSOCKET_MINIMAL_TRACE_POP();
            return websocket_error_not_connected;
        }
        WEBSOCKET_MINIMAL_TRACE_POP();
        return websocket_error_no_readable_messages;
    }

    websocket_received_message_t * const msg = ws->recv_msg_head;
    LL_REMOVE(msg, prev, next, ws->recv_msg_head, ws->recv_msg_tail);
    *out_message = (websocket_message_t){
        .client = ws->client,
        .message_type = msg->type,
        .region = msg->region.consted,
    };

    // user is responsible for freeing the actual underlying message, however the linked list msg part must be freed.
    heap_free(ws->client->heap, msg, MALLOC_TAG);

#ifdef _WS_SHIM_SUPPORT
    ws->ws_shim_support.last_message = *out_message;
#endif
    WEBSOCKET_MINIMAL_TRACE_POP();
    return websocket_error_none;
}

void websocket_free_message(const websocket_message_t message, const char * const tag) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    heap_free(message.client->heap, (void *)message.region.ptr, tag);
    WEBSOCKET_MINIMAL_TRACE_POP();
}

#ifdef _WS_SHIM_SUPPORT
void ws_shim_support_free_message(websocket_t * ws, const char * const tag) {
    websocket_free_message(ws->ws_shim_support.last_message, tag);
    ZEROMEM(&ws->ws_shim_support.last_message);
}
#endif

static void ws_free(websocket_t * const ws) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    ws_free_connect_only_handle(ws->client, ws->connection.connect_only_handle);
#ifdef _WS_SHIM_SUPPORT
    curl_common_free_certs(ws->client->heap, NULL, &ws->ws_shim_support.certs.custom_certs, MALLOC_TAG);
#endif
    {
        websocket_received_message_t * msg = ws->recv_msg_head;
        while (msg) {
            websocket_received_message_t * const next = msg->next;
            heap_free(ws->client->heap, msg->region.ptr, MALLOC_TAG);
            heap_free(ws->client->heap, msg, MALLOC_TAG);
            msg = next;
        }
    }
    {
        websocket_send_message_t * msg = ws->send_msg_head;
        while (msg) {
            websocket_send_message_t * const next = msg->next;
            heap_free(ws->client->heap, msg->region.ptr, MALLOC_TAG);
            heap_free(ws->client->heap, msg, MALLOC_TAG);
            msg = next;
        }
    }
    if (ws->http_handshake.requested_protocols.ptr) {
        heap_free(ws->client->heap, ws->http_handshake.requested_protocols.ptr, MALLOC_TAG);
    }
    if (ws->http_handshake.headers.region.ptr) {
        heap_free(ws->client->heap, ws->http_handshake.headers.region.ptr, MALLOC_TAG);
    }
    if (ws->receive_buffer.ptr) {
        heap_free(ws->client->heap, ws->receive_buffer.ptr, MALLOC_TAG);
    }
    if (ws->send_buffer.buffer.ptr) {
        heap_free(ws->client->heap, ws->send_buffer.buffer.ptr, MALLOC_TAG);
    }
    ws_free_temporary_handshake_data(ws);
    curl_common_free_custom_certs(ws->client->heap, NULL, ws->ssl_ctx_data.custom_certs, MALLOC_TAG);
    ws->ssl_ctx_data.custom_certs = NULL;
    heap_free(ws->client->heap, ws, MALLOC_TAG);
    WEBSOCKET_MINIMAL_TRACE_POP();
}

void websocket_close(websocket_t * const ws, const http_status_code_e * const optional_status_code) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    if (ws->status == websocket_status_closed) {
        LL_REMOVE(ws, prev, next, ws->client->ws_list_head, ws->client->ws_list_tail);
        ws_free(ws);
    } else {
        ws->status = websocket_status_user_initiated_closing;
        ws->close_message.status_code = optional_status_code ? *optional_status_code : http_status_websocket_going_away;
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
}

websocket_status_e websocket_get_status(const websocket_t * const ws) {
    return ws->status;
}

websocket_error_e websocket_get_error(const websocket_t * const ws) {
    return ws->error;
}

static bool ws_tick_curl(websocket_client_t * const websocket_client) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    int num_running_handles;
    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(websocket_client->curl_ctx);
    WEBSOCKET_MINIMAL_TRACE_PUSH("curl_multi_perform");
    curl_multi_perform(websocket_client->curl_multi, &num_running_handles);
    WEBSOCKET_MINIMAL_TRACE_POP();
    for (;;) {
        int msgs_in_queue;
        CURLMsg * const msg = curl_multi_info_read(websocket_client->curl_multi, &msgs_in_queue);
        if (!msg) {
            break;
        }
        ASSERT(msg->msg == CURLMSG_DONE);

        char * info_private;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &info_private);

        curl_connect_only_handle_t * const handle = (curl_connect_only_handle_t *)info_private;
        ASSERT(handle->curl_handle == msg->easy_handle);

        const CURLcode result = msg->data.result;

        if (result == CURLE_OK) {
            handle->result = curl_result_ok;
        } else if (result == CURLE_OPERATION_TIMEDOUT) {
            handle->result = curl_result_timeout;
        } else {
            handle->result = curl_result_error;
        }

        // notify the connect only handle that it is connected to the peer and ready to have curl_easy_send/recv operations used on it.
        handle->status = curl_status_ready;
        // we do not free the handles here since we're in connect only and need them to linger.
    }
    adk_curl_set_context(ctx_old);
    WEBSOCKET_MINIMAL_TRACE_POP();
    return num_running_handles > 0;
}

static void ws_check_handshake_timeout(websocket_t * const ws, const milliseconds_t curr_timestamp) {
    if (ws->connection.max_handshake_timeout.ms && (curr_timestamp.ms - ws->connection.connection_start_timestamp.ms > ws->connection.max_handshake_timeout.ms)) {
        ws->status = websocket_status_closed;
        ws->error = websocket_error_connection_handshake_timeout;
    }
}

bool websocket_client_tick(websocket_client_t * const websocket_client) {
    WEBSOCKET_MINIMAL_TRACE_PUSH_FN();
    const uint64_t epoch_ms_timestamp = ws_get_epoch_timestamp_u64();
    ws_tick_curl(websocket_client);
    websocket_t * ws = websocket_client->ws_list_head;
    const milliseconds_t curr_timestamp = adk_read_millisecond_clock();
    while (ws != NULL) {
        if (ws->status == websocket_status_connecting) {
            if (!ws->connection.socket_init) {
                ws_read_http_connection(ws);
            }
            if (ws->connection.socket_init && (ws->headers_status != handshake_headers_sent)) {
                ws_send_http_handshake(ws);
            }
            if (ws->connection.socket_init && (ws->headers_status == handshake_headers_sent)) {
                ws_read_http_handshake(ws);
            }
            if (ws->connection.socket_init) {
                ws_check_sock_error(ws);
            }
            ws_check_handshake_timeout(ws, curr_timestamp);
        }
        if (ws->connection.socket_init && ((ws->status == websocket_status_connected) || (ws->status == websocket_status_closing) || (ws->status == websocket_status_user_initiated_closing))) {
            ASSERT(ws->status != websocket_status_user_initiated_closed);
            ws_read_from_sock(ws);
            if (ws->status != websocket_status_user_initiated_closed) {
                ws_send_to_sock(ws, epoch_ms_timestamp);
            }
            if (ws->status != websocket_status_user_initiated_closed) {
                ws_check_sock_error(ws);
            }
        }
        if (ws->status == websocket_status_user_initiated_closed || (!ws->connection.socket_init && ws->status == websocket_status_user_initiated_closing)) {
            websocket_t * const next = ws->next;
            LL_REMOVE(ws, prev, next, websocket_client->ws_list_head, websocket_client->ws_list_tail);
            ws_free(ws);
            ws = next;
        } else {
            ws = ws->next;
        }
    }
    WEBSOCKET_MINIMAL_TRACE_POP();
    return !!websocket_client->ws_list_head;
}

const char * websocket_error_to_str(const websocket_error_e ws_error) {
    const char * error_strings[] = {
        [websocket_error_none] = "No errors/operation succeeded.",
        [websocket_error_protocol] = "Invalid action per protocol spec. Either missing data, data at wrong time, or other issues.",
        [websocket_error_payload_too_large] = "Received too large of a payload for us to process.",
        [websocket_error_allocation_failure] = "Could not allocate sufficient memory for an operation (heap out of memory).",
        [websocket_error_timeout] = "Too much time has passed since the last ping and the connection is assumed dead.",
        [websocket_error_upgrade_failed_send_failure] = "Underlying socket send failed during initial handshake.",
        [websocket_error_upgrade_failed_read_failure] = "Underlying socket read failed during initial handshake.",
        [websocket_error_upgrade_failed_handshake_failure] = "Received a non 101 status code for upgrade. Or other handshake failure.",
        [websocket_error_connection] = "Generic connection failure during initial connection to the remote end.",
        [websocket_error_connection_handshake_timeout] = "Attempting to connect to the remote endpoint and exchange handshakes took too long",
        [websocket_error_socket_read_failure] = "Underlying socket read operation failure.",
        [websocket_error_socket_send_failure] = "Underlying socket send operation failure.",
        [websocket_error_socket_exception] = "Underlying socket exception.",
        [websocket_error_no_readable_messages] = "No readable messages in queue, try again later.",
        [websocket_error_not_connected] = "Operation failed, websocket is not connected.",
    };

    ASSERT(ws_error >= websocket_error_none && ws_error < websocket_error_last);
    return error_strings[ws_error];
}

const char * websocket_get_accepted_protocols(const websocket_t * const ws) {
    if (ws->status != websocket_status_connecting) {
        return ws->http_handshake.accepted_protocols;
    }
    return NULL;
}

mem_region_t websocket_get_response_header_bytes(const websocket_t * const ws) {
    if (ws->status != websocket_status_connecting) {
        return ws->http_handshake.headers.region;
    }
    return (mem_region_t){0};
}

http_status_code_e websocket_get_response_http_status_code(const websocket_t * const ws) {
    return ws->http_handshake.http_status;
}
