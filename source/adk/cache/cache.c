/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cache.c

 ADK cache component
*/

#include "cache.h"

#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_http_utils.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef _CACHE_TRACE
#include "source/adk/telemetry/telemetry.h"
#define CACHE_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define CACHE_TRACE_PUSH(_name) TRACE_PUSH(_name)
#define CACHE_TRACE_POP() TRACE_POP()
#else
#define CACHE_TRACE_PUSH_FN()
#define CACHE_TRACE_PUSH(_name)
#define CACHE_TRACE_POP()
#endif

#define TAG_CACHE FOURCC('C', 'A', 'C', 'H')

enum {
    cache_max_url_length = 2084,
    cache_max_etag_length = 256,
};

struct cache_t {
    heap_t * heap;
    char subdirectory[sb_max_path_length];
};

/// In the case of 'atomic' caching, two paths are used to store the resource (resulting in 'resource states'):
/// - `cache-dir/p/$KEY` is used to store the resource while in-transit
/// - `cache-dir/f/$KEY` is used to store the 'finalized' resource
/// The transition from partial to final occurs through an `sb_rename` operation.
typedef enum cache_resource_state_e {
    cache_resource_state_partial,
    cache_resource_state_final
} cache_resource_state_e;

static void cache_build_file_path(
    char * const cache_file_path,
    const size_t max_length,
    const char * const subdirectory,
    const char * const key,
    const cache_resource_state_e state) {
    const size_t cache_file_path_length = strlen(subdirectory) + strlen(key) + 1;
    VERIFY(cache_file_path_length <= max_length);

    sprintf_s(
        cache_file_path,
        max_length,
        "%s%s%s",
        subdirectory,
        (state == cache_resource_state_partial) ? "p/" : "f/",
        key);
}

static void cache_create_directories(cache_t * const cache) {
    CACHE_TRACE_PUSH_FN();

    VERIFY(sb_create_directory_path(sb_app_cache_directory, cache->subdirectory));

    char path[sb_max_path_length];

    cache_build_file_path(path, ARRAY_SIZE(path), cache->subdirectory, "", cache_resource_state_partial);
    VERIFY(sb_create_directory_path(sb_app_cache_directory, path));

    cache_build_file_path(path, ARRAY_SIZE(path), cache->subdirectory, "", cache_resource_state_final);
    VERIFY(sb_create_directory_path(sb_app_cache_directory, path));

    CACHE_TRACE_POP();
}

cache_t * cache_create(const char * const subdirectory, mem_region_t region) {
    CACHE_TRACE_PUSH_FN();

    heap_t * const heap = heap_emplace_init_with_region(region, 8, 0, "cache");

    cache_t * const cache = heap_alloc(heap, sizeof(cache_t), MALLOC_TAG);
    ZEROMEM(cache);

    cache->heap = heap;

    strcpy_s(cache->subdirectory, ARRAY_SIZE(cache->subdirectory), subdirectory);
    cache_create_directories(cache);

    CACHE_TRACE_POP();

    return cache;
}

void cache_destroy(cache_t * const cache) {
    heap_t * const heap = cache->heap;

    heap_free(heap, cache, MALLOC_TAG);

#ifndef NDEBUG
    heap_debug_print_leaks(heap);
#endif

    heap_destroy(heap, MALLOC_TAG);
}

void cache_clear(cache_t * const cache) {
    CACHE_TRACE_PUSH_FN();
    VERIFY(sb_delete_directory(sb_app_cache_directory, cache->subdirectory) == sb_directory_delete_success);
    cache_create_directories(cache);
    CACHE_TRACE_POP();
}

typedef enum cache_file_header_type_e {
    cache_file_header_type_http = 1
} cache_file_header_type_e;

typedef PACK(struct cache_file_header_t {
    uint8_t version;
    uint8_t type;
    uint16_t etag_length;
    uint32_t content_length;
}) cache_file_header_t;

STATIC_ASSERT(sizeof(cache_file_header_t) == 8);

typedef enum http_request_ctx_recv_status_e {
    http_request_ctx_recv_status_init = 0,
    http_request_ctx_recv_status_append = 1,
    http_request_ctx_recv_status_skip = 2
} http_request_ctx_recv_status_e;

typedef struct http_request_ctx_t {
    cache_t * cache;

    char key[sb_max_path_length];
    char url[cache_max_url_length];
    char etag[cache_max_etag_length];
    cache_update_mode_e update_mode;
    size_t content_size;
    uint8_t recv_status;
    size_t recv_count;
    sb_file_t * recv_file;
    long response_code;
    adk_httpx_result_e result;
    cache_fetch_status_e fetch_status;
} http_request_ctx_t;

static bool on_http_header_recv(const const_mem_region_t header, void * const userdata) {
    CACHE_TRACE_PUSH_FN();

    http_request_ctx_t * const ctx = (http_request_ctx_t *)userdata;

    ctx->fetch_status = cache_fetch_success;

    if (ctx->response_code == 0) {
        ctx->response_code = http_parse_header_for_status_code(header);
        if (ctx->response_code != 200) {
            ctx->recv_status = http_request_ctx_recv_status_skip;
        }
    }

    {
        const const_mem_region_t value = http_parse_header_for_key("ETag", header);
        if (value.ptr != NULL) {
            memcpy(ctx->etag, value.ptr, value.size);
        }
    }

    {
        const const_mem_region_t value = http_parse_header_for_key("Content-Length", header);

        if (value.ptr != NULL) {
            // Parse header value as integer
            char content_length_value_buffer[128] = {0};
            memcpy(content_length_value_buffer, value.ptr, value.size);
            ctx->content_size = (size_t)strtol(content_length_value_buffer, NULL, 10);
        }
    }

    CACHE_TRACE_POP();

    return true;
}

static bool on_http_recv(const const_mem_region_t body, void * const userdata) {
    CACHE_TRACE_PUSH_FN();

    http_request_ctx_t * const ctx = userdata;
    cache_t * const cache = ctx->cache;

    // previous failure
    if (ctx->fetch_status != cache_fetch_success) {
        CACHE_TRACE_POP();
        return false;
    }

    if (ctx->content_size <= 0) {
        LOG_ERROR(TAG_CACHE, "Invalid or no content length returned in response header.");
        ctx->fetch_status = cache_fetch_invalid_content_length;
        CACHE_TRACE_POP();
        return false;
    }

    char cache_file_path[sb_max_path_length];
    cache_build_file_path(
        cache_file_path,
        ARRAY_SIZE(cache_file_path),
        cache->subdirectory,
        ctx->key,
        (ctx->update_mode == cache_update_mode_atomic) ? cache_resource_state_partial : cache_resource_state_final);

    switch (ctx->recv_status) {
        case http_request_ctx_recv_status_init: {
            const size_t etag_length = strlen(ctx->etag);

            sb_file_t * const cached_file = sb_fopen(sb_app_cache_directory, cache_file_path, "wb");

            if (cached_file != NULL) {
                const cache_file_header_t new_header = {
                    .version = 1,
                    .type = (uint8_t)cache_file_header_type_http,
                    .etag_length = (uint16_t)etag_length,
                    .content_length = (uint32_t)ctx->content_size,
                };

                sb_fwrite(&new_header, sizeof(uint8_t), sizeof(new_header), cached_file);
                sb_fwrite(ctx->etag, sizeof(uint8_t), etag_length, cached_file);

                ctx->recv_file = cached_file;
                ctx->recv_status = http_request_ctx_recv_status_append;
            } else {
                LOG_ERROR(TAG_CACHE, "Failed to open file: %s", cache_file_path);
                ctx->fetch_status = cache_fetch_file_open_failure;
                CACHE_TRACE_POP();
                return false;
            }
        }

        // fall through
        case http_request_ctx_recv_status_append: {
            if (ctx->recv_file != NULL) {
                sb_fwrite(body.ptr, sizeof(uint8_t), body.size, ctx->recv_file);
                ctx->recv_count += body.size;

                if (ctx->recv_count >= ctx->content_size) {
                    sb_fclose(ctx->recv_file);
                    ctx->recv_file = NULL;

                    if (ctx->update_mode == cache_update_mode_atomic) {
                        char final_path[sb_max_path_length];
                        cache_build_file_path(
                            final_path,
                            ARRAY_SIZE(final_path),
                            cache->subdirectory,
                            ctx->key,
                            cache_resource_state_final);

                        if (!sb_rename(sb_app_cache_directory, cache_file_path, final_path)) {
                            LOG_ERROR(TAG_CACHE, "Failed to move key (%s) is atomic-cache action!", ctx->key);
                            ctx->fetch_status = cache_fetch_key_move_failure;
                            CACHE_TRACE_POP();
                            return false;
                        }
                    }
                }
            } else {
                ctx->recv_status = http_request_ctx_recv_status_skip;
            }

            break;
        }

        case http_request_ctx_recv_status_skip:
        default:
            break;
    }

    CACHE_TRACE_POP();
    return true;
}

static void on_http_complete(
    adk_httpx_result_e result,
    int32_t response_code,
    void * const userdata) {
    CACHE_TRACE_PUSH_FN();
    http_request_ctx_t * const ctx = userdata;

    if (ctx->recv_file != NULL) { // not expected to occur
        LOG_ERROR(TAG_CACHE, "HTTP request completed before resource fully received/written: '%s'", ctx->key);

        sb_fclose(ctx->recv_file);
        ctx->recv_file = NULL;
    }

    ctx->result = result;
    if (adk_httpx_ok != result) {
        LOG_ERROR(TAG_CACHE, "HTTP request failed: %s: %d", ctx->url, result);
    }

    ctx->response_code = response_code;

    CACHE_TRACE_POP();
}

bool cache_get_content(
    cache_t * const cache,
    const char * const key,
    sb_file_t ** cached_file_content,
    size_t * cached_file_content_size) {
    CACHE_TRACE_PUSH_FN();
    ASSERT(cached_file_content != NULL);
    ASSERT(cached_file_content_size != NULL);

    char cache_file_path[sb_max_path_length];
    cache_build_file_path(
        cache_file_path,
        ARRAY_SIZE(cache_file_path),
        cache->subdirectory,
        key,
        cache_resource_state_final);

    sb_file_t * const resource = sb_fopen(sb_app_cache_directory, cache_file_path, "rb");

    if (resource == NULL) {
        CACHE_TRACE_POP();
        return false;
    }

    cache_file_header_t header = {0};
    const size_t num_header_bytes = sb_fread((void *)&header, sizeof(uint8_t), sizeof(header), resource);

    if (num_header_bytes != sizeof(header)) {
        sb_fclose(resource);
        LOG_ERROR(TAG_CACHE, "Failed to read header of %s", key);
        CACHE_TRACE_POP();
        return false;
    }

    LOG_DEBUG(TAG_CACHE, "Reading cached version of %s", key);

    ASSERT(header.version == 1);
    ASSERT(header.type == (uint8_t)cache_file_header_type_http);

    // Skip ETag portion of header
    sb_fseek(resource, header.etag_length, sb_seek_cur);

    {
        // Verify the cache content is of the correct length (according to the header)

        const long head = sb_ftell(resource);

        sb_fseek(resource, 0, sb_seek_end);
        const long tail = sb_ftell(resource);

        sb_fseek(resource, head, sb_seek_set); // Set to original position (immediately after header)

        if ((tail - head) != (long)header.content_length) {
            sb_fclose(resource);
            LOG_ERROR(TAG_CACHE, "Resource content length does not match header for %s: expected: %d, actual: %d", key, header.content_length, (tail - head));
            CACHE_TRACE_POP();
            return false;
        }
    }

    *cached_file_content = resource;
    *cached_file_content_size = header.content_length;

    CACHE_TRACE_POP();
    return true;
}

static void construct_request_header(
    mem_region_t header,
    const cache_file_header_t * const file_header,
    sb_file_t * const cached_file) {
    CACHE_TRACE_PUSH_FN();

    static const char if_none_match_key[] = "If-None-Match: ";
    const size_t if_none_match_key_length = strlen(if_none_match_key);

    // Copy key and value (etag) into file_header string => "key: value"
    const size_t etag_length = file_header->etag_length;
    ASSERT(header.size >= if_none_match_key_length + etag_length + 1);
    memcpy(header.byte_ptr, if_none_match_key, if_none_match_key_length);
    sb_fread(header.byte_ptr + if_none_match_key_length, sizeof(uint8_t), etag_length, cached_file);
    header.byte_ptr[if_none_match_key_length + etag_length] = '\0';

    CACHE_TRACE_POP();
}

cache_fetch_status_e cache_fetch_resource_from_url(
    cache_t * const cache,
    const char * const key,
    const char * const url,
    const cache_update_mode_e update_mode) {
    CACHE_TRACE_PUSH_FN();
    char cache_file_path[sb_max_path_length];
    cache_build_file_path(
        cache_file_path,
        ARRAY_SIZE(cache_file_path),
        cache->subdirectory,
        key,
        cache_resource_state_final);

    cache_file_header_t file_header = {0};

    char header_buffer[1024];
    const char * request_headers[] = {NULL};
    size_t num_request_headers = 0;

    sb_file_t * const cached_file
        = sb_fopen(sb_app_cache_directory, cache_file_path, "rb");

    if (cached_file != NULL) {
        sb_fread((void *)&file_header, sizeof(uint8_t), sizeof(file_header), cached_file);

        if (file_header.version != 1) {
            CACHE_TRACE_POP();
            return cache_fetch_invalid_version;
        }

        if (file_header.type != (uint8_t)cache_file_header_type_http) {
            CACHE_TRACE_POP();
            return cache_fetch_invalid_file_header_type;
        }

        mem_region_t header = MEM_REGION(.ptr = header_buffer, .size = ARRAY_SIZE(header_buffer));
        construct_request_header(header, &file_header, cached_file);

        request_headers[0] = header_buffer;
        num_request_headers = 1;

        sb_fclose(cached_file);
    }

    http_request_ctx_t ctx = {0};
    ctx.cache = cache;
    strcpy_s(ctx.key, ARRAY_SIZE(ctx.key), key);
    strcpy_s(ctx.url, ARRAY_SIZE(ctx.url), url);
    ctx.update_mode = update_mode;

    {
        CACHE_TRACE_PUSH("http-fetch");

        adk_httpx_fetch_callbacks_t callbacks = {
            .on_header = on_http_header_recv,
            .on_body = on_http_recv,
            .on_complete = on_http_complete,
            .userdata = &ctx,
        };

        adk_httpx_fetch(cache->heap, ctx.url, callbacks, request_headers, num_request_headers);

        CACHE_TRACE_POP();
    }

    // Check for any error from the callbacks
    if (ctx.fetch_status != cache_fetch_success) {
        CACHE_TRACE_POP();
        return ctx.fetch_status;
    }

    if (ctx.response_code == 304) {
        LOG_DEBUG(TAG_CACHE, "Received response of already cached version for %s", url);
    } else if (ctx.response_code == 200) {
        file_header.content_length = (uint32_t)ctx.content_size;
    } else {
        LOG_ERROR(TAG_CACHE, "Failed to fetch resource(%s): result: %d, response: %d", url, ctx.result, ctx.response_code);

        file_header.content_length = 0;

        CACHE_TRACE_POP();
        return cache_fetch_http_request_failed;
    }

    if (file_header.content_length <= 0) {
        CACHE_TRACE_POP();
        return cache_fetch_invalid_cache_file;
    }

    CACHE_TRACE_POP();
    return cache_fetch_success;
}

void cache_delete_key(cache_t * const cache, const char * const key) {
    CACHE_TRACE_PUSH_FN();
    char cache_file_path[sb_max_path_length];
    cache_build_file_path(
        cache_file_path,
        ARRAY_SIZE(cache_file_path),
        cache->subdirectory,
        key,
        cache_resource_state_final);

    sb_delete_file(sb_app_cache_directory, cache_file_path);
    CACHE_TRACE_POP();
}
