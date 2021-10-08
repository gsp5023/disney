/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cache_tests.c

 Unit tests for the ADK cache component
*/

#include "source/adk/cache/cache.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_http_utils.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_socket.h"
#include "testapi.h"

struct {
    mem_region_t region;
    cache_t * cache;
} statics;

static int setup_group(void ** state) {
    enum { httpx_test_heap_size = 2 * 1024 * 1024 };
    statics.region = MEM_REGION(malloc(httpx_test_heap_size), httpx_test_heap_size);
    TRAP_OUT_OF_MEMORY(statics.region.ptr);

    return 0;
}

static int teardown_group(void ** state) {
    free((void *)statics.region.ptr);

    return 0;
}

static int setup(void ** state) {
    cache_t * const cache = cache_create("tests/", statics.region);
    cache_clear(cache);

    statics.cache = cache;

    return 0;
}

static int teardown(void ** state) {
    cache_destroy(statics.cache);

    return 0;
}

static const_mem_region_t create_const_mem_region_from_string(const char * str) {
    return CONST_MEM_REGION(.ptr = str, .size = strlen(str));
}

static void test_http_header_for_value(const char * header, const char * key, const char * expected_value) {
    const const_mem_region_t value = http_parse_header_for_key(key, create_const_mem_region_from_string(header));

    assert_non_null(value.ptr);

    print_message("parsed-header: %.*s\n", (int)value.size, (const char *)value.ptr);

    assert_int_equal(value.size, strlen(expected_value));
    assert_int_equal(0, memcmp(value.ptr, expected_value, value.size));
}

static void test_http_header(void ** state) {
    test_http_header_for_value("ETag: \"some-tag-with-stuff-here\"\r\n", "ETag", "\"some-tag-with-stuff-here\"");
    test_http_header_for_value("Content-Length: 123456\r\n", "Content-Length", "123456");

    // Header is case-insensitive
    test_http_header_for_value("Foo: bar\r\n", "FOO", "bar");

    // White-space after ':' is optional
    test_http_header_for_value("boo:baz\r\n", "boo", "baz");

    // Header field values can contain colons
    test_http_header_for_value("boo: baz:bop\r\n", "boo", "baz:bop");

    // Error cases (should return NULL)

    // Empty header, empty key
    assert_null(http_parse_header_for_key("", create_const_mem_region_from_string("")).ptr);

    // Non-empty header, empty key
    assert_null(http_parse_header_for_key("Foo: bar\r\n", create_const_mem_region_from_string("")).ptr);

    // Empty header, non-empty key
    assert_null(http_parse_header_for_key("", create_const_mem_region_from_string("foo")).ptr);

    // invalid header
    assert_null(http_parse_header_for_key("foo", create_const_mem_region_from_string("foo-bar")).ptr);

    // valid header, missing key
    assert_null(http_parse_header_for_key("foo:bar\r\n", create_const_mem_region_from_string("baz")).ptr);

    // Whitespace between header-key and `:` is invalid
    assert_null(http_parse_header_for_key("foo : bar", create_const_mem_region_from_string("foo")).ptr);

    // missing value
    assert_null(http_parse_header_for_key("foo:\r\n", create_const_mem_region_from_string("foo")).ptr);

    // missing value and CRLF
    assert_null(http_parse_header_for_key("foo:", create_const_mem_region_from_string("foo")).ptr);
}

static void test_cache_fetching(void ** state) {
    cache_t * const cache = statics.cache;

    static const char key[] = "logo-disney-plus";
    static const char url[] = "https://cannonball-cdn.bamgrid.com/assets/originals/logo-nopad.svg";

    // Fetch twice, second will load from cache (with initially cleared cache)
    for (size_t i = 0; i < 2; ++i) {
        sb_file_t * file = NULL;
        size_t file_content_size = 0;

        const bool is_cached = cache_get_content(cache, key, &file, &file_content_size);
        if (i == 0) {
            assert_false(is_cached);
        } else {
            assert_true(is_cached);
            assert_non_null(file);
            assert_int_equal(0x1466, file_content_size);
        }

        assert_true(cache_fetch_success == cache_fetch_resource_from_url(cache, key, url, cache_update_mode_atomic));

        sb_fclose(file);
    }
}

static void test_cache_fetch_in_place(void ** state) {
    cache_t * const cache = statics.cache;

    static const char key[] = "logo-disney-plus";
    static const char url[] = "https://cannonball-cdn.bamgrid.com/assets/originals/logo-nopad.svg";

    sb_file_t * file = NULL;
    size_t file_content_size = 0;

    assert_false(cache_get_content(cache, key, &file, &file_content_size));

    assert_true(cache_fetch_success == cache_fetch_resource_from_url(cache, key, url, cache_update_mode_in_place));

    assert_true(cache_get_content(cache, key, &file, &file_content_size));
    assert_non_null(file);
    assert_int_equal(0x1466, file_content_size);

    sb_fclose(file);
}

static void test_cache_delete_key(void ** state) {
    cache_t * const cache = statics.cache;

    static const char key[] = "logo-disney-plus";
    static const char url[] = "https://cannonball-cdn.bamgrid.com/assets/originals/logo-nopad.svg";

    sb_file_t * file = NULL;
    size_t file_content_size = 0;

    assert_false(cache_get_content(cache, key, &file, &file_content_size));
    assert_null(file);

    assert_true(cache_fetch_success == cache_fetch_resource_from_url(cache, key, url, cache_update_mode_in_place));

    assert_true(cache_get_content(cache, key, &file, &file_content_size));
    assert_non_null(file);

    // Note: `sb_delete_file` (i.e. `remove`) fails with outstanding open handles on Windows (used by `cache_delete_key`)
    sb_fclose(file);
    file = NULL;

    cache_delete_key(cache, key);

    assert_false(cache_get_content(cache, key, &file, &file_content_size));
    assert_null(file);
}

static void test_cache_corrupted_content(void ** state) {
    cache_t * const cache = statics.cache;

    static const char key[] = "logo-disney-plus";
    static const char url[] = "https://cannonball-cdn.bamgrid.com/assets/originals/logo-nopad.svg";

    sb_file_t * file = NULL;
    size_t file_content_size = 0;

    assert_true(cache_fetch_success == cache_fetch_resource_from_url(cache, key, url, cache_update_mode_in_place));
    assert_true(cache_get_content(cache, key, &file, &file_content_size));
    assert_non_null(file);
    sb_fclose(file);

    {
        // This tests reaches into the internals of the cache filesystem in order to write 'extra' data to a cached resource.
        // When the length of the cached file doesn't match the cache header, a failure will occur.

        char cache_file_path[sb_max_path_length];
        sprintf_s(cache_file_path, sb_max_path_length, "tests/f/%s", key);

        sb_file_t * const cache_file = sb_fopen(sb_app_cache_directory, cache_file_path, "wb+");

        const char extra[] = "extra";
        sb_fseek(cache_file, 0, sb_seek_end);
        sb_fwrite(extra, sizeof(char), strlen(extra), cache_file);
        sb_fclose(cache_file);
    }

    assert_false(cache_get_content(cache, key, &file, &file_content_size));
}

static void test_cache_content(void ** state) {
    cache_t * const cache = statics.cache;

    static const char key[] = "hello-cache";
    static const char content[] = "hello-cache";
    static const char url[] = "http://httpbin.org/base64/aGVsbG8tY2FjaGU=";

    sb_file_t * file = NULL;
    size_t file_content_size = 0;

    assert_true(cache_fetch_success == cache_fetch_resource_from_url(cache, key, url, cache_update_mode_in_place));
    assert_true(cache_get_content(cache, key, &file, &file_content_size));
    assert_non_null(file);
    assert_int_equal(file_content_size, strlen(content));

    uint8_t buffer[1024];
    const size_t num_bytes_read = sb_fread(buffer, sizeof(uint8_t), ARRAY_SIZE(buffer), file);
    assert_int_equal(num_bytes_read, file_content_size);
    assert_memory_equal(content, buffer, num_bytes_read);

    sb_fclose(file);
}

int test_cache() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_http_header, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cache_fetching, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cache_fetch_in_place, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cache_delete_key, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cache_corrupted_content, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cache_content, setup, teardown),
    };

    return cmocka_run_group_tests(tests, setup_group, teardown_group);
}
