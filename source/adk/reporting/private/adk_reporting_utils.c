/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
adk_reporting_utils.c

adk_reporting implementation for util functions
*/

#include "source/adk/reporting/adk_reporting_utils.h"

#include "source/adk/reporting/private/adk_reporting_sentry.h"
#include "source/adk/steamboat/sb_thread.h"

#define TAG_ADK_REPORTING FOURCC('R', 'E', 'P', 'O')

static void alloc_and_copy(adk_reporting_instance_t * const instance, char ** dest, const char * src, const size_t buffer_size, const char * const tag) {
    *dest = (char *)adk_reporting_malloc(instance, buffer_size, tag);
    memcpy(*dest, src, buffer_size);
    (*dest)[buffer_size - 1] = 0;
}

static void realloc_and_append(adk_reporting_instance_t * const instance, char ** dest, const char * append, const size_t old_buffer_size, const size_t new_buffer_size, const char * const tag) {
    *dest = (char *)adk_reporting_realloc(instance, *dest, new_buffer_size, tag);
    memcpy(*dest + old_buffer_size - 1, append, new_buffer_size - old_buffer_size + 1);
    (*dest)[new_buffer_size - 1] = 0;
}

static size_t search_and_strip(adk_reporting_instance_t * const instance, bool use_left_side_token, char ** search_str, size_t * search_str_size, const char * needle, bool strip, const size_t needle_length, bool include_needle, bool strip_needle, char ** dest, const char * const tag) {
    const char * ret = strstr(*search_str, needle);
    if (ret != NULL) {
        const size_t found_word_length_left = ret - *search_str;
        const size_t word_length_to_copy = use_left_side_token ? found_word_length_left : (*search_str_size - found_word_length_left - 1);
        const size_t buffer_size = word_length_to_copy + 1 + (size_t)(include_needle ? needle_length : 0);
        alloc_and_copy(instance, dest, use_left_side_token ? *search_str : ret, buffer_size, tag);

        if (strip) {
            // update search_str_size
            const size_t strip_off_length = buffer_size - 1 + (strip_needle && !include_needle ? needle_length : 0);

            // strip begin/end
            if (use_left_side_token) {
                (*search_str) += strip_off_length;
            } else {
                (*search_str)[*search_str_size - strip_off_length] = 0;
            }

            *search_str_size -= strip_off_length;
        }

        return buffer_size;
    }
    return 0;
}

adk_reporting_url_info_t * adk_reporting_parse_href(adk_reporting_instance_t * const instance, const char * const url, const char * const tag) {
    ASSERT(url != NULL && strlen(url));
    const size_t url_buffer_length = strlen(url) + 1;

    adk_reporting_url_info_t * const u = adk_reporting_malloc(instance, sizeof(adk_reporting_url_info_t), tag);
    *u = (struct adk_reporting_url_info_t){0};

    // Parse/modify off url_ptr
    char * url_copy = NULL;
    alloc_and_copy(instance, &url_copy, url, url_buffer_length, tag);
    char * url_ptr = url_copy;
    size_t current_url_length = url_buffer_length - 1;

    // Parse protocol (not required)
    size_t protocol_buffer_size = search_and_strip(instance, true, &url_ptr, &current_url_length, "//", true, 2, false, true, &u->protocol, tag);
    if (u->protocol != NULL) {
        alloc_and_copy(instance, &u->origin, u->protocol, protocol_buffer_size, tag);
        realloc_and_append(instance, &u->origin, "//", protocol_buffer_size, protocol_buffer_size + 2, tag);
        protocol_buffer_size += 2;
    }

    // Parse auth, username, password (not required)
    size_t auth_buff_size = search_and_strip(instance, true, &url_ptr, &current_url_length, "@", true, 1, false, true, &u->auth_info.auth, tag);
    if (u->auth_info.auth != NULL) {
        search_and_strip(instance, true, &u->auth_info.auth, &auth_buff_size, ":", false, 1, false, false, &u->auth_info.username, tag);
        if (u->auth_info.username == NULL) {
            alloc_and_copy(instance, &u->auth_info.username, u->auth_info.auth, auth_buff_size, tag);
        } else {
            search_and_strip(instance, false, &u->auth_info.auth, &auth_buff_size, ":", false, 1, false, false, &u->auth_info.password, tag);
        }
    }

    // Parse hash (not required)
    search_and_strip(instance, false, &url_ptr, &current_url_length, "#", true, 1, true, true, &u->hash, tag);

    // Parse search & query (not required)
    const size_t search_buffer_size = search_and_strip(instance, false, &url_ptr, &current_url_length, "?", true, 1, true, true, &u->path_info.search, tag);
    if (u->path_info.search != NULL) {
        alloc_and_copy(instance, &u->path_info.query, u->path_info.search, search_buffer_size - 1, tag);
    }

    // Parse pathname & set path (not required)
    const size_t pathname_buffer_size = search_and_strip(instance, false, &url_ptr, &current_url_length, "/", true, 1, true, true, &u->path_info.pathname, tag);
    alloc_and_copy(instance, &u->path_info.path, u->path_info.pathname, pathname_buffer_size, tag);
    if (u->path_info.search != NULL) {
        realloc_and_append(instance, &u->path_info.path, u->path_info.search, pathname_buffer_size, pathname_buffer_size + search_buffer_size, tag);
    }

    // Set host (required)
    const size_t host_buffer_size = current_url_length + 1;
    if (current_url_length < 1) {
        adk_reporting_free(instance, url_copy, tag);
        adk_reporting_free_url_info(instance, u, tag);
#ifndef NDEBUG
        TRAP("'%s' does not contain a host field", url);
#endif
        return NULL;
    }
    alloc_and_copy(instance, &u->host_info.host, url_ptr, host_buffer_size, tag);

    // Parse hostname/port (port not required)
    size_t ignore = 0;
    search_and_strip(instance, true, &u->host_info.host, &ignore, ":", false, 1, false, false, &u->host_info.hostname, tag);
    if (u->host_info.hostname == NULL) {
        alloc_and_copy(instance, &u->host_info.hostname, u->host_info.host, host_buffer_size, tag);
    } else {
        search_and_strip(instance, false, &u->host_info.host, &ignore, ":", false, 1, false, false, &u->host_info.port, tag);
    }

    // Set/Append origin [protocol?][host]
    realloc_and_append(instance, &u->origin, u->host_info.host, protocol_buffer_size, protocol_buffer_size + host_buffer_size, tag);

    // Set href
    adk_reporting_create_href(instance, u, tag);

    adk_reporting_free(instance, url_copy, tag);
    url_copy = NULL;
    return u;
}

static void free_url_info_string(adk_reporting_instance_t * const instance, char ** const s, const char * const tag) {
    ASSERT(s);
    if (*s != NULL) {
        adk_reporting_free(instance, *s, tag);
        *s = NULL;
    }
}

void adk_reporting_free_url_info(adk_reporting_instance_t * const instance, adk_reporting_url_info_t * const u, const char * const tag) {
    if (u == NULL) {
        return;
    }

    free_url_info_string(instance, &u->protocol, tag);
    free_url_info_string(instance, &u->origin, tag);
    free_url_info_string(instance, &u->hash, tag);
    free_url_info_string(instance, &u->href, tag);
    free_url_info_string(instance, &u->auth_info.auth, tag);
    free_url_info_string(instance, &u->auth_info.username, tag);
    free_url_info_string(instance, &u->auth_info.password, tag);
    free_url_info_string(instance, &u->host_info.host, tag);
    free_url_info_string(instance, &u->host_info.hostname, tag);
    free_url_info_string(instance, &u->host_info.port, tag);
    free_url_info_string(instance, &u->path_info.path, tag);
    free_url_info_string(instance, &u->path_info.pathname, tag);
    free_url_info_string(instance, &u->path_info.query, tag);
    free_url_info_string(instance, &u->path_info.search, tag);

    adk_reporting_free(instance, u, tag);
}

void adk_reporting_create_href(adk_reporting_instance_t * const instance, adk_reporting_url_info_t * out_url_info, const char * const tag) {
    ASSERT(out_url_info != NULL);
    ASSERT(out_url_info->host_info.host != NULL);
    if (out_url_info->href != NULL) {
        return;
    }

    //href = [protocol?]//[auth?]@[host][path?][hash?]
    static const char protocol_append_double_slash[] = "//";
    static const char auth_append_at_sign[] = "@";

    const char * const protocol = out_url_info->protocol != NULL ? out_url_info->protocol : "";
    const char * const protocol_append = out_url_info->protocol != NULL ? protocol_append_double_slash : "";
    const char * const auth = out_url_info->auth_info.auth != NULL ? out_url_info->auth_info.auth : "";
    const char * const auth_append = out_url_info->auth_info.auth != NULL ? auth_append_at_sign : "";
    const char * const host = out_url_info->host_info.host != NULL ? out_url_info->host_info.host : "";
    const char * const path = out_url_info->path_info.path != NULL ? out_url_info->path_info.path : "";
    const char * const hash = out_url_info->hash != NULL ? out_url_info->hash : "";

    const size_t href_buffer_length = (out_url_info->protocol != NULL ? strlen(out_url_info->protocol) + strlen(protocol_append_double_slash) : 0) + (out_url_info->auth_info.auth != NULL ? strlen(out_url_info->auth_info.auth) + strlen(auth_append_at_sign) : 0) + (out_url_info->host_info.host != NULL ? strlen(out_url_info->host_info.host) : 0) + (out_url_info->path_info.path != NULL ? strlen(out_url_info->path_info.path) : 0) + (out_url_info->hash != NULL ? strlen(out_url_info->hash) : 0) + 1;

    out_url_info->href = adk_reporting_malloc(instance, href_buffer_length, tag);
    ZEROMEM(out_url_info->href);
    sprintf_s(out_url_info->href, href_buffer_length, "%s%s%s%s%s%s%s", protocol, protocol_append, auth, auth_append, host, path, hash);
}