/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
 cache.c

 ADK cache component
*/

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cache_t cache_t;

/// Creates a new `cache` with the given `subdirectory`
cache_t * cache_create(const char * const subdirectory, mem_region_t region);

/// Releases the `cache` instance
void cache_destroy(cache_t * const cache);

/// Clears all contents of `cache`
void cache_clear(cache_t * const cache);

/// Retrieves fp to the already-cache instance of `key` from `cache`
/// - If found in cache, returns `true` and assigns args for an opened fp (at offset of contents start) and the content size
/// - Else if resource (associated with `key`) is not in the cache, returns `false`
bool cache_get_content(
    cache_t * const cache,
    const char * const key,
    sb_file_t ** cached_file_content,
    size_t * cached_file_content_size);

typedef enum cache_update_mode_e {
    cache_update_mode_atomic,
    cache_update_mode_in_place,
} cache_update_mode_e;

typedef enum cache_fetch_status_e {
    cache_fetch_success,
    // The file was cached with an version not supported by the fetch
    cache_fetch_invalid_version,
    // The file was cached with a cache_file_header_type_e not supported by the fetch
    cache_fetch_invalid_file_header_type,
    // The HTTP request did not return a success code
    cache_fetch_http_request_failed,
    // The cached file is not valid
    cache_fetch_invalid_cache_file,
    // A valid HTTP response header for content length was not returned
    cache_fetch_invalid_content_length,
    // The data could not be written completely written to the cache file because the file could not be opened for writing.
    cache_fetch_file_open_failure,
    // could not move the cache key
    cache_fetch_key_move_failure
} cache_fetch_status_e;

/// Add/update the cached file for `key` from the provided `url` with respect to the `cache`
///
/// - If the resource is already cached and the resource provided by `url` is the same, keep the cached version and return `true`
/// - If the resource is not cached or the resource at `url` is different, cache the new resource and return `true`
/// - If a failure occurred while attempting to cache the resource, return `false`
cache_fetch_status_e cache_fetch_resource_from_url(
    cache_t * const cache,
    const char * const key,
    const char * const url,
    const cache_update_mode_e update_mode);

/// Deletes (i.e. removes) the resource associated with `key` from the `cache`
void cache_delete_key(cache_t * const cache, const char * const key);

#ifdef __cplusplus
}
#endif