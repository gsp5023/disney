/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_curl_context.h"

#ifdef _CURL_TRACE
#include "source/adk/telemetry/telemetry.h"
#define CURL_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define CURL_TRACE_POP() TRACE_POP()
#else
#define CURL_TRACE_PUSH_FN()
#define CURL_TRACE_POP()
#endif

#ifdef _CONSOLE_NATIVE
#define FUNCNAME(NAME) NAME##_httpx
#else
#define FUNCNAME(NAME) NAME
#endif

void * adk_httpx_malloc(void * const ctx, const size_t size, const char * const tag);
void adk_httpx_free(void * const ctx, void * const ptr, const char * const tag);

static char * adk_httpx_strdup(void * const ctx, const char * const str, const char * const tag) {
    const size_t len = strlen(str);
    char * const p = adk_httpx_malloc(ctx, len + 1, tag);
    strcpy_s(p, len + 1, str);
    return p;
}

static struct {
    adk_httpx_api_t * api;
    adk_httpx_client_t * client;

    struct adk_curl_handle_t * handles_head;
    struct adk_curl_handle_t * handles_tail;
} statics;

typedef struct adk_httpx_json_deflate_callback_t {
    void (*callback)(void * const args);
    void * args;
} adk_httpx_json_deflate_callback_t;

struct adk_curl_handle_t {
    struct adk_curl_handle_t * prev;
    struct adk_curl_handle_t * next;

    adk_httpx_request_t * request;
    adk_httpx_response_t * response;

    char * url;
    adk_httpx_method_e method;
    adk_curl_handle_buffer_mode_e buffering_mode;
    bool follow_location;
    bool verbose;
    const uint8_t * request_body;
    long request_body_size;
    const char * user_agent;
    adk_httpx_encoding_e encoding;
    long timeout;
    long preferred_receive_buffer_size;

    // NOTE(lifetime): `headers` is owned by the client of this API and must be released manually on completion of request
    const adk_curl_slist_t * headers;

    adk_curl_callbacks_t callbacks;
    adk_httpx_json_deflate_callback_t json_deflate_callback;
};

struct adk_curl_slist_t {
    struct adk_curl_slist_t * next;
    char * value;
};

bool FUNCNAME(adk_curl_api_init)(
    const mem_region_t region,
    const mem_region_t fragments_region,
    const uint32_t fragment_size,
    const system_guard_page_mode_e guard_page_mode,
    adk_curl_http_init_mode_e init_mode) {
    CURL_TRACE_PUSH_FN();

    statics.api = adk_httpx_api_create(
        region,
        fragments_region,
        fragment_size,
        guard_page_mode,
        (init_mode == adk_curl_http_init_normal) ? adk_httpx_init_normal : adk_httpx_init_minimal,
        "adk-curl");

    statics.client = adk_httpx_client_create(statics.api);

    CURL_TRACE_POP();
    return true;
}

void FUNCNAME(adk_curl_api_shutdown)() {
    CURL_TRACE_PUSH_FN();
    adk_httpx_client_free(statics.client);
    adk_httpx_api_free(statics.api);
    CURL_TRACE_POP();
}

void FUNCNAME(adk_curl_api_shutdown_all_handles)() {
    CURL_TRACE_PUSH_FN();
    for (adk_curl_handle_t *handle = statics.handles_head, *next; handle != NULL; handle = next) {
        next = handle->next;

        adk_curl_close_handle(handle);
    }
    CURL_TRACE_POP();
}

void FUNCNAME(adk_curl_dump_heap_usage)() {
    CURL_TRACE_PUSH_FN();
    adk_httpx_api_dump_heap_usage(statics.api);
    CURL_TRACE_POP();
}

bool FUNCNAME(adk_curl_run_callbacks)() {
    CURL_TRACE_PUSH_FN();
    const bool status = adk_httpx_client_tick(statics.client);
    CURL_TRACE_POP();
    return status;
}

adk_curl_handle_t * FUNCNAME(adk_curl_open_handle)() {
    CURL_TRACE_PUSH_FN();
    adk_curl_handle_t * const handle = adk_httpx_malloc(statics.api, sizeof(adk_curl_handle_t), MALLOC_TAG);
    ZEROMEM(handle);

    handle->method = adk_httpx_method_get;

    LL_ADD(handle, prev, next, statics.handles_head, statics.handles_tail);

    CURL_TRACE_POP();
    return handle;
}

void FUNCNAME(adk_curl_close_handle)(adk_curl_handle_t * const handle) {
    CURL_TRACE_PUSH_FN();

    LL_REMOVE(handle, prev, next, statics.handles_head, statics.handles_tail);

    adk_httpx_response_free(handle->response);

    adk_httpx_free(statics.api, handle->url, MALLOC_TAG);
    adk_httpx_free(statics.api, handle, MALLOC_TAG);

    CURL_TRACE_POP();
}

adk_curl_handle_buffer_mode_e FUNCNAME(adk_curl_get_buffering_mode)(adk_curl_handle_t * const handle) {
    return handle->buffering_mode;
}

void FUNCNAME(adk_curl_set_buffering_mode)(adk_curl_handle_t * const handle, const adk_curl_handle_buffer_mode_e buffer_mode) {
    handle->buffering_mode = buffer_mode;
}

const_mem_region_t FUNCNAME(adk_curl_get_http_body)(adk_curl_handle_t * const handle) {
    return adk_httpx_response_get_body(handle->response);
}

const_mem_region_t FUNCNAME(adk_curl_get_http_header)(adk_curl_handle_t * const handle) {
    return adk_httpx_response_get_headers(handle->response);
}

adk_curl_slist_t * FUNCNAME(adk_curl_slist_append)(adk_curl_slist_t * list, const char * const sz) {
    CURL_TRACE_PUSH_FN();

    // Appends to end of list by walking to end of `list` and adding new node with `sz`

    adk_curl_slist_t ** node = &list;
    while ((*node) != NULL) {
        node = &(*node)->next;
    }

    (*node) = adk_httpx_malloc(statics.api, sizeof(adk_curl_slist_t), MALLOC_TAG);
    (*node)->value = adk_httpx_strdup(statics.api, sz, MALLOC_TAG);
    (*node)->next = NULL;

    CURL_TRACE_POP();
    return list;
}

void FUNCNAME(adk_curl_slist_free_all)(adk_curl_slist_t * const list) {
    CURL_TRACE_PUSH_FN();

    for (adk_curl_slist_t *node = list, *next; node != NULL; node = next) {
        next = node->next;

        adk_httpx_free(statics.api, node->value, MALLOC_TAG);
        adk_httpx_free(statics.api, node, MALLOC_TAG);
    }

    CURL_TRACE_POP();
}

void FUNCNAME(adk_curl_set_opt_long)(adk_curl_handle_t * const handle, const adk_curl_option_e option, const long value) {
    CURL_TRACE_PUSH_FN();

    switch (option) {
        case adk_curl_opt_buffer_size: {
            handle->preferred_receive_buffer_size = value;
            break;
        }
        case adk_curl_opt_post_field_size: {
            handle->request_body_size = value;
            break;
        }
        case adk_curl_opt_http_get: {
            handle->method = adk_httpx_method_get;
            break;
        }
        case adk_curl_opt_post:
        case adk_curl_opt_http_post: {
            handle->method = adk_httpx_method_post;
            break;
        }
        case adk_curl_opt_timeout: {
            handle->timeout = value;
            break;
        }
        case adk_curl_opt_follow_location: {
            handle->follow_location = value != 0;
            break;
        }
        case adk_curl_opt_verbose: {
            handle->verbose = value != 0;
            break;
        }
        default: {
            TRAP("Option not implemented: %d", option);
            break;
        }
    }

    CURL_TRACE_POP();
}

static bool list_contains(const char * list, const char * const element, const char * const delimiters) {
    CURL_TRACE_PUSH_FN();

    const size_t element_len = strlen(element);
    while (*list) {
        list += strspn(list, delimiters); // skip over delimiters

        const size_t value_len = strcspn(list, delimiters);
        if (value_len == element_len && strncmp(list, element, element_len) == 0) {
            CURL_TRACE_POP();
            return true;
        }

        list += value_len; // skip over value (not `element`)
    }

    CURL_TRACE_POP();
    return false;
}

void FUNCNAME(adk_curl_set_opt_ptr)(adk_curl_handle_t * const handle, const adk_curl_option_e option, const void * const value) {
    CURL_TRACE_PUSH_FN();

    switch (option) {
        case adk_curl_opt_post_fields: {
            handle->request_body = (const uint8_t *)value;
            break;
        }
        case adk_curl_opt_http_header: {
            handle->headers = (const adk_curl_slist_t *)value;
            break;
        }
        case adk_curl_opt_url: {
            handle->url = adk_httpx_strdup(statics.api, value, MALLOC_TAG);
            break;
        }
        case adk_curl_opt_custom_request: {
            const char * const method = (const char *)value;
            if (strcmp("GET", method) == 0) {
                handle->method = adk_httpx_method_get;
            } else if (strcmp("POST", method) == 0) {
                handle->method = adk_httpx_method_post;
            } else if (strcmp("PUT", method) == 0) {
                handle->method = adk_httpx_method_put;
            } else if (strcmp("PATCH", method) == 0) {
                handle->method = adk_httpx_method_patch;
            } else if (strcmp("DELETE", method) == 0) {
                handle->method = adk_httpx_method_delete;
            } else if (strcmp("HEAD", method) == 0) {
                handle->method = adk_httpx_method_head;
            } else {
                TRAP("Invalid custom request: %s", method);
            }

            break;
        }
        case adk_curl_opt_accept_encoding: {
            const char * const encoding = (const char *)value;
            if (encoding == NULL) {
                handle->encoding = adk_httpx_encoding_none;
            } else if (list_contains(encoding, "gzip", ", ")) {
                handle->encoding = adk_httpx_encoding_gzip;
            } else {
                TRAP("Invalid encoding: %s", encoding);
            }

            break;
        }
        case adk_curl_opt_user_agent: {
            handle->user_agent = (const char *)value;
            break;
        }
        default: {
            TRAP("Option not implemented: %d", option);
            break;
        }
    }

    CURL_TRACE_POP();
}

adk_curl_result_e FUNCNAME(adk_curl_get_info_long)(adk_curl_handle_t * const handle, const adk_curl_info_e info, long * const out) {
    CURL_TRACE_PUSH_FN();

    ASSERT(adk_httpx_response_get_status(handle->response) == adk_future_status_ready);

    switch (info) {
        case adk_curl_info_response_code: {
            *out = (long)adk_httpx_response_get_response_code(handle->response);

            switch (adk_httpx_response_get_result(handle->response)) {
                case adk_httpx_ok:
                    CURL_TRACE_POP();
                    return adk_curl_result_ok;
                case adk_httpx_timeout:
                    CURL_TRACE_POP();
                    return adk_curl_result_operation_timeouted;
                case adk_httpx_error:
                default:
                    CURL_TRACE_POP();
                    return adk_curl_result_got_nothing;
            }
        }
        default: {
            TRAP("Info option not implemented: %d", info);
            CURL_TRACE_POP();
            return adk_curl_result_got_nothing;
        }
    }

    CURL_TRACE_POP();
}

static bool adk_http_curl_on_header(adk_httpx_response_t * const response, const const_mem_region_t header, void * userdata) {
    CURL_TRACE_PUSH_FN();
    adk_curl_handle_t * const handle = userdata;
    bool status = true;
    if (handle->callbacks.on_http_header_recv != NULL) {
        status = handle->callbacks.on_http_header_recv(handle, header, &handle->callbacks);
    }
    return status;
    CURL_TRACE_POP();
}

static bool adk_http_curl_on_body(adk_httpx_response_t * const response, const const_mem_region_t body, void * userdata) {
    CURL_TRACE_PUSH_FN();
    adk_curl_handle_t * const handle = userdata;
    bool status = true;
    if (handle->callbacks.on_http_recv != NULL) {
        status = handle->callbacks.on_http_recv(handle, body, &handle->callbacks);
    }
    return status;
    CURL_TRACE_POP();
}

static adk_curl_result_e adk_httpx_response_get_curl_result(adk_httpx_response_t * const response) {
    CURL_TRACE_PUSH_FN();
    switch (adk_httpx_response_get_result(response)) {
        case adk_httpx_ok:
            CURL_TRACE_POP();
            return adk_curl_result_ok;
        case adk_httpx_timeout:
            CURL_TRACE_POP();
            return adk_curl_result_operation_timeouted;
        case adk_httpx_error:
        default:
            CURL_TRACE_POP();
            return adk_curl_result_got_nothing;
    }
}

static void adk_http_curl_on_complete(adk_httpx_response_t * const response, void * userdata) {
    CURL_TRACE_PUSH_FN();

    adk_curl_handle_t * const handle = userdata;

    if (handle->json_deflate_callback.callback) {
        handle->json_deflate_callback.callback(handle->json_deflate_callback.args);
    }

    if (handle->callbacks.on_complete != NULL) {
        handle->callbacks.on_complete(handle, adk_httpx_response_get_curl_result(response), &handle->callbacks);
    }

    CURL_TRACE_POP();
}

void FUNCNAME(adk_curl_async_perform)(adk_curl_handle_t * const handle, const adk_curl_callbacks_t callbacks) {
    CURL_TRACE_PUSH_FN();

    handle->callbacks = callbacks;

    handle->request = adk_httpx_client_request(statics.client, handle->method, handle->url);

    adk_httpx_request_set_verbose(handle->request, handle->verbose);
    adk_httpx_request_set_follow_location(handle->request, handle->follow_location);
    adk_httpx_request_accept_encoding(handle->request, handle->encoding);
    adk_httpx_request_set_user_agent(handle->request, handle->user_agent);

    if (handle->preferred_receive_buffer_size > 0) {
        adk_httpx_request_set_preferred_receive_buffer_size(handle->request, (uint32_t)handle->preferred_receive_buffer_size);
    }

    // Default timeout is 0 (zero) which means it never times out during transfer.
    adk_httpx_request_set_timeout(handle->request, handle->timeout);

    // Note that (in this case) the enumeration values for buffering mode are identical between ADK curl and httpx
    adk_httpx_request_set_buffering_mode(handle->request, (adk_httpx_buffering_mode_e)handle->buffering_mode);

    for (const adk_curl_slist_t * node = handle->headers; node != NULL; node = node->next) {
        adk_httpx_request_set_header(handle->request, node->value);
    }

    if (handle->request_body != NULL) {
        adk_httpx_request_set_body(handle->request, handle->request_body, handle->request_body_size);
    }

    adk_httpx_request_set_on_header(handle->request, adk_http_curl_on_header);
    adk_httpx_request_set_on_body(handle->request, adk_http_curl_on_body);
    adk_httpx_request_set_on_complete(handle->request, adk_http_curl_on_complete);
    adk_httpx_request_set_userdata(handle->request, handle);

    handle->response = adk_httpx_send(handle->request);

    CURL_TRACE_POP();
}

adk_curl_result_e FUNCNAME(adk_curl_async_get_result)(adk_curl_handle_t * const handle) {
    return (adk_httpx_response_get_status(handle->response) == adk_future_status_ready)
               ? adk_httpx_response_get_curl_result(handle->response)
               : adk_curl_result_busy;
}

void FUNCNAME(adk_curl_set_json_deflate_callback)(
    adk_curl_handle_t * const handle,
    void (*json_deflate_callback)(void * const),
    void * args) {
    handle->json_deflate_callback.callback = json_deflate_callback;
    handle->json_deflate_callback.args = args;
}
