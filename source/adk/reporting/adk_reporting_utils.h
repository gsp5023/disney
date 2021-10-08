/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
    ADK reporting utils.
*/

#pragma once

#include "adk_reporting.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/runtime/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// See https://url.spec.whatwg.org/
typedef struct adk_reporting_url_info_t {
    char * protocol;
    char * origin; // [protocol]//[host]
    struct auth_info {
        char * username;
        char * password;
        char * auth; // [username]:[password]
    } auth_info;
    struct host_info {
        char * hostname; // NOTE: Minimum required to create href. All other fields are optional
        char * port;
        char * host; // [hostname]:[port]
    } host_info;
    struct path_info {
        char * pathname;
        char * query;
        char * search; // [?][query]
        char * path; // [pathname][search]
    } path_info;
    char * hash;
    char * href; // [protocol?]//[auth?]@[host][path?][hash?]
} adk_reporting_url_info_t;

// Extract href into adk_reporting_url_info_t structure. Returns heap allocated adk_reporting_url_info_t*
adk_reporting_url_info_t * adk_reporting_parse_href(adk_reporting_instance_t * const instance, const char * const url, const char * const tag);
void adk_reporting_free_url_info(adk_reporting_instance_t * const instance, adk_reporting_url_info_t * const u, const char * const tag);

// Build href string from adk_reporting_url_info_t structure. out_url_info->href will be set.
void adk_reporting_create_href(adk_reporting_instance_t * const instance, adk_reporting_url_info_t * out_url_info, const char * const tag);

#ifdef __cplusplus
}
#endif