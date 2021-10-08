/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/private/adk_httpx_network_pump.h"

#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"

#define HTTPX_NETWORK_PUMP FOURCC('H', 'X', 'N', 'P')

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

struct adk_httpx_network_pump_t {
    adk_httpx_api_t * api;
    mem_region_t fragment_buffers;

    bool network_pump_running;
    sb_thread_id_t network_pump_thread;

    // free list
    sb_condition_variable_t * free_fragment_enqued;
    sb_mutex_t * free_list_lock;
    adk_httpx_network_pump_fragment_t * free_list_head;
    adk_httpx_network_pump_fragment_t * free_list_tail;

    sb_mutex_t * incoming_requests_lock;
    adk_httpx_request_t * incoming_requests_head;
    adk_httpx_request_t * incoming_requests_tail;
};

// Borrow the httpx alloc functions so allocations come from the httpx heap and are synchronized with
// general httpx allocations
extern void * adk_httpx_malloc(adk_httpx_api_t * const api, const size_t size, const char * const tag);
extern void adk_httpx_free(adk_httpx_api_t * const api, void * const ptr, const char * const tag);

static THREAD_LOCAL bool did_get_data = false;

static adk_httpx_network_pump_fragment_t * blocking_get_free_fragment(adk_httpx_network_pump_t * const pump) {
    HTTPX_TRACE_PUSH_FN()

    adk_httpx_network_pump_fragment_t * fragment = NULL;
    sb_lock_mutex(pump->free_list_lock);
    {
        while (pump->free_list_head == NULL) {
            sb_wait_condition(pump->free_fragment_enqued, pump->free_list_lock, sb_timeout_infinite);
        }

        fragment = pump->free_list_head;

        LL_REMOVE(fragment, prev, next, pump->free_list_head, pump->free_list_tail);
    }
    sb_unlock_mutex(pump->free_list_lock);

    ASSERT(fragment);
    fragment->size = 0;

    HTTPX_TRACE_POP();
    return fragment;
}

static size_t network_pump_on_header_received(const char * const ptr, const size_t size, const size_t nmemb, void * const userdata) {
    HTTPX_TRACE_PUSH_FN();

    did_get_data = true;

    const size_t num_bytes = size * nmemb;

    adk_httpx_handle_t * const handle = userdata;
    adk_httpx_request_t * const request = handle->request;
    adk_httpx_network_pump_t * const pump = request->client->api->network_pump;

    adk_httpx_network_pump_fragment_t * fragment = request->active_header_fragment;
    if (!fragment) {
        request->active_header_fragment = blocking_get_free_fragment(pump);
        fragment = request->active_header_fragment;
        fragment->type = adk_httpx_network_pump_header_fragment;
    }

    // Make sure that the received header can fit into a single fragment buffer
    ASSERT(num_bytes <= fragment->region.size);

    size_t fragment_space_left = fragment->region.size - fragment->size;
    if (fragment_space_left < num_bytes) {
        adk_httpx_network_pump_fragment_t * const fresh_fragment = blocking_get_free_fragment(pump);
        // Fresh fragment would be the tail of the list on flush operation
        fresh_fragment->prev = request->active_header_fragment;
        request->active_header_fragment->next = fresh_fragment;
        fragment = request->active_header_fragment = fresh_fragment;
        fragment->type = adk_httpx_network_pump_header_fragment;
        fragment_space_left = fragment->region.size;
    }

    memcpy(fragment->region.byte_ptr + fragment->size, ptr, num_bytes);

    fragment->size += num_bytes;

    HTTPX_TRACE_POP();
    return !request->aborted_by_callback ? num_bytes : 0;
}

static size_t network_pump_on_data_received(const char * const ptr, const size_t size, const size_t nmemb, void * const userdata) {
    HTTPX_TRACE_PUSH_FN();

    did_get_data = true;

    const size_t num_bytes = size * nmemb;

    adk_httpx_handle_t * const handle = userdata;
    adk_httpx_request_t * const request = handle->request;
    adk_httpx_network_pump_t * const pump = request->client->api->network_pump;

    // Flush header fragments to make them available for the main thread
    if (request->active_header_fragment) {
        adk_httpx_network_pump_fragment_t * const tail_fragment = request->active_header_fragment;
        adk_httpx_network_pump_fragment_t * fragment = tail_fragment;
        while (fragment->prev) {
            fragment = fragment->prev;
        }

        sb_lock_mutex(request->fragments_lock);
        {
            if (request->fragments_tail) {
                request->fragments_tail->next = fragment;
                fragment->prev = request->fragments_tail;
            } else {
                request->fragments_head = fragment;
            }

            request->fragments_tail = tail_fragment;
        }
        sb_unlock_mutex(request->fragments_lock);

        request->active_header_fragment = NULL;
    }

    adk_httpx_network_pump_fragment_t * fragment = request->active_body_fragment;
    if (!fragment) {
        request->active_body_fragment = blocking_get_free_fragment(pump);
        fragment = request->active_body_fragment;
        fragment->type = adk_httpx_network_pump_body_fragment;
    }

    size_t bytes_left_to_write = num_bytes;
    size_t ptr_offset = 0;
    while (bytes_left_to_write > 0) {
        size_t fragment_space_left = fragment->region.size - fragment->size;
        if (fragment_space_left == 0) {
            sb_lock_mutex(request->fragments_lock);
            {
                LL_ADD(fragment, prev, next, request->fragments_head, request->fragments_tail);
            }
            sb_unlock_mutex(request->fragments_lock);

            fragment = request->active_body_fragment = blocking_get_free_fragment(pump);
            fragment->type = adk_httpx_network_pump_body_fragment;
            fragment_space_left = fragment->region.size;
        }

        size_t bytes_to_write = bytes_left_to_write < fragment_space_left ? bytes_left_to_write : fragment_space_left;

        memcpy(fragment->region.byte_ptr + fragment->size, ptr + ptr_offset, bytes_to_write);

        fragment->size += bytes_to_write;
        bytes_left_to_write -= bytes_to_write;
        ptr_offset += bytes_to_write;
    }

    HTTPX_TRACE_POP();

    return !request->aborted_by_callback ? num_bytes : 0;
}

// This function should be called after add_client, that creates a corresponding entry that stores information about
// requests being processed.
void adk_httpx_network_pump_add_request(adk_httpx_network_pump_t * const pump, adk_httpx_handle_t * const handle) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_request_t * const request = handle->request;
    adk_httpx_client_t * const client = request->client;

    CURL_PUSH_CTX(client->api->ctx);
    curl_easy_setopt(request->curl, CURLOPT_PRIVATE, handle);

    curl_easy_setopt(request->curl, CURLOPT_HTTPHEADER, request->headers);

    // Network Pump will point to its own functions
    curl_easy_setopt(request->curl, CURLOPT_HEADERDATA, handle);
    curl_easy_setopt(request->curl, CURLOPT_HEADERFUNCTION, network_pump_on_header_received);

    // Instead of using a handle here we can pass a fragment?
    curl_easy_setopt(request->curl, CURLOPT_WRITEDATA, handle);
    curl_easy_setopt(request->curl, CURLOPT_WRITEFUNCTION, network_pump_on_data_received);

    sb_lock_mutex(pump->incoming_requests_lock);
    {
        LL_ADD(request, prev, next, pump->incoming_requests_head, pump->incoming_requests_tail);
    }
    sb_unlock_mutex(pump->incoming_requests_lock);
    CURL_POP_CTX();
    HTTPX_TRACE_POP();
}

static adk_httpx_client_t * deque_client(adk_httpx_api_t * const api) {
    sb_lock_mutex(api->clients_list_lock);
    while (api->network_pump->network_pump_running) {
        {
            for (adk_httpx_client_t * client = api->clients_list_head; client; client = client->next) {
                if (!client->destroyed) {
                    LL_REMOVE(client, prev, next, api->clients_list_head, api->clients_list_tail);
                    sb_unlock_mutex(api->clients_list_lock);
                    return client;
                }
            }
        }
        sb_wait_condition(api->client_enqued, api->clients_list_lock, sb_timeout_infinite);
    }
    sb_unlock_mutex(api->clients_list_lock);
    return NULL;
}

static void enque_client(adk_httpx_api_t * const api, adk_httpx_client_t * const client) {
    sb_lock_mutex(api->clients_list_lock);
    {
        LL_ADD(client, prev, next, api->clients_list_head, api->clients_list_tail);
    }
    sb_unlock_mutex(api->clients_list_lock);
    sb_condition_wake_all(api->client_enqued);
}

static int network_pump_proc(void * userdata) {
    LOG_INFO(HTTPX_NETWORK_PUMP, "Starting network pump thread");

    adk_httpx_network_pump_t * const pump = userdata;

    adk_httpx_client_t * client = deque_client(pump->api);
    while (client) {
        HTTPX_TRACE_PUSH("network_pump cycle");

        ASSERT(client->api == pump->api);

        adk_curl_set_context(client->api->ctx);

        sb_lock_mutex(pump->incoming_requests_lock);
        {
            adk_httpx_request_t * request = pump->incoming_requests_head;
            while (request) {
                adk_httpx_request_t * const next = request->next;

                // Client could be freed at any time, since we may still have requests for client that were destroyed, we grab
                // only those that were schedule for the current client. Those that belong to the destroyed client, must be
                // removed from the queue with a separate call to "adk_httpx_network_pump_delete_enqued_requests" upon the
                // client's destruction.
                if (request->client == client) {
                    VERIFY(curl_multi_add_handle(client->multi, request->curl) == CURLM_OK);
                    LL_REMOVE(request, prev, next, pump->incoming_requests_head, pump->incoming_requests_tail);
                }

                request = next;
            }
        }
        sb_unlock_mutex(pump->incoming_requests_lock);

        bool did_multi_perform = false;
        while (true) {
            if (!did_multi_perform) {
                CURLMcode mc;
                int running_handles;

                did_get_data = false;
                mc = curl_multi_perform(client->multi, &running_handles);
                if (mc != CURLM_OK) {
                    LOG_ERROR(HTTPX_NETWORK_PUMP, "curl_multi_perform failed with code %d", mc);
                    break;
                }

                did_multi_perform = true;
            }

            int msgs_in_queue;
            CURLMsg * const msg = curl_multi_info_read(client->multi, &msgs_in_queue);

            if (!msg) {
                break;
            }

            ASSERT(msg->msg == CURLMSG_DONE);

            char * info_private;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &info_private);

            adk_httpx_handle_t * const handle = (adk_httpx_handle_t *)info_private;
            ASSERT(handle->request->curl == msg->easy_handle);
            curl_multi_remove_handle(client->multi, handle->request->curl);

            sb_lock_mutex(handle->response_lock);
            {
                if (handle->response) {
                    const CURLcode result = msg->data.result;
                    if (result == CURLE_OK) {
                        // response->result is already initialized to adk_httpx_ok.
                        // no assignment is necessary here to signal OK, and we want to avoid
                        // racing with a status that may have been set off-thread.

                        long response_code;
                        curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
                        handle->response->response_code = (int64_t)response_code;
                    } else if (result == CURLE_OPERATION_TIMEDOUT) {
                        handle->response->result = adk_httpx_timeout;
                    } else {
                        handle->response->result = adk_httpx_error;

                        const char * const error = curl_easy_strerror(result);
                        const size_t length = strlen(error) + 1;
                        handle->response->error = adk_httpx_malloc(client->api, length, MALLOC_TAG);
                        memcpy(handle->response->error, error, length);
                    }
                }
            }
            sb_unlock_mutex(handle->response_lock);

            adk_httpx_request_t * request = handle->request;
            sb_lock_mutex(request->fragments_lock);
            {
                if (request->active_header_fragment) {
                    ASSERT(request->active_header_fragment->size > 0);
                    LL_ADD(request->active_header_fragment, prev, next, request->fragments_head, request->fragments_tail);
                    request->active_header_fragment = NULL;
                }

                if (request->active_body_fragment) {
                    ASSERT(request->active_body_fragment->size > 0);
                    LL_ADD(request->active_body_fragment, prev, next, request->fragments_head, request->fragments_tail);
                    request->active_body_fragment = NULL;
                }
                handle->data_stream_ended = true;
            }
            sb_unlock_mutex(request->fragments_lock);
        }

        enque_client(pump->api, client);
        if (!did_get_data) {
            sb_thread_sleep((milliseconds_t){.ms = 1});
        }
        client = deque_client(pump->api);

        HTTPX_TRACE_POP();
    }

    return 0;
}

static void create_fragment_regions(adk_httpx_network_pump_t * const pump, const size_t fragment_size) {
    HTTPX_TRACE_PUSH_FN();

    const size_t fragment_alignment = ALIGN_OF(adk_httpx_network_pump_fragment_t);

    mem_region_t arena = pump->fragment_buffers;
    arena.adr = ALIGN_PTR(arena.byte_ptr, fragment_alignment);
    arena.size = arena.size - (arena.adr - pump->fragment_buffers.adr);

    uint32_t num_fragments = 0;

    const size_t header_size = sizeof(adk_httpx_network_pump_fragment_t);
    const size_t fragment_data_size = fragment_size - header_size;

    const size_t aligned_fragment_size = ALIGN_INT(fragment_size, fragment_alignment);
    VERIFY_MSG(arena.size >= aligned_fragment_size, "Network pump's memory size must be at least the size of a single fragment.");

    for (size_t offset = 0; offset < arena.size; offset += aligned_fragment_size) {
        uint8_t * const fragment_offset = arena.byte_ptr + offset;
        uint8_t * const fragment_data = fragment_offset + header_size;

        adk_httpx_network_pump_fragment_t * const header = (adk_httpx_network_pump_fragment_t * const)fragment_offset;
        ZEROMEM(header);

        header->region = MEM_REGION(.ptr = fragment_data, .size = fragment_data_size);

        LL_ADD(header, prev, next, pump->free_list_head, pump->free_list_tail);

        num_fragments++;
    }

    HTTPX_TRACE_POP();
    LOG_DEBUG(HTTPX_NETWORK_PUMP, "Created %d fragments of size %d", num_fragments, fragment_size);
}

void adk_httpx_network_pump_init(
    adk_httpx_api_t * const api,
    const mem_region_t region,
    const size_t fragment_size,
    const system_guard_page_mode_e guard_page_mode) {
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_network_pump_t * const network_pump = adk_httpx_malloc(api, sizeof(adk_httpx_network_pump_t), MALLOC_TAG);
    ZEROMEM(network_pump);
    network_pump->api = api;
    api->network_pump = network_pump;

    network_pump->fragment_buffers = region;

    if (!network_pump->fragment_buffers.ptr) {
#ifdef GUARD_PAGE_SUPPORT
        if (guard_page_mode == system_guard_page_mode_enabled) {
            network_pump->fragment_buffers = debug_sys_map_pages(PAGE_ALIGN_INT(region.size), system_page_protect_read_write, MALLOC_TAG);
        } else
#endif
        {
            network_pump->fragment_buffers = sb_map_pages(PAGE_ALIGN_INT(region.size), system_page_protect_read_write);
        }
        TRAP_OUT_OF_MEMORY(network_pump->fragment_buffers.ptr);
    }

    create_fragment_regions(network_pump, fragment_size);

    network_pump->free_list_lock = sb_create_mutex(MALLOC_TAG);
    network_pump->free_fragment_enqued = sb_create_condition_variable(MALLOC_TAG);

    network_pump->incoming_requests_lock = sb_create_mutex(MALLOC_TAG);

    network_pump->network_pump_running = true;
    network_pump->network_pump_thread = sb_create_thread("adk_net_pump", sb_thread_default_options, &network_pump_proc, network_pump, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

void adk_httpx_network_pump_shutdown(adk_httpx_network_pump_t * const pump) {
    LOG_INFO(HTTPX_NETWORK_PUMP, "Terminating network pump\n");
    HTTPX_TRACE_PUSH_FN();

    adk_httpx_api_t * const api = pump->api;

    pump->network_pump_running = false;
    sb_condition_wake_all(api->client_enqued);
    sb_join_thread(pump->network_pump_thread);

    sb_destroy_mutex(pump->free_list_lock, MALLOC_TAG);
    sb_destroy_mutex(pump->incoming_requests_lock, MALLOC_TAG);
    sb_destroy_condition_variable(pump->free_fragment_enqued, MALLOC_TAG);

    adk_httpx_free(api, pump, MALLOC_TAG);

    HTTPX_TRACE_POP();
}

void adk_httpx_network_pump_fragments_free(
    adk_httpx_network_pump_t * const pump,
    adk_httpx_network_pump_fragment_t * const fragments_head,
    adk_httpx_network_pump_fragment_t * const fragments_tail) {
    HTTPX_TRACE_PUSH_FN();

    sb_lock_mutex(pump->free_list_lock);
    {
        if (pump->free_list_tail) {
            pump->free_list_tail->next = fragments_head;
            fragments_head->prev = pump->free_list_tail;
        } else {
            pump->free_list_head = fragments_head;
        }

        pump->free_list_tail = fragments_tail;
    }
    sb_unlock_mutex(pump->free_list_lock);
    sb_condition_wake_all(pump->free_fragment_enqued);

    HTTPX_TRACE_POP();
}

void adk_httpx_network_pump_delete_enqued_requests(adk_httpx_network_pump_t * const pump, const adk_httpx_client_t * const client) {
    sb_lock_mutex(pump->incoming_requests_lock);
    {
        for (adk_httpx_request_t * request = pump->incoming_requests_head; request != NULL;) {
            adk_httpx_request_t * const next = request->next;
            if (request->client == client) {
                LL_REMOVE(request, prev, next, pump->incoming_requests_head, pump->incoming_requests_tail);
            }
            request = next;
        }
    }
    sb_unlock_mutex(pump->incoming_requests_lock);
}
