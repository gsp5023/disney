/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "mbedtls/ssl.h"
#include "source/adk/file/file.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_curl_common.h"
#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/http/private/adk_curl_shared.h"
#include "source/adk/http/private/adk_httpx_network_pump.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"
#include "source/adk/telemetry/telemetry.h"

#if defined(__linux__) || defined(_LEIA) || defined(_VADER)
#include <netinet/in.h>
#endif

#if defined(_LEIA) || defined(_VADER)
#include <net.h>
extern const char * __vader_get_proxy_server();
#endif
#if defined(_VADER)
#include <net6.h>
#include <netinet6/in6.h>
#endif

#include <limits.h>

#define HTTPX_CURL_TAG FOURCC('H', 'T', 'P', 'X')

enum {
    adk_httpx_max_connection_cache_size = 4,
};

static void lock(adk_httpx_client_t * const client) {
    if (client->mutex) {
        sb_lock_mutex(client->mutex);
    }
}

static void unlock(adk_httpx_client_t * const client) {
    if (client->mutex) {
        sb_unlock_mutex(client->mutex);
    }
}

void * adk_httpx_malloc(adk_httpx_client_t * const client, const size_t size, const char * const tag) {
    lock(client);
    void * const r = heap_alloc(client->heap, size, tag);
    unlock(client);
    return r;
}

void * adk_httpx_calloc(adk_httpx_client_t * const client, const size_t size, const char * const tag) {
    lock(client);
    void * const r = heap_calloc(client->heap, size, tag);
    unlock(client);
    return r;
}

void adk_httpx_free(adk_httpx_client_t * const client, void * const ptr, const char * const tag) {
    if (ptr == NULL) {
        return;
    }

    lock(client);
    heap_free(client->heap, ptr, tag);
    unlock(client);
}

void * adk_httpx_realloc(adk_httpx_client_t * const client, void * const ptr, const size_t size, const char * const tag) {
    lock(client);
    void * const r = heap_realloc(client->heap, ptr, size, tag);
    unlock(client);
    return r;
}

void adk_httpx_client_dump_heap_usage(adk_httpx_client_t * const client) {
    heap_dump_usage(client->heap);
}

heap_metrics_t adk_httpx_client_get_heap_metrics(adk_httpx_client_t * const client) {
    return heap_get_metrics(client->heap);
}

// Handle should be freed only after the request has went through the networking pump and been completed.
static void free_handle(adk_httpx_client_t * const client, adk_httpx_handle_t * const handle) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_request_t * request = handle->request;

    if (handle->response) {
        handle->response->handle = NULL;
    }

    LL_REMOVE(handle, prev, next, client->handle_head, client->handle_tail);

    CURL_PUSH_CTX(client->ctx);
    curl_slist_free_all(request->headers);
    curl_easy_cleanup(request->curl);
    CURL_POP_CTX();

    if (request->fragments_head && client->network_pump) {
        adk_httpx_network_pump_fragments_free(client->network_pump, request->fragments_head, request->fragments_tail);
        request->fragments_head = request->fragments_tail = NULL;
    }

    sb_destroy_mutex(handle->response_lock, MALLOC_TAG);

    sb_destroy_mutex(request->fragments_lock, MALLOC_TAG);

    adk_httpx_free(client, request->body.ptr, MALLOC_TAG);
    adk_httpx_free(client, request, MALLOC_TAG);
    adk_httpx_free(client, handle, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

static struct {
    bool use_global_certs;
    curl_common_ca_chain_t * ca_chain;
    mem_region_t region;
    bool init;
    adk_httpx_http2_options http2;
} statics = {
    .init = false,
    .http2 = {
        .enabled = false,
        .use_multiplexing = false,
        .multiplex_wait_for_existing_connection = false,
    },
};

enum {
    // Static size determined by size of loaded cert files (doubled just in case).
    global_certs_region_size = 1 * 1024 * 512
};

void adk_httpx_init_global_certs(bool use_global_certs) {
    ASSERT_IS_MAIN_THREAD();

    statics.use_global_certs = use_global_certs;
    if (statics.use_global_certs) {
        ASSERT_MSG(statics.ca_chain == NULL, "Global certs can only be initialized once");
        LOG_DEBUG(HTTPX_CURL_TAG, "Using global certs");
        statics.region = sb_map_pages(PAGE_ALIGN_INT(global_certs_region_size), system_page_protect_read_write);
        heap_t * const heap = heap_emplace_init_with_region(statics.region, 8, 0, "httpx_global_certs");

        curl_common_certs_t certs = {.custom_certs = {.head = NULL, .tail = NULL}};

        certs.default_certs = curl_common_load_default_certs_into_memory(heap, NULL, MALLOC_TAG);
        curl_common_parse_certs_to_ca_chain(&certs, &statics.ca_chain);
        curl_common_free_certs(heap, NULL, &certs.default_certs, MALLOC_TAG);

#ifndef NDEBUG
        heap_debug_print_leaks(heap);
#endif

        heap_destroy(heap, MALLOC_TAG);
        sb_unmap_pages(statics.region);
    }

    statics.init = true;
}

void adk_httpx_free_global_certs() {
    ASSERT_IS_MAIN_THREAD();

    ASSERT(statics.init && statics.use_global_certs);
    curl_common_free_ca_chain(statics.ca_chain);
    statics.ca_chain = NULL;
}

void adk_httpx_enable_http2(adk_httpx_http2_options options) {
    statics.http2 = options;

#ifndef ADK_CURL_HTTP2
    if (options.enabled) {
        LOG_WARN(HTTPX_CURL_TAG, "HTTP/2 was enabled but is not supported!");
    }
#endif
}

adk_httpx_client_t * adk_httpx_client_create(
    const mem_region_t region,
    const mem_region_t fragments_region,
    const uint32_t fragment_size,
    const uint32_t pump_sleep_period,
    const system_guard_page_mode_e guard_page_mode,
    const adk_httpx_init_mode_e init_mode,
    const char * const name) {
    HTTPX_TRACE_PUSH_FN();
    ASSERT(statics.init);
    if (statics.use_global_certs) {
        ASSERT(statics.ca_chain);
    }

    heap_t * heap;
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        heap = debug_heap_emplace_init(region.size, 8, 0, name, guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap = heap_emplace_init_with_region(region, 8, 0, name);
    }

    adk_httpx_client_t * const client = heap_alloc(heap, sizeof(adk_httpx_client_t), MALLOC_TAG);
    ZEROMEM(client);
    client->thread_id = sb_get_current_thread_id();

    client->heap = heap;

    client->mutex = (init_mode == adk_httpx_init_normal) ? sb_create_mutex(MALLOC_TAG) : NULL;

    client->ctx = adk_curl_context_create(
        (void *)client,
        (adk_curl_context_callbacks_t){
            .malloc = (adk_curl_context_malloc_t)adk_httpx_malloc,
            .calloc = (adk_curl_context_calloc_t)adk_httpx_calloc,
            .free = (adk_curl_context_free_t)adk_httpx_free,
            .realloc = (adk_curl_context_realloc_t)adk_httpx_realloc,
        });

    if (!statics.use_global_certs) {
        client->certs.default_certs = curl_common_load_default_certs_into_memory(client->heap, client->mutex, MALLOC_TAG);
    }

#ifdef ADK_CURL_HTTP2
    // default: CURLPIPE_MULTIPLEX
    if (statics.http2.enabled && !statics.http2.use_multiplexing) {
        curl_multi_setopt(client->multi, CURLMOPT_PIPELINING, CURLPIPE_NOTHING);
    }
#endif

    CURL_PUSH_CTX(client->ctx);
    CURLM * const multi = curl_multi_init();
    VERIFY(multi);
    client->multi = multi;

    curl_multi_setopt(client->multi, CURLMOPT_MAXCONNECTS, adk_httpx_max_connection_cache_size);

    CURL_POP_CTX();

    adk_httpx_network_pump_init(client, fragments_region, fragment_size, pump_sleep_period, guard_page_mode);

    HTTPX_TRACE_POP();
    return client;
}

void adk_httpx_client_free(adk_httpx_client_t * const client) {
    HTTPX_TRACE_PUSH_FN();

    ASSERT_IS_SAME_THREAD(client->thread_id);
    // Network pump depends on the api's curl context to run queries
    // and should be terminated before the context destruction
    adk_httpx_network_pump_shutdown(client->network_pump);

    if (!statics.use_global_certs) {
        curl_common_free_certs(client->heap, client->mutex, &client->certs.default_certs, MALLOC_TAG);
        curl_common_free_certs(client->heap, client->mutex, &client->certs.custom_certs, MALLOC_TAG);
    }

    for (adk_httpx_handle_t *handle = client->handle_head, *next; handle != NULL; handle = next) {
        next = handle->next;

        free_handle(client, handle);
    }

    CURL_PUSH_CTX(client->ctx);
    curl_multi_cleanup(client->multi);
    CURL_POP_CTX();

    adk_curl_context_destroy(client->ctx);

    sb_mutex_t * const mutex = client->mutex;
    if (mutex != NULL) {
        sb_destroy_mutex(mutex, MALLOC_TAG);
    }

    heap_t * const heap = client->heap;

    heap_free(heap, client, MALLOC_TAG);

#ifndef NDEBUG
    heap_debug_print_leaks(heap);
#endif

    heap_destroy(heap, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

bool adk_httpx_client_tick(adk_httpx_client_t * const client) {
    HTTPX_TRACE_PUSH_FN();
    ASSERT_IS_SAME_THREAD(client->thread_id);

    bool active_handles = false;

    // NOTE: the client->lock() does not need to be captured here because we do not
    // modify or destroy data on the client. the network pump can safely operate on this
    // client while we process here.
    adk_httpx_handle_t * handle = client->handle_head;
    while (handle) {
        adk_httpx_handle_t * const next_handle = handle->next;
        adk_httpx_request_t * const request = handle->request;
        adk_httpx_response_t * const response = handle->response;
        const bool response_freed = response == NULL;

        const bool buffered_headers_fragments = (request->on_header == NULL) || (request->buffering_mode & adk_httpx_buffering_mode_header) != 0;
        const bool buffered_body_fragments = (request->on_body == NULL) || (request->buffering_mode & adk_httpx_buffering_mode_body) != 0;

        sb_lock_mutex(request->fragments_lock);
        adk_httpx_network_pump_fragment_t * const fragments_head = request->fragments_head;
        adk_httpx_network_pump_fragment_t * const fragments_tail = request->fragments_tail;
        request->fragments_head = request->fragments_tail = NULL;
        const bool data_stream_ended = handle->data_stream_ended;
        sb_unlock_mutex(request->fragments_lock);

        // Processing fragments.
        // If the response has been freed, this section is ignored and callbacks won't be called. Fragment would still be returned to the pump's pool.
        for (const adk_httpx_network_pump_fragment_t * fragment = fragments_head; fragment != NULL && !response_freed && !request->aborted_by_callback; fragment = fragment->next) {
            ASSERT(fragment->size > 0);
            if (fragment->type == adk_httpx_network_pump_header_fragment) {
                if ((request->on_header != NULL)) {
                    size_t offset = 0;
                    while (offset < fragment->size) {
                        const uint8_t * header = fragment->region.byte_ptr + offset;

                        do {
                            ++offset;
                        } while (fragment->region.byte_ptr[offset - 1] != '\n');

                        const size_t header_length = (fragment->region.adr + offset) - (uintptr_t)header;
                        const mem_region_t header_value = MEM_REGION(.ptr = header, .size = header_length);
                        if (!request->on_header(response, header_value.consted, request->userdata)) {
                            // Storing response->result here does race with the pump if it happens
                            // to encounter an error. In this case, by definition the result is undefined since technically
                            // both things happened: callback was aborted, and curl had an error. Which error code
                            // wins is not functionally important since an error will be signaled.
                            handle->response->result = adk_httpx_error;
                            request->aborted_by_callback = true;
                            break;
                        }
                    }
                }

                if (buffered_headers_fragments) {
                    response->headers.ptr = adk_httpx_realloc(client, response->headers.ptr, response->headers.size + fragment->size + 1, MALLOC_TAG);
                    memcpy(response->headers.byte_ptr + response->headers.size, fragment->region.ptr, fragment->size);
                    response->headers.size += fragment->size;
                    // add nul so string functions will always find a nul inside our buffer regardless of what data we receive.
                    response->headers.byte_ptr[response->headers.size] = 0;
                }
            } else {
                ASSERT(fragment->type == adk_httpx_network_pump_body_fragment);
                if ((request->on_body != NULL) && !request->on_body(response, CONST_MEM_REGION(.byte_ptr = fragment->region.byte_ptr, .size = fragment->size), request->userdata)) {
                    // Storing response->result here does race with the pump if it happens
                    // to encounter an error. In this case, by definition the result is undefined since technically
                    // both things happened: callback was aborted, and curl had an error. Which error code
                    // wins is not functionally important since an error will be signaled.
                    handle->response->result = adk_httpx_error;
                    request->aborted_by_callback = true;
                    break;
                }

                if (buffered_body_fragments) {
                    response->body.ptr = adk_httpx_realloc(client, response->body.ptr, response->body.size + fragment->size + 1, MALLOC_TAG);
                    memcpy(response->body.byte_ptr + response->body.size, fragment->region.ptr, fragment->size);
                    response->body.size += fragment->size;
                    // add nul so string functions will always find a nul inside our buffer regardless of what data we receive.
                    response->body.byte_ptr[response->body.size] = 0;
                }
            }
        }

        if (fragments_head) {
            adk_httpx_network_pump_fragments_free(client->network_pump, fragments_head, fragments_tail);
        }

        if (data_stream_ended) {
            // Ensure that response wasn't freed before the end of curl operation on the request
            if (response_freed == false) {
                handle->response->status = adk_future_status_ready;
                if (handle->request->on_complete != NULL) {
                    handle->request->on_complete(handle->response, handle->request->userdata);
                    // handle may have been freed by on_complete.
                    // if it was _not_ freed then this will be released in the next tick call.
                }
            }
            free_handle(client, handle);
        } else {
            active_handles = true;
        }

        handle = next_handle;
    }

    HTTPX_TRACE_POP();
    return active_handles;
}

static void adk_httpx_set_http2_options(CURL * const curl) {
#ifdef ADK_CURL_HTTP2
    if (statics.http2.enabled) {
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS); // default
        curl_easy_setopt(curl, CURLOPT_PIPEWAIT, (long)statics.http2.multiplex_wait_for_existing_connection);
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    }
#endif
}

adk_httpx_request_t * adk_httpx_client_request(
    adk_httpx_client_t * const client,
    const adk_httpx_method_e method,
    const char * const url) {
    HTTPX_TRACE_PUSH_FN();
    ASSERT_IS_SAME_THREAD(client->thread_id);

    adk_httpx_request_t * const request = (adk_httpx_request_t *)adk_httpx_malloc(client, sizeof(adk_httpx_request_t), MALLOC_TAG);
    ZEROMEM(request);

    request->client = client;

    {
        adk_curl_context_t * const ctx_temp = adk_curl_get_context();
        adk_curl_set_context(client->ctx);
        request->curl = curl_easy_init();

        curl_easy_setopt(request->curl, CURLOPT_URL, url);

        curl_common_set_socket_callbacks(request->curl, client->ctx);

#if defined(_LEIA) || defined(_VADER)
        const char * proxy = __vader_get_proxy_server();
        if (proxy) {
            curl_easy_setopt(request->curl, CURLOPT_PROXY, proxy);
            curl_easy_setopt(request->curl, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1);
        }
#endif

        adk_httpx_request_set_follow_location(request, true);

        switch (method) {
            case adk_httpx_method_get:
                break;
            case adk_httpx_method_post:
                curl_easy_setopt(request->curl, CURLOPT_POST, 1);
                curl_easy_setopt(request->curl, CURLOPT_POSTFIELDSIZE, 0);
                break;
            case adk_httpx_method_put:
                curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "PUT");
                break;
            case adk_httpx_method_patch:
                curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
                break;
            case adk_httpx_method_delete:
                curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case adk_httpx_method_head:
                curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "HEAD");
                break;
        }
        adk_curl_set_context(ctx_temp);
    }

    HTTPX_TRACE_POP();
    return request;
}

void adk_httpx_request_set_verbose(adk_httpx_request_t * const request, bool verbose) {
    HTTPX_TRACE_PUSH_FN();

    CURL_PUSH_CTX(request->client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_VERBOSE, (long)verbose);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_follow_location(adk_httpx_request_t * const request, bool follow_location) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_FOLLOWLOCATION, (long)follow_location);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_preferred_receive_buffer_size(
    adk_httpx_request_t * const request,
    uint32_t preferred_receive_buffer_size) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_BUFFERSIZE, (long)preferred_receive_buffer_size);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_header(adk_httpx_request_t * const request, const char * const header) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->ctx);
    request->headers = curl_slist_append(request->headers, header);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_timeout(adk_httpx_request_t * const request, uint64_t timeout) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_TIMEOUT, (long)timeout);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_body(
    adk_httpx_request_t * const request,
    const uint8_t * const body,
    const size_t body_size) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_client_t * const client = request->client;
    request->body.ptr = adk_httpx_malloc(client, body_size, MALLOC_TAG);
    request->body.size = body_size;

    memcpy(request->body.ptr, (void *)body, body_size);

    CURL_PUSH_CTX(client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_POSTFIELDS, request->body.ptr);
    curl_easy_setopt(request->curl, CURLOPT_POSTFIELDSIZE, body_size);
    CURL_POP_CTX();

    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_user_agent(
    adk_httpx_request_t * const request,
    const char * const user_agent) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->ctx);
    curl_easy_setopt(request->curl, CURLOPT_USERAGENT, user_agent);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_on_header(adk_httpx_request_t * const request, adk_httpx_on_header_t on_header) {
    request->on_header = on_header;
}

void adk_httpx_request_set_on_body(adk_httpx_request_t * const request, adk_httpx_on_body_t on_body) {
    request->on_body = on_body;
}

void adk_httpx_request_set_on_complete(adk_httpx_request_t * const request, adk_httpx_on_complete_t on_complete) {
    request->on_complete = on_complete;
}

void adk_httpx_request_set_userdata(adk_httpx_request_t * const request, void * const userdata) {
    request->userdata = userdata;
}

void adk_httpx_request_set_buffering_mode(adk_httpx_request_t * const request, adk_httpx_buffering_mode_e mode) {
    request->buffering_mode = mode;
}

void adk_httpx_request_accept_encoding(adk_httpx_request_t * const request, adk_httpx_encoding_e encoding) {
    HTTPX_TRACE_PUSH_FN();
    switch (encoding) {
        case adk_httpx_encoding_none:
            CURL_PUSH_CTX(request->client->ctx);
            curl_easy_setopt(request->curl, CURLOPT_ACCEPT_ENCODING, NULL);
            CURL_POP_CTX();
            break;
        case adk_httpx_encoding_gzip:
            CURL_PUSH_CTX(request->client->ctx);
            curl_easy_setopt(request->curl, CURLOPT_ACCEPT_ENCODING, (void *)"gzip");
            CURL_POP_CTX();
            break;
        default:
            TRAP("Invalid encoding: %d", encoding);
    }
    HTTPX_TRACE_POP();
}

adk_httpx_response_t * adk_httpx_send(
    adk_httpx_request_t * const request) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_client_t * const client = request->client;
    ASSERT(client);

    adk_httpx_response_t * const response = (adk_httpx_response_t *)adk_httpx_malloc(client, sizeof(adk_httpx_response_t), MALLOC_TAG);
    ZEROMEM(response);
    response->client = client;
    response->status = adk_future_status_pending;
    response->result = adk_httpx_ok;

    adk_httpx_handle_t * const handle = (adk_httpx_handle_t *)adk_httpx_malloc(client, sizeof(adk_httpx_handle_t), MALLOC_TAG);
    ZEROMEM(handle);
    LL_ADD(handle, prev, next, client->handle_head, client->handle_tail);
    handle->request = request;
    handle->response = response;
    handle->data_stream_ended = false;
    handle->response_lock = sb_create_mutex(MALLOC_TAG);

    response->handle = handle;

    if (statics.use_global_certs) {
        curl_common_set_ssl_ctx_ca_chain(request->curl, client->ctx, statics.ca_chain);
    } else {
        client->ssl_ctx_data.heap = client->heap;
        client->ssl_ctx_data.certs = &client->certs;
        curl_common_set_ssl_ctx(&client->ssl_ctx_data, request->curl, client->ctx);
    }

    adk_httpx_set_http2_options(request->curl);

    request->fragments_lock = sb_create_mutex(MALLOC_TAG);

    adk_httpx_network_pump_add_request(handle);

    HTTPX_TRACE_POP();
    return response;
}

adk_future_status_e adk_httpx_response_get_status(const adk_httpx_response_t * const response) {
    return sb_atomic_load((sb_atomic_int32_t *)&response->status, memory_order_relaxed);
}

adk_httpx_result_e adk_httpx_response_get_result(const adk_httpx_response_t * const response) {
    return response->result;
}

const char * adk_httpx_response_get_error(const adk_httpx_response_t * const response) {
    return response->error;
}

int64_t adk_httpx_response_get_response_code(const adk_httpx_response_t * const response) {
    return response->response_code;
}

const_mem_region_t adk_httpx_response_get_headers(const adk_httpx_response_t * const response) {
    return response->headers.consted;
}

const_mem_region_t adk_httpx_response_get_body(const adk_httpx_response_t * const response) {
    return response->body.consted;
}

void adk_httpx_response_free(adk_httpx_response_t * const response) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_client_t * const client = response->client;
    adk_httpx_handle_t * const handle = response->handle;

    if (handle) {
        sb_lock_mutex(handle->response_lock);
        {
            handle->response = NULL;
        }
        sb_unlock_mutex(handle->response_lock);
    }

    adk_httpx_free(client, response->error, MALLOC_TAG);
    adk_httpx_free(client, response->headers.ptr, MALLOC_TAG);
    adk_httpx_free(client, response->body.ptr, MALLOC_TAG);
    adk_httpx_free(client, response, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

static size_t adk_httpx_fetch_header_callback(const char * const ptr, const size_t size, const size_t nmemb, void * const userdata) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_fetch_callbacks_t * const callbacks = (adk_httpx_fetch_callbacks_t *)userdata;

    const size_t num_bytes = size * nmemb;
    const bool should_continue = (callbacks->on_header)(CONST_MEM_REGION(.ptr = ptr, .size = num_bytes), callbacks->userdata);

    HTTPX_TRACE_POP();

    return should_continue ? num_bytes : 0;
}

static size_t adk_httpx_fetch_write_callback(const char * const ptr, const size_t size, const size_t nmemb, void * const userdata) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_fetch_callbacks_t * const callbacks = (adk_httpx_fetch_callbacks_t *)userdata;

    const size_t num_bytes = size * nmemb;
    const bool should_continue = (callbacks->on_body)(CONST_MEM_REGION(.ptr = ptr, .size = num_bytes), callbacks->userdata);

    HTTPX_TRACE_POP();

    return should_continue ? num_bytes : 0;
}

void adk_httpx_fetch(
    heap_t * const heap,
    const char * const url,
    adk_httpx_fetch_callbacks_t callbacks,
    const char ** headers,
    const size_t num_headers,
    const seconds_t timeout) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_client_t * const client = heap_alloc(heap, sizeof(adk_httpx_client_t), MALLOC_TAG);
    ZEROMEM(client);
    if (statics.use_global_certs) {
        ASSERT(statics.ca_chain);
    }

    client->heap = heap;

    client->ctx = adk_curl_context_create(
        (void *)client,
        (adk_curl_context_callbacks_t){
            .malloc = (adk_curl_context_malloc_t)adk_httpx_malloc,
            .calloc = (adk_curl_context_calloc_t)adk_httpx_calloc,
            .free = (adk_curl_context_free_t)adk_httpx_free,
            .realloc = (adk_curl_context_realloc_t)adk_httpx_realloc,
        });

    if (!statics.use_global_certs) {
        client->certs.default_certs = curl_common_load_default_certs_into_memory(client->heap, client->mutex, MALLOC_TAG);
    }

    client->ssl_ctx_data.custom_certs = NULL;
    adk_httpx_result_e result = adk_httpx_error;
    long response_code = -1;

    {
        adk_curl_context_t * const ctx_temp = adk_curl_get_context();
        adk_curl_set_context(client->ctx);

        CURL * const curl = curl_easy_init();
        if (curl) {
            if (statics.use_global_certs) {
                curl_common_set_ssl_ctx_ca_chain(curl, client->ctx, statics.ca_chain);
            } else {
                client->ssl_ctx_data.heap = client->heap;
                client->ssl_ctx_data.certs = &client->certs;
                curl_common_set_ssl_ctx(&client->ssl_ctx_data, curl, client->ctx);
            }

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout.seconds);

            adk_httpx_set_http2_options(curl);

            curl_common_set_socket_callbacks(curl, client->ctx);

            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &callbacks);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, adk_httpx_fetch_header_callback);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callbacks);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, adk_httpx_fetch_write_callback);
#if defined(_LEIA) || defined(_VADER)
            const char * proxy = __vader_get_proxy_server();
            if (proxy) {
                curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
                curl_easy_setopt(curl, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1);
            }
#endif

            struct curl_slist * list = NULL;
            for (size_t i = 0; i < num_headers; ++i) {
                list = curl_slist_append(list, headers[i]);
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

            const CURLcode res = curl_easy_perform(curl);

            if (res == CURLE_OK) {
                result = adk_httpx_ok;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            }

            curl_slist_free_all(list);
            curl_easy_cleanup(curl);
        }
        adk_curl_set_context(ctx_temp);
    }

    (callbacks.on_complete)(result, (int32_t)response_code, callbacks.userdata);

    if (!statics.use_global_certs) {
        curl_common_free_certs(heap, client->mutex, &client->certs.default_certs, MALLOC_TAG);
    }
    adk_curl_context_destroy(client->ctx);
    heap_free(heap, client, MALLOC_TAG);

    HTTPX_TRACE_POP();
}
