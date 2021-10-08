/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
bundle_tests.c

bundle library test fixture
*/

#include "source/adk/bundle/bundle.h"
#include "source/adk/file/file.h"
#include "source/adk/manifest/manifest.h"
#include "source/adk/merlin/drivers/minnie/resources.h"
#include "source/adk/runtime/private/file.h"
#include "testapi.h"

static const char bundle_path[] = "tests/data/test-bundle.zip";
static const char not_present_path[] = "a/b/c/d/does_not_exist";
static const char test_dir_name[] = "__bundle_test_directory_delete_me__/";

static const char max_length_file_path[]
    = "assets/files/"
      "very-long-filename-"
      "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901"
      "2345678901234567890123456789012345678901234567890123456";

static const char * const bundle_file_names[] = {
    "assets/fonts/Avenir-Heavy.ttf",
    "assets/fonts/Avenir-Light.ttf",
    "assets/fonts/Avenir-Medium.ttf",
    "assets/fonts/Avenir-Roman.ttf",
    "assets/images/bleed-texture-mask.png",
    "assets/images/BlurredBackground.png",
    "assets/images/button-pw-visibility-hidden-focus.png",
    "assets/images/button-pw-visibility-hidden-norm.png",
    "assets/images/button-pw-visibility-shown-focus.png",
    "assets/images/button-pw-visibility-shown-norm.png",
    "assets/images/collection-ga.png",
    "assets/images/Full-bleed-hero_darkening-layer.png",
    "assets/images/icon-notification-alert-28.png",
    "assets/images/icon-notification-alert-40.png",
    "assets/images/icon-notification-alert-48.png",
    "assets/images/icon0.png",
    "assets/images/icon_adornment.png",
    "assets/images/icon_behavior.png",
    "assets/images/icon_control.png",
    "assets/images/icon_presentation.png",
    "assets/images/icon_tweendata.png",
    "assets/images/icon_validation.png",
    "assets/images/icon_validationresponder.png",
    "assets/images/originals-logo-spritesheet.png",
    "assets/images/pic1.png",
    "assets/images/save_data.png",
    "assets/images/white.png",
    "bin/.config",
    "bin/app.wasm",
};

static struct {
    sb_stat_result_t sr;
    bundle_t * bundle;
    sb_file_t * open_bfps[ARRAY_SIZE(bundle_file_names)];
} statics;

static void read_full_file(const char * const name, sb_file_t * const file) {
    statics.sr = adk_stat(sb_app_root_directory, name);
    assert_true(statics.sr.error == sb_stat_success);

    static char buf[8 * 1024];

    size_t tr = 0;
    while (true) {
        size_t const expected = min_size_t(sizeof(buf), statics.sr.stat.size - tr);
        size_t const rc = adk_fread(buf, 1, sizeof(buf), file);
        if (rc == 0) {
            adk_fread(buf, 1, sizeof(buf), file);
            assert_true(adk_feof(file));
            break;
        }
        assert_true(rc == expected);
        tr += rc;
    }

    assert_true(tr == statics.sr.stat.size);
}

static void test_stat_after_init(void ** state) {
    // Stat test directory before bundle is mounted, should succeed
    statics.sr = adk_stat(sb_app_config_directory, test_dir_name);
    assert_true(statics.sr.error == sb_stat_success);

    // Stat bundle file before mounting it, should succeed
    statics.sr = adk_stat(sb_app_root_directory, bundle_path);
    assert_true(statics.sr.error == sb_stat_success);
}

static void test_bundle_open(void ** state) {
    // Attempt to open non-existent bundle, should fail
    statics.bundle = bundle_open(sb_app_root_directory, not_present_path);
    assert_true(statics.bundle == NULL);

    // Open the bundle, should succeed
    statics.bundle = bundle_open(sb_app_root_directory, bundle_path);
    assert_true(statics.bundle != NULL);

    // Extra bundle open, should succeed
    bundle_t * bundle2 = bundle_open(sb_app_root_directory, bundle_path);
    assert_true(bundle2 != NULL);
    assert_true(bundle2 != statics.bundle);

    bundle_close(bundle2);
}

static void test_bundle_mount(void ** state) {
    // Mount the bundle, should succeed
    assert_true(adk_mount_bundle(statics.bundle));

    // Extra mount should fail
    assert_true(!adk_mount_bundle(statics.bundle));

    // Unmount should succeed
    assert_true(adk_unmount_bundle(statics.bundle));

    // Remount the bundle, should succeed
    assert_true(adk_mount_bundle(statics.bundle));
}

static void test_stat_after_mount(void ** state) {
    // Stat bundle zipfile after mounting it, should fail, no longer visible
    statics.sr = adk_stat(sb_app_root_directory, bundle_path);
    assert_true(statics.sr.error == sb_stat_error_no_entry);

    // Stat test directory after bundle is mounted, should succeed
    statics.sr = adk_stat(sb_app_config_directory, test_dir_name);
    assert_true(statics.sr.error == sb_stat_success);

    // Stat missing file, should fail
    statics.sr = adk_stat(sb_app_root_directory, not_present_path);
    assert_true(statics.sr.error == sb_stat_error_no_entry);
}

static void test_stat_bundle_files(void ** state) {
    // Stat all the files in bundle, should succeed
    for (int ix = 1; ix < ARRAY_SIZE(bundle_file_names); ++ix) {
        statics.sr = adk_stat(sb_app_root_directory, bundle_file_names[ix]);
        assert_true(statics.sr.error == sb_stat_success);
    }
}

static void test_directory_updates_after_mount(void ** state) {
    // All directory updates should fail
    assert_true(!adk_create_directory_path(sb_app_root_directory, test_dir_name));

    // Note that zipfiles can't contain empty directories, so this test would fail even if it was passed through to libzip
    assert_true(adk_delete_directory(sb_app_root_directory, "bin") == sb_directory_delete_invalid_input);

    assert_true(!adk_delete_file(sb_app_root_directory, bundle_app_wasm_path));
}

static void test_open_close_missing_file(void ** state) {
    // Open missing file should fail
    sb_file_t * file = adk_fopen(sb_app_root_directory, not_present_path, "rb");
    assert_true(file == NULL);

    // Close null file handle should fail
    assert_false(adk_fclose(file));
}

static void test_open_writable(void ** state) {
    // Opening with any writeable mode should fail
    const char * const write_modes[] = {"r+", "rb+", "w", "wb", "a", "a+", "ab+"};
    for (int ix = 0; ix < ARRAY_SIZE(write_modes); ++ix) {
        sb_file_t * file = adk_fopen(sb_app_root_directory, bundle_app_wasm_path, write_modes[ix]);
        assert_true(file == NULL);
    }
}

static void test_open_bundle_files(void ** state) {
    // Open all known files, should succeed, tests multiple open bundle files
    for (int ix = 0; ix < ARRAY_SIZE(bundle_file_names); ++ix) {
        statics.open_bfps[ix] = adk_fopen(sb_app_root_directory, bundle_file_names[ix], "rb");
        assert_true(statics.open_bfps[ix] != NULL);
    }
}

static void test_read_bundle_file(void ** state) {
    // Read all known files, should succeed
    for (int ix = 0; ix < ARRAY_SIZE(statics.open_bfps); ++ix) {
        read_full_file(bundle_file_names[ix], statics.open_bfps[ix]);
    }
}

static void test_premature_cleanup(void ** state) {
    // Early unmount should fail
    assert_true(!adk_unmount_bundle(statics.bundle));

    // Close while mounted, should fail
    assert_true(!bundle_close(statics.bundle));
}

static void test_close_bundle_files(void ** state) {
    // Close all files, should succeed
    for (int ix = 0; ix < ARRAY_SIZE(statics.open_bfps); ++ix) {
        assert_true(adk_fclose(statics.open_bfps[ix]));
    }
}

static void test_cleanup(void ** state) {
    // Unmount should succeed
    assert_true(adk_unmount_bundle(statics.bundle));

    // Extra unmount should fail
    assert_true(!adk_unmount_bundle(statics.bundle));

    assert_true(bundle_close(statics.bundle));
    statics.bundle = NULL;

    bundle_shutdown();

    // Stat bundle file after unmounting, should succeed
    statics.sr = adk_stat(sb_app_root_directory, bundle_path);
    assert_true(statics.sr.error == sb_stat_success);
}

typedef struct bundle_test_context_t {
    const char * const path;
    runtime_configuration_t expected;
} bundle_test_context_t;

static void compare_runtime_configs(runtime_configuration_t results, runtime_configuration_t expected) {
    assert_true(results.wasm_memory_size == expected.wasm_memory_size);
#ifndef _SHIP
    assert_true(results.guard_page_mode == expected.guard_page_mode);
#else
    assert_true(results.guard_page_mode == (system_guard_page_mode_e)default_guard_page_mode);
#endif
    assert_int_equal(memcmp(&expected.memory_reservations, &results.memory_reservations, sizeof(adk_memory_reservations_t)), 0);
    assert_int_equal(memcmp(&expected.bundle_fetch, &results.bundle_fetch, sizeof(fetch_retry_context_t)), 0);
}

static void test_bundle_ingest(const bundle_test_context_t ct, const runtime_configuration_t base) {
    bundle_t * bfs = bundle_open(sb_app_root_directory, ct.path);
    assert_non_null(bfs);

    // wasm is required
    sb_stat_result_t bs = bundle_stat(bfs, bundle_app_wasm_path);
    assert_true(bs.error == sb_stat_success);

    runtime_configuration_t results = base;

    // config is optional
    sb_stat_result_t cs = bundle_stat(bfs, bundle_config_path);
    assert_true(cs.error == sb_stat_success);

    bundle_file_t * config_file = bundle_fopen(bfs, bundle_config_path);
    assert_non_null(config_file);

    adk_system_metrics_t system_metrics;
    adk_get_system_metrics(&system_metrics);
    manifest_init(&system_metrics);

    mem_region_t bundle_config_blob = manifest_alloc_bundle_file_blob(cs.stat.size);
    assert_non_null(bundle_config_blob.ptr);
    assert_int_equal(bundle_fread(bundle_config_blob.ptr, sizeof(uint8_t), cs.stat.size, config_file), cs.stat.size);
    assert_true(bundle_config_parse_overwrite(CONST_MEM_REGION(.ptr = bundle_config_blob.ptr, .size = bundle_config_blob.size), &results));

    manifest_free_file_blob(bundle_config_blob);
    manifest_shutdown();

    assert_true(bundle_fclose(config_file));

    bundle_close(bfs);

    compare_runtime_configs(results, ct.expected);
}

static void test_bundle_config_parse_overwrite(void ** state) {
    const runtime_configuration_t base = {
        .wasm_memory_size = 0,
        .guard_page_mode = default_guard_page_mode,
        .memory_reservations = {
            .high = {.canvas = 34 * 1024 * 1024},
            .low = {
                .runtime = 16 * 1024,
                .rhi = 256 * 1024,
                .render_device = 128 * 1024,
                .bundle = 128 * 1024,
                .canvas_font_scratchpad = 2 * 1024 * 1024,
                .cncbus = 2 * 1024 * 1024,
                .curl = 2 * 1024 * 1024,
                .json_deflate = 10 * 1024 * 1024,
                .default_thread_pool = 256 * 1024,
                .http2 = 2 * 1024 * 1024,
                .httpx = 2 * 1024 * 1024,
            }}};

    runtime_configuration_t unchanged_except_wasm = base;
    unchanged_except_wasm.wasm_memory_size = 48 * 1024 * 1024;

    runtime_configuration_t unchanged_except_wasm_and_gp = base;
    unchanged_except_wasm_and_gp.wasm_memory_size = 48 * 1024 * 1024;
    unchanged_except_wasm_and_gp.guard_page_mode = system_guard_page_mode_enabled;

    const runtime_configuration_t all_diff_full = {
        .wasm_memory_size = 48 * 1024 * 1024,
        .guard_page_mode = system_guard_page_mode_minimal,
        .memory_reservations = {
            .high = {.canvas = 35 * 1024 * 1024},
            .low = {
                .runtime = 17 * 1024,
                .rhi = 512 * 1024,
                .render_device = 256 * 1024,
                .bundle = 256 * 1024,
                .canvas_font_scratchpad = 3 * 1024 * 1024,
                .cncbus = 3 * 1024 * 1024,
                .curl = 3 * 1024 * 1024,
                .json_deflate = 9 * 1024 * 1024,
                .default_thread_pool = 128 * 1024,
                .http2 = 1 * 1024 * 1024,
                .httpx = 3 * 1024 * 1024,
            }}};

    const runtime_configuration_t all_diff_partial = {
        .wasm_memory_size = 48 * 1024 * 1024,
        .guard_page_mode = default_guard_page_mode,
        .memory_reservations = {
            .high = {.canvas = 34 * 1024 * 1024},
            .low = {
                .runtime = 16 * 1024,
                .rhi = 256 * 1024,
                .render_device = 256 * 1024,
                .bundle = 256 * 1024,
                .canvas_font_scratchpad = 2 * 1024 * 1024,
                .cncbus = 3 * 1024 * 1024,
                .curl = 3 * 1024 * 1024,
                .json_deflate = 10 * 1024 * 1024,
                .default_thread_pool = 512 * 1024,
                .http2 = 3 * 1024 * 1024,
                .httpx = 2 * 1024 * 1024,
            }}};

    const runtime_configuration_t half_diff_full = {
        .wasm_memory_size = 48 * 1024 * 1024,
        .guard_page_mode = system_guard_page_mode_disabled,
        .memory_reservations = {
            .high = {.canvas = 34 * 1024 * 1024},
            .low = {
                .runtime = 16 * 1024,
                .rhi = 512 * 1024,
                .render_device = 128 * 1024,
                .bundle = 256 * 1024,
                .canvas_font_scratchpad = 3 * 1024 * 1024,
                .cncbus = 2 * 1024 * 1024,
                .curl = 3 * 1024 * 1024,
                .json_deflate = 11 * 1024 * 1024,
                .default_thread_pool = 256 * 1024,
                .http2 = 3 * 1024 * 1024,
                .httpx = 2 * 1024 * 1024,
            }}};

    const runtime_configuration_t half_diff_partial = {
        .wasm_memory_size = 48 * 1024 * 1024,
        .guard_page_mode = default_guard_page_mode,
        .memory_reservations = {
            .high = {.canvas = 35 * 1024 * 1024},
            .low = {
                .runtime = 17 * 1024,
                .rhi = 256 * 1024,
                .render_device = 128 * 1024,
                .bundle = 128 * 1024,
                .canvas_font_scratchpad = 2 * 1024 * 1024,
                .cncbus = 2 * 1024 * 1024,
                .curl = 2 * 1024 * 1024,
                .json_deflate = 10 * 1024 * 1024,
                .default_thread_pool = 256 * 1024,
                .http2 = 2 * 1024 * 1024,
                .httpx = 3 * 1024 * 1024,
            }}};

    const runtime_configuration_t bundle_fetch = {
        .wasm_memory_size = 48 * 1024 * 1024,
        .guard_page_mode = system_guard_page_mode_enabled,
        .bundle_fetch = {
            .retry_max_attempts = 10,
            .retry_backoff_ms = {.ms = 1000}},
        .memory_reservations = {.high = {.canvas = 34 * 1024 * 1024}, .low = {.runtime = 16 * 1024, .rhi = 256 * 1024, .render_device = 128 * 1024, .bundle = 128 * 1024, .canvas_font_scratchpad = 2 * 1024 * 1024, .cncbus = 2 * 1024 * 1024, .curl = 2 * 1024 * 1024, .json_deflate = 10 * 1024 * 1024, .default_thread_pool = 256 * 1024, .http2 = 2 * 1024 * 1024, .httpx = 2 * 1024 * 1024}}};

    const bundle_test_context_t tests[] = {
        {.path = "tests/bundles/config_tests/all-diff-full/bundle.zip", .expected = all_diff_full}, // gp: minimal
        {.path = "tests/bundles/config_tests/all-diff-partial/bundle.zip", .expected = all_diff_partial}, // gp: default
        {.path = "tests/bundles/config_tests/half-diff-full/bundle.zip", .expected = half_diff_full}, // gp: disabled
        {.path = "tests/bundles/config_tests/half-diff-partial/bundle.zip", .expected = half_diff_partial}, // gp: default
        {.path = "tests/bundles/config_tests/empty-config/bundle.zip", .expected = base}, // gp: default
        {.path = "tests/bundles/config_tests/empty-memres/bundle.zip", .expected = unchanged_except_wasm_and_gp}, // gp: enabled
        {.path = "tests/bundles/config_tests/empty-sysparams/bundle.zip", .expected = base}, // gp: default
        {.path = "tests/bundles/config_tests/no-diff-full/bundle.zip", .expected = unchanged_except_wasm_and_gp}, // gp: enabled
        {.path = "tests/bundles/config_tests/no-diff-partial/bundle.zip", .expected = unchanged_except_wasm}, // gp: default
        {.path = "tests/bundles/config_tests/bundle-fetch/bundle.zip", .expected = bundle_fetch}};

    for (int i = 0; i < ARRAY_SIZE(tests); ++i) {
        test_bundle_ingest(tests[i], base);
    }
}

static void setup(void ** s) {
    assert_true(adk_create_directory_path(sb_app_config_directory, test_dir_name));

    bundle_init();
}

static void teardown(void ** s) {
    if (statics.bundle) {
        assert_true(bundle_close(statics.bundle));
        statics.bundle = NULL;

        bundle_shutdown();
    }

    // Delete test directory after unmounting bundle, should succeed
    adk_delete_directory(sb_app_config_directory, test_dir_name);
}

int test_bundle() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(setup),
        cmocka_unit_test(test_stat_after_init),
        cmocka_unit_test(test_bundle_open),
        cmocka_unit_test(test_bundle_mount),
        cmocka_unit_test(test_stat_after_mount),
        cmocka_unit_test(test_stat_bundle_files),
        cmocka_unit_test(test_directory_updates_after_mount),
        cmocka_unit_test(test_open_close_missing_file),
        cmocka_unit_test(test_open_writable),
        cmocka_unit_test(test_open_bundle_files),
        cmocka_unit_test(test_read_bundle_file),
        cmocka_unit_test(test_premature_cleanup),
        cmocka_unit_test(test_close_bundle_files),
        cmocka_unit_test(test_bundle_config_parse_overwrite),
        cmocka_unit_test(test_cleanup),
        cmocka_unit_test(teardown)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
