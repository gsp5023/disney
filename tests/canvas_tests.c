/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_tests.c

canvas test fixture
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/canvas/cg.h"
#include "source/adk/canvas/private/cg_font.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/runtime/screenshot.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_socket.h"
#include "testapi.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <errno.h>
#include <sys/stat.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

extern const adk_api_t * api;

enum {
    filename_max_len = 1024,
    image_tolerance = 2,
    screenshot_baseline_version = 24, // if the test case output has changed in any way, increment this number
};

static struct {
    bool baselines_out_of_date;
    char orig_config_dir[sb_max_path_length];
    uint32_t num_failing_tests;
    sb_enumerate_display_modes_result_t display_results;
    sb_display_mode_t display_mode;
    mem_region_t baseline_screenshot_region;
    mem_region_t testcase_screenshot_region;
} statics;

static union {
    struct {
        cg_font_file_t * dj_font;
        cg_font_file_t * tech_font;
        cg_font_file_t * font_with_missing_glyph_indicator;
        cg_font_file_t * cjk_sc_font;
        cg_font_file_t * cjk_tc_font;
        cg_font_file_t * avenir_roman_font;
    };
    cg_font_file_t * files[6];
} font_files;

static struct {
    cg_image_t * image;
    cg_image_t * image_gif;
    cg_image_t * grayscale_image;
    cg_image_t * bif_image;
    cg_image_t * badge;
    cg_image_t * alpha_mask;
    cg_image_t * jpg_image;
    cg_image_t * exif_image;
#if defined(_LEIA) || defined(_VADER)
    cg_image_t * gnf_image;
#endif
} images;

static struct {
    cg_font_context_t * font_ctx;
    cg_font_context_t * font_ctx_20px;
    cg_font_context_t * font1_ctx;
    cg_font_context_t * font2_ctx;
    cg_font_context_t * font_with_missing_glyph_ctx;
    cg_font_context_t * avenir_black;
    cg_font_context_t * cjk_sc_ctx;
    cg_font_context_t * cjk_tc_ctx;
    cg_font_context_t * avenir_roman_font_ctx;
} fonts;

// Turning off clang-format so these macro aren't mangled
// clang-format off
#ifdef _RPI
#ifdef _RPI4
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(gl_es/rpi4)
#else //RPI2
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(gl_es/rpi2)
#endif
#elif defined(NEXUS_PLATFORM)
#define TEST_SCREENSHOT_USE_CONFIG_SUBFOLDER
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(gl_es/broadcom/) STRINGIZE(NEXUS_PLATFORM)
#elif defined(_LEIA)
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(leia)
#elif defined(_VADER)
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(vader)
#else
#define TEST_SCREENSHOT_PLATFORM STRINGIZE(gl_core)
#endif
// clang-format on

#ifdef TEST_SCREENSHOT_USE_CONFIG_SUBFOLDER
#ifdef NDEBUG // release or ship configuration
#define TEST_SCREENSHOT_CONFIG STRINGIZE(release)
#else // debug
#define TEST_SCREENSHOT_CONFIG STRINGIZE(debug)
#endif
#else
#define TEST_SCREENSHOT_CONFIG
#endif

static const char * const testcase_screenshot_folder = "tests/screenshots/canvas/" TEST_SCREENSHOT_PLATFORM "/" TEST_SCREENSHOT_CONFIG "/";
static const char * const baseline_screenshot_folder = "screenshots/canvas/" TEST_SCREENSHOT_PLATFORM "/" TEST_SCREENSHOT_CONFIG "/";
static const sb_file_directory_e baseline_root_folder = sb_app_config_directory;
static const sb_file_directory_e testcase_root_folder = sb_app_cache_directory;

static const char * const baseline_name = ".baseline";
static const char * const image_file_extension = ".tga";
static const char * const baseline_version_name = "baseline_version";
static bool is_save_into_memory_region_test_done = false;

static void update_config_dir_for_tests(void) {
    ZEROMEM(statics.orig_config_dir);
    strcpy(statics.orig_config_dir, adk_get_file_directory_path(sb_app_config_directory));
    char directory_buff[sb_max_path_length];
    sprintf_s(directory_buff, ARRAY_SIZE(directory_buff), "%s/tests", adk_get_file_directory_path(sb_app_root_directory));
    adk_set_directory_path(sb_app_config_directory, directory_buff);
}

static bool create_canvas_baseline_path(const char * const filepath) {
    return sb_create_directory_path(baseline_root_folder, filepath);
}

static bool create_canvas_testcase_path(const char * const filepath) {
    return sb_create_directory_path(testcase_root_folder, filepath);
}

static void save_baseline(const char * const filename, const mem_region_t screenshot_region, const sb_display_mode_t display_mode_used) {
    char filename_buff[filename_max_len];
    sprintf_s(filename_buff, filename_max_len, "%s/%s%s_%dx%d%s%s", adk_get_file_directory_path(baseline_root_folder), baseline_screenshot_folder, filename, display_mode_used.width, display_mode_used.height, baseline_name, image_file_extension);
    image_t screenshot;

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_screenshot,
        &screenshot,
        screenshot_region,
        MALLOC_TAG);

    render_flush_cmd_stream(&the_app.render_device->default_cmd_stream, render_wait);

    stbi_write_tga(filename_buff, screenshot.width, screenshot.height, screenshot.bpp, screenshot.data);
}

static bool check_if_baseline_exists(const char * const filename, sb_display_mode_t display_mode_used) {
    char filename_buff[filename_max_len];
    sprintf_s(filename_buff, filename_max_len, "%s%s_%dx%d%s%s", baseline_screenshot_folder, filename, display_mode_used.width, display_mode_used.height, baseline_name, image_file_extension);

    sb_file_t * const fp = sb_fopen(baseline_root_folder, filename_buff, "rb");
    if (fp) {
        sb_fclose(fp);
        return true;
    }
    return false;
}

static void compare_screenshot_file_with_mem(image_t * const testcase_screenshot, const char * const filename, const sb_display_mode_t display_mode_used) {
    // Perform one-off test: Save screenshot into memory region and compare compression size with baseline file size
    char baseline_buff[filename_max_len];

    sprintf_s(baseline_buff, filename_max_len, "%s%s_%dx%d%s%s", baseline_screenshot_folder, filename, display_mode_used.width, display_mode_used.height, baseline_name, image_file_extension);
    sb_file_t * const baseline_file = sb_fopen(baseline_root_folder, baseline_buff, "rb");

    VERIFY_MSG(baseline_file, "failed to open baseline %s", baseline_buff);
    sb_fseek(baseline_file, 0, sb_seek_end);
    const size_t baseline_file_size = sb_ftell(baseline_file);
    sb_fclose(baseline_file);

    mem_region_t screenshot_buffer = MEM_REGION(
            .ptr = malloc(baseline_file_size),
            .size = baseline_file_size);
    TRAP_OUT_OF_MEMORY(screenshot_buffer.ptr);

    // Save screenshot into mem region
    const size_t screenshot_compress_size = adk_write_screenshot_mem_user_by_type(
        testcase_screenshot,
        screenshot_buffer,
        image_save_tga);

    free(screenshot_buffer.ptr);
    assert_int_equal(baseline_file_size, screenshot_compress_size);

    is_save_into_memory_region_test_done = true;
}

static void compare_test_binary_files(image_t * const testcase_screenshot, const char * const filename, const sb_display_mode_t display_mode_used, const mem_region_t baseline_pixel_region) {
    char baseline_buff[filename_max_len];

    sprintf_s(baseline_buff, filename_max_len, "%s%s_%dx%d%s%s", baseline_screenshot_folder, filename, display_mode_used.width, display_mode_used.height, baseline_name, image_file_extension);
    sb_file_t * const baseline_file = sb_fopen(baseline_root_folder, baseline_buff, "rb");

    VERIFY_MSG(baseline_file, "failed to open base_line");
    sb_fseek(baseline_file, 0, sb_seek_end);

    const long baseline_file_size = sb_ftell(baseline_file);
    sb_fseek(baseline_file, 0, sb_seek_set);

    mem_region_t baseline_file_bytes = MEM_REGION(.ptr = malloc(baseline_file_size), .size = baseline_file_size);
    VERIFY(sb_fread(baseline_file_bytes.ptr, 1, baseline_file_bytes.size, baseline_file) == (size_t)baseline_file_bytes.size);
    sb_fclose(baseline_file);

    image_t baseline_screenshot;
    adk_load_screenshot(&baseline_screenshot, baseline_file_bytes.consted, baseline_pixel_region, MEM_REGION(.ptr = 0, .size = 0));

    adk_screenshot_t testcase = {.image = *testcase_screenshot}, baseline = {.image = baseline_screenshot};

    // set log level to error so screenshot compare doesn't spam output
    log_set_min_level(log_level_error);
    bool is_screenshot_match = adk_screenshot_compare(&testcase, &baseline, image_tolerance);
    log_set_min_level(log_level_warn);

    if (!is_screenshot_match) {
        char failing_test_prefix[filename_max_len];
        sprintf_s(failing_test_prefix, ARRAY_SIZE(failing_test_prefix), "%s%s_%dx%d", testcase_screenshot_folder, filename, display_mode_used.width, display_mode_used.height);
        char testcase_filename[filename_max_len];
        sprintf_s(testcase_filename, ARRAY_SIZE(testcase_filename), "%s.tga", failing_test_prefix);
        adk_save_screenshot(&testcase.image, image_save_tga, testcase_root_folder, testcase_filename);
        adk_screenshot_dump_deltas(&testcase, &baseline, image_tolerance, image_save_tga, testcase_root_folder, failing_test_prefix);

        print_message("Test failed: %s\n", testcase_filename);
        ++statics.num_failing_tests;
    }

    free(baseline_file_bytes.ptr);
}

static void init_display_for_tests(const char * const app_name) {
    int display_index = 0;
    sb_enumerate_display_modes_result_t display_result;
    for (; sb_enumerate_display_modes(display_index, 0, &display_result); ++display_index) {
        if (display_result.status & sb_display_modes_primary_display) {
            break;
        }
    }

    int display_mode_index = 0;
    int preferred_mode_index = -1;
    int fallback_mode_index = -1;

    for (; sb_enumerate_display_modes(display_index, display_mode_index, &display_result); ++display_mode_index) {
        // some of our laptops are still on 1080p screens, so having this set to 1080 makes it difficult to debug the apps as they may be full screen
        if (display_result.display_mode.height == 720) {
            preferred_mode_index = display_mode_index;
            break;
        } else {
            fallback_mode_index = display_mode_index;
        }
    }

    ASSERT((fallback_mode_index != -1) || (preferred_mode_index != -1));

    app_init_main_display(display_index, preferred_mode_index == -1 ? fallback_mode_index : preferred_mode_index, app_name);
}

static void cg_draw_image_test(cg_image_t * const image, const int mouse_x, const int mouse_y, const int window_width) {
    float mx = (float)mouse_x, my = (float)mouse_y;

    cg_context_scale((cg_vec2_t){.x = 2, .y = 2});

    cg_context_draw_image(image, (cg_vec2_t){0});

    cg_context_set_feather(40);
    cg_context_begin_path(MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    cg_context_rounded_rect((cg_rect_t){.x = mx, .y = my, .width = 400, .height = 300}, 30, MALLOC_TAG);
    cg_context_fill_style_image((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255}, image);
    cg_context_fill(MALLOC_TAG);
    cg_context_set_feather(1);

    cg_context_identity();
}

static void cg_draw_image_alpha_mask_test(cg_image_t * const image, cg_image_t * const mask) {
    cg_context_identity();
    const cg_rect_t rect = {.width = (float)image->image.width, .height = (float)image->image.height};
    cg_context_draw_image_rect_alpha_mask(image, mask, rect, rect);
}

static void cg_draw_image_9slice_test(cg_image_t * const image, const int mouse_x, const int mouse_y, const int window_width) {
    cg_context_identity();
    cg_context_draw_image_9slice(image, (cg_margins_t){.left = 80, .right = 60, .top = 40, .bottom = 20}, (cg_rect_t){10, 10, 400, 400});
}

static void cg_draw_arcs_test() {
    cg_context_set_clip_rect((cg_rect_t){.x = 0, .y = 150, .width = 125, .height = 130});
    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    for (int i = 0; i <= 3; i++) {
        for (int j = 0; j <= 2; j++) {
            cg_context_begin_path(MALLOC_TAG);
            const float x = 25.f + j * 50.f;
            const float y = 25.f + i * 50.f;
            const float radius = 20.f;
            const cg_rads_t start = {.rads = 0};
            const cg_rads_t end = {.rads = CG_PI + (CG_PI * j) / 2};
            const cg_rotation_e ccw = i % 2 == 1 ? cg_rotation_counter_clock_wise : cg_rotation_clock_wise;

            cg_context_arc((cg_vec2_t){.x = x, .y = y + 150}, radius, start, end, ccw, MALLOC_TAG); // outer circle

            if (i > 1) {
                cg_context_fill(MALLOC_TAG);
            } else {
                cg_context_stroke(MALLOC_TAG);
            }
        }
    }
}

static void cg_draw_clipped_arcs_test() {
    cg_context_set_clip_state(cg_clip_state_enabled);
    cg_context_set_clip_rect((cg_rect_t){.x = 0, .y = 150, .width = 125, .height = 130});
    cg_context_set_clip_rect((cg_rect_t){.x = 50, .y = 0, .width = 200, .height = 300});
    cg_context_save();
    cg_context_set_clip_rect((cg_rect_t){.x = 0, .y = 0, .width = 10, .height = 10});
    cg_context_set_clip_state(cg_clip_state_disabled);
    cg_context_restore();
    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    for (int i = 0; i <= 3; i++) {
        for (int j = 0; j <= 2; j++) {
            cg_context_begin_path(MALLOC_TAG);
            const float x = 25.f + j * 50.f;
            const float y = 25.f + i * 50.f;
            const float radius = 20.f;
            const cg_rads_t start = {.rads = 0};
            const cg_rads_t end = {.rads = CG_PI + (CG_PI * j) / 2};
            const cg_rotation_e ccw = i % 2 == 1 ? cg_rotation_counter_clock_wise : cg_rotation_clock_wise;

            cg_context_arc((cg_vec2_t){.x = x, .y = y + 150}, radius, start, end, ccw, MALLOC_TAG); // outer circle

            if (i > 1) {
                cg_context_fill(MALLOC_TAG);
            } else {
                cg_context_stroke(MALLOC_TAG);
            }
        }
    }
    cg_context_set_clip_state(cg_clip_state_disabled);
}

static void cg_draw_bubble_test(cg_image_t * const image) {
    cg_context_begin_path(MALLOC_TAG);
    cg_context_move_to((cg_vec2_t){.x = 75, .y = 25}, MALLOC_TAG);
    cg_context_quad_bezier_to(25, 25, 25, 62.5, MALLOC_TAG);
    cg_context_quad_bezier_to(25, 100, 50, 100, MALLOC_TAG);
    cg_context_quad_bezier_to(50, 120, 30, 125, MALLOC_TAG);
    cg_context_quad_bezier_to(60, 120, 65, 100, MALLOC_TAG);
    cg_context_quad_bezier_to(125, 100, 125, 62.5, MALLOC_TAG);
    cg_context_quad_bezier_to(125, 25, 75, 25, MALLOC_TAG);
    cg_context_fill_style_image((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255}, image);
    cg_context_fill_with_options(cg_path_options_concave, MALLOC_TAG);
    cg_context_set_line_width(4);
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 0, .b = 0, .a = 255});
    cg_context_stroke(MALLOC_TAG);
}

static void cg_draw_rounded_rect_test(cg_image_t * const image_gif) {
    cg_context_set_line_width(2.0);
    cg_context_begin_path(MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    cg_context_fill_style_image_hex(0xFFF, image_gif);
    cg_context_rounded_rect((cg_rect_t){.x = 300, .y = 25, .width = 300, .height = 100}, 20, MALLOC_TAG);
    cg_context_fill(MALLOC_TAG);
    cg_context_stroke(MALLOC_TAG);
}

static void cg_draw_face_test() {
    cg_context_set_line_width(2.0);
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_translate((cg_vec2_t){.x = 150, .y = 0});
    cg_context_begin_path(MALLOC_TAG);
    cg_context_arc((cg_vec2_t){.x = 75, .y = 75}, 50, (cg_rads_t){.rads = 0}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_counter_clock_wise, MALLOC_TAG); // face
    cg_context_move_to((cg_vec2_t){.x = 110, .y = 75}, MALLOC_TAG);
    cg_context_arc((cg_vec2_t){.x = 75, .y = 75}, 35, (cg_rads_t){.rads = 0}, (cg_rads_t){.rads = CG_PI}, cg_rotation_clock_wise, MALLOC_TAG); // mouth clockwise
    cg_context_move_to((cg_vec2_t){.x = 65, .y = 65}, MALLOC_TAG);
    cg_context_arc((cg_vec2_t){.x = 60, .y = 65}, 5, (cg_rads_t){.rads = 0}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_counter_clock_wise, MALLOC_TAG); // left eye
    cg_context_move_to((cg_vec2_t){.x = 95, .y = 65}, MALLOC_TAG);
    cg_context_arc((cg_vec2_t){.x = 90, .y = 65}, 5, (cg_rads_t){.rads = 0}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_counter_clock_wise, MALLOC_TAG); // right eye
    cg_context_stroke(MALLOC_TAG);
}

static void cg_clear_rect_test() {
    cg_context_translate((cg_vec2_t){.x = 0, .y = 120});
    cg_context_fill_style_hex(0x09F);
    cg_context_fill_rect((cg_rect_t){.x = 25, .y = 25, .width = 100, .height = 100}, MALLOC_TAG);
    cg_context_clear_rect((cg_rect_t){.x = 45, .y = 45, .width = 60, .height = 60}, MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 255, .b = 0, .a = 255});
    cg_context_stroke_rect((cg_rect_t){.x = 50, .y = 50, .width = 50, .height = 50}, MALLOC_TAG);
}

static void cg_tris_test() {
    cg_context_fill_style((cg_color_t){.r = 128, .g = 0, .b = 255, .a = 255});
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 0, .b = 255, .a = 255});
    cg_context_translate((cg_vec2_t){.x = 120, .y = 0});
    cg_context_begin_path(MALLOC_TAG);
    cg_context_move_to((cg_vec2_t){.x = 25, .y = 25}, MALLOC_TAG);
    cg_context_line_to((cg_vec2_t){.x = 105, .y = 25}, MALLOC_TAG);
    cg_context_line_to((cg_vec2_t){.x = 25, .y = 105}, MALLOC_TAG);
    cg_context_fill(MALLOC_TAG);
    cg_context_begin_path(MALLOC_TAG);
    cg_context_move_to((cg_vec2_t){.x = 125, .y = 125}, MALLOC_TAG);
    cg_context_line_to((cg_vec2_t){.x = 125, .y = 45}, MALLOC_TAG);
    cg_context_line_to((cg_vec2_t){.x = 45, .y = 125}, MALLOC_TAG);
    cg_context_close_path(MALLOC_TAG);
    cg_context_stroke(MALLOC_TAG);
}

static void cg_global_alpha_test() {
    cg_context_translate((cg_vec2_t){.x = 150, .y = 50});
    cg_context_scale((cg_vec2_t){.x = 1.2f, .y = 1.2f});
    cg_context_fill_style_hex(0xFD0);
    cg_context_fill_rect((cg_rect_t){.x = 0, .y = 0, .width = 75, .height = 75}, MALLOC_TAG);
    cg_context_fill_style_hex(0x6C0);
    cg_context_fill_rect((cg_rect_t){.x = 75, .y = 0, .width = 75, .height = 75}, MALLOC_TAG);
    cg_context_fill_style_hex(0x09F);
    cg_context_fill_rect((cg_rect_t){.x = 0, .y = 75, .width = 75, .height = 75}, MALLOC_TAG);
    cg_context_fill_style_hex(0xF30);
    cg_context_fill_rect((cg_rect_t){.x = 75, .y = 75, .width = 75, .height = 75}, MALLOC_TAG);

    cg_context_set_global_alpha(0.2f);
    cg_context_fill_style_hex(0xFFF);
}

static void cg_random_arcs_test() {
    for (int i = 0; i < 7; i++) {
        cg_context_begin_path(MALLOC_TAG);
        cg_context_arc((cg_vec2_t){.x = 75.f, .y = 75.f}, 10.f + 10.f * i, (cg_rads_t){.rads = 0}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_counter_clock_wise, MALLOC_TAG);
        cg_context_fill(MALLOC_TAG);
    }
}

static void cg_text_test(cg_font_context_t * font_ctx) {
    cg_context_save();
    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 890.0f, .y = 150.0f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 0.0f, .b = 128.0f, .a = 255.0f});
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "Dreamcast", cg_font_fill_options_align_center);
    cg_context_restore();
}

static void cg_combined_test(cg_font_context_t * font_ctx, cg_image_t * const image, cg_image_t * const image_gif, const int mouse_x, const int mouse_y, const int window_width) {
    cg_draw_image_test(image, mouse_x, mouse_y, window_width);
    cg_draw_image_9slice_test(image, mouse_x, mouse_y + 100, window_width);
    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 2, .y = 2});
    cg_draw_arcs_test();
    cg_draw_bubble_test(image);
    cg_draw_rounded_rect_test(image_gif);
    cg_draw_face_test();
    cg_clear_rect_test();
    cg_tris_test();
    cg_global_alpha_test();
    cg_random_arcs_test();
    cg_text_test(font_ctx);
}

static void cg_combined_with_clip_test(cg_font_context_t * font_ctx, cg_image_t * const image, cg_image_t * const image_gif, const int mouse_x, const int mouse_y, const int window_width) {
    cg_draw_image_test(image, mouse_x, mouse_y, window_width);
    cg_draw_image_9slice_test(image, mouse_x, mouse_y + 100, window_width);
    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 2, .y = 2});
    cg_draw_clipped_arcs_test();
    cg_draw_bubble_test(image);
    cg_draw_rounded_rect_test(image_gif);
    cg_draw_face_test();
    cg_clear_rect_test();
    cg_tris_test();
    cg_global_alpha_test();
    cg_random_arcs_test();
    cg_text_test(font_ctx);
}

static void draw_text_block_background(cg_rect_t text_rect) {
    cg_context_fill_style_hex(0x77777777);
    cg_context_fill_rect(text_rect, MALLOC_TAG);
    cg_context_fill_style_hex(0xffffffff);
}

static void cg_text_block_terrible_input_aligned_test(cg_font_context_t * font_ctx, cg_text_block_options_e alignment) {
    const cg_rect_t text_rect = {.x = 0.0f, .y = 0.0f, .width = 300, .height = 100};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0.f, 0, "a\nwordper\n line kinda", NULL, alignment | cg_text_block_allow_block_bounds_overflow);
}

static void cg_text_block_normal_input_aligned_test(cg_font_context_t * font_ctx, cg_text_block_options_e alignment) {
    const cg_rect_t text_rect = {.x = 200.0f, .y = 200.0f, .width = 300, .height = 1000};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0.f, 0, "a\nword per\nline kinda", NULL, alignment);
}

static void cg_text_block_offset_test(cg_font_context_t * font_ctx, cg_text_block_options_e discard_options) {
    const cg_rect_t text_rect = {.x = 100.f, .y = 100.f, .width = 300, .height = 330};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, -60.f, 0, "a\nword per\nline kinda", NULL, cg_text_block_align_line_right | discard_options);
}

static void cg_text_block_space_edge_cases(cg_font_context_t * font_ctx, cg_text_block_options_e alignment_options) {
    const cg_rect_t text_rect = {.x = 0, .y = 0, .width = 100, .height = 400};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, "This is a full long sentence", NULL, alignment_options);
}

static void cg_text_alignment_draw_background() {
    cg_context_begin_path(MALLOC_TAG);
    cg_context_rounded_rect((cg_rect_t){.x = 0.0, .y = 0.0, .width = 256.0, .height = 128.0}, 8.0, MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 128, .b = 0, .a = 255});
    cg_context_fill(MALLOC_TAG);

    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
}

static void cg_text_alignment_test(cg_font_context_t * const font_ctx) {
    cg_context_translate((cg_vec2_t){.x = 10.0, .y = 80.0});
    cg_text_alignment_draw_background();

    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 256.0 / 2, .y = 128.0 / 2}, "Centered", cg_font_fill_options_align_center);

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 180.0});
    cg_text_alignment_draw_background();
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0, .y = 128.0 / 2}, "Left", cg_font_fill_options_align_left | cg_font_fill_options_align_top);

    cg_context_save();
    cg_context_translate((cg_vec2_t){.x = 300.0, .y = 0});
    cg_text_alignment_draw_background();
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 256.0 / 2, .y = 128.0 / 2}, "Top", cg_font_fill_options_align_top);
    cg_context_restore();

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 180.0});
    cg_text_alignment_draw_background();
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 256.0, .y = 128.0 / 2}, "Right", cg_font_fill_options_align_right | cg_font_fill_options_align_bottom);

    cg_context_save();
    cg_context_translate((cg_vec2_t){.x = 300.0, .y = 0});
    cg_text_alignment_draw_background();
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 256.0 / 2, .y = 128.0 / 2}, "Bottom", cg_font_fill_options_align_bottom);
    cg_context_restore();
}

const char * const long_text_test_str = "a long thing of various amounts o words until eventually ellipses gets triggered so I can consider what its supposed to look like sanely without some small example ok";

static void cg_text_ellipses_test(cg_font_context_t * const font_ctx) {
    const cg_rect_t text_rect = {.x = 200, .y = 200, .width = 700, .height = 240};

    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, long_text_test_str, "...", cg_text_block_align_line_left);
}

static void cg_text_ellipses_newline_test(cg_font_context_t * const font_ctx) {
    const cg_rect_t text_rect = {.x = 200, .y = 200, .width = 700, .height = 240};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, long_text_test_str, "...", cg_text_block_align_line_right);
}

static void cg_text_spacing_test_absolute(cg_font_context_t * font_ctx) {
    const cg_rect_t text_rect = {.x = 200, .y = 200, .width = 700, .height = 240};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 20, long_text_test_str, NULL, cg_text_block_align_line_right);
}

static void cg_text_spacing_test_relative(cg_font_context_t * font_ctx) {
    const cg_rect_t text_rect = {.x = 0, .y = 0, .width = 700, .height = 640};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 1.4f, long_text_test_str, NULL, cg_text_block_align_line_left | cg_text_block_line_space_relative);
}

static void cg_text_block_pathological(cg_font_context_t * font_ctx, cg_rect_t text_rect, const char * const text, const char * const ellipses, cg_text_block_options_e options) {
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, text, ellipses, options);
}

static void cg_text_block_with_missing_glyphs(cg_font_context_t * const font_ctx) {
    const cg_rect_t text_rect = {.x = 0, .y = 0, .width = 700, .height = 640};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, "This is a full sentence one.\nThis is a full sentence two.\n\nThis is a full sentence three.\tThis is a full sentence four.\n\tThis is a full sentence five.", "...", cg_text_block_align_line_left);
}

static void cg_text_massive_amount_of_text(cg_font_context_t * const font_ctx_20px) {
    char large_str_buff[40000];
    size_t i = 0;
    for (; i < ARRAY_SIZE(large_str_buff); ++i) {
        large_str_buff[i] = '.';
    }
    large_str_buff[i - 1] = 0;
    cg_context_fill_text_block_with_options(font_ctx_20px, (cg_rect_t){.x = 0, .y = 0, .width = 1280, .height = 720}, 0, -10, large_str_buff, NULL, cg_text_block_align_line_left);
}

static void cg_simple_grayscale_image(cg_image_t * const grayscale_image) {
    cg_context_draw_image_scale(grayscale_image, (cg_rect_t){.x = 0, .y = 0, .width = 60, .height = 60});
}

static void cg_bif_index(cg_image_t * const bif_image) {
    const uint32_t bif_image_count = cg_context_get_image_frame_count(bif_image);
    assert_true(bif_image_count > 1);
    cg_context_set_image_frame_index(bif_image, 2);
    thread_pool_drain(&the_app.default_thread_pool);
    cg_context_draw_image(bif_image, (cg_vec2_t){.x = 0, .y = 0});
}

static void cg_jpg_test(cg_image_t * const jpg_image) {
    cg_context_draw_image(jpg_image, (cg_vec2_t){.x = 0, .y = 0});
}

static void cg_gnf_test(cg_image_t * const gnf_image) {
    cg_context_draw_image(gnf_image, (cg_vec2_t){.x = 0, .y = 0});
}

static void cg_cjk_test(cg_font_context_t * const font_ctx, const char * const text) {
    const cg_rect_t text_rect = {.x = 0, .y = 0, .width = 1280, .height = 720};
    draw_text_block_background(text_rect);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, text, NULL, cg_text_block_align_line_left);
}

static void cg_video_rect_test(cg_image_t * const image, cg_font_context_t * font_ctx) {
    cg_context_identity();

    const cg_rect_t video_rect = {.x = 0, .y = 0, .width = 500, .height = 720};
    cg_context_blit_video_frame(video_rect);

    cg_context_set_alpha_test_threshold(0.75f);

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_blit);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 400, .y = 0.0});
    cg_context_scale((cg_vec2_t){.x = 0.05f, .y = 3.0f});
    cg_context_draw_image(image, (cg_vec2_t){0});

    // Drawing images over video rect with various blends
    cg_context_set_punchthrough_blend_mode(cg_blend_mode_blit);

    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 0.5, .y = 0.25});
    cg_context_draw_image(image, (cg_vec2_t){0});

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_src_alpha_all);

    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 0.5, .y = 0.25});
    cg_context_draw_image(image, (cg_vec2_t){.x = 0, .y = 500});

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_src_alpha_rgb);

    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 0.5, .y = 0.25});
    cg_context_draw_image(image, (cg_vec2_t){.x = 0, .y = 1000});

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_alpha_test);

    cg_context_identity();
    cg_context_scale((cg_vec2_t){.x = 0.5, .y = 0.25});
    cg_context_draw_image(image, (cg_vec2_t){.x = 0, .y = 1500});

    // Drawing fonts over video rect with various blends
    cg_context_set_punchthrough_blend_mode(cg_blend_mode_blit);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 500.0f, .y = 500.0f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 255.0f, .b = 255.0f, .a = 255.0f});
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "LoremIpsum", cg_font_fill_options_align_center);

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_src_alpha_all);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 500.0f, .y = 550.0f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 255.0f, .b = 255.0f, .a = 255.0f});
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "LoremIpsum", cg_font_fill_options_align_center);

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_src_alpha_rgb);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 500.0f, .y = 600.0f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 255.0f, .b = 255.0f, .a = 255.0f});
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "LoremIpsum", cg_font_fill_options_align_center);

    cg_context_set_punchthrough_blend_mode(cg_blend_mode_alpha_test);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 500.0f, .y = 650.0f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 255.0f, .b = 255.0f, .a = 255.0f});
    cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "LoremIpsum", cg_font_fill_options_align_center);
}

static void cg_ellipses_vert_reservation_edge_case(cg_font_context_t * const avenir_roman_font_ctx) {
    // this tests an edge case where using ellipses increases the total number of glyphs we'd draw (compared to the length of the provided string).
    cg_context_identity();
    cg_context_fill_text_block_with_options(avenir_roman_font_ctx, (cg_rect_t){.width = 225, .height = 34}, 0, 0, "qwertyuioplkjhgfdsa", "...", cg_text_block_align_line_left);
}

static void cg_draw_missing_glyph_indicator(cg_font_context_t * const font_ctx, const cg_rect_t text_rect, const char * const text, const char * const ellipses, const char * const missing_glyph_indicator, cg_text_block_options_e options) {
    draw_text_block_background(text_rect);
    cg_context_set_global_missing_glyph_indicator(missing_glyph_indicator);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, text, ellipses, options);
    cg_context_set_global_missing_glyph_indicator(NULL);
}

static void cg_draw_missing_glyph_indicator_for_font_ctx(cg_font_context_t * const font_ctx, const cg_rect_t text_rect, const char * const text, const char * const missing_glyph_indicator) {
    draw_text_block_background(text_rect);
    cg_context_set_font_context_missing_glyph_indicator(font_ctx, missing_glyph_indicator);
    cg_context_fill_text_block_with_options(font_ctx, text_rect, 0, 0, text, NULL, cg_text_block_align_line_left);
    cg_context_set_font_context_missing_glyph_indicator(font_ctx, NULL);
}

static void cg_draw_sdf_demo() {
    cg_context_save();

    cg_context_set_global_alpha(1.0f);
    cg_context_translate((cg_vec2_t){.x = 175, .y = 25});
    cg_context_fill_style((cg_color_t){.r = 125.f, .g = 125.f, .b = 30.f, .a = 255.f});
    cg_context_sdf_fill_rect_rounded((cg_rect_t){.x = -5, .y = -5, .width = 110, .height = 110}, (cg_sdf_rect_params_t){.roundness = 1000});
    cg_context_fill_style((cg_color_t){.r = 125.f, .g = 30.f, .b = 30.f, .a = 255.f});
    cg_context_scale((cg_vec2_t){1 / 2.f, 1 / 2.f});
    cg_context_sdf_fill_rect_rounded((cg_rect_t){.x = 25, .y = 50, .width = 150, .height = 100}, (cg_sdf_rect_params_t){.roundness = 1000});

    cg_context_restore();
}

static void cg_context_text_measure_whitespace_test(cg_font_context_t * const font_ctx) {
    const float close_float_epsilon = 0.001f;
    const float a_width = cg_context_text_measure(font_ctx, "a").bounds.width;
    assert_true(a_width > 0);
    const float space_width = cg_context_text_measure(font_ctx, " ").bounds.width;
    assert_true(space_width > 0);

    assert_true(fabs(cg_context_text_measure(font_ctx, "").bounds.width) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, " ").bounds.width - space_width) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "   ").bounds.width - space_width * 3) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "a").bounds.width - a_width) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "aaa").bounds.width - a_width * 3) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "aa  ").bounds.width - (a_width * 2 + space_width * 2)) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "  aa").bounds.width - (a_width * 2 + space_width * 2)) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "aa    ").bounds.width - (a_width * 2 + space_width * 4)) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "   a   a ").bounds.width - (a_width * 2 + space_width * 7)) < close_float_epsilon);

    // multiline test
    assert_true(fabs(cg_context_text_measure(font_ctx, "aaaa\na\naa\naaaaa\na").bounds.width - (a_width * 5)) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "aaaa\na\naa\n        \na").bounds.width - (space_width * 8)) < close_float_epsilon);
    assert_true(fabs(cg_context_text_measure(font_ctx, "aaaa\na\naa\n\n  aaaa  ").bounds.width - (a_width * 4 + space_width * 4)) < close_float_epsilon);
}

static void cg_image_load_cancel_test(void ** ignored) {
    // verify we don't leak canceled images
    cg_image_t * const some_image = cg_context_load_image_async("https://lumiere-a.akamaihd.net/v1/images/hb_huludisneyplusespnbundle_logo_dpluscentered_19230_b33a7b2d.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    cg_context_image_free(some_image, MALLOC_TAG);
}

static void cg_malformed_url_test(void ** ignored) {
    cg_image_t * const image_URL_missing_arg = cg_context_load_image_async("https://prod-ripcut-delivery.disney-plus.net/v1/variant/disney/D3D02B19B1EBF1F15183029CB5C6520B8C919B86483959F5F3733C29B433E134/scale?width=1282&partner=disney&format=pvr&texture=etc1&textureQuality=NumETCModes&roundCornerRadius=4", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        if (cg_get_image_load_status(image_URL_missing_arg) != cg_image_async_load_pending) {
            break;
        }
        sb_thread_sleep((milliseconds_t){1});
    }

    /* Test ripcut error for malformed URL
     * The value of the "X-BAMTECH-ERROR" HTTP header (defined at
     * https://wiki.disneystreaming.com/display/SSDE/Ripcut+Custom+Error+Messages)
     * is expected to be 1000 (i.e., "Default image returned") for an URL
     * missing a required argument */
    VERIFY(image_URL_missing_arg->status == cg_image_async_load_ripcut_error);
    VERIFY_MSG(image_URL_missing_arg->ripcut_error_code == 1000, "error code: %i", image_URL_missing_arg->ripcut_error_code);
    VERIFY(image_URL_missing_arg->image.data == NULL);
    cg_context_image_free(image_URL_missing_arg, MALLOC_TAG);
}

static void cg_font_caching_test(void ** ignored) {
    cg_context_font_precache_glyphs(fonts.font_ctx, "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-_,<.>/?;:'\"\\|]}[{`~ABCDEFGHIJKLMNOPQRSTUVXWYZ");
    cg_context_font_clear_glyph_cache();
}

static void cg_image_font_oom_test(void ** ignored) {
    // verify oom for fonts and images are handled.
    {
        // can't open too many fonts to load or we blow the curl heap due to tons of handles. 4 has been reliable though (the first font file fails to load)
        cg_font_file_t * oom_font_files[5];
        cg_image_t * oom_images[40];

        // attempting to figure out how to get just one font to fail over is difficult... so dump a bunch and hope that something fails.
        for (int i = 0; i < ARRAY_SIZE(oom_font_files); ++i) {
            oom_font_files[i] = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/MXiangHeHeiTCforDSS-Bk.otf", cg_memory_region_high, cg_font_load_opts_none, MALLOC_TAG);
        }

        for (int i = 0; i < ARRAY_SIZE(oom_images); ++i) {
            oom_images[i] = cg_context_load_image_async("tests/images/dss/features/full_bleed/720p/nemo.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
        }

        while (true) {
            adk_curl_run_callbacks();
            thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
            int oom_images_completed = 0;
            for (; oom_images_completed < ARRAY_SIZE(oom_images); ++oom_images_completed) {
                if (cg_get_image_load_status(oom_images[oom_images_completed]) == cg_image_async_load_pending) {
                    break;
                }
            }
            int oom_fonts_completed = 0;
            for (; oom_fonts_completed < ARRAY_SIZE(oom_font_files); ++oom_fonts_completed) {
                if (cg_get_font_load_status(oom_font_files[oom_fonts_completed]) == cg_font_async_load_pending) {
                    break;
                }
            }
            if ((oom_images_completed == ARRAY_SIZE(oom_images)) && (oom_fonts_completed == ARRAY_SIZE(oom_font_files))) {
                break;
            }
            sb_thread_sleep((milliseconds_t){1});
        }

        bool any_image_oom = false;
        for (int i = 0; i < ARRAY_SIZE(oom_images); ++i) {
            if (cg_get_image_load_status(oom_images[i]) == cg_image_async_load_out_of_memory) {
                any_image_oom = true;
                break;
            }
        }
        VERIFY(any_image_oom);

        bool any_font_oom = false;
        for (int i = 0; i < ARRAY_SIZE(oom_font_files); ++i) {
            if (cg_get_font_load_status(oom_font_files[i]) == cg_font_async_load_out_of_memory) {
                any_font_oom = true;
                break;
            }
        }
        VERIFY(any_font_oom);

        for (int i = 0; i < ARRAY_SIZE(oom_images); ++i) {
            cg_context_image_free(oom_images[i], MALLOC_TAG);
        }
        for (int i = 0; i < ARRAY_SIZE(oom_font_files); ++i) {
            cg_context_font_file_free(oom_font_files[i], MALLOC_TAG);
        }
    }
}

static void render_canvas_begin() {
    sb_enumerate_display_modes_result_t display_mode_result;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_write_set_display_size,
        display_mode_result.display_mode.width,
        display_mode_result.display_mode.height,
        MALLOC_TAG);
    cg_context_begin((milliseconds_t){0});
}

static void render_and_swap() {
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_write_present,
        (rhi_swap_interval_t){0},
        MALLOC_TAG);
}
static void save_baseline_version(const char * filename) {
    sb_file_t * const file = sb_fopen(baseline_root_folder, filename, "wb");
    VERIFY_MSG(file, "failed to save baseline's version file: %s", filename);
    const int version = screenshot_baseline_version;
    sb_fwrite(&version, sizeof(version), 1, file);
    sb_fclose(file);
}

static bool should_update_baselines() {
    char baseline_version_filename[filename_max_len];
    sprintf_s(baseline_version_filename, filename_max_len, "%s%s", baseline_screenshot_folder, baseline_version_name);
    sb_file_t * const file = sb_fopen(baseline_root_folder, baseline_version_filename, "rb");
    if (!file) {
        save_baseline_version(baseline_version_filename);
        return true;
    }
    int version = 0;
    VERIFY(sb_fread(&version, sizeof(version), 1, file) == 1);
    sb_fclose(file);

    if (version != screenshot_baseline_version) {
        save_baseline_version(baseline_version_filename);
    }
    return version != screenshot_baseline_version;
}

static void wait_render_present() {
    render_device_frame(the_app.render_device);
    flush_render_device(the_app.render_device);
}

// FIX: this digusting hack of having to loop over canvas calls several times before they present properly.
#define CG_PERFORM_TEST(cg_func_and_args, _filename)                                                                         \
    do {                                                                                                                     \
        const char * const filename = _filename;                                                                             \
        image_t testcase_screenshot;                                                                                         \
        render_canvas_begin();                                                                                               \
        cg_func_and_args;                                                                                                    \
        cg_context_end(MALLOC_TAG);                                                                                          \
                                                                                                                             \
        if (statics.baselines_out_of_date || !check_if_baseline_exists(filename, statics.display_mode)) {                    \
            save_baseline(filename, statics.baseline_screenshot_region, statics.display_mode);                               \
        }                                                                                                                    \
        adk_take_screenshot(&testcase_screenshot, statics.testcase_screenshot_region);                                       \
                                                                                                                             \
        render_and_swap();                                                                                                   \
        wait_render_present();                                                                                               \
        compare_test_binary_files(&testcase_screenshot, filename, statics.display_mode, statics.baseline_screenshot_region); \
        if (!is_save_into_memory_region_test_done) {                                                                         \
            compare_screenshot_file_with_mem(&testcase_screenshot, filename, statics.display_mode);                          \
        }                                                                                                                    \
    } while (false)

#define CG_CJK_TEST(font_ctx, test_str_filepath, test_name)                              \
    do {                                                                                 \
        sb_file_t * const fp = sb_fopen(sb_app_root_directory, test_str_filepath, "rb"); \
        char buff[1024] = {0};                                                           \
        const size_t str_len = sb_fread(buff, 1, ARRAY_SIZE(buff), fp);                  \
        VERIFY(str_len < ARRAY_SIZE(buff));                                              \
        sb_fclose(fp);                                                                   \
        CG_PERFORM_TEST(cg_cjk_test(font_ctx, buff), test_name);                         \
    } while (false)

static int canvas_test_init(void ** ignored) {
    the_app.display_settings._720p_hack = true;
    thread_pool_init(&the_app.default_thread_pool, the_app.api->mmap.default_thread_pool.region, unit_test_guard_page_mode, 1, "test_pool", MALLOC_TAG);
    VERIFY(adk_curl_api_init(the_app.api->mmap.curl.region, the_app.api->mmap.httpx_fragment_buffers.region, network_pump_fragment_size, network_pump_sleep_period, unit_test_guard_page_mode, adk_curl_http_init_normal));

    update_config_dir_for_tests();

    create_canvas_baseline_path(baseline_screenshot_folder);
    create_canvas_testcase_path(testcase_screenshot_folder);
    init_display_for_tests("canvas_tests");

    statics.baselines_out_of_date = should_update_baselines();

    VERIFY(sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &statics.display_results));
    statics.display_mode = statics.display_results.display_mode;

    int width, height;
    sb_get_window_client_area(the_app.window, &width, &height);
    VERIFY(width == statics.display_mode.width && height == statics.display_mode.height);

    font_files.dj_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/dj.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    font_files.tech_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/tech.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    font_files.font_with_missing_glyph_indicator = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/avenir-black.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    font_files.cjk_sc_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/MXiangHeHeiSCStd-Medium.otf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    font_files.cjk_tc_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/MXiangHeHeiTCforDSS-Bk.otf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    font_files.avenir_roman_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/avenir-roman.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);

    images.image = cg_context_load_image_async("tests/images/dss/features/full_bleed/720p/nemo.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.image_gif = cg_context_load_image_async("tests/images/dss/anim/starwars.gif", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.grayscale_image = cg_context_load_image_async("tests/images/dss/tv_14_rating_scale.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.bif_image = cg_context_load_image_async("tests/images/dss/roku-moana.bif", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.badge = cg_context_load_image_async("tests/images/dss/badges/aladdin.jpeg", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.alpha_mask = cg_context_load_image_async("tests/images/rounded.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.jpg_image = cg_context_load_image_async("tests/images/dss/features/nemo.jpeg", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    images.exif_image = cg_context_load_image_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/gui_samples/static_splash.jpg", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
#if defined(_LEIA)
    images.gnf_image = cg_context_load_image_async("https://qa-ripcut-delivery.disney-plus.net/v1/variant/disney/CFB0563B69DAF82FABA9ADE9AE2450DB87623DDDBB81A69C1C612D600F8A61FC/scale?width=552&library=1&format=gnf", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
#elif defined(_VADER)
    images.gnf_image = cg_context_load_image_async("tests/images/dss/brave.gnf", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
#endif

    const int num_images = sizeof(images) / sizeof(cg_image_t *);
    const cg_image_t * const images_src = (const cg_image_t *)&images;

    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        int num_images_loaded = 0;
        int num_fonts_loaded = 0;
        for (int i = 0; i < num_images; ++i) {
            const cg_image_async_load_status_e load_status = cg_get_image_load_status(&images_src[i]);
            if (load_status == cg_image_async_load_complete) {
                ++num_images_loaded;
            }
            VERIFY(load_status >= cg_image_async_load_complete);
        }
        for (int i = 0; i < ARRAY_SIZE(font_files.files); ++i) {
            const cg_font_async_load_status_e load_status = cg_get_font_load_status(font_files.files[i]);
            if (load_status == cg_font_async_load_complete) {
                ++num_fonts_loaded;
            }
            VERIFY(load_status >= cg_font_async_load_complete);
        }
        if ((num_images_loaded == num_images) && (num_fonts_loaded == ARRAY_SIZE(font_files.files))) {
            break;
        }
        sb_thread_sleep((milliseconds_t){1});
    }

    //// can't open too many fonts to load or we blow the curl heap due to tons of handles. 4 has been reliable though (the first font file fails to load)
    //cg_font_file_t * oom_font_files[4];
    //cg_image_t * oom_images[40];

    fonts.font_ctx = cg_context_create_font_context(font_files.dj_font, 80.0f, 4, MALLOC_TAG);
    fonts.font_ctx_20px = cg_context_create_font_context(font_files.dj_font, 20.0f, 4, MALLOC_TAG);
    fonts.font1_ctx = cg_context_create_font_context(font_files.tech_font, 50.0f, 4, MALLOC_TAG);
    fonts.font2_ctx = cg_context_create_font_context(font_files.dj_font, 40.0f, 4, MALLOC_TAG);
    fonts.font_with_missing_glyph_ctx = cg_context_create_font_context(font_files.font_with_missing_glyph_indicator, 40.f, 4, MALLOC_TAG);
    fonts.avenir_black = cg_context_create_font_context(font_files.font_with_missing_glyph_indicator, 70.f, 4, MALLOC_TAG);

    fonts.cjk_sc_ctx = cg_context_create_font_context(font_files.cjk_sc_font, 80.f, 4, MALLOC_TAG);
    fonts.cjk_tc_ctx = cg_context_create_font_context(font_files.cjk_tc_font, 80.f, 4, MALLOC_TAG);
    fonts.avenir_roman_font_ctx = cg_context_create_font_context(font_files.avenir_roman_font, 34.f, 4, MALLOC_TAG);

    const int screenshot_size = statics.display_mode.width * statics.display_mode.height * 4;
    statics.baseline_screenshot_region = MEM_REGION(.ptr = malloc(screenshot_size), .size = screenshot_size);
    statics.testcase_screenshot_region = MEM_REGION(.ptr = malloc(screenshot_size), .size = screenshot_size);
    return 0;
}

static int canvas_test_teardown(void ** ignored) {
    free(statics.testcase_screenshot_region.ptr);
    free(statics.baseline_screenshot_region.ptr);

    const int num_images = sizeof(images) / sizeof(cg_image_t *);
    cg_image_t ** const images_src = (cg_image_t **)&images;

    const int num_fonts = sizeof(fonts) / sizeof(cg_font_context_t *);
    cg_font_context_t ** const fonts_src = (cg_font_context_t **)&fonts;

    for (int i = 0; i < ARRAY_SIZE(font_files.files); ++i) {
        cg_context_font_file_free(font_files.files[i], MALLOC_TAG);
    }
    for (int i = 0; i < num_fonts; ++i) {
        cg_context_font_context_free(fonts_src[i], MALLOC_TAG);
    }
    for (int i = 0; i < num_images; ++i) {
        cg_context_image_free(images_src[i], MALLOC_TAG);
    }

    thread_pool_drain(the_app.canvas.context.thread_pool);

    cg_gl_state_free(&the_app.canvas.cg_gl);
    cg_context_free(&the_app.canvas.context, MALLOC_TAG);

    render_release(&the_app.render_device->resource, MALLOC_TAG);

    sb_destroy_main_window();
    the_app.window = NULL;

    adk_curl_api_shutdown();

    thread_pool_shutdown(&the_app.default_thread_pool, MALLOC_TAG);
    adk_set_directory_path(sb_app_config_directory, statics.orig_config_dir);
    return 0;
}

static void cg_missing_glyph_multiple_font_test(const char * const text, const char * const ellipses, const char * const missing_glyph_indicator, cg_text_block_options_e options) {
    const int num_fonts = sizeof(fonts) / sizeof(cg_font_context_t *);
    cg_font_context_t ** const fonts_src = (cg_font_context_t **)&fonts;
    cg_font_metrics_t metrics;
    const cg_rect_t text_rect = (cg_rect_t){.x = 50.0f, .y = 25.0f, .width = 1000.0f, .height = 720.0f};
    draw_text_block_background(text_rect);

    cg_context_font_precache_glyphs(fonts.font_with_missing_glyph_ctx, "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-_,<.>/?;:'\"\\|]}[{`~ABCDEFGHIJKLMNOPQRSTUVXWYZ");
    cg_context_font_clear_glyph_cache();

    cg_rect_t rect = (cg_rect_t){.x = 50.0f, .y = 25.0f, .width = 1000.0f, .height = 70.0f};
    for (int font_ind = 0; font_ind < num_fonts; ++font_ind) {
        cg_context_set_global_missing_glyph_indicator(missing_glyph_indicator);
        metrics = cg_context_fill_text_block_with_options(fonts_src[font_ind], rect, 0, 0, text, ellipses, options);
        cg_context_set_global_missing_glyph_indicator(NULL);
        rect.y = max_float(metrics.bounds.y, rect.y) + metrics.bounds.height * 1.2f;
    }
}

static void cg_image_test(void ** ignored) {
    statics.num_failing_tests = 0;
    CG_PERFORM_TEST(cg_draw_image_test(images.image, 0, 0, statics.display_mode.width), "draw_image_test");
    CG_PERFORM_TEST(cg_draw_image_9slice_test(images.badge, 0, 0, statics.display_mode.width), "draw_image_9slice_test");
    CG_PERFORM_TEST(cg_draw_arcs_test(), "draw_arc_test");
    CG_PERFORM_TEST(cg_draw_clipped_arcs_test(), "draw_clipped_arc_test");
    CG_PERFORM_TEST(cg_draw_bubble_test(images.image), "draw_bubble_test");
    CG_PERFORM_TEST(cg_draw_rounded_rect_test(images.image_gif), "draw_rounded_rect_test");
    CG_PERFORM_TEST(cg_draw_face_test(), "draw_face_test");
    CG_PERFORM_TEST(cg_clear_rect_test(), "draw_clear_rect_test");
    CG_PERFORM_TEST(cg_tris_test(), "draw_trist_test");
    CG_PERFORM_TEST(cg_global_alpha_test(), "draw_global_alpha_test");
    CG_PERFORM_TEST(cg_random_arcs_test(), "draw_random_arcs_test");
    CG_PERFORM_TEST(cg_draw_sdf_demo(), "cg_sdf_demo_test");
    CG_PERFORM_TEST(cg_combined_test(fonts.font_ctx, images.image, images.image_gif, 0, 0, statics.display_mode.width), "draw_combined_test");
    CG_PERFORM_TEST(cg_combined_with_clip_test(fonts.font_ctx, images.image, images.image_gif, 0, 0, statics.display_mode.width), "draw_clipped_combined_test");
    CG_PERFORM_TEST(cg_simple_grayscale_image(images.grayscale_image), "draw_grayscale_image");

    CG_PERFORM_TEST(cg_bif_index(images.bif_image), "draw_bif_with_index");
    CG_PERFORM_TEST(cg_jpg_test(images.jpg_image), "draw_jpg_test");
#if defined(_LEIA) || defined(_VADER)
    CG_PERFORM_TEST(cg_gnf_test(images.gnf_image), "draw_gnf_test");
#endif
    assert_int_equal(statics.num_failing_tests, 0);
}

static void cg_font_test(void ** ignored) {
    statics.num_failing_tests = 0;
    CG_PERFORM_TEST(cg_context_text_measure_whitespace_test(fonts.font_ctx), "cg_context_text_measure_whitespace_test");
    CG_PERFORM_TEST(cg_text_block_terrible_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_left), "draw_text_block_terrible_input_left");
    CG_PERFORM_TEST(cg_text_block_terrible_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_center), "draw_text_block_terrible_input_center");
    CG_PERFORM_TEST(cg_text_block_terrible_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_right), "draw_text_block_terrible_input_right");

    CG_PERFORM_TEST(cg_text_block_normal_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_left), "draw_text_block_normal_input_left");
    CG_PERFORM_TEST(cg_text_block_normal_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_center), "draw_text_block_normal_input_center");
    CG_PERFORM_TEST(cg_text_block_normal_input_aligned_test(fonts.font_ctx, cg_text_block_align_line_right), "draw_text_block_normal_input_right");

    CG_PERFORM_TEST(cg_text_block_offset_test(fonts.font_ctx, 0), "draw_text_block_offset_discard_on_bounds");
    CG_PERFORM_TEST(cg_text_block_offset_test(fonts.font_ctx, cg_text_block_allow_block_bounds_overflow), "draw_text_block_offset_include_on_bounds");
    CG_PERFORM_TEST(cg_text_alignment_test(fonts.font1_ctx), "draw_text_alignment");

    CG_PERFORM_TEST(cg_text_block_space_edge_cases(fonts.font2_ctx, cg_text_block_align_line_left), "draw_text_block_space_linebreak_left");
    CG_PERFORM_TEST(cg_text_block_space_edge_cases(fonts.font2_ctx, cg_text_block_align_line_center), "draw_text_block_space_linebreak_center");
    CG_PERFORM_TEST(cg_text_block_space_edge_cases(fonts.font2_ctx, cg_text_block_align_line_right), "draw_text_block_space_linebreak_right");

    CG_PERFORM_TEST(cg_text_ellipses_test(fonts.font2_ctx), "draw_text_block_ellipses");
    CG_PERFORM_TEST(cg_text_ellipses_newline_test(fonts.font2_ctx), "draw_text_block_ellipses_newline");
    CG_PERFORM_TEST(cg_text_spacing_test_absolute(fonts.font2_ctx), "draw_text_block_absolute_line_space");
    CG_PERFORM_TEST(cg_text_spacing_test_relative(fonts.font2_ctx), "draw_text_block_relative_line_space");

    CG_PERFORM_TEST(cg_text_massive_amount_of_text(fonts.font_ctx_20px), "draw_text_block_multi_bank_stress_test");

    CG_CJK_TEST(fonts.cjk_sc_ctx, "tests/images/text_samples/cjk_sc_sample.txt", "cjk_sc_test");
    CG_CJK_TEST(fonts.cjk_tc_ctx, "tests/images/text_samples/cjk_tc_sample.txt", "cjk_tc_test");

    CG_PERFORM_TEST(cg_text_block_with_missing_glyphs(fonts.font_with_missing_glyph_ctx), "draw_text_block_with_missing_glyph_indicator");
    CG_PERFORM_TEST(cg_ellipses_vert_reservation_edge_case(fonts.avenir_roman_font_ctx), "draw_ellipses_reservation_edge_case");
    assert_int_equal(statics.num_failing_tests, 0);
}

static const char * const edge_case_text[] = {
    [0] = "A string\n                                       of misc words                    ",
    [1] = "A string\n                                       of misc words \n      and their  various\r potentially aaaaaaaaaaaaaaaaa\n\n\r broken spacing        ",
    [2] = "A string\nof misc words \n     \n\n\n\n\n\n and their  various\r potentially aaaaaaaaaaaaaaaaa\n\n\r broken spacing        ", // + ellipses
    [3] = "A string\nof misc words \n          and their various\r potentially aaaaaaaaaaaaaaaaa\n\n\r broken spacing        ", // + ellipses
    [4] = "A string\nof misc wor \n     \n\n\n\n\n\n and their  various\r potentially aaaaaaaaaaaaaaaaa\n\n\r broken spacing        ", // + ellipses
    [5] = "A string\nof misc words \n", // + ellipses
    [6] = "A string\nof misc words\n", // + align right (nothing should happen, should read same as above)
    [7] = "A string\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", // some characters could fall off if not handled properly..
    [8] = "A string\nwordy muchlonger excessive testcases",
    [9] = "A string\r\nwordy muchlonger excessive testcases",
    [10] = "A string\n\rwordy muchlonger excessive testcases",
    [11] = "A string wordy muchlonger excessive testcases",
    [12] = "a\n\n\n\npile\n\nof\n\nreally\n\nspaced\n\n\n\n\nlong\ntext",
};

static const cg_text_block_options_e line_alignments[] = {
    [0] = cg_text_block_align_line_left,
    [1] = cg_text_block_align_line_center,
    [2] = cg_text_block_align_line_right,
};

static const cg_text_block_options_e block_alignments[] = {
    [0] = cg_text_block_align_text_center,
};

static const char * const ellipses[] = {
    [0] = NULL,
    [1] = "...",
};

static void cg_font_edgecase_test(void ** ignored) {
    statics.num_failing_tests = 0;
    const cg_rect_t rects[2] = {
        [0] = (cg_rect_t){.x = 200.0f, .y = 100.0f, .width = 700.0f, .height = 320.0f},
        [1] = (cg_rect_t){.x = 100.0f, .y = 200.0f, .width = 900.0f, .height = 500.0f}};

    const int text_test_count = ARRAY_SIZE(edge_case_text) * ARRAY_SIZE(line_alignments) * ARRAY_SIZE(block_alignments) * ARRAY_SIZE(ellipses) * ARRAY_SIZE(rects);
    print_message("Running [%i] text block edgecase tests\n", text_test_count);

    char testname_buff[1024];
    for (int text_ind = 0; text_ind < ARRAY_SIZE(edge_case_text); ++text_ind) {
        for (int line_align_ind = 0; line_align_ind < ARRAY_SIZE(line_alignments); ++line_align_ind) {
            for (int block_align_ind = 0; block_align_ind < ARRAY_SIZE(block_alignments); ++block_align_ind) {
                for (int ellipses_ind = 0; ellipses_ind < ARRAY_SIZE(ellipses); ++ellipses_ind) {
                    for (int rects_ind = 0; rects_ind < ARRAY_SIZE(rects); ++rects_ind) {
                        sprintf_s(testname_buff, ARRAY_SIZE(testname_buff), "draw_pathological_cases_text[%i]_line_align[%i]_block_align[%i]_ellipses[%i]_rects[%i]", text_ind, line_align_ind, block_align_ind, ellipses_ind, rects_ind);
                        CG_PERFORM_TEST(cg_text_block_pathological(fonts.avenir_black, rects[rects_ind], edge_case_text[text_ind], ellipses[ellipses_ind], line_alignments[line_align_ind] | block_alignments[block_align_ind]), testname_buff);
                    }
                }
            }
        }
    }
    assert_int_equal(statics.num_failing_tests, 0);
}

static const char * const text_files[] = {
    [0] = "tests/images/text_samples/cjk_jp_sample.txt",
    [1] = "tests/images/text_samples/cjk_tc_sample.txt",
    [2] = "tests/images/text_samples/cjk_tc_sample_repeat.txt",
};

static const char * const missing_glyph_indicators[] = {
    [0] = ".",
    [1] = "*",
    [2] = "?",
    [3] = "m",
    [4] = "\xE9\xAD\x94", // bytes for utf8 character
};

static void cg_missing_glyph_stress_test(void ** ignored) {
    statics.num_failing_tests = 0;
    char text_buff[2048] = {0};
    const cg_rect_t rects[2] = {
        [0] = (cg_rect_t){.x = 200.0f, .y = 100.0f, .width = 700.0f, .height = 320.0f},
        [1] = (cg_rect_t){.x = 100.0f, .y = 200.0f, .width = 900.0f, .height = 500.0f}};

    const int text_test_count = ARRAY_SIZE(missing_glyph_indicators) * ARRAY_SIZE(text_files) * ARRAY_SIZE(line_alignments) * ARRAY_SIZE(block_alignments) * ARRAY_SIZE(ellipses) * ARRAY_SIZE(rects) + 1;
    print_message("Running [%i] missing glyph edgecase tests\n", text_test_count);

    {
        sb_file_t * const text_fp = sb_fopen(sb_app_root_directory, text_files[2], "rt");
        sb_fread(text_buff, 1, ARRAY_SIZE(text_buff) - 1, text_fp);
        sb_fclose(text_fp);
        CG_PERFORM_TEST(cg_draw_missing_glyph_indicator_for_font_ctx(fonts.avenir_roman_font_ctx, rects[0], text_buff, missing_glyph_indicators[2]), "cg_draw_missing_glyph_indicator_for_font_ctx");
    }

    char testname_buff[1024];
    for (int text_file_ind = 0; text_file_ind < ARRAY_SIZE(text_files); ++text_file_ind) {
        ZEROMEM(&text_buff);
        sb_file_t * const text_fp = sb_fopen(sb_app_root_directory, text_files[text_file_ind], "rt");
        sb_fread(text_buff, 1, ARRAY_SIZE(text_buff) - 1, text_fp);
        sb_fclose(text_fp);
        for (int missing_glyph_indicator_ind = 0; missing_glyph_indicator_ind < ARRAY_SIZE(missing_glyph_indicators); ++missing_glyph_indicator_ind) {
            for (int line_align_ind = 0; line_align_ind < ARRAY_SIZE(line_alignments); ++line_align_ind) {
                for (int block_align_ind = 0; block_align_ind < ARRAY_SIZE(block_alignments); ++block_align_ind) {
                    for (int ellipses_ind = 0; ellipses_ind < ARRAY_SIZE(ellipses); ++ellipses_ind) {
                        for (int rects_ind = 0; rects_ind < ARRAY_SIZE(rects); ++rects_ind) {
                            sprintf_s(testname_buff, ARRAY_SIZE(testname_buff), "cg_draw_missing_glyph_indicator_cases_text_file[%i]_missing_glyph_indicator[%i]_line_align[%i]_block_align[%i]_ellipses[%i]_rects[%i]", text_file_ind, missing_glyph_indicator_ind, line_align_ind, block_align_ind, ellipses_ind, rects_ind);
                            CG_PERFORM_TEST(cg_draw_missing_glyph_indicator(fonts.avenir_roman_font_ctx, rects[rects_ind], text_buff, ellipses[ellipses_ind], missing_glyph_indicators[missing_glyph_indicator_ind], line_alignments[line_align_ind] | block_alignments[block_align_ind]), testname_buff);
                        }
                    }
                }
            }
        }
    }
    assert_int_equal(statics.num_failing_tests, 0);
}

static void cg_missing_glyph_font_test(void ** ignored) {
    statics.num_failing_tests = 0;
    const int num_fonts = sizeof(fonts) / sizeof(cg_font_context_t *);
    cg_font_context_t ** const fonts_src = (cg_font_context_t **)&fonts;
    const int line_align_ind = 0;
    const int block_align_ind = 0;
    const int ellipses_ind = 1;
    const cg_rect_t rect = (cg_rect_t){.x = 200.0f, .y = 100.0f, .width = 700.0f, .height = 320.0f};

    const int text_test_count = ARRAY_SIZE(missing_glyph_indicators) + ARRAY_SIZE(missing_glyph_indicators) * ARRAY_SIZE(text_files) * num_fonts;
    print_message("Running [%i] missing glyph font tests\n", text_test_count);

    char text_buff[2048] = {0};
    char testname_buff[1024];
    for (int font_ind = 0; font_ind < num_fonts; ++font_ind) {
        for (int text_file_ind = 0; text_file_ind < ARRAY_SIZE(text_files); ++text_file_ind) {
            ZEROMEM(&text_buff);
            sb_file_t * const text_fp = sb_fopen(sb_app_root_directory, text_files[text_file_ind], "rt");
            sb_fread(text_buff, 1, ARRAY_SIZE(text_buff) - 1, text_fp);
            sb_fclose(text_fp);
            for (int missing_glyph_indicator_ind = 0; missing_glyph_indicator_ind < ARRAY_SIZE(missing_glyph_indicators); ++missing_glyph_indicator_ind) {
                sprintf_s(testname_buff, ARRAY_SIZE(testname_buff), "cg_font_missing_glyph_indicator_cases_text_file[%i]_missing_glyph_indicator[%i]_font[%i]", text_file_ind, missing_glyph_indicator_ind, font_ind);
                CG_PERFORM_TEST(cg_draw_missing_glyph_indicator(fonts_src[font_ind], rect, text_buff, ellipses[ellipses_ind], missing_glyph_indicators[missing_glyph_indicator_ind], line_alignments[line_align_ind] | block_alignments[block_align_ind]), testname_buff);
            }
        }
    }

    ZEROMEM(&text_buff);
    sb_file_t * const text_fp = sb_fopen(sb_app_root_directory, text_files[1], "rt");
    sb_fread(text_buff, 1, ARRAY_SIZE(text_buff) - 1, text_fp);
    sb_fclose(text_fp);
    for (int missing_glyph_indicator_ind = 0; missing_glyph_indicator_ind < ARRAY_SIZE(missing_glyph_indicators); ++missing_glyph_indicator_ind) {
        sprintf_s(testname_buff, ARRAY_SIZE(testname_buff), "cg_multiple_font_missing_glyph_indicator_cases_missing_glyph_indicator[%i]", missing_glyph_indicator_ind);
        CG_PERFORM_TEST(cg_missing_glyph_multiple_font_test(text_buff, ellipses[ellipses_ind], missing_glyph_indicators[missing_glyph_indicator_ind], line_alignments[line_align_ind] | block_alignments[block_align_ind]), testname_buff);
    }
    assert_int_equal(statics.num_failing_tests, 0);
}

static void cg_text_height_test(void ** ignored) {
    const float expected_heights[] = {
        [0] = 720.f,
        [1] = 2800.f,
        [6] = 640.f,
        [7] = 1200.f,
        [12] = 1360.f,
    };
    const float widths[] = {
        [0] = 200.f,
        [1] = 200.f,
        [6] = 200.f,
        [7] = 200.f,
        [12] = 1000.f,
    };

    for (int ind = 0; ind < ARRAY_SIZE(expected_heights); ++ind) {
        if (expected_heights[ind] != 0.f) {
            float widest_lined_ignored = 0.f;
            const cg_rect_t box_extents = mosaic_context_get_text_block_extents(fonts.font_ctx->mosaic_ctx, widths[ind], 0.f, edge_case_text[ind], &widest_lined_ignored, cg_text_block_align_line_left);
            VERIFY_MSG(box_extents.height - expected_heights[ind] < 0.001f, "expected height: [%f]\ngot: [%f]", expected_heights[ind], box_extents.height);
        }
    }
}

static void cg_page_count_test(void ** ignored) {
    const cg_rect_t page_rect = {.x = 200, .y = 100, .width = 700, .height = 320};
    uint32_t curr_page_total_offset = 0;
    const int sample_text_ind = 1;
    int page_count = 0;
    while (*(edge_case_text[sample_text_ind] + curr_page_total_offset) != '\0') {
        const cg_text_block_page_offsets_t page_offsets = cg_context_get_text_block_page_offsets(
            fonts.font_ctx,
            page_rect,
            0,
            0,
            edge_case_text[sample_text_ind] + curr_page_total_offset,
            cg_text_block_align_line_left);

        curr_page_total_offset += page_offsets.end_offset;
        ++page_count;
    }
    assert_int_equal(curr_page_total_offset, (uint32_t)strlen(edge_case_text[sample_text_ind]));
    assert_int_equal(page_count, 4);
}

static void cg_combined_tests_test(void ** ignored) {
    statics.num_failing_tests = 0;
    CG_PERFORM_TEST(cg_combined_test(fonts.font_ctx, images.image, images.image_gif, 0, 0, statics.display_mode.width), "draw_combined_test");
    CG_PERFORM_TEST(cg_combined_with_clip_test(fonts.font_ctx, images.image, images.image_gif, 0, 0, statics.display_mode.width), "draw_clipped_combined_test");
    CG_PERFORM_TEST(cg_video_rect_test(images.image, fonts.font2_ctx), "video_rect_test");
    assert_int_equal(statics.num_failing_tests, 0);
}

static int canvas_empty_setup(void ** ignored) {
    // make sure that when we enter low memory, we can't succeed in loading an image with guard pages enabled.
    the_app.display_settings._720p_hack = true;
    thread_pool_init(&the_app.default_thread_pool, the_app.api->mmap.default_thread_pool.region, unit_test_guard_page_mode, 1, "test_pool", MALLOC_TAG);
    VERIFY(adk_curl_api_init(the_app.api->mmap.curl.region, the_app.api->mmap.httpx_fragment_buffers.region, network_pump_fragment_size, network_pump_sleep_period, unit_test_guard_page_mode, adk_curl_http_init_normal));
    init_display_for_tests("canvas_tests");

    VERIFY(sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &statics.display_results));
    statics.display_mode = statics.display_results.display_mode;

    const int screenshot_size = statics.display_mode.width * statics.display_mode.height * 4;
    statics.baseline_screenshot_region = MEM_REGION(.ptr = malloc(screenshot_size), .size = screenshot_size);
    statics.testcase_screenshot_region = MEM_REGION(.ptr = malloc(screenshot_size), .size = screenshot_size);

    create_canvas_baseline_path(baseline_screenshot_folder);
    create_canvas_testcase_path(testcase_screenshot_folder);
    update_config_dir_for_tests();

    return 0;
}

static int canvas_empty_teardown(void ** ignored) {
    thread_pool_drain(the_app.canvas.context.thread_pool);

    cg_gl_state_free(&the_app.canvas.cg_gl);
    cg_context_free(&the_app.canvas.context, MALLOC_TAG);

    render_release(&the_app.render_device->resource, MALLOC_TAG);

    sb_destroy_main_window();
    the_app.window = NULL;

    adk_curl_api_shutdown();

    thread_pool_shutdown(&the_app.default_thread_pool, MALLOC_TAG);

    adk_set_directory_path(sb_app_config_directory, statics.orig_config_dir);

    free(statics.baseline_screenshot_region.ptr);
    free(statics.testcase_screenshot_region.ptr);
    return 0;
}

static void cg_low_memory_oom_with_guard_pages_enabled_check(void ** ignored) {
    cg_context_set_memory_mode(&the_app.canvas.context, cg_memory_mode_low);
    cg_image_t * const image = cg_context_load_image_async("tests/images/dss/features/full_bleed/720p/nemo.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);

        if (cg_get_image_load_status(image) != cg_image_async_load_pending) {
            break;
        }
    }

    const cg_image_async_load_status_e image_load_status = cg_get_image_load_status(image);
    cg_context_image_free(image, MALLOC_TAG);
    cg_context_set_memory_mode(&the_app.canvas.context, cg_memory_mode_high);
    assert_int_equal(image_load_status, cg_image_async_load_out_of_memory);
}

static void cg_draw_etc1_test(void ** ignored) {
    cg_image_t * const etc1_image = cg_context_load_image_async("tests/images/pvr_test_images/pvrtest_0_ETC1_UNB_lRGB_393_700_1_1_1_1_0.pvr", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        if (cg_get_image_load_status(etc1_image) != cg_image_async_load_pending) {
            break;
        }
    }
    CG_PERFORM_TEST(cg_context_draw_image(etc1_image, (cg_vec2_t){0}), "draw_etc1_test");
    cg_context_image_free(etc1_image, MALLOC_TAG);
}

static void cg_bif_cmd_buffer_submission_test(void ** ignored) {
    // Perform a test to make sure that bifs do not submit a command buffer before depenent cmd_buf_t(s) have been submitted.

    cg_image_t * const bif_image = cg_context_load_image_async("tests/images/dss/roku-moana.bif", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        if (cg_get_image_load_status(bif_image) == cg_image_async_load_complete) {
            // force a bif decode job via changing the index from 0
            cg_context_set_image_frame_index(bif_image, 1);
            break;
        }
    }

    // wait until the decode job has completed
    while (bif_image->bif->async_image_data.decode_job_running) {
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
    }
    // tick the renderer
    // at this point if we are violating submission orders we would crash in the RHI trying to access a null rhi_texture_t.
    render_canvas_begin();
    cg_context_end(MALLOC_TAG);
    render_and_swap();
    wait_render_present();

    cg_context_image_free(bif_image, MALLOC_TAG);
}

static void cg_gif_cmd_buffer_submission_test(void ** ignored) {
    // Perform a test to make sure that gifs do not submit a command buffer before depenent cmd_buf_t(s) have been submitted.

    cg_image_t * const gif_image = cg_context_load_image_async("tests/images/dss/anim/starwars.gif", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    cg_context_set_image_animation_state(gif_image, cg_image_animation_running);
    while (true) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        if ((cg_get_image_load_status(gif_image) == cg_image_async_load_complete)) {
            // force a gif decode job by giving us a sufficient large delta time
            // as strange as it looks, it is valid to call cg_context_begin() without calling end _assuming_ you don't need any of the rendering side effects.
            // we will internally call a few gl functions to setup uniforms, but those can be overriden later.
            cg_context_begin((milliseconds_t){10 * 1000});
            break;
        }
    }

    // wait until the decode job has completed
    while (gif_image->gif->async_image_data.decode_job_running) {
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
    }
    // tick the renderer
    // at this point if we are violating submission orders we would crash in the RHI trying to access a null rhi_texture_t.
    render_canvas_begin();
    cg_context_end(MALLOC_TAG);
    render_and_swap();
    wait_render_present();

    cg_context_image_free(gif_image, MALLOC_TAG);
}

int test_canvas() {
    const struct CMUnitTest empty_setup_tests[] = {
        cmocka_unit_test(cg_low_memory_oom_with_guard_pages_enabled_check),
        cmocka_unit_test(cg_draw_etc1_test),
    };
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(cg_bif_cmd_buffer_submission_test),
        cmocka_unit_test(cg_gif_cmd_buffer_submission_test),
        cmocka_unit_test(cg_image_load_cancel_test),
        cmocka_unit_test(cg_image_font_oom_test),
        cmocka_unit_test(cg_malformed_url_test),
        cmocka_unit_test(cg_font_caching_test),
        cmocka_unit_test(cg_image_test),
        cmocka_unit_test(cg_font_test),
        cmocka_unit_test(cg_combined_tests_test),
        cmocka_unit_test(cg_text_height_test),
        cmocka_unit_test(cg_page_count_test),
    };
    return cmocka_run_group_tests(empty_setup_tests, canvas_empty_setup, canvas_empty_teardown) + cmocka_run_group_tests(tests, canvas_test_init, canvas_test_teardown);
}

int test_canvas_integration() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(cg_font_edgecase_test),
        cmocka_unit_test(cg_missing_glyph_stress_test),
        cmocka_unit_test(cg_missing_glyph_font_test),
        cmocka_unit_test(cg_font_test),
        cmocka_unit_test(cg_font_edgecase_test),
    };
    return cmocka_run_group_tests(tests, canvas_test_init, canvas_test_teardown);
}
