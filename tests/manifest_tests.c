/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/bundle/bundle.h"
#include "source/adk/file/file.h"
#include "source/adk/manifest/manifest.h"
#include "source/adk/steamboat/sb_file.h"
#include "testapi.h"

#include <math.h>

static struct {
    adk_system_metrics_t system_metrics;

} statics;

static int setup(void ** state) {
    adk_get_system_metrics(&statics.system_metrics);
    manifest_init(&statics.system_metrics);
    return 0;
}

static int teardown(void ** state) {
    manifest_shutdown();
    return 0;
}

static void compare_runtime_configs(runtime_configuration_t results, runtime_configuration_t expected) {
    assert_true(results.wasm_low_memory_size == expected.wasm_low_memory_size);
    assert_true(results.wasm_high_memory_size == expected.wasm_high_memory_size);
    assert_true(results.wasm_heap_allocation_threshold == expected.wasm_heap_allocation_threshold);
    assert_true(results.network_pump_fragment_size == expected.network_pump_fragment_size);
    assert_true(results.network_pump_sleep_period == expected.network_pump_sleep_period);
    assert_true(results.watchdog.enabled == expected.watchdog.enabled);
    assert_true(results.watchdog.warning_delay_ms == expected.watchdog.warning_delay_ms);
    assert_true(results.watchdog.fatal_delay_ms == expected.watchdog.fatal_delay_ms);
    assert_true(results.memory_reservations.low.runtime == expected.memory_reservations.low.runtime);
    assert_true(results.memory_reservations.low.rhi == expected.memory_reservations.low.rhi);
    assert_true(results.memory_reservations.low.render_device == expected.memory_reservations.low.render_device);
    assert_true(results.memory_reservations.low.bundle == expected.memory_reservations.low.bundle);
    assert_true(results.memory_reservations.high.canvas == expected.memory_reservations.high.canvas);
    assert_true(results.memory_reservations.low.canvas_font_scratchpad == expected.memory_reservations.low.canvas_font_scratchpad);
    assert_true(results.memory_reservations.low.cncbus == expected.memory_reservations.low.cncbus);
    assert_true(results.memory_reservations.low.curl == expected.memory_reservations.low.curl);
    assert_true(results.memory_reservations.low.json_deflate == expected.memory_reservations.low.json_deflate);
    assert_true(results.memory_reservations.low.default_thread_pool == expected.memory_reservations.low.default_thread_pool);
    assert_true(results.memory_reservations.low.ssl == expected.memory_reservations.low.ssl);
    assert_true(results.memory_reservations.low.http2 == expected.memory_reservations.low.http2);
    assert_true(results.memory_reservations.low.httpx == expected.memory_reservations.low.httpx);
}

// TODO: add tests for rules, 'sample' (dice rolling)

static void test_simple_manifest(void ** state) {
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/simple-manifest.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_int_equal(manifest.resource_type, manifest_resource_url);
    assert_string_equal(manifest.resource, "http://0.0.0.0:8000/build/bundle/rust_app.bundle.zip");
    assert_string_equal(manifest.interpreter, "wasm3");
    assert_string_equal(manifest.signature, "aFsnm+NY2pNgPm+lWZWP8cxuJEA2rr2ifAbBk7neWE4=");
}

static void test_manifest_with_wasm_file_path(void ** state) {
    const uint8_t manifest_content[] =
        "{"
        "  \"v1\": {"
        "    \"options\": [{"
        "      \"bundle\": [{"
        "      \"file\": \"build/bundle/dplus_demo.bundle.zip\""
        "    }]"
        "    }]"
        "  }"
        "}";

    const manifest_t manifest = manifest_parse(
        CONST_MEM_REGION(.byte_ptr = manifest_content, .size = ARRAY_SIZE(manifest_content) - 1));

    assert_int_equal(manifest.resource_type, manifest_resource_file);
    assert_string_equal(manifest.resource, "build/bundle/dplus_demo.bundle.zip");
}

static void test_simple_manifest_with_runtime_config(void ** state) {
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/simple-local-mem-manifest.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_int_equal(manifest.resource_type, manifest_resource_file);
    assert_string_equal(manifest.resource, "build/bundle/rust_app.bundle.zip");

    // TODO: add specific assertions on memory reservation values
}

static void test_manifest_rules(void ** state) {
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/test-ncp-manifest.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_non_null(manifest.resource);
    assert_string_not_equal(manifest.resource, "");

    // TODO: add specific assertions on rules
}

static void test_manifest_rules_lists(void ** state) {
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/test-ncp-manifest-lists.json", "rb");

    {
        strcpy_s((char *)&statics.system_metrics.device_id.bytes, ARRAY_SIZE(statics.system_metrics.device_id.bytes), "e04f432955f1");
        const manifest_t manifest
            = manifest_parse_fp(manifest_file, (size_t)get_file_size(manifest_file));
        assert_string_equal(manifest.resource, "https://cdn.disney.com/app1.bundle.zip");
    }

    {
        strcpy_s((char *)&statics.system_metrics.device_id.bytes, ARRAY_SIZE(statics.system_metrics.device_id.bytes), "e04f432955f2");

        const manifest_t manifest
            = manifest_parse_fp(manifest_file, (size_t)get_file_size(manifest_file));
        assert_string_equal(manifest.resource, "https://cdn.disney.com/app2.bundle.zip");
    }

    {
        strcpy_s((char *)&statics.system_metrics.device_id.bytes, ARRAY_SIZE(statics.system_metrics.device_id.bytes), "e04f432955f0");
        const manifest_t manifest
            = manifest_parse_fp(manifest_file, (size_t)get_file_size(manifest_file));
        assert_string_equal(manifest.resource, "https://cdn.disney.com/app3.bundle.zip");
    }

    sb_fclose(manifest_file);
}

static void test_memory_reservations(void ** _) {
    const adk_memory_reservations_t expected = {
        .high = {.canvas = 123456},
        .low = {
            .runtime = 11,
            .rhi = 22,
            .render_device = 33,
            .bundle = 777,
            .canvas = 44,
            .canvas_font_scratchpad = 55,
            .cncbus = 66,
            .curl = 77,
            .curl_fragment_buffers = 7777,
            .json_deflate = 99,
            .default_thread_pool = 1010,
            .ssl = 1000,
            .http2 = 1111,
            .httpx = 1212,
            .httpx_fragment_buffers = 1414,
            .reporting = 1414,
        },
    };

    const uint32_t * const reservations = (uint32_t *)&expected;
    for (size_t i = 0; i < sizeof(adk_memory_reservations_t) / sizeof(uint32_t); ++i) {
        VERIFY_MSG(reservations[i] != 0, "Failed to fill out reservation value at offset [%i]", (int32_t)(i * sizeof(uint32_t)));
    }

    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/manifest-reservations-test.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);
    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);
    sb_fclose(manifest_file);

    assert_int_equal(memcmp(&manifest.runtime_config.memory_reservations, &expected, sizeof(adk_memory_reservations_t)), 0);
#ifndef _SHIP
    assert_int_equal(manifest.runtime_config.guard_page_mode, system_guard_page_mode_minimal);
#else
    assert_int_equal(manifest.runtime_config.guard_page_mode, system_guard_page_mode_disabled);
#endif
    assert_int_equal(manifest.runtime_config.http_max_pooled_connections, 3);

    assert_int_equal(manifest.runtime_config.http.httpx_global_certs, true);
}

static void test_guard_page_enabled_override(void ** _) {
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/manifest-guard-pages-no-reservation-overrides.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);
    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);
    sb_fclose(manifest_file);

#ifndef _SHIP
    assert_int_equal(manifest.runtime_config.guard_page_mode, system_guard_page_mode_enabled);
#else
    assert_int_equal(manifest.runtime_config.guard_page_mode, system_guard_page_mode_disabled);
#endif
}

static void test_no_overrides(void ** _) {
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/manifest-no-overrides.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);
    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);
    sb_fclose(manifest_file);

    assert_int_equal(manifest.runtime_config.guard_page_mode, default_guard_page_mode);

    assert_int_equal(manifest.runtime_config.renderer.device.num_cmd_buffers, 32);
    assert_int_equal(manifest.runtime_config.renderer.device.cmd_buf_size, 64 * 1024);
    assert_false(manifest.runtime_config.renderer.rhi_command_diffing.enabled);
    assert_false(manifest.runtime_config.renderer.rhi_command_diffing.verbose);
    assert_false(manifest.runtime_config.renderer.rhi_command_diffing.tracking.enabled);
    assert_int_equal(manifest.runtime_config.renderer.rhi_command_diffing.tracking.buffer_size, 4096);
}

static void test_manifest_config_override(void ** _) {
    runtime_configuration_t config = {0};

    runtime_configuration_t expected_config = {
        .memory_reservations = {
            .high = {.canvas = 123456},
            .low = {
                .runtime = 11,
                .rhi = 22,
                .render_device = 33,
                .bundle = 777,
                .canvas = 44,
                .canvas_font_scratchpad = 55,
                .cncbus = 66,
                .curl = 77,
                .json_deflate = 99,
                .default_thread_pool = 1010,
                .ssl = 1000,
                .http2 = 1111,
                .httpx = 1212,
                .reporting = 1414,
            },
        },
        .guard_page_mode = system_guard_page_mode_minimal,
        .http_max_pooled_connections = 3,
        .wasm_low_memory_size = 0,
        .wasm_high_memory_size = 0,
        .wasm_heap_allocation_threshold = 0,
        .network_pump_fragment_size = 0,
        .network_pump_sleep_period = 0,
        .http = {.httpx_global_certs = true}};

    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/manifest-reservations-test.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    manifest_config_parse_overwrite(&config);

    compare_runtime_configs(config, expected_config);
}

typedef struct bundle_test_context_t {
    const char * const path;
    runtime_configuration_t expected;
} bundle_test_context_t;

static void test_is_empty(void ** state) {
    manifest_t not_empty = {.signature = "dummy-signature"};
    assert_false(manifest_is_empty(&not_empty));
    manifest_t empty = {0};
    assert_true(manifest_is_empty(&empty));
}

static uint32_t mock_pick = 0;
static uint32_t mock_total = 0;

// selector that returns a global variable that
static uint32_t mock_selector(const uint32_t total) {
    assert_int_equal(total, mock_total);
    return mock_pick;
}

// Tests the non-friendly path
// Currently tests is the resource is returned as an empty string as that is
// what the function currently does on a bundle failure.
static void test_bundle_selection_failure(const char * path) {
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, path, "rb");
    assert_non_null(manifest_file);
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);
    assert_true(manifest_file_size > 0);
    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);
    sb_fclose(manifest_file);
    assert_string_equal(manifest.resource, "");
    assert_string_equal(manifest.signature, "");
}

static void test_pick_with_mock_selection(const char * const path, const char * expected_url, const uint32_t pick, const uint32_t total) {
    mock_total = total;
    mock_pick = pick;
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, path, "rb");
    assert_non_null(manifest_file);
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);
    assert_true(manifest_file_size > 0);
    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);
    sb_fclose(manifest_file);
    assert_int_equal(manifest.resource_type, manifest_resource_url);
    assert_string_equal(manifest.interpreter, "wasm3");
    assert_string_equal(manifest.signature, "aFsnm+NY2pNgPm+lWZWP8cxuJEA2rr2ifAbBk7neWE4=");
    assert_string_equal(manifest.resource, expected_url);
}

// Test the A/B selection functionality of the manifest.
static void test_bundle_selection_by_samples(void ** state) {
    //sanity_check_linear_selector();
    manifest_set_bundle_selector(&mock_selector);

    // single bundle
    const char * path = "tests/manifest/manifest-weighted-single.json";
    const char * expected_one[] = {"http://0.0.0.0:8000/build/bundle/ONE.bundle.zip"};
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_one); pick++) {
        test_pick_with_mock_selection(path, expected_one[pick], pick, ARRAY_SIZE(expected_one));
    }

    // basic two bundles, equally weighted at one
    path = "tests/manifest/manifest-weighted-simple.json";
    const char * expected_one_one[] = {"http://0.0.0.0:8000/build/bundle/ONE.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip"};
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_one_one); pick++) {
        test_pick_with_mock_selection(path, expected_one_one[pick], pick, ARRAY_SIZE(expected_one_one));
    }

    // two bundles - no sample weights - the missing samples are expected to be treated as 1s
    path = "tests/manifest/manifest-weighted-simple.json";
    // used same expected as above
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_one_one); pick++) {
        test_pick_with_mock_selection(path, expected_one_one[pick], pick, ARRAY_SIZE(expected_one_one));
    }

    // two bundles - one missing sample - the missing sample is expected to be treated as a 1
    path = "tests/manifest/manifest-weighted-simple-one-sample.json";
    const char * expected_three_one[] = {"http://0.0.0.0:8000/build/bundle/ONE.bundle.zip", "http://0.0.0.0:8000/build/bundle/ONE.bundle.zip", "http://0.0.0.0:8000/build/bundle/ONE.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip"};
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_three_one); pick++) {
        test_pick_with_mock_selection(path, expected_three_one[pick], pick, ARRAY_SIZE(expected_three_one));
    }

    // three progressively weighted ascending
    path = "tests/manifest/manifest-weighted-three-ascending.json";
    const char * expected_one_two_three[] = {"http://0.0.0.0:8000/build/bundle/ONE.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip", "http://0.0.0.0:8000/build/bundle/THREE.bundle.zip", "http://0.0.0.0:8000/build/bundle/THREE.bundle.zip", "http://0.0.0.0:8000/build/bundle/THREE.bundle.zip"};
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_one_two_three); pick++) {
        test_pick_with_mock_selection(path, expected_one_two_three[pick], pick, ARRAY_SIZE(expected_one_two_three));
    }

    // three progressively weighted descending
    path = "tests/manifest/manifest-weighted-three-descending.json";
    const char * expected_three_two_one[] = {"http://0.0.0.0:8000/build/bundle/THREE.bundle.zip", "http://0.0.0.0:8000/build/bundle/THREE.bundle.zip", "http://0.0.0.0:8000/build/bundle/THREE.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip", "http://0.0.0.0:8000/build/bundle/TWO.bundle.zip", "http://0.0.0.0:8000/build/bundle/ONE.bundle.zip"};
    for (uint32_t pick = 0; pick < ARRAY_SIZE(expected_three_two_one); pick++) {
        test_pick_with_mock_selection(path, expected_three_two_one[pick], pick, ARRAY_SIZE(expected_three_two_one));
    }

    manifest_set_bundle_selector(NULL);

    test_bundle_selection_failure("tests/manifest/manifest-empty-bundle.json");

    test_bundle_selection_failure("tests/manifest/manifest-missing-bundle.json");
}

static void test_platform_settings(void ** _) {
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/manifest-platform-settings-test.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    bool property1 = false;
    bool property2 = true;
    int property3 = 0;
    double property4 = 0.0;
    char * property5 = "";

    // Test properties which exist
    assert_true(manifest_get_platform_setting_bool("platform1", "property1", &property1));
    assert_true(property1);

    assert_true(manifest_get_platform_setting_bool("platform1", "property2", &property2));
    assert_false(property2);

    assert_true(manifest_get_platform_setting_int("platform1", "property3", &property3));
    assert_int_equal(property3, 42);

    assert_true(manifest_get_platform_setting_double("platform1", "property4", &property4));
    assert_true(fabs(42.42 - property4) < DBL_EPSILON);

    assert_true(manifest_get_platform_setting_string("platform1", "property5", &property5));
    assert_string_equal(property5, "The Answer");

    // Test properties which do not exist
    assert_false(manifest_get_platform_setting_bool("platform1", "DoesNotExist", &property1));
    assert_false(manifest_get_platform_setting_int("platform1", "DoesNotExist", &property3));
    assert_false(manifest_get_platform_setting_double("platform1", "DoesNotExist", &property4));
    assert_false(manifest_get_platform_setting_string("platform1", "DoesNotExist", &property5));

    // Ensure null out params are not written to
    assert_false(manifest_get_platform_setting_bool("platform1", "property1", NULL));
    assert_false(manifest_get_platform_setting_int("platform1", "property3", NULL));
    assert_false(manifest_get_platform_setting_double("platform1", "property4", NULL));
    assert_false(manifest_get_platform_setting_string("platform1", "property5", NULL));

    // Test non-existant properties on a platform which exists
    assert_false(manifest_get_platform_setting_bool("platform2", "property1", NULL));
    assert_false(manifest_get_platform_setting_int("platform2", "property3", NULL));
    assert_false(manifest_get_platform_setting_double("platform2", "property4", NULL));
    assert_false(manifest_get_platform_setting_string("platform2", "property5", NULL));
}

static void test_no_platform_settings(void ** _) {
    // Platform settings are not required in manifests. Just ensure the property retrieval methods
    // dont blow up when there is no platform_settings dict
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/manifest-no-platform-settings-test.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_false(manifest_get_platform_setting_bool("platform1", "property1", NULL));
    assert_false(manifest_get_platform_setting_int("platform1", "property2", NULL));
    assert_false(manifest_get_platform_setting_double("platform1", "property3", NULL));
    assert_false(manifest_get_platform_setting_string("platform1", "property4", NULL));
}

static void test_resolution_settings(void ** _) {
    // Platform settings are not required in manifests. Just ensure the property retrieval methods
    // dont blow up when there is no platform_settings dict
    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "source/adk/manifest/examples/manifest-resolution-test.json", "rb");

    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    int out_width = 0;
    int out_height = 0;

    assert_true(manifest_get_platform_setting_int("vader", "max_width", &out_width));
    assert_true(manifest_get_platform_setting_int("vader", "max_height", &out_height));
    assert_true(out_width == 3840 && out_height == 2160);

    assert_true(manifest_get_platform_setting_int("leia", "max_width", &out_width));
    assert_true(manifest_get_platform_setting_int("leia", "max_height", &out_height));
    assert_true(out_width == 1920 && out_height == 1080);

    assert_true(manifest_get_platform_setting_int("nexus", "display_width", &out_width));
    assert_true(manifest_get_platform_setting_int("nexus", "display_height", &out_height));
    assert_true(out_width == 1280 && out_height == 720);

    assert_false(manifest_get_platform_setting_int("doesntexist", "max_width", &out_width));
    assert_false(manifest_get_platform_setting_int("doesntexist", "max_height", &out_height));
}

static void test_thread_pool_size(void ** _) {
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, "tests/manifest/manifest-with-thread-pool.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_int_equal(manifest.runtime_config.thread_pool_thread_count, 2);
}

static void test_wasm_memory_config(void ** _) {
    print_message("test_wasm_memory_config\n");

    sb_file_t * const manifest_file = sb_fopen(
        sb_app_root_directory, "tests/manifest/manifest-wasm-backwards-compatibility.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_int_equal(manifest.runtime_config.wasm_low_memory_size, 0);
    assert_int_equal(manifest.runtime_config.wasm_high_memory_size, 50331648);
    assert_int_equal(manifest.runtime_config.wasm_heap_allocation_threshold, 0);
}

static void test_adk_websocket(void ** state) {
    sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/adk-websocket-override-test.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
    const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);

    assert_int_equal(manifest.runtime_config.websocket.backend, adk_websocket_backend_websocket);
    assert_int_equal(manifest.runtime_config.websocket.config.ping_timeout.ms, 1);
    assert_int_equal(manifest.runtime_config.websocket.config.no_activity_wait_period.ms, 2);
    assert_int_equal(manifest.runtime_config.websocket.config.max_handshake_timeout.ms, 3);
    assert_int_equal(manifest.runtime_config.websocket.config.max_receivable_message_size, 4);
    assert_int_equal(manifest.runtime_config.websocket.config.receive_buffer_size, 5);
    assert_int_equal(manifest.runtime_config.websocket.config.send_buffer_size, 6);
    assert_int_equal(manifest.runtime_config.websocket.config.header_buffer_size, 7);
    assert_int_equal(manifest.runtime_config.websocket.config.maximum_redirects, 8);

    sb_fclose(manifest_fp);
}

static void test_adk_websocket_null_backend(void ** _) {
    sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/adk-websocket-backend-null.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
    const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);

    assert_int_equal(manifest.runtime_config.websocket.backend, adk_websocket_backend_null);

    sb_fclose(manifest_fp);
}

static void test_canvas(void ** state) {
    sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/canvas-override-test.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
    const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);

    assert_int_equal(manifest.runtime_config.canvas.internal_limits.max_states, 1);
    assert_int_equal(manifest.runtime_config.canvas.internal_limits.max_tessellation_steps, 13);
    assert_int_equal(manifest.runtime_config.canvas.enable_punchthrough_blend_mode_fix, true);
    assert_int_equal(manifest.runtime_config.canvas.font_atlas.width, 2);
    assert_int_equal(manifest.runtime_config.canvas.font_atlas.height, 3);
    assert_true(manifest.runtime_config.canvas.text_mesh_cache.enabled);
    assert_int_equal(manifest.runtime_config.canvas.text_mesh_cache.size, 33);
    assert_int_equal(manifest.runtime_config.canvas.gl.internal_limits.max_verts_per_vertex_bank, 7001);
    assert_int_equal(manifest.runtime_config.canvas.gl.internal_limits.num_vertex_banks, 3);
    assert_int_equal(manifest.runtime_config.canvas.gl.internal_limits.num_meshes, 7);
    assert_int_equal(manifest.runtime_config.canvas.gzip_limits.working_space, 7000);

    sb_fclose(manifest_fp);
}

static void test_renderer(void ** state) {
    sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "source/adk/manifest/examples/renderer-override-test.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
    const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);
    sb_fclose(manifest_fp);

    assert_int_equal(manifest.runtime_config.renderer.device.num_cmd_buffers, 123);
    assert_int_equal(manifest.runtime_config.renderer.device.cmd_buf_size, 456);

    assert_true(manifest.runtime_config.renderer.rhi_command_diffing.enabled);
    assert_true(manifest.runtime_config.renderer.rhi_command_diffing.verbose);
    assert_true(manifest.runtime_config.renderer.rhi_command_diffing.tracking.enabled);
    assert_int_equal(manifest.runtime_config.renderer.rhi_command_diffing.tracking.buffer_size, 12345);
    assert_int_equal(manifest.runtime_config.renderer.render_resource_tracking.periodic_logging, logging_tty_and_metrics);
}

static void test_reporting(void ** state) {
    {
        sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "tests/manifest/manifest-empty-bundle.json", "rb");
        const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
        const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);

        // Defaults:
        assert_true(manifest.runtime_config.reporting.capture_logs);
        assert_true(manifest.runtime_config.reporting.minimum_event_level == event_level_error);
        assert_string_equal(manifest.runtime_config.reporting.sentry_dsn, "https://d922c6eded824f99b3aeb083fefb999e@disney.my.sentry.io/31");
        assert_int_equal(manifest.runtime_config.reporting.send_queue_size, 256);

        sb_fclose(manifest_fp);
    }

    {
        sb_file_t * const manifest_fp = sb_fopen(sb_app_root_directory, "tests/manifest/reporting-override-test.json", "rb");
        const size_t manifest_file_size = (size_t)get_file_size(manifest_fp);
        const manifest_t manifest = manifest_parse_fp(manifest_fp, manifest_file_size);

        // Overrides:
        assert_true(!manifest.runtime_config.reporting.capture_logs);
        assert_true(manifest.runtime_config.reporting.minimum_event_level == event_level_info);
        assert_string_equal(manifest.runtime_config.reporting.sentry_dsn, "foo-bar-dsn");
        assert_int_equal(manifest.runtime_config.reporting.send_queue_size, 128);

        sb_fclose(manifest_fp);
    }
}

static void test_network_pump(void ** state) {
    sb_file_t * const manifest_file = sb_fopen(sb_app_root_directory, "tests/manifest/manifest-network-pump.json", "rb");
    const size_t manifest_file_size = (size_t)get_file_size(manifest_file);

    const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_size);

    sb_fclose(manifest_file);

    assert_int_equal(manifest.runtime_config.network_pump_fragment_size, 1024);
    assert_int_equal(manifest.runtime_config.network_pump_sleep_period, 512);
}

int test_manifest() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_manifest_rules_lists),
        cmocka_unit_test(test_simple_manifest),
        cmocka_unit_test(test_manifest_with_wasm_file_path),
        cmocka_unit_test(test_simple_manifest_with_runtime_config),
        cmocka_unit_test(test_manifest_rules),
        cmocka_unit_test(test_memory_reservations),
        cmocka_unit_test(test_guard_page_enabled_override),
        cmocka_unit_test(test_no_overrides),
        cmocka_unit_test(test_manifest_config_override),
        cmocka_unit_test(test_bundle_selection_by_samples),
        cmocka_unit_test(test_is_empty),
        cmocka_unit_test(test_platform_settings),
        cmocka_unit_test(test_no_platform_settings),
        cmocka_unit_test(test_resolution_settings),
        cmocka_unit_test(test_thread_pool_size),
        cmocka_unit_test(test_wasm_memory_config),
        cmocka_unit_test(test_adk_websocket),
        cmocka_unit_test(test_adk_websocket_null_backend),
        cmocka_unit_test(test_canvas),
        cmocka_unit_test(test_renderer),
        cmocka_unit_test(test_reporting),
        cmocka_unit_test(test_network_pump)};

    return cmocka_run_group_tests(tests, setup, teardown);
}
