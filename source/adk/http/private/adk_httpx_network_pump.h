/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_curl_shared.h"

void adk_httpx_network_pump_init(adk_httpx_api_t * const api, const mem_region_t region, const size_t fragment_size, const system_guard_page_mode_e guard_page_mode);

void adk_httpx_network_pump_shutdown(adk_httpx_network_pump_t * const network_pump);

void adk_httpx_network_pump_add_request(adk_httpx_network_pump_t * const pump, adk_httpx_handle_t * const handle);

void adk_httpx_network_pump_delete_enqued_requests(adk_httpx_network_pump_t * const pump, const adk_httpx_client_t * const client);

typedef enum adk_httpx_network_pump_fragment_type_e {
    adk_httpx_network_pump_header_fragment,
    adk_httpx_network_pump_body_fragment
} adk_httpx_network_pump_fragment_type_e;

struct adk_httpx_network_pump_fragment_t {
    adk_httpx_network_pump_fragment_type_e type;

    mem_region_t region;
    size_t size;

    struct adk_httpx_network_pump_fragment_t * prev;
    struct adk_httpx_network_pump_fragment_t * next;
};

void adk_httpx_network_pump_fragments_free(adk_httpx_network_pump_t * const network_pump, adk_httpx_network_pump_fragment_t * const head, adk_httpx_network_pump_fragment_t * const tail);
