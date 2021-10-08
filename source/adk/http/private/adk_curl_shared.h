/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/http/private/adk_curl_common.h"
#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_thread.h"

#define CURL_NO_OLDIES
#include "curl/curl.h"

typedef struct adk_httpx_network_pump_t adk_httpx_network_pump_t;
typedef struct adk_httpx_network_pump_fragment_t adk_httpx_network_pump_fragment_t;

struct adk_httpx_client_t;

struct adk_httpx_api_t {
    adk_curl_context_t * ctx;
    adk_httpx_network_pump_t * network_pump;
    heap_t * heap;
    sb_mutex_t * mutex;
    curl_common_certs_t certs;

    sb_mutex_t * clients_list_lock;
    sb_condition_variable_t * client_enqued;
    struct adk_httpx_client_t * clients_list_head;
    struct adk_httpx_client_t * clients_list_tail;
};

struct adk_httpx_client_t {
    adk_httpx_api_t * api;

    // Any updates on the mutli object should be performed on the pump's thread,
    // any access from the main thread would cause a potential race condition.
    CURLM * multi;

    bool destroyed;

    struct adk_httpx_client_t * prev;
    struct adk_httpx_client_t * next;

    struct adk_httpx_handle_t * handle_head;
    struct adk_httpx_handle_t * handle_tail;
};

struct adk_httpx_response_t {
    adk_httpx_client_t * client;
    struct adk_httpx_handle_t * handle;

    adk_future_status_e status;

    int64_t response_code;
    mem_region_t body;
    mem_region_t headers;

    adk_httpx_result_e result;
    char * error;
};

typedef struct adk_httpx_handle_t {
    struct adk_httpx_handle_t * prev;
    struct adk_httpx_handle_t * next;

    bool did_on_complete;

    // This status is set by the network pump, when all headers and data were flushed from the socket,
    // available for processing and request is considered completed (successful or not).
    bool data_stream_ended;

    adk_httpx_request_t * request;

    // Client may free the response anytime, while being used by the network pump to flush data
    // about the curl response to it.
    sb_mutex_t * response_lock;
    adk_httpx_response_t * response;
} adk_httpx_handle_t;

struct adk_httpx_request_t {
    adk_httpx_client_t * client;
    CURL * curl;
    struct curl_slist * headers;
    mem_region_t body;

    adk_httpx_buffering_mode_e buffering_mode;

    adk_httpx_on_header_t on_header;
    adk_httpx_on_body_t on_body;
    adk_httpx_on_complete_t on_complete;
    bool aborted_by_callback;

    adk_httpx_network_pump_fragment_t * active_header_fragment;
    adk_httpx_network_pump_fragment_t * active_body_fragment;

    sb_mutex_t * fragments_lock;
    adk_httpx_network_pump_fragment_t * fragments_head;
    adk_httpx_network_pump_fragment_t * fragments_tail;

    void * userdata;

    adk_httpx_request_t * prev;
    adk_httpx_request_t * next;
};
