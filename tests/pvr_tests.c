/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/steamboat/sb_file.h"
#include "testapi.h"

/* loads a mem_region_t with the contents of a PVR file*/
static void load_pvr_from_file(const char * const path, const_mem_region_t * const out) {
    sb_file_t * const file = sb_fopen(sb_app_root_directory, path, "rb");
    VERIFY(file);

    VERIFY(sb_fseek(file, 0, sb_seek_end));
    out->size = sb_ftell(file);

    VERIFY(out->size > 0);
    out->ptr = malloc(out->size);
    TRAP_OUT_OF_MEMORY(out->ptr);

    VERIFY(sb_fseek(file, 0, sb_seek_set));
    size_t read = sb_fread((void *)out->adr, sizeof(uint8_t), out->size, file);

    VERIFY(sb_fclose(file));

    VERIFY(read == out->size);
}

static void run_pvr_test_success(const char * const path, const image_t expected) {
    const_mem_region_t mem = {0};
    image_t image;

    load_pvr_from_file(path, &mem);

    VERIFY(imagelib_load_pvr_from_memory(mem, &image));
    VERIFY(image.height == expected.height);
    VERIFY(image.width == expected.width);
    VERIFY(image.depth == expected.depth);
    VERIFY(image.data_len == expected.data_len);
    VERIFY(image.x == expected.x);
    VERIFY(image.y == expected.y);
    VERIFY(image.pitch == expected.pitch);
    VERIFY(image.spitch == expected.spitch);

    free((void *)mem.adr);
}

static void run_pvr_test_fail(const char * const path) {
    const_mem_region_t mem = {0};
    image_t image;

    load_pvr_from_file(path, &mem);

    VERIFY(false == imagelib_load_pvr_from_memory(mem, &image));

    free((void *)mem.adr);
}

typedef struct pvr_test_context_t {
    const bool is_expecting_pass;
    const char * const file_path;
    const image_t expected_results;
} pvr_test_context_t;

static void test_pvr_load_etc1_with_alpha(void ** ignored) {
    pvr_test_context_t context = {
        .is_expecting_pass = true,
        .file_path = "tests/images/pvr_test_images/pvr-etc1-alpha.pvr",
        .expected_results = {.encoding = image_encoding_etc1, .x = 0, .y = 0, .width = 544, .height = 181, .depth = 1, .bpp = 0, .pitch = 0, .spitch = 0, .data_len = 50048}
        // NOTE: .data is not checked in the test, so it is not set
    };
    run_pvr_test_success(context.file_path, context.expected_results);
}

static void test_pvr_load_small_etc1_with_metadata(void ** ignored) {
    pvr_test_context_t context = {
        .is_expecting_pass = true,
        .file_path = "tests/images/pvr_test_images/pvrtest_0_ETC1_UNB_lRGB_110_197_1_1_1_8_15.pvr",
        .expected_results = {.encoding = image_encoding_etc1, .x = 0, .y = 0, .width = 197, .height = 110, .depth = 1, .bpp = 0, .pitch = 0, .spitch = 0, .data_len = 11200}
        // NOTE: .data is not checked in the test, so it is not set
    };
    run_pvr_test_success(context.file_path, context.expected_results);
}

static void test_pvr_load_large_etc1_with_metadata(void ** ignored) {
    pvr_test_context_t context = {
        .is_expecting_pass = true,
        .file_path = "tests/images/pvr_test_images/pvrtest_0_ETC1_UNB_lRGB_180_320_1_1_1_1_15.pvr",
        .expected_results = {.encoding = image_encoding_etc1, .x = 0, .y = 0, .width = 320, .height = 180, .depth = 1, .bpp = 0, .pitch = 0, .spitch = 0, .data_len = 28800}};
    run_pvr_test_success(context.file_path, context.expected_results);
}

static void test_pvr_load_large_etc1_no_metadata(void ** ignored) {
    pvr_test_context_t context = {
        .is_expecting_pass = true,
        .file_path = "tests/images/pvr_test_images/pvrtest_0_ETC1_UNB_lRGB_393_700_1_1_1_1_0.pvr",
        .expected_results = {.encoding = image_encoding_etc1, .x = 0, .y = 0, .width = 700, .height = 393, .depth = 1, .bpp = 0, .pitch = 0, .spitch = 0, .data_len = 138600}};
    run_pvr_test_success(context.file_path, context.expected_results);
}

static void test_pvr_load_small_etc1_no_metadata(void ** ignored) {
    pvr_test_context_t context = {
        .is_expecting_pass = true,
        .file_path = "tests/images/pvr_test_images/pvrtest_0_ETC1_UNB_lRGB_803_535_1_1_1_1_0.pvr",
        .expected_results = {.encoding = image_encoding_etc1, .x = 0, .y = 0, .width = 535, .height = 803, .depth = 1, .bpp = 0, .pitch = 0, .spitch = 0, .data_len = 215472}};
    run_pvr_test_success(context.file_path, context.expected_results);
}

static void test_pvr_unsupported_manual_sRGB(void ** ignored) {
    run_pvr_test_fail("tests/images/pvr_test_images/pvrtest_0_RGBA8888_UNB_sRGB_110_197_1_1_1_8_15.pvr");
}

static void test_pvr_unsupported_manual_lRGB(void ** ignored) {
    run_pvr_test_fail("tests/images/pvr_test_images/pvrtest_0_RGBA32323232_SI_lRGB_180_320_1_1_1_1_15.pvr");
}

static void test_pvr_unsupported_ETC2(void ** ignored) {
    run_pvr_test_fail("tests/images/pvr_test_images/pvrtest_0_ETC2_UNB_lRGB_393_700_1_1_1_1_0.pvr");
}

static void test_not_pvr(void ** ignored) {
    run_pvr_test_fail("tests/images/pvr_test_images/wrong.png");
}

int test_pvr() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_pvr_load_etc1_with_alpha, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_load_small_etc1_with_metadata, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_load_large_etc1_with_metadata, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_load_large_etc1_no_metadata, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_load_small_etc1_no_metadata, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_unsupported_manual_sRGB, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_unsupported_manual_lRGB, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_pvr_unsupported_ETC2, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_not_pvr, NULL, NULL)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}