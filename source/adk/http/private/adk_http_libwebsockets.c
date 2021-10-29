/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 http_libwebsockets.c

 libwebsockets integration for adk_http, adk_websockets, and other http related features
*/

#include "source/adk/file/file.h"
#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/adk_websocket_backend_selector.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_thread.h"

// this is not a clean library. so we disable the warnings the headers can trigger, then re-enable them for us.
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4214) // non standard bit field
#endif
#include "libwebsockets.h"
#ifdef _WIN32
#pragma warning(pop)
#endif

#define SHIM_NAME(NAME) http2_shim_##NAME

#include <time.h>

#define CLIENT_PROTOCOL "lws-m5"
#define TAG_LWS FOURCC('L', 'W', 'S', 'S')

const char adk_http_init_default_cert_path[] = "certs/ca-bundle.crt";
const char adk_http_init_default_charles_path[] = "certs/charles.pem";

struct adk_http_handle_base_t;
typedef void (*destroy_vtable_call)(struct adk_http_handle_base_t * const handle);

typedef struct adk_http_handle_base_t {
    destroy_vtable_call destroy;
    struct adk_http_handle_base_t * next;
    struct adk_http_handle_base_t * prev;
    const char * destruction_tag;
} adk_http_handle_base_t;

static struct {
    heap_t heap;
    sb_mutex_t * mutex;
    struct lws_context * context;
    struct lws_vhost * vhost; // Default global vhost
    char proxy_address[128]; // Used for vhost with alternate SSL context
    char socks_address[128]; // Used for vhost with alternate SSL context
    int num_open_handles;
    adk_http_handle_base_t * alive_handles_head;
    adk_http_handle_base_t * alive_handles_tail;
} statics;

static const uint32_t lws_backoff_ms[] = {1000, 2000, 3000, 4000, 5000};

static const lws_retry_bo_t lws_retry_policy = {
    .retry_ms_table = lws_backoff_ms,
    .retry_ms_table_count = LWS_ARRAY_SIZE(lws_backoff_ms),
    .conceal_count = LWS_ARRAY_SIZE(lws_backoff_ms),

    .secs_since_valid_ping = 3,
    .secs_since_valid_hangup = 10,

    .jitter_percent = 20,
};

struct adk_http_header_list_t {
    unsigned char * name;
    unsigned char * value;
    adk_http_header_list_t * next;
};

struct adk_http_handle_t {
    struct lws * client_wsi;
};

typedef struct adk_websocket_message_t {
    struct adk_websocket_message_t * next;
    mem_region_t message;
    adk_websocket_message_type_e message_type;
    bool completed;
} adk_websocket_message_t;

typedef struct adk_websocket_write_message_t {
    struct adk_websocket_write_message_t * next;
    mem_region_t message;
    adk_websocket_message_type_e message_type;
    adk_websocket_callbacks_t write_callbacks;
} adk_websocket_write_message_t;

struct adk_websocket_handle_t {
    adk_http_handle_base_t http_user_base;
    lws_sorted_usec_list_t sul;
    struct lws * client_wsi;
    struct lws_vhost * vhost; // only different from global for alternate SSL context.
    uint16_t retry_count;

    // these fields are only here to make sure we keep the values alive long enough for creating the context.
    const char * url_server_address;
    const char * url_path;
    const char * url_protocol;
    char * url_buffer; // backing storage for server_address + path
    const char * accepted_websocket_protocols;

    adk_http_header_list_t * header_list;

    adk_websocket_callbacks_t callbacks;

    adk_websocket_write_message_t * write_message_head;
    adk_websocket_write_message_t * write_message_tail;
    adk_websocket_message_t * received_message_head;
    adk_websocket_message_t * received_message_tail;

    int port;
    int lws_ssl_connection_flags;
    adk_websocket_status_e socket_status;

    bool closed;
};

// see example at extern/libwebsockets/libwebsockets/lib/core/alloc.c:108
// if we pass in a size, we want to allocator or reallocate (if the pointer is null or not, typical realloc functionality)
// if we do not pass in a size, we want to free the pointer..
// for some reason lws passes a 'reason' to the allocator (and does nothing with it).. the reasons aren't very useful.
static void * lws_allocator(void * const ptr, const size_t alloc_size, const char * ignored) {
    void * _ptr = NULL;
    sb_lock_mutex(statics.mutex);
    if (alloc_size) {
        _ptr = heap_realloc(&statics.heap, ptr, alloc_size, MALLOC_TAG);
    } else if (ptr) {
        heap_free(&statics.heap, ptr, MALLOC_TAG);
    }
    sb_unlock_mutex(statics.mutex);
    return _ptr;
}

static void lws_log_emit(int level, const char * line) {
    switch (level) {
        case LLL_ERR:
            LOG_ERROR(TAG_LWS, "LWS: %s", line);
            break;
        case LLL_WARN:
            LOG_WARN(TAG_LWS, "LWS: %s", line);
            break;
        case LLL_DEBUG:
            LOG_DEBUG(TAG_LWS, "LWS: %s", line);
            break;
        case LLL_NOTICE:
        case LLL_INFO:
        case LLL_PARSER:
        case LLL_HEADER:
        case LLL_EXT:
        case LLL_CLIENT:
        case LLL_LATENCY:
        case LLL_USER:
        case LLL_THREAD:
        default:
            LOG_INFO(TAG_LWS, "LWS: %s", line);
            break;
    }
}

static void push_handle(adk_http_handle_base_t * const handle) {
    LL_ADD(handle, prev, next, statics.alive_handles_head, statics.alive_handles_tail);
}

static void remove_handle(adk_http_handle_base_t * const handle) {
    ASSERT(handle != NULL);
    LL_REMOVE(handle, prev, next, statics.alive_handles_head, statics.alive_handles_tail);
}

static void adk_websocket_destroy(adk_websocket_handle_t * const ws_handle) {
    LOG_INFO(TAG_LWS, "Websocket handle destroyed (%s)", ws_handle->url_server_address);

    struct lws_vhost * vhost = ws_handle->vhost;

    sb_lock_mutex(statics.mutex);
    {
        adk_websocket_write_message_t *next_msg, *msg = ws_handle->write_message_head;
        while (msg) {
            next_msg = msg->next;
            heap_free(&statics.heap, msg->message.ptr, ws_handle->http_user_base.destruction_tag);
            heap_free(&statics.heap, msg, ws_handle->http_user_base.destruction_tag);
            msg = next_msg;
        }
    }

    {
        adk_websocket_message_t *next_msg, *msg = ws_handle->received_message_head;
        while (msg) {
            next_msg = msg->next;
            heap_free(&statics.heap, msg->message.ptr, ws_handle->http_user_base.destruction_tag);
            heap_free(&statics.heap, msg, ws_handle->http_user_base.destruction_tag);
            msg = next_msg;
        }
    }

    if (ws_handle->url_buffer) {
        heap_free(&statics.heap, (void *)ws_handle->url_buffer, ws_handle->http_user_base.destruction_tag);
    }

    if (ws_handle->accepted_websocket_protocols) {
        heap_free(&statics.heap, (void *)ws_handle->accepted_websocket_protocols, ws_handle->http_user_base.destruction_tag);
    }

    if (ws_handle->header_list) {
        adk_http_header_list_t * next = ws_handle->header_list;
        for (adk_http_header_list_t * curr = next; curr != NULL; curr = next) {
            next = curr->next;
            heap_free(&statics.heap, curr->name, MALLOC_TAG);
            heap_free(&statics.heap, curr->value, MALLOC_TAG);
            heap_free(&statics.heap, curr, MALLOC_TAG);
        }
    }

    remove_handle((adk_http_handle_base_t *)ws_handle);
    lws_sul_schedule(statics.context, 0, &ws_handle->sul, NULL, LWS_SET_TIMER_USEC_CANCEL);
    if (ws_handle->client_wsi) {
        lws_set_wsi_user(ws_handle->client_wsi, NULL);
    }
    heap_free(&statics.heap, ws_handle, ws_handle->http_user_base.destruction_tag);
    sb_unlock_mutex(statics.mutex);

    if (vhost != statics.vhost) {
        lws_vhost_destroy(vhost);
        vhost = NULL;
    }

    --statics.num_open_handles;
    ASSERT(statics.num_open_handles >= 0);
}

static void connect_client(lws_sorted_usec_list_t * const sul) {
    adk_websocket_handle_t * const ws_handle = lws_container_of(sul, adk_websocket_handle_t, sul);

    struct lws_client_connect_info client_info = {0};

    client_info.context = statics.context;
    client_info.vhost = ws_handle->vhost;
    client_info.port = ws_handle->port;
    client_info.address = ws_handle->url_server_address;
    client_info.path = ws_handle->url_path;
    client_info.host = client_info.address;
    client_info.ssl_connection = ws_handle->lws_ssl_connection_flags;
    client_info.protocol = ws_handle->accepted_websocket_protocols;
    client_info.alpn = "h2;http/1.1";
    client_info.local_protocol_name = CLIENT_PROTOCOL;
    client_info.pwsi = &ws_handle->client_wsi;
    client_info.retry_and_idle_policy = &lws_retry_policy;
    client_info.userdata = ws_handle;

    if (!lws_client_connect_via_info(&client_info)) {
        if (lws_retry_sul_schedule(statics.context, 0, sul, &lws_retry_policy, connect_client, &ws_handle->retry_count)) {
            ws_handle->socket_status = adk_websocket_status_connection_failed;
            LOG_WARN(TAG_LWS, "Websocket encountered an error attempting to connect to: %s%s%s", ws_handle->url_protocol, ws_handle->url_server_address, ws_handle->url_path);
            // on failure destroy the existing handle. we never hit on_error or on_close for the user to clean them up, so they must be cleaned automatically.

            adk_websocket_write_message_t * msg = ws_handle->write_message_head;
            while (msg) {
                if (msg->write_callbacks.error) {
                    msg->write_callbacks.error(ws_handle, &msg->write_callbacks, ws_handle->socket_status);
                }
                ZEROMEM(&msg->write_callbacks);
                msg = msg->next;
            }

            if (ws_handle->callbacks.error) {
                ws_handle->callbacks.error(ws_handle, &ws_handle->callbacks, ws_handle->socket_status);
            }
            ZEROMEM(&ws_handle->callbacks);
        }
    }
}

static int websocket_callback_retry(struct lws * const wsi, adk_websocket_handle_t * const ws_handle) {
    // the lws function returns non zero on failure to schedule.
    if (lws_retry_sul_schedule_retry_wsi(wsi, &ws_handle->sul, connect_client, &ws_handle->retry_count)) {
        LOG_WARN(TAG_LWS, "Websocket exhausted max retries of [%i] (%s)", ws_handle->retry_count - 1, ws_handle->url_server_address);
        return -1;
    }

    return 0;
}

static bool is_write_queue_empty(adk_websocket_handle_t * const ws_handle) {
    sb_lock_mutex(statics.mutex);
    bool ret = ws_handle->write_message_head == NULL;
    sb_unlock_mutex(statics.mutex);
    return ret;
}

static adk_websocket_write_message_t * alloc_write_message(const const_mem_region_t message) {
    sb_lock_mutex(statics.mutex);
    adk_websocket_write_message_t * const msg = heap_calloc(&statics.heap, sizeof(adk_websocket_write_message_t), MALLOC_TAG);
    msg->message.ptr = heap_calloc(&statics.heap, LWS_PRE + message.size, MALLOC_TAG);
    memcpy((uint8_t *)msg->message.ptr + LWS_PRE, message.byte_ptr, message.size);
    sb_unlock_mutex(statics.mutex);
    return msg;
}

static void free_write_message(adk_websocket_write_message_t * const msg) {
    sb_lock_mutex(statics.mutex);
    heap_free(&statics.heap, msg->message.ptr, MALLOC_TAG);
    heap_free(&statics.heap, msg, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
}

static void enqueue_write_message(adk_websocket_handle_t * const ws_handle, adk_websocket_write_message_t * const msg) {
    ASSERT(ws_handle);

    sb_lock_mutex(statics.mutex);
    if (ws_handle->write_message_head == NULL) {
        ws_handle->write_message_head = ws_handle->write_message_tail = msg;
    } else {
        ws_handle->write_message_tail->next = msg;
        ws_handle->write_message_tail = msg;
    }
    sb_unlock_mutex(statics.mutex);
}

static adk_websocket_write_message_t * dequeue_write_message(adk_websocket_handle_t * const ws_handle) {
    ASSERT(ws_handle);
    sb_lock_mutex(statics.mutex);
    adk_websocket_write_message_t * const msg = ws_handle->write_message_head;
    if (msg) {
        ws_handle->write_message_head = ws_handle->write_message_head->next;
    }
    if (ws_handle->write_message_head == NULL) {
        ws_handle->write_message_tail = NULL;
    }
    sb_unlock_mutex(statics.mutex);
    return msg;
}

adk_websocket_status_e SHIM_NAME(adk_websocket_send)(
    adk_websocket_handle_t * const ws_handle,
    const const_mem_region_t message,
    const adk_websocket_message_type_e message_type,
    const adk_websocket_callbacks_t write_status_callback) {
    if ((ws_handle->socket_status != adk_websocket_status_connected) && (ws_handle->socket_status != adk_websocket_status_connecting)) {
        if (write_status_callback.error) {
            write_status_callback.error(ws_handle, &write_status_callback, ws_handle->socket_status);
        }
        return ws_handle->socket_status;
    }

    ASSERT(message.ptr);
    ASSERT(message.size > 0);
    ASSERT(ws_handle);

    adk_websocket_write_message_t * const msg = alloc_write_message(message);

    msg->message.size = message.size;
    msg->message_type = message_type;
    msg->write_callbacks = write_status_callback;
    // If the queue is empty before we write, then we will be the first message in the queue
    bool first_message = is_write_queue_empty(ws_handle);
    enqueue_write_message(ws_handle, msg);
    if (first_message && (ws_handle->socket_status == adk_websocket_status_connected)) {
        lws_callback_on_writable(ws_handle->client_wsi);
    }
    return ws_handle->socket_status;
}

adk_websocket_message_type_e SHIM_NAME(adk_websocket_begin_read)(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message) {
    sb_lock_mutex(statics.mutex);
    if (ws_handle->received_message_head == NULL) {
        *message = (const_mem_region_t){0};
        sb_unlock_mutex(statics.mutex);
        return ws_handle->socket_status == adk_websocket_status_connected ? adk_websocket_message_none : adk_websocket_message_error;
    }
    *message = ws_handle->received_message_head->message.consted;
    sb_unlock_mutex(statics.mutex);
    return ws_handle->received_message_head->message_type;
}

void SHIM_NAME(adk_websocket_end_read)(adk_websocket_handle_t * const ws_handle, const char * const tag) {
    sb_lock_mutex(statics.mutex);
    adk_websocket_message_t * const current_message = ws_handle->received_message_head;
    VERIFY(current_message);
    ws_handle->received_message_head = ws_handle->received_message_head->next;

    heap_free(&statics.heap, current_message->message.ptr, tag);
    heap_free(&statics.heap, current_message, tag);

    if (ws_handle->received_message_head == NULL) {
        ws_handle->received_message_tail = NULL;
    }
    sb_unlock_mutex(statics.mutex);
}

static enum lws_write_protocol sb_to_lws_message_type(const adk_websocket_message_type_e msg_type) {
    switch (msg_type) {
        case adk_websocket_message_binary:
            return LWS_WRITE_BINARY;
        case adk_websocket_message_text:
            return LWS_WRITE_TEXT;
        default:
            TRAP("unhandled LWS write protocol: %d", msg_type);
            return 0;
    }
}

static void websocket_write(struct lws * const wsi, adk_websocket_handle_t * const ws_handle) {
    adk_websocket_write_message_t * const message = dequeue_write_message(ws_handle);
    if (!message) {
        return;
    }
    // If there are more messages in the queue, we want to send LWS_CALLBACK_CLIENT_WRITEABLE after the
    // current write.  The lws_callback_on_writable should do that.
    if (!is_write_queue_empty(ws_handle)) {
        lws_callback_on_writable(wsi);
    }

    const size_t num_bytes_to_send = message->message.size;
    const int write_flags = lws_write_ws_flags(sb_to_lws_message_type(message->message_type), true, true);
    const int num_bytes_written = lws_write(wsi, message->message.byte_ptr + LWS_PRE, num_bytes_to_send, write_flags);

    if (num_bytes_written != (int)num_bytes_to_send) {
        // lws documents that it either works or returns -1.
        // and LWS does not provide a way to handle partial writes.
        VERIFY(num_bytes_written == -1);
        if (message->write_callbacks.error) {
            message->write_callbacks.error(ws_handle, &message->write_callbacks, adk_websocket_status_internal_error);
        }
        // request we kill this connection.
        lws_set_timeout(ws_handle->client_wsi, NO_PENDING_TIMEOUT, LWS_TO_KILL_ASYNC);
    } else {
        if (message->write_callbacks.success) {
            message->write_callbacks.success(ws_handle, &message->write_callbacks);
        }
    }

    ZEROMEM(&message->write_callbacks);

    free_write_message(message);
}

static void websocket_receive(
    struct lws * const wsi,
    adk_websocket_handle_t * const ws_handle,
    const const_mem_region_t received_msg,
    const adk_websocket_message_type_e msg_type) {
    // prevent us from saving/writing more data if we're never going to read it.
    if (ws_handle->socket_status != adk_websocket_status_connected) {
        return;
    }

    sb_lock_mutex(statics.mutex);
    if (ws_handle->received_message_head == NULL || ws_handle->received_message_tail->completed) {
        adk_websocket_message_t * const new_message = heap_calloc(&statics.heap, sizeof(adk_websocket_message_t), MALLOC_TAG);

        if (ws_handle->received_message_head == NULL) {
            ws_handle->received_message_tail = ws_handle->received_message_head = new_message;
        } else {
            ws_handle->received_message_tail->next = new_message;
            ws_handle->received_message_tail = new_message;
        }
    }

    adk_websocket_message_t * const received_buffer = ws_handle->received_message_tail;
    received_buffer->message.ptr = heap_realloc(&statics.heap, received_buffer->message.ptr, received_buffer->message.size + received_msg.size, MALLOC_TAG);

    if (lws_is_first_fragment(wsi)) {
        received_buffer->message_type = msg_type;
        VERIFY_MSG(received_buffer->message.size == 0, "Received another 'begin of message' before receiving an 'end of message' when parsing websocket reads\n");
    } else {
        VERIFY_MSG(received_buffer->message_type == msg_type, "Received a differing message type while buffering a message.\nexpected a message key of: %i\nbut found: %i\n", received_buffer->message_type, msg_type);
    }

    memcpy(received_buffer->message.byte_ptr + received_buffer->message.size, received_msg.ptr, received_msg.size);
    received_buffer->message.size += received_msg.size;

    if (lws_is_final_fragment(wsi)) {
        received_buffer->completed = true;
    }
    sb_unlock_mutex(statics.mutex);
}

static int websocket_callback(struct lws * wsi, enum lws_callback_reasons reason, void * user, void * in, size_t len) {
    adk_websocket_handle_t * const ws_handle = lws_wsi_user(wsi);

    // check for NULL handle.
    // spurious LWS callback after ADK has released its handles are expected
    // NULL ws_handle also occurs at LWS startup during protocol init.
    if (ws_handle) {
        switch (reason) {
            case LWS_CALLBACK_WSI_DESTROY: {
                // callback_wsi_destroy is the last call, whenever our connection is ultimately dead.
                // after this LWS will free the wsi pointer.

                if (ws_handle->socket_status != adk_websocket_status_connection_closed_by_user) {
                    // spontaneous closure, not initiated by the client of this handle.
                    // don't destroy the handle here, let the client notice it's closed and release
                    // the handle themsevles.

                    ws_handle->socket_status = adk_websocket_status_connection_closed;

                    adk_websocket_write_message_t * curr_msg = ws_handle->write_message_head;
                    while (curr_msg) {
                        if (curr_msg->write_callbacks.error) {
                            curr_msg->write_callbacks.error(ws_handle, &curr_msg->write_callbacks, ws_handle->socket_status);
                        }
                        ZEROMEM(&curr_msg->write_callbacks);
                        curr_msg = curr_msg->next;
                    }

                    ws_handle->client_wsi = NULL;
                }

                if (ws_handle->closed) {
                    // handle was marked for release by client.
                    adk_websocket_destroy(ws_handle);
                }

                lws_set_wsi_user(wsi, NULL);
                break;
            }

            case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
                if (ws_handle->header_list) {
                    unsigned char ** header_position = in;
                    unsigned char * const end = *header_position + len;

                    for (adk_http_header_list_t * header_list = ws_handle->header_list; ws_handle->header_list != NULL; header_list = ws_handle->header_list) {
                        if (lws_add_http_header_by_name(wsi, header_list->name, header_list->value, (int)strlen((char *)header_list->value), header_position, end)) {
                            return -1;
                        }

                        adk_http_header_list_t * const next = header_list->next;

                        sb_lock_mutex(statics.mutex);
                        heap_free(&statics.heap, header_list->name, MALLOC_TAG);
                        heap_free(&statics.heap, header_list->value, MALLOC_TAG);
                        heap_free(&statics.heap, header_list, MALLOC_TAG);
                        sb_unlock_mutex(statics.mutex);

                        ws_handle->header_list = next;
                    }
                }
                break;
            }
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                LOG_WARN(TAG_LWS, "Websocket encountered a client connection error: [%.*s] (%s)", (int)len, (const char *)in, ws_handle->url_server_address);

                // if we have an http error code, we must bail immediately vs trying to hide, or the handle will never be marked as dead
                if (lws_http_client_http_response(wsi) >= 400) {
                    ws_handle->socket_status = adk_websocket_status_connection_failed;

                } else {
                    // if the user is polling our websocket status to determine if they should close the connection, and we've failed (as per normal)
                    // but we are retrying. hide the status update until we've failed all our retries.
                    if ((ws_handle->socket_status == adk_websocket_status_connecting) && (ws_handle->retry_count < ARRAY_SIZE(lws_backoff_ms) - 1)) {
                        break;
                    }
                    ws_handle->socket_status = adk_websocket_status_internal_error;
                }
                adk_websocket_write_message_t * curr_msg = ws_handle->write_message_head;
                while (curr_msg) {
                    if (curr_msg->write_callbacks.error) {
                        curr_msg->write_callbacks.error(ws_handle, &curr_msg->write_callbacks, ws_handle->socket_status);
                    }
                    ZEROMEM(&curr_msg->write_callbacks);
                    curr_msg = curr_msg->next;
                }
                return -1;
            case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
                break;
            case LWS_CALLBACK_CLIENT_RECEIVE: {
                const mem_region_t received_msg = MEM_REGION(.ptr = in, .size = len);
                websocket_receive(
                    wsi,
                    ws_handle,
                    received_msg.consted,
                    (lws_frame_is_binary(wsi) ? adk_websocket_message_binary : adk_websocket_message_text));
            } break;
            case LWS_CALLBACK_CLIENT_WRITEABLE:
                // libwebsockets does not allow for arbitrary closes at any random point, however..
                // their route for 'canceling'/closing a websocket is to return a negative/nonzero in this callback, which will cause the connection to close.
                if (ws_handle->socket_status == adk_websocket_status_connection_closed_by_user) {
                    static const char client_close_reason[] = "Client closed connection";
                    // lws does not edit the buffer, they just don't obey making everything const. when its not touched.
                    lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)client_close_reason, strlen(client_close_reason));
                    return -1;
                }
                websocket_write(wsi, ws_handle);
                break;
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                LOG_INFO(TAG_LWS, "Websocket client established connection (%s)", ws_handle->url_server_address);

                ws_handle->socket_status = adk_websocket_status_connected;
                if (ws_handle->callbacks.success) {
                    ws_handle->callbacks.success(ws_handle, &ws_handle->callbacks);
                }
                ZEROMEM(&ws_handle->callbacks);
                // Handle case where messages were added to queue before connection completed
                if (!is_write_queue_empty(ws_handle)) {
                    lws_callback_on_writable(wsi);
                }
                break;
            case LWS_CALLBACK_CLIENT_CLOSED: {
                // if we're going to close, delay destroying the websocket until wsi_destroy is called.
                if ((ws_handle->socket_status == adk_websocket_status_connection_closed_by_user) || (ws_handle->socket_status == adk_websocket_status_internal_error)) {
                    break;
                }
                ws_handle->socket_status = adk_websocket_status_connecting;
                ws_handle->retry_count = 0;
                LOG_INFO(TAG_LWS, "Websocket client connection closed by peer - reconnecting (%s)", ws_handle->url_server_address);
                return websocket_callback_retry(wsi, ws_handle);
            }
            default:
                break;
        }
    } else {
        LOG_DEBUG(TAG_LWS, "websocket_callback(reason = %d) on websocket [NULL]", reason);
    }
    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

adk_http_header_list_t * SHIM_NAME(adk_http_append_header_list)(adk_http_header_list_t * list, const char * const name, const char * const value, const char * const tag) {
    const size_t name_length = strlen(name) + 1;
    const size_t value_length = strlen(value) + 1;

    sb_lock_mutex(statics.mutex);
    adk_http_header_list_t * const new_node = heap_alloc(&statics.heap, sizeof(adk_http_header_list_t), tag);
    ZEROMEM(new_node);

    new_node->next = list;

    new_node->name = heap_alloc(&statics.heap, name_length, tag);
    new_node->value = heap_alloc(&statics.heap, value_length, tag);
    sb_unlock_mutex(statics.mutex);

    memcpy(new_node->name, name, name_length);
    memcpy(new_node->value, value, value_length);

    return new_node;
}

static bool is_ssl_connection(const char * const url) {
    static const char https_str[] = "https://";
    static const char wss_str[] = "wss://";
    return ((memcmp(wss_str, url, ARRAY_SIZE(wss_str) - 1) == 0) || (memcmp(https_str, url, ARRAY_SIZE(https_str) - 1) == 0));
}

static void parse_url_into_parts(const char * const url, const size_t url_len, char * const backing_buffer, int * out_port, const char ** const out_protocol, const char ** const out_server_address, const char ** const out_path) {
    static const char url_protocol_delimiter[] = "://";
    static const size_t url_protocol_delimiter_len = ARRAY_SIZE(url_protocol_delimiter) - 1;

    // calculate the lengths and offsets of the 3 parts of our url to connect to. <Protocol><Address><Path>
    // we're storing the 3 sub string parts 'protocol' 'address' and 'path' inside the same buffer, all 3 strings expect to be null terminated, so we need to have a nul between them.
    // port will be subtracted from the 'address' and will be set to the handler. 'address' will not contain the port
    static const size_t server_address_buffer_offset = 1;
    static const size_t path_buffer_offset = 2;

    const char * const url_server_address = strstr(url, url_protocol_delimiter) + url_protocol_delimiter_len;
    const size_t url_protocol_len = url_server_address - url;
    // Start grabbing buffer with url protocol
    memcpy(backing_buffer, url, url_protocol_len);

    // Port parsing logic
    const char * port_parsing_ptr = url_server_address;
    // Pointer to ':' position (if it has)
    const char * url_port = NULL;

    if (*port_parsing_ptr == '[') {
        // Request has Ipv6 Address
        LOG_ERROR(TAG_LWS, "Url has IPv6 address. Todo: IPv6 Verification: https://jira.disneystreaming.com/browse/M5-916");
        while ((*port_parsing_ptr) && (*port_parsing_ptr != ']')) {
            port_parsing_ptr++;
        }
        if (*port_parsing_ptr == ']') {
            port_parsing_ptr++;
        }
    } else {
        while ((*port_parsing_ptr) && (*port_parsing_ptr != ':') && (*port_parsing_ptr != '/')) {
            port_parsing_ptr++;
        }
    }

    // Set default port based on protocol
    *out_port = is_ssl_connection(url) ? 443 : 80;

    // Override port number (if URL has port)
    if ((*port_parsing_ptr) && (*port_parsing_ptr == ':')) {
        url_port = port_parsing_ptr;
        port_parsing_ptr++;
        *out_port = atoi(port_parsing_ptr);
    }

    const char * const url_path = strstr(url + url_protocol_len, "/");
    // Server address length without port number
    size_t server_address_len = url_len - url_protocol_len;
    if (url_port) {
        server_address_len = (url_port - url) - url_protocol_len;
    } else if (url_path) {
        server_address_len = (url_path - url) - url_protocol_len;
    }

    const size_t server_address_with_port_len = url_path ? (url_path - url - url_protocol_len) : (url_len - url_protocol_len);
    const size_t path_len = url_len - (server_address_with_port_len + url_protocol_len);

    const size_t server_address_offset = url_protocol_len;
    const size_t path_offset = server_address_offset + server_address_with_port_len;

    // the strings are expected to be null terminated, so we have an implicit NUL spot for them.
    memcpy(backing_buffer + server_address_offset + server_address_buffer_offset, url + server_address_offset, server_address_len);
    if (url_path) {
        memcpy(backing_buffer + path_offset + path_buffer_offset, url + path_offset, path_len);
    } else {
        memcpy(backing_buffer + path_offset + path_buffer_offset, "/", 2);
    }

    *out_server_address = backing_buffer + server_address_offset + server_address_buffer_offset;
    *out_path = backing_buffer + path_offset + path_buffer_offset;
    *out_protocol = backing_buffer;
}

static adk_websocket_handle_t * websocket_create_handle(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    const char * const tag) {
    ASSERT(url && (strlen(url) > 0));

    const size_t url_len = strlen(url) + 1; // implicit space for the nul at the end
    const size_t protocol_len = strlen(supported_protocols) + 1;

    sb_lock_mutex(statics.mutex);
    adk_websocket_handle_t * const ws_handle = heap_calloc(&statics.heap, sizeof(adk_websocket_handle_t), tag);

    // space for 2 additional nuls so we can index immediately into the buffer and use them as c strings. And two more bytes '/ ' if url doesn't have path.
    ws_handle->url_buffer = heap_calloc(&statics.heap, url_len + 4, tag);
    ws_handle->accepted_websocket_protocols = heap_alloc(&statics.heap, protocol_len, tag);

    push_handle((adk_http_handle_base_t *)ws_handle);
    sb_unlock_mutex(statics.mutex);

    ws_handle->callbacks = callbacks;
    ws_handle->http_user_base.destroy = (destroy_vtable_call)adk_websocket_destroy;
    ws_handle->socket_status = adk_websocket_status_connecting;
    ws_handle->header_list = header_list;
    ws_handle->closed = false;
    ws_handle->vhost = statics.vhost; // initially set to global context

    parse_url_into_parts(url, url_len, (char *)ws_handle->url_buffer, &ws_handle->port, &ws_handle->url_protocol, &ws_handle->url_server_address, &ws_handle->url_path);

    memcpy((void *)ws_handle->accepted_websocket_protocols, supported_protocols, protocol_len);

    ws_handle->lws_ssl_connection_flags = is_ssl_connection(url) ? LCCSCF_USE_SSL : 0;

    LOG_INFO(TAG_LWS, "Websocket handle created (%s)", ws_handle->url_server_address);
    ++statics.num_open_handles;
    return ws_handle;
}

static struct lws_context_creation_info create_lws_creation_info() {
    struct lws_context_creation_info info = {0};

    static const struct lws_protocols protocols[] = {
        {CLIENT_PROTOCOL, websocket_callback, 0, 0},
        {NULL, NULL, 0, 0} // terminator
    };

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    // see https://github.com/warmcat/libwebsockets/blob/master/minimal-examples/http-client/minimal-http-client-certinfo/minimal-http-client-certinfo.c line 162
    info.fd_limit_per_thread = 3;

    return info;
}

static void websocket_set_ssl_ctx(
    adk_websocket_handle_t * ws_handle,
    mem_region_t ssl_ca,
    mem_region_t ssl_client) {
    ASSERT(ws_handle->vhost == statics.vhost);
    struct lws_context_creation_info vhost_info = create_lws_creation_info();
    vhost_info.client_ssl_ca_mem = ssl_ca.ptr;
    vhost_info.client_ssl_ca_mem_len = (unsigned int)ssl_ca.size;
    vhost_info.client_ssl_cert_mem = ssl_client.ptr;
    vhost_info.client_ssl_cert_mem_len = (unsigned int)ssl_client.size;
    ws_handle->vhost = lws_create_vhost(statics.context, &vhost_info);
    VERIFY(ws_handle->vhost);
}

static void websocket_connect(adk_websocket_handle_t * ws_handle) {
    lws_sul_schedule(statics.context, 0, &ws_handle->sul, connect_client, 1);
}

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create)(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    const char * const tag) {
    adk_websocket_handle_t * ws_handle = websocket_create_handle(url, supported_protocols, header_list, callbacks, tag);
    websocket_connect(ws_handle);
    return ws_handle;
}

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create_with_ssl_ctx)(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    mem_region_t ssl_ca,
    mem_region_t ssl_client,
    const char * const tag) {
    adk_websocket_handle_t * ws_handle = websocket_create_handle(url, supported_protocols, header_list, callbacks, tag);

    websocket_set_ssl_ctx(ws_handle, ssl_ca, ssl_client);

    if (statics.proxy_address[0] != '\0') {
        lws_set_proxy(ws_handle->vhost, statics.proxy_address);
    }

    if (statics.socks_address[0] != '\0') {
        lws_set_socks(ws_handle->vhost, statics.socks_address);
    }

    websocket_connect(ws_handle);

    return ws_handle;
}

void SHIM_NAME(adk_websocket_close)(adk_websocket_handle_t * const ws_handle, const char * const tag) {
    LOG_INFO(TAG_LWS, "Websocket close handle [%s] [ws_handle->client_wsi == %p] [ws_handle->closed == %d]", ws_handle->url_server_address, ws_handle->client_wsi, (int)ws_handle->closed);

    ASSERT(!ws_handle->closed);
    ws_handle->closed = true;

    if (ws_handle->socket_status != adk_websocket_status_connection_closed) {
        // Invoke error callback on pending messages if the connection is not already closed (which can occur on LWS_CALLBACK_WSI_DESTROY)

        adk_websocket_write_message_t * curr_msg = ws_handle->write_message_head;
        while (curr_msg) {
            if (curr_msg->write_callbacks.error) {
                curr_msg->write_callbacks.error(ws_handle, &curr_msg->write_callbacks, adk_websocket_status_connection_closed_by_user);
            }
            ZEROMEM(&curr_msg->write_callbacks);
            curr_msg = curr_msg->next;
        }
    }

    if ((ws_handle->socket_status == adk_websocket_status_connected) || (ws_handle->socket_status == adk_websocket_status_connecting)) {
        ws_handle->http_user_base.destruction_tag = tag;
        ws_handle->socket_status = adk_websocket_status_connection_closed_by_user;
    } else {
        ws_handle->http_user_base.destruction_tag = MALLOC_TAG;
    }

    if (ws_handle->client_wsi) {
        // ask LWS to close the connection.
        // LWS informs us that it will no longer invoke a particular lws* via the LWS_CALLBACK_WSI_DESTROY
        lws_sul_schedule(statics.context, 0, &ws_handle->sul, NULL, LWS_SET_TIMER_USEC_CANCEL);
        lws_set_timeout(ws_handle->client_wsi, NO_PENDING_TIMEOUT, LWS_TO_KILL_ASYNC);
    } else {
        // LWS_CALLBACK_WSI_DESTROY has already run.
        adk_websocket_destroy(ws_handle);
    }
}

adk_websocket_status_e SHIM_NAME(adk_websocket_get_status)(adk_websocket_handle_t * const ws_handle) {
    return ws_handle->socket_status;
}

static void configure_client_ssl_ca(struct lws_context_creation_info * info) {
    mem_region_t charles_ca_file_bytes = {0};
    mem_region_t default_ca_file_bytes = {0};

    const int charles_ca_file_size = get_artifact_size(sb_app_root_directory, adk_http_init_default_charles_path);
    if (charles_ca_file_size) {
        charles_ca_file_bytes = MEM_REGION(.ptr = heap_calloc(&statics.heap, charles_ca_file_size, MALLOC_TAG), .size = charles_ca_file_size);

        if (load_artifact_data(sb_app_root_directory, charles_ca_file_bytes, adk_http_init_default_charles_path, 0)) {
            LOG_INFO(TAG_LWS, "CA Cert: Loaded %s.", adk_http_init_default_charles_path);
        } else {
            LOG_ERROR(TAG_LWS, "CA Cert: not loaded: %s ", adk_http_init_default_charles_path);

            heap_free(&statics.heap, charles_ca_file_bytes.ptr, MALLOC_TAG);
            charles_ca_file_bytes = MEM_REGION(.ptr = NULL, .size = 0);
        }
    } else {
        LOG_INFO(TAG_LWS, "CA Cert: not found: %s", adk_http_init_default_charles_path);
    }

    if (info->client_ssl_ca_filepath) {
        const int default_ca_file_size = get_artifact_size(sb_app_root_directory, info->client_ssl_ca_filepath);
        if (default_ca_file_size) {
            default_ca_file_bytes = MEM_REGION(.ptr = heap_calloc(&statics.heap, default_ca_file_size, MALLOC_TAG), .size = default_ca_file_size);

            if (load_artifact_data(sb_app_root_directory, default_ca_file_bytes, info->client_ssl_ca_filepath, 0)) {
                LOG_INFO(TAG_LWS, "CA Cert: Loaded %s.", info->client_ssl_ca_filepath);
            } else {
                LOG_ERROR(TAG_LWS, "CA Cert: not loaded: %s", info->client_ssl_ca_filepath);

                heap_free(&statics.heap, default_ca_file_bytes.ptr, MALLOC_TAG);
                default_ca_file_bytes = MEM_REGION(.ptr = NULL, .size = 0);
            }
        } else {
            LOG_ERROR(TAG_LWS, "CA Cert: not found: %s", info->client_ssl_ca_filepath);
        }

        info->client_ssl_ca_filepath = NULL;
    }

    const size_t buflen = default_ca_file_bytes.size + charles_ca_file_bytes.size;

    if (buflen) {
        uint8_t * buf = heap_calloc(&statics.heap, buflen, MALLOC_TAG);
        size_t offset = 0;

        if (default_ca_file_bytes.size) {
            memcpy(buf, default_ca_file_bytes.ptr, default_ca_file_bytes.size);
            offset += default_ca_file_bytes.size;
            heap_free(&statics.heap, default_ca_file_bytes.ptr, MALLOC_TAG);
        }

        if (charles_ca_file_bytes.size) {
            memcpy(buf + offset, charles_ca_file_bytes.ptr, charles_ca_file_bytes.size);
            offset += charles_ca_file_bytes.size;
            heap_free(&statics.heap, charles_ca_file_bytes.ptr, MALLOC_TAG);
        }

        info->client_ssl_ca_mem = buf;
        info->client_ssl_ca_mem_len = (unsigned int)buflen;
    }
}

static void cleanup_client_ssl_ca(struct lws_context_creation_info * info) {
    if (info->client_ssl_ca_mem_len) {
        uint8_t * buf = (uint8_t *)info->client_ssl_ca_mem;

        heap_free(&statics.heap, buf, MALLOC_TAG);

        info->client_ssl_ca_mem = NULL;
        info->client_ssl_ca_mem_len = 0;
    }
}

void SHIM_NAME(adk_http_init)(const char * const ssl_certificate_path, const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
    ZEROMEM(&statics);

    statics.mutex = sb_create_mutex(tag);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode != system_guard_page_mode_disabled) {
        debug_heap_init(&statics.heap, region.size, 8, 0, "adk_http_lws_heap", guard_page_mode, tag);
    } else
#endif
    {
        heap_init_with_region(&statics.heap, region, 8, 0, "adk_http_lws_heap");
    }

    lws_set_allocator(lws_allocator);

    struct lws_context_creation_info lws_create_info = create_lws_creation_info();
    lws_create_info.client_ssl_ca_filepath = ssl_certificate_path;

    // lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT, lws_log_emit);
    lws_set_log_level(LLL_ERR, lws_log_emit);

    configure_client_ssl_ca(&lws_create_info);

    statics.context = lws_create_context(&lws_create_info);
    VERIFY(statics.context);

    // lws-context-vhost.h:904 `lws_vhost_destroy(...);` paraphrased:
    // calling lws_destroy_context() will take care of everything, and you shouldn't have to manually free the vhosts (unless you want to.)
    // This is considered the default vhost for most connections.

    statics.vhost = lws_create_vhost(statics.context, &lws_create_info);
    VERIFY(statics.vhost); // can't be null else other API calls will crash

    cleanup_client_ssl_ca(&lws_create_info);
}

void SHIM_NAME(adk_http_shutdown)(const char * const tag) {
    lws_cancel_service(statics.context);

    for (adk_http_handle_base_t *handle = statics.alive_handles_head, *next = NULL; handle != NULL; handle = next) {
        handle->destruction_tag = tag;
        next = handle->next;
        handle->destroy(handle);
    }

    lws_context_destroy(statics.context);

#ifndef NDEBUG
    heap_debug_print_leaks(&statics.heap);
#endif

    heap_destroy(&statics.heap, tag);
    sb_destroy_mutex(statics.mutex, tag);
}

void SHIM_NAME(adk_http_dump_heap_usage)() {
    sb_lock_mutex(statics.mutex);
    heap_dump_usage(&statics.heap);
    sb_unlock_mutex(statics.mutex);
}

heap_metrics_t SHIM_NAME(adk_http_get_heap_metrics)() {
    sb_lock_mutex(statics.mutex);
    const heap_metrics_t metrics = heap_get_metrics(&statics.heap);
    sb_unlock_mutex(statics.mutex);
    return metrics;
}

void SHIM_NAME(adk_http_set_proxy)(const char * const proxy) {
    ASSERT(statics.vhost);
    ASSERT(proxy);
    strcpy_s(statics.proxy_address, sizeof(statics.proxy_address), proxy);
    VERIFY_MSG(lws_set_proxy(statics.vhost, proxy) == 0, "Invalid proxy\nexpected format: \"<host>:<ip>\" e.g. \"127.0.0.1:8888\"\nreceived: [%s]", proxy);
}

void SHIM_NAME(adk_http_set_socks)(const char * const socks) {
    ASSERT(statics.vhost);
    ASSERT(socks);
    strcpy_s(statics.socks_address, sizeof(statics.socks_address), socks);
    VERIFY_MSG(lws_set_socks(statics.vhost, socks) == 0, "Invalid socks5\nexpected format: \"<host>:<ip>\" e.g. \"127.0.0.1:8888\"\nreceived: [%s]", socks);
}

bool SHIM_NAME(adk_http_tick)() {
    // always tick LWS, even after we release our handles LWS may have shutdown to perform
    lws_service(statics.context, 0);

    return statics.num_open_handles > 0;
}

adk_websocket_vtable_t adk_websocket_vtable_http2 = {
    .adk_http_init = SHIM_NAME(adk_http_init),
    .adk_http_shutdown = SHIM_NAME(adk_http_shutdown),
    .adk_http_dump_heap_usage = SHIM_NAME(adk_http_dump_heap_usage),
    .adk_http_get_heap_metrics = SHIM_NAME(adk_http_get_heap_metrics),
    .adk_http_set_proxy = SHIM_NAME(adk_http_set_proxy),
    .adk_http_set_socks = SHIM_NAME(adk_http_set_socks),
    .adk_http_tick = SHIM_NAME(adk_http_tick),

    .adk_http_append_header_list = SHIM_NAME(adk_http_append_header_list),
    .adk_websocket_create = SHIM_NAME(adk_websocket_create),
    .adk_websocket_create_with_ssl_ctx = SHIM_NAME(adk_websocket_create_with_ssl_ctx),
    .adk_websocket_close = SHIM_NAME(adk_websocket_close),
    .adk_websocket_get_status = SHIM_NAME(adk_websocket_get_status),
    .adk_websocket_send = SHIM_NAME(adk_websocket_send),
    .adk_websocket_begin_read = SHIM_NAME(adk_websocket_begin_read),
    .adk_websocket_end_read = SHIM_NAME(adk_websocket_end_read),
};