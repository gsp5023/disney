/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "extern/curl/curl/include/curl/curl.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/x509.h"
#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct curl_common_certs_node_t {
    struct curl_common_certs_node_t * prev;
    struct curl_common_certs_node_t * next;
    mem_region_t cert;
    char * name;
} curl_common_certs_node_t;

typedef struct curl_common_certs_list_t {
    curl_common_certs_node_t * head;
    curl_common_certs_node_t * tail;
} curl_common_certs_list_t;

typedef struct curl_common_certs_t {
    curl_common_certs_list_t default_certs;
    curl_common_certs_list_t custom_certs;
} curl_common_certs_t;

// if provided with CURLOPT_SOCKOPTDATA will be called once (after opensocket).
// used to provide desired (custom) values based on fields below:
// unused?
typedef struct curl_common_sockopt_callback_data_t {
    sb_socket_blocking_e blocking;
    sb_socket_tcp_delay_mode_e delay_mode;
    sb_socket_linger_mode_e linger_mode;
    int linger_time;
} curl_common_sockopt_callback_data_t;

typedef struct curl_common_custom_certs_t {
    mem_region_t cert;
    mem_region_t key;
    struct curl_common_custom_certs_t * next;
} curl_common_custom_certs_t;

typedef struct curl_common_ssl_ctx_data_t {
    curl_common_certs_t * certs;
    heap_t * heap;
    curl_common_custom_certs_t * custom_certs;
} curl_common_ssl_ctx_data_t;

curl_common_certs_list_t curl_common_load_default_certs_into_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, const char * const tag);
void curl_common_load_cert_into_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const char * const filepath, const char * const tag);
void curl_common_load_cert_from_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const const_mem_region_t cert_region, const char * const name, const char * const tag);

void curl_common_free_certs(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const char * const tag);
void curl_common_free_custom_certs(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_custom_certs_t * const certs, const char * const tag);
void curl_common_set_ssl_ctx(curl_common_ssl_ctx_data_t * const data, CURL * const curl, adk_curl_context_t * const ctx);
void curl_common_set_socket_callbacks(CURL * const curl, adk_curl_context_t * const ctx);

/*
    Functions below should be used if using global certs
*/
typedef struct mbedtls_x509_crt curl_common_ca_chain_t;

void curl_common_set_ssl_ctx_ca_chain(CURL * const curl, adk_curl_context_t * const ctx, void * ca_chain);
void curl_common_parse_certs_to_ca_chain(curl_common_certs_t * const certs, curl_common_ca_chain_t ** out_ca_chain);
void curl_common_free_ca_chain(curl_common_ca_chain_t * ca_chain);

#ifdef __cplusplus
}
#endif
