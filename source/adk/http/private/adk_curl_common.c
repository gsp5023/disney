/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/private/adk_curl_common.h"

#include "extern/curl/curl/include/curl/curl.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/platform.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/sha1.h"
#include "mbedtls/ssl.h"
#include "source/adk/file/file.h"
#include "source/adk/http/private/adk_curl_context.h"
#include "source/adk/http/websockets/base64_encode.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/telemetry/telemetry.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/error.h"

#if defined(__linux__) || defined(_LEIA) || defined(_VADER)
#include <netinet/in.h>
#endif

#ifdef _LEIA
#define CURL_COMMON_IPV4_ONLY
#endif
#if defined(_LEIA) || defined(_VADER)
#include <net.h>
#endif
#if defined(_VADER)
#include <net6.h>
#include <netinet6/in6.h>
#endif

#include <limits.h>

#define TAG_CURL_COMMON FOURCC('C', 'R', 'L', 'C')

struct curl_wrapper_t {
    CURL * const curl;
    adk_curl_context_t * ctx;
};

static const char * const ca_bundle_path = "certs/ca-bundle.crt";
#if !defined(_SHIP) || defined(_SHIP_DEV)
static const char * const charles_pem_path = "certs/charles.pem";
#endif

static int load_cert(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const char * const cert_path, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();

    sb_file_t * const cert_bundle_fp = sb_fopen(sb_app_root_directory, cert_path, "rb");
    if (cert_bundle_fp == NULL) {
        CURL_COMMON_TRACE_POP();
        return 0;
    }

    sb_fseek(cert_bundle_fp, 0, sb_seek_end);
    const size_t cert_file_size = (size_t)sb_ftell(cert_bundle_fp);
    sb_fseek(cert_bundle_fp, 0, sb_seek_set);
    const size_t cert_path_len = strlen(cert_path) + 1;

    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }
    curl_common_certs_node_t * const cert_node = heap_calloc(heap, sizeof(curl_common_certs_node_t), MALLOC_TAG);
    // Add 1 for the string terminator. mBedTls needs this to detect the buffer as a PEM file.
    // adk_CURL_COMMON_calloc will handle setting this for us.
    cert_node->cert = MEM_REGION(.ptr = heap_calloc(heap, cert_file_size + 1, tag), .size = cert_file_size + 1);
    cert_node->name = heap_alloc(heap, cert_path_len, MALLOC_TAG);
    if (optional_mutex) {
        sb_unlock_mutex(optional_mutex);
    }

    sb_fread(cert_node->cert.ptr, cert_file_size, 1, cert_bundle_fp);
    sb_fclose(cert_bundle_fp);

    memcpy(cert_node->name, cert_path, cert_path_len);
    LL_ADD(cert_node, prev, next, certs->head, certs->tail);

    LOG_INFO(TAG_CURL_COMMON, "Cert loaded: %s", cert_path);
    CURL_COMMON_TRACE_POP();
    return 1;
}

curl_common_certs_list_t curl_common_load_default_certs_into_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();
    curl_common_certs_list_t certs = {0};
    int num_certs_loaded = load_cert(heap, optional_mutex, &certs, ca_bundle_path, tag);
    if (num_certs_loaded == 0) {
        LOG_ERROR(TAG_CURL_COMMON, "Failed to load cert: %s", ca_bundle_path);
    }
#if !defined(_SHIP) || defined(_SHIP_DEV)
    if (load_cert(heap, optional_mutex, &certs, charles_pem_path, tag) == 0) {
#ifdef _RESTRICTED
        LOG_WARN(TAG_CURL_COMMON, "Failed to load charles cert: %s", charles_pem_path);
#endif
    } else {
        num_certs_loaded++;
    }
#endif
    LOG_DEBUG(TAG_CURL_COMMON, "Certs loaded: [%i]", num_certs_loaded);
    CURL_COMMON_TRACE_POP();
    return certs;
}

void curl_common_load_cert_into_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const char * const filepath, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();
    const int num_certs_loaded = load_cert(heap, optional_mutex, certs, filepath, tag);
    if (num_certs_loaded == 0) {
        LOG_ERROR(TAG_CURL_COMMON, "Failed to load cert: %s", filepath);
    }
    LOG_DEBUG(TAG_CURL_COMMON, "Certs loaded: [%i]", num_certs_loaded);
    CURL_COMMON_TRACE_POP();
}

void curl_common_load_cert_from_memory(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const const_mem_region_t cert_region, const char * const name, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();
    const size_t name_len = strlen(name) + 1;

    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }
    curl_common_certs_node_t * const cert_node = heap_calloc(heap, sizeof(curl_common_certs_node_t), tag);
    cert_node->cert = MEM_REGION(.ptr = heap_alloc(heap, cert_region.size, MALLOC_TAG), .size = cert_region.size);
    cert_node->name = heap_alloc(heap, name_len, MALLOC_TAG);
    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }

    memcpy(cert_node->cert.ptr, cert_region.ptr, cert_region.size);
    memcpy(cert_node->name, name, name_len);
    LL_ADD(cert_node, prev, next, certs->head, certs->tail);
    CURL_COMMON_TRACE_POP();
}

void curl_common_free_certs(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_certs_list_t * const certs, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();

    curl_common_certs_node_t * curr = certs->head;
    while (curr) {
        curl_common_certs_node_t * const next = curr->next;
        LL_REMOVE(curr, prev, next, certs->head, certs->tail);
        LOG_DEBUG(TAG_CURL_COMMON, "Cert unloaded: [%s]", curr->name);
        if (optional_mutex) {
            sb_lock_mutex(optional_mutex);
        }
        heap_free(heap, curr->cert.ptr, tag);
        heap_free(heap, curr->name, tag);
        heap_free(heap, curr, tag);
        if (optional_mutex) {
            sb_unlock_mutex(optional_mutex);
        }
        curr = next;
    }

    ZEROMEM(certs);

    CURL_COMMON_TRACE_POP();
}

void curl_common_free_custom_certs(heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_custom_certs_t * const certs, const char * const tag) {
    CURL_COMMON_TRACE_PUSH_FN();
    curl_common_custom_certs_t * curr = certs;
    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }
    while (curr) {
        heap_free(heap, curr->cert.ptr, tag);
        if (curr->key.ptr) {
            heap_free(heap, curr->key.ptr, tag);
        }
        curr = curr->next;
    }
    if (optional_mutex) {
        sb_unlock_mutex(optional_mutex);
    }
    CURL_COMMON_TRACE_POP();
}

//Parses custom certs. First the certificates are parsed. If certificates are parsed correctly, the cert is checked for a private key.
static CURLcode ssl_load_parse_custom_certs(mbedtls_ssl_config * const ssl_config, const mem_region_t cert_region, const char * const name, heap_t * const heap, sb_mutex_t * const optional_mutex, curl_common_custom_certs_t * custom_certs) {
    CURL_COMMON_TRACE_PUSH_FN();
    //Getting certificates
    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }
    mbedtls_x509_crt * const crt = heap_calloc(heap, sizeof(mbedtls_x509_crt), MALLOC_TAG);
    if (optional_mutex) {
        sb_unlock_mutex(optional_mutex);
    }

    mbedtls_x509_crt_init(crt);
    const int crt_parse_error = mbedtls_x509_crt_parse(crt, cert_region.ptr, cert_region.size);
    if (crt_parse_error == 0) {
        LOG_DEBUG(TAG_CURL_COMMON, "Using cert bundle [%s] from memory successful.", name);
    } else if (crt_parse_error > 0) {
        LOG_ERROR(TAG_CURL_COMMON, "Using cert bundle [%s] from memory failed. %d certificates did not parse.", name, crt_parse_error);
        CURL_COMMON_TRACE_POP();
        return CURLE_ABORTED_BY_CALLBACK;
    } else {
        LOG_ERROR(TAG_CURL_COMMON, "Using cert bundle [%s] from memory failed with error %d", name, crt_parse_error);
        CURL_COMMON_TRACE_POP();
        return CURLE_ABORTED_BY_CALLBACK;
    }

    if (optional_mutex) {
        sb_lock_mutex(optional_mutex);
    }
    if (!custom_certs) {
        custom_certs = heap_calloc(heap, sizeof(curl_common_custom_certs_t), MALLOC_TAG);
    } else {
        curl_common_custom_certs_t * new_node = heap_calloc(heap, sizeof(curl_common_custom_certs_t), MALLOC_TAG);
        ZEROMEM(new_node);
        *new_node = *custom_certs;
        custom_certs->next = new_node;
    }
    custom_certs->cert.ptr = crt;
    custom_certs->cert.size = sizeof(mbedtls_x509_crt);

    //Getting private key
    mbedtls_pk_context * const pk = heap_calloc(heap, sizeof(mbedtls_pk_context), MALLOC_TAG);
    if (optional_mutex) {
        sb_unlock_mutex(optional_mutex);
    }
    mbedtls_pk_init(pk);
    const int key_parse_error = mbedtls_pk_parse_key(pk, cert_region.ptr, cert_region.size, NULL, 0);
    if (!key_parse_error) {
        custom_certs->key.ptr = pk;
        custom_certs->key.size = sizeof(mbedtls_pk_context);
        LOG_DEBUG(TAG_CURL_COMMON, "Key from cert bundle [%s] loaded successfully", name);
    } else {
        if (optional_mutex) {
            sb_lock_mutex(optional_mutex);
        }
        heap_free(heap, pk, MALLOC_TAG);
        if (optional_mutex) {
            sb_unlock_mutex(optional_mutex);
        }
        custom_certs->key.ptr = NULL;
        custom_certs->key.size = 0;
        char buf[200] = {0};
        mbedtls_strerror(key_parse_error, buf, 200);
        LOG_DEBUG(TAG_CURL_COMMON, "Key from cert bundle [%s] couldn't be loaded. Code: -0x%04X (%s)", name, (int) -key_parse_error, buf);
    }
    const int add_custom_cert_error = mbedtls_ssl_conf_own_cert(ssl_config, custom_certs->cert.ptr, custom_certs->key.ptr);
    if (!add_custom_cert_error) {
        CURL_COMMON_TRACE_POP();
        return CURLE_OK;
    } else {
        LOG_DEBUG(TAG_CURL_COMMON, "Could not add cert bundle [%s] to custom cert list %d", name, add_custom_cert_error);
        CURL_COMMON_TRACE_POP();
        return CURLE_ABORTED_BY_CALLBACK;
    }
}

static CURLcode ssl_load_parse_certs(mbedtls_x509_crt * const chain, const mem_region_t cert_region, const char * const name) {
    CURL_COMMON_TRACE_PUSH_FN();
    const int crt_parse_error = mbedtls_x509_crt_parse(chain, cert_region.ptr, cert_region.size);

    if (crt_parse_error == 0) {
        LOG_DEBUG(TAG_CURL_COMMON, "Using cert bundle [%s] from memory successful.", name);
        CURL_COMMON_TRACE_POP();
        return CURLE_OK;
    } else if (crt_parse_error > 0) {
        LOG_ERROR(TAG_CURL_COMMON, "Using cert bundle [%s] from memory failed. %d certificates did not parse.", name, crt_parse_error);
    } else {
        LOG_ERROR(TAG_CURL_COMMON, "Using cert bundle [%s] from memory failed with error %d", name, crt_parse_error);
    }
    CURL_COMMON_TRACE_POP();
    return CURLE_ABORTED_BY_CALLBACK;
}

static CURLcode ssl_ctx_handler_ca_chain(CURL * curl, void * sslctx, void * parm) {
    ASSERT(parm);
    mbedtls_x509_crt * const ca_chain = parm;
    mbedtls_ssl_config * config = sslctx;
    config->ca_chain = ca_chain;
    return CURLE_OK;
}

static CURLcode ssl_ctx_handler(CURL * curl, void * sslctx, void * parm) {
    curl_common_ssl_ctx_data_t * data = parm;
    mbedtls_ssl_config * ssl_config = sslctx;
    CURL_COMMON_TRACE_PUSH_FN();
    curl_common_certs_list_t * const default_certs = &data->certs->default_certs;
    curl_common_certs_list_t * const custom_certs = &data->certs->custom_certs;

    curl_common_certs_node_t * curr = default_certs->head;
    while (curr) {
        const CURLcode cert_ret = ssl_load_parse_certs(ssl_config->ca_chain, curr->cert, curr->name);
        if (cert_ret != CURLE_OK) {
            CURL_COMMON_TRACE_POP();
            return cert_ret;
        }
        curr = curr->next;
    }

    curr = custom_certs->head;
    while (curr) {
        const CURLcode cert_ret = ssl_load_parse_custom_certs(ssl_config, curr->cert, curr->name, data->heap, NULL, data->custom_certs);
        if (cert_ret != CURLE_OK) {
            CURL_COMMON_TRACE_POP();
            return cert_ret;
        }
        curr = curr->next;
    }
    CURL_COMMON_TRACE_POP();
    return CURLE_OK;
}

void curl_common_parse_certs_to_ca_chain(curl_common_certs_t * const certs, curl_common_ca_chain_t ** out_ca_chain) {
    ASSERT(*out_ca_chain == NULL);
    // TODO - This will resolve to nake malloc. Remove when https://jira.disneystreaming.com/browse/M5-1813 is finished
    *out_ca_chain = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));

    void * const ca_chain = *out_ca_chain;
    curl_common_certs_list_t * const cert_lists = (curl_common_certs_list_t *)certs;
    for (size_t i = 0; i < sizeof(curl_common_certs_t) / sizeof(curl_common_certs_list_t); ++i) {
        curl_common_certs_node_t * curr = cert_lists[i].head;
        while (curr) {
            const CURLcode cert_ret = ssl_load_parse_certs(ca_chain, curr->cert, curr->name);
            if (cert_ret != CURLE_OK) {
                return;
            }
            curr = curr->next;
        }
    }

    (*out_ca_chain)->keep = 1;
}

void curl_common_free_ca_chain(curl_common_ca_chain_t * ca_chain) {
    if (!ca_chain) {
        return;
    }
    ca_chain->keep = 0;
    mbedtls_x509_crt_free(ca_chain);
}

void curl_common_set_ssl_ctx_ca_chain(CURL * const curl, adk_curl_context_t * const ctx, void * ca_chain) {
    /* Turn off the default CA locations, otherwise libcurl will load CA
     * certificates from the locations that were detected/specified at
     * build-time
     */
    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(ctx);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_handler_ca_chain);
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, ca_chain);
    adk_curl_set_context(ctx_old);
}

void curl_common_set_ssl_ctx(curl_common_ssl_ctx_data_t * const data, CURL * const curl, adk_curl_context_t * const ctx) {
    /* Turn off the default CA locations, otherwise libcurl will load CA
     * certificates from the locations that were detected/specified at
     * build-time
     */
    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(ctx);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_handler);
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, data);
    adk_curl_set_context(ctx_old);
}

static curl_socket_t open_socket_callback(void * clientp, curlsocktype purpose, struct curl_sockaddr * address) {
    CURL_COMMON_TRACE_PUSH_FN();
    ASSERT(purpose == CURLSOCKTYPE_IPCXN);

    bool connected = false;

    sb_socket_family_e socket_family;
    switch (address->family) {
        case AF_INET:
            socket_family = sb_socket_family_IPv4;
            break;
        case AF_INET6:
            socket_family = sb_socket_family_IPv6;
            break;
        default:
            socket_family = sb_socket_family_unsupported;
    }

    CURL_COMMON_TRACE_PUSH("sb_create_socket");
    sb_socket_t new_socket;
    int platform_error = sb_create_socket(socket_family, sb_socket_type_stream, sb_socket_protocol_tcp, &new_socket);
    CURL_COMMON_TRACE_POP();

    if (platform_error == 0) {
        // While unlikely to happen, verify that socket_id can fit into curl's expected (INT)socket id
        VERIFY(new_socket.socket_id <= INT_MAX);

        CURL_COMMON_TRACE_PUSH("curl_sockaddr conversion");
        sb_sockaddr_t sb_sockaddr = {0};
        switch (socket_family) {
            case sb_socket_family_IPv4: {
                struct sockaddr_in * ipv4_sockaddr = (struct sockaddr_in *)&address->addr;
                sb_sockaddr.sin_family = sb_socket_family_IPv4;
                sb_sockaddr.sin_port = ipv4_sockaddr->sin_port;
                memcpy(&sb_sockaddr.sin_addr, &ipv4_sockaddr->sin_addr, sizeof(ipv4_sockaddr->sin_addr));
                break;
            }
            case sb_socket_family_IPv6: {
#ifndef CURL_COMMON_IPV4_ONLY
                const struct sockaddr_in6 * ipv6_sockaddr = (struct sockaddr_in6 *)&address->addr;
                sb_sockaddr.sin_family = sb_socket_family_IPv6;
                sb_sockaddr.sin_port = ipv6_sockaddr->sin6_port;
                sb_sockaddr.ipv6_flowinfo = ipv6_sockaddr->sin6_flowinfo;
                sb_sockaddr.ipv6_scope_id = ipv6_sockaddr->sin6_scope_id;
                memcpy(&sb_sockaddr.sin_addr, &ipv6_sockaddr->sin6_addr, sizeof(ipv6_sockaddr->sin6_addr));
#else
                TRAP("Invalid socket family (IPV6) -- this target only supports IPV4");
#endif
                break;
            }
            default:
                TRAP("Invalid socket family %i", socket_family);
        }
        CURL_COMMON_TRACE_POP();

        sb_enable_blocking_socket(new_socket, sb_socket_blocking_disabled);
        CURL_COMMON_TRACE_PUSH("sb_connect_socket");
        const sb_socket_connect_result_t connect_result = sb_connect_socket(new_socket, &sb_sockaddr);
        CURL_COMMON_TRACE_POP();
        sb_socket_connect_error_e allowed_states[] = {
            sb_socket_connect_in_progress,
            sb_socket_connect_success,
            sb_socket_connect_would_block,
        };
        for (size_t i = 0; i < ARRAY_SIZE(allowed_states); ++i) {
            if (connect_result.result == allowed_states[i]) {
                connected = true;
                break;
            }
        }
        if (!connected) {
            CURL_COMMON_TRACE_PUSH("sb_close_socket");
            sb_close_socket(new_socket);
            CURL_COMMON_TRACE_POP();
        }
    }

    CURL_COMMON_TRACE_POP();
    return connected ? (int)new_socket.socket_id : CURL_SOCKET_BAD;
}

static int close_socket_callback(void * clientp, curl_socket_t item) {
    CURL_COMMON_TRACE_PUSH_FN();
    (void)clientp;
    const sb_socket_t close_socket = {.socket_id = item};
    sb_close_socket(close_socket);
    CURL_COMMON_TRACE_POP();
    return 0;
}

// clientp is always user defined, define with CURLOPT_SOCKOPTDATA, this is called once (after opensocket).
static int sockopt_callback(void * clientp, curl_socket_t curlfd, curlsocktype purpose) {
    CURL_COMMON_TRACE_PUSH_FN();
    (void)purpose;

    if (clientp) {
        curl_common_sockopt_callback_data_t * sock_settings = (curl_common_sockopt_callback_data_t *)clientp;
        sb_socket_t sockfd = {.socket_id = curlfd};
        sb_enable_blocking_socket(sockfd, sock_settings->blocking);
        sb_socket_set_tcp_no_delay(sockfd, sock_settings->delay_mode);
        sb_socket_set_linger(sockfd, sock_settings->linger_mode, sock_settings->linger_time);
    }

    CURL_COMMON_TRACE_POP();
    return CURL_SOCKOPT_ALREADY_CONNECTED;
}

void curl_common_set_socket_callbacks(CURL * const curl, adk_curl_context_t * const ctx) {
    adk_curl_context_t * const ctx_old = adk_curl_get_context();
    adk_curl_set_context(ctx);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, (void *)open_socket_callback);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, close_socket_callback);
    curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
    adk_curl_set_context(ctx_old);
}
