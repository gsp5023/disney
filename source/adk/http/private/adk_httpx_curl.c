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
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"

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

#ifdef _HTTPX_TRACE
#include "source/adk/telemetry/telemetry.h"
#define HTTPX_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define HTTPX_TRACE_PUSH(_name) TRACE_PUSH(_name)
#define HTTPX_TRACE_POP() TRACE_POP()
#else
#define HTTPX_TRACE_PUSH_FN()
#define HTTPX_TRACE_PUSH(_name)
#define HTTPX_TRACE_POP()
#endif

#define HTTPX_CURL_TAG FOURCC('H', 'T', 'P', 'X')

enum {
    adk_httpx_max_connection_cache_size = 4,
};

static void lock(adk_httpx_api_t * const api) {
    if (api->mutex) {
        sb_lock_mutex(api->mutex);
    }
}

static void unlock(adk_httpx_api_t * const api) {
    if (api->mutex) {
        sb_unlock_mutex(api->mutex);
    }
}

void * adk_httpx_malloc(adk_httpx_api_t * const api, const size_t size, const char * const tag) {
    lock(api);
    void * const r = heap_alloc(api->heap, size, tag);
    unlock(api);
    return r;
}

void * adk_httpx_calloc(adk_httpx_api_t * const api, const size_t size, const char * const tag) {
    lock(api);
    void * const r = heap_calloc(api->heap, size, tag);
    unlock(api);
    return r;
}

void adk_httpx_free(adk_httpx_api_t * const api, void * const ptr, const char * const tag) {
    if (ptr == NULL) {
        return;
    }

    lock(api);
    heap_free(api->heap, ptr, tag);
    unlock(api);
}

void * adk_httpx_realloc(adk_httpx_api_t * const api, void * const ptr, const size_t size, const char * const tag) {
    lock(api);
    void * const r = heap_realloc(api->heap, ptr, size, tag);
    unlock(api);
    return r;
}

adk_httpx_api_t * adk_httpx_api_create(
    const mem_region_t region,
    const mem_region_t fragments_region,
    const uint32_t fragment_size,
    const system_guard_page_mode_e guard_page_mode,
    const adk_httpx_init_mode_e init_mode,
    const char * const name) {
    heap_t * heap;

    HTTPX_TRACE_PUSH_FN();
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        heap = debug_heap_emplace_init(region.size, 8, 0, name, guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap = heap_emplace_init_with_region(region, 8, 0, name);
    }

    adk_httpx_api_t * const api = heap_alloc(heap, sizeof(adk_httpx_api_t), MALLOC_TAG);
    ZEROMEM(api);

    sb_mutex_t * const mutex = (init_mode == adk_httpx_init_normal) ? sb_create_mutex(MALLOC_TAG) : NULL;

    api->heap = heap;
    api->mutex = mutex;

    api->clients_list_lock = sb_create_mutex(MALLOC_TAG);
    api->client_enqued = sb_create_condition_variable(MALLOC_TAG);

    api->ctx = adk_curl_context_create(
        (void *)api,
        (adk_curl_context_callbacks_t){
            .malloc = (adk_curl_context_malloc_t)adk_httpx_malloc,
            .calloc = (adk_curl_context_calloc_t)adk_httpx_calloc,
            .free = (adk_curl_context_free_t)adk_httpx_free,
            .realloc = (adk_curl_context_realloc_t)adk_httpx_realloc,
        });

    api->certs.default_certs = curl_common_load_default_certs_into_memory(api->heap, api->mutex, MALLOC_TAG);

    adk_httpx_network_pump_init(api, fragments_region, fragment_size, guard_page_mode);

    HTTPX_TRACE_POP();
    return api;
}

void adk_httpx_api_dump_heap_usage(adk_httpx_api_t * const api) {
    heap_dump_usage(api->heap);
}

void adk_httpx_api_free(adk_httpx_api_t * const api) {
    HTTPX_TRACE_PUSH_FN();
    curl_common_free_certs(api->heap, api->mutex, &api->certs.default_certs, MALLOC_TAG);
    curl_common_free_certs(api->heap, api->mutex, &api->certs.custom_certs, MALLOC_TAG);

    // Network pump depends on the api's curl context to run queries
    // and should be terminated before the context destruction
    adk_httpx_network_pump_shutdown(api->network_pump);
    adk_curl_context_destroy(api->ctx);

    sb_destroy_condition_variable(api->client_enqued, MALLOC_TAG);
    sb_destroy_mutex(api->clients_list_lock, MALLOC_TAG);

    heap_t * const heap = api->heap;
    sb_mutex_t * const mutex = api->mutex;

    heap_free(heap, api, MALLOC_TAG);

#ifndef NDEBUG
    heap_debug_print_leaks(heap);
#endif

    heap_destroy(heap, MALLOC_TAG);

    if (mutex != NULL) {
        sb_destroy_mutex(mutex, MALLOC_TAG);
    }

    HTTPX_TRACE_POP();
}

// Handle should be freed only after the request has went through the networking pump and been completed.
static void free_handle(adk_httpx_client_t * const client, adk_httpx_handle_t * const handle) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_request_t * request = handle->request;

    if (handle->response) {
        handle->response->handle = NULL;
    }

    LL_REMOVE(handle, prev, next, client->handle_head, client->handle_tail);

    CURL_PUSH_CTX(client->api->ctx);
    curl_slist_free_all(request->headers);
    curl_easy_cleanup(request->curl);
    CURL_POP_CTX();

    // At this point Handle / Request won't be touched by the Network pump, since the request has either been completed or
    // the client has been removed from the network pump's processing queue.
    if (request->fragments_head) {
        adk_httpx_network_pump_fragments_free(client->api->network_pump, request->fragments_head, request->fragments_tail);
        request->fragments_head = request->fragments_tail = NULL;
    }

    sb_destroy_mutex(handle->response_lock, MALLOC_TAG);

    sb_destroy_mutex(request->fragments_lock, MALLOC_TAG);

    adk_httpx_free(client->api, request->body.ptr, MALLOC_TAG);
    adk_httpx_free(client->api, request, MALLOC_TAG);
    adk_httpx_free(client->api, handle, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

adk_httpx_client_t * adk_httpx_client_create(adk_httpx_api_t * const api) {
    HTTPX_TRACE_PUSH_FN();
    adk_httpx_client_t * const client = adk_httpx_malloc(api, sizeof(adk_httpx_client_t), MALLOC_TAG);
    ZEROMEM(client);

    client->api = api;
    client->destroyed = false;

    CURL_PUSH_CTX(api->ctx);
    CURLM * const multi = curl_multi_init();
    VERIFY(multi);
    client->multi = multi;

    curl_multi_setopt(client->multi, CURLMOPT_MAXCONNECTS, adk_httpx_max_connection_cache_size);
    CURL_POP_CTX();

    sb_lock_mutex(api->clients_list_lock);
    {
        LL_ADD(client, prev, next, api->clients_list_head, api->clients_list_tail);
    }
    sb_unlock_mutex(api->clients_list_lock);

    sb_condition_wake_all(api->client_enqued);

    HTTPX_TRACE_POP();
    return client;
}

void adk_httpx_client_free(adk_httpx_client_t * const client) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_api_t * const api = client->api;

    sb_lock_mutex(api->clients_list_lock);
    {
        client->destroyed = true;
        while (true) {
            bool found = false;
            for (adk_httpx_client_t * it = api->clients_list_head; it != NULL; it = it->next) {
                if (it == client) {
                    found = true;
                    break;
                }
            }

            if (found) {
                break;
            }

            sb_wait_condition(api->client_enqued, api->clients_list_lock, sb_timeout_infinite);
        }

        LL_REMOVE(client, prev, next, api->clients_list_head, api->clients_list_tail);
    }
    sb_unlock_mutex(api->clients_list_lock);

    // At this point network pump won't pick the client for processing, since it was removed from the queue, hence no locking is needed

    for (adk_httpx_handle_t *handle = client->handle_head, *next; handle != NULL; handle = next) {
        next = handle->next;

        free_handle(client, handle);
    }

    adk_httpx_network_pump_delete_enqued_requests(api->network_pump, client);

    CURL_PUSH_CTX(client->api->ctx);
    curl_multi_cleanup(client->multi);
    CURL_POP_CTX();

    adk_httpx_free(client->api, client, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

bool adk_httpx_client_tick(adk_httpx_client_t * const client) {
    HTTPX_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();

    adk_httpx_api_t * const api = client->api;
    adk_httpx_network_pump_t * const pump = api->network_pump;

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
                    response->headers.ptr = adk_httpx_realloc(client->api, response->headers.ptr, response->headers.size + fragment->size + 1, MALLOC_TAG);
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
                    response->body.ptr = adk_httpx_realloc(client->api, response->body.ptr, response->body.size + fragment->size + 1, MALLOC_TAG);
                    memcpy(response->body.byte_ptr + response->body.size, fragment->region.ptr, fragment->size);
                    response->body.size += fragment->size;
                    // add nul so string functions will always find a nul inside our buffer regardless of what data we receive.
                    response->body.byte_ptr[response->body.size] = 0;
                }
            }
        }

        if (fragments_head) {
            adk_httpx_network_pump_fragments_free(pump, fragments_head, fragments_tail);
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

adk_httpx_request_t * adk_httpx_client_request(
    adk_httpx_client_t * const client,
    const adk_httpx_method_e method,
    const char * const url) {
    HTTPX_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();

    adk_httpx_request_t * const request = (adk_httpx_request_t *)adk_httpx_malloc(client->api, sizeof(adk_httpx_request_t), MALLOC_TAG);
    ZEROMEM(request);

    request->client = client;

    {
        adk_curl_context_t * const ctx_temp = adk_curl_get_context();
        adk_curl_set_context(client->api->ctx);

        request->curl = curl_easy_init();

        curl_easy_setopt(request->curl, CURLOPT_URL, url);

        curl_common_set_socket_callbacks(request->curl, client->api->ctx);

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

    CURL_PUSH_CTX(request->client->api->ctx);
    curl_easy_setopt(request->curl, CURLOPT_VERBOSE, (long)verbose);
    CURL_POP_CTX();

    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_follow_location(adk_httpx_request_t * const request, bool follow_location) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->api->ctx);
    curl_easy_setopt(request->curl, CURLOPT_FOLLOWLOCATION, (long)follow_location);
    CURL_POP_CTX();

    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_preferred_receive_buffer_size(
    adk_httpx_request_t * const request,
    uint32_t preferred_receive_buffer_size) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->api->ctx);
    curl_easy_setopt(request->curl, CURLOPT_BUFFERSIZE, (long)preferred_receive_buffer_size);
    CURL_POP_CTX();

    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_header(adk_httpx_request_t * const request, const char * const header) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->api->ctx);
    request->headers = curl_slist_append(request->headers, header);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_timeout(adk_httpx_request_t * const request, uint64_t timeout) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->api->ctx);
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
    request->body.ptr = adk_httpx_malloc(client->api, body_size, MALLOC_TAG);
    request->body.size = body_size;

    memcpy(request->body.ptr, (void *)body, body_size);

    CURL_PUSH_CTX(client->api->ctx);
    curl_easy_setopt(request->curl, CURLOPT_POSTFIELDS, request->body.ptr);
    curl_easy_setopt(request->curl, CURLOPT_POSTFIELDSIZE, body_size);
    CURL_POP_CTX();

    HTTPX_TRACE_POP();
}

void adk_httpx_request_set_user_agent(
    adk_httpx_request_t * const request,
    const char * const user_agent) {
    HTTPX_TRACE_PUSH_FN();
    CURL_PUSH_CTX(request->client->api->ctx);
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
            CURL_PUSH_CTX(request->client->api->ctx);
            curl_easy_setopt(request->curl, CURLOPT_ACCEPT_ENCODING, NULL);
            CURL_POP_CTX();
            break;
        case adk_httpx_encoding_gzip:
            CURL_PUSH_CTX(request->client->api->ctx);
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

    adk_httpx_network_pump_t * const pump = client->api->network_pump;

    adk_httpx_response_t * const response = (adk_httpx_response_t *)adk_httpx_malloc(client->api, sizeof(adk_httpx_response_t), MALLOC_TAG);
    ZEROMEM(response);
    response->client = client;
    response->status = adk_future_status_pending;
    response->result = adk_httpx_ok;

    adk_httpx_handle_t * const handle = (adk_httpx_handle_t *)adk_httpx_malloc(client->api, sizeof(adk_httpx_handle_t), MALLOC_TAG);
    ZEROMEM(handle);
    LL_ADD(handle, prev, next, client->handle_head, client->handle_tail);
    handle->request = request;
    handle->response = response;
    handle->data_stream_ended = false;
    handle->response_lock = sb_create_mutex(MALLOC_TAG);

    response->handle = handle;

    curl_common_set_ssl_ctx(&client->api->certs, request->curl, client->api->ctx);

    request->fragments_lock = sb_create_mutex(MALLOC_TAG);

    adk_httpx_network_pump_add_request(pump, handle);

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

sb_socket_t adk_httpx_response_get_socket(adk_httpx_response_t * const response) {
    curl_socket_t curl_sockfd;
    curl_easy_getinfo(response->handle->request->curl, CURLINFO_ACTIVESOCKET, &curl_sockfd);
    return (sb_socket_t){(uint64_t)curl_sockfd};
}

void * adk_httpx_response_get_curl_handle(adk_httpx_response_t * const response) {
    return response->handle->request->curl;
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

    adk_httpx_free(client->api, response->error, MALLOC_TAG);
    adk_httpx_free(client->api, response->headers.ptr, MALLOC_TAG);
    adk_httpx_free(client->api, response->body.ptr, MALLOC_TAG);
    adk_httpx_free(client->api, response, MALLOC_TAG);

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
    const size_t num_headers) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_api_t * const api = heap_alloc(heap, sizeof(adk_httpx_api_t), MALLOC_TAG);
    ZEROMEM(api);

    api->heap = heap;

    api->ctx = adk_curl_context_create(
        (void *)api,
        (adk_curl_context_callbacks_t){
            .malloc = (adk_curl_context_malloc_t)adk_httpx_malloc,
            .calloc = (adk_curl_context_calloc_t)adk_httpx_calloc,
            .free = (adk_curl_context_free_t)adk_httpx_free,
            .realloc = (adk_curl_context_realloc_t)adk_httpx_realloc,
        });

    api->certs.default_certs = curl_common_load_default_certs_into_memory(api->heap, api->mutex, MALLOC_TAG);
    adk_httpx_result_e result = adk_httpx_error;
    long response_code = -1;

    {
        adk_curl_context_t * const ctx_temp = adk_curl_get_context();
        adk_curl_set_context(api->ctx);

        CURL * const curl = curl_easy_init();
        if (curl) {
            curl_common_set_ssl_ctx(&api->certs, curl, api->ctx);

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

            curl_common_set_socket_callbacks(curl, api->ctx);

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

    curl_common_free_certs(api->heap, api->mutex, &api->certs.default_certs, MALLOC_TAG);
    adk_curl_context_destroy(api->ctx);
    heap_free(heap, api, MALLOC_TAG);

    HTTPX_TRACE_POP();
}
