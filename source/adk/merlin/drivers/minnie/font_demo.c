/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
font_demo.c

Canvas rendering test
*/

#include "source/adk/merlin/drivers/minnie/merlin.h"

#if _MERLIN_DEMOS

#include "extern/cjson/cJSON.h"
#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/canvas/private/cg_font.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/file/file.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/adk_sample_common.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_platform.h"

#include <math.h>

#define TAG_FONT_DEMO FOURCC('F', 'O', 'N', 'T')
enum {
    max_font_tests = 10,
    font_demo_heap_size = 16 * 1024
};

typedef struct font_test {
    cg_color_t color;
    cJSON * test_json;
    cg_font_context_t ** ctx;
    float font_multiplier;
} font_test_t;

static struct {
    bool mouse_dragging;
    bool view_atlas;
    bool atlas_squares;
    int32_t mouse_pos_x;
    int32_t mouse_pos_y;
    int32_t mouse_start_pos_x;
    int32_t mouse_start_pos_y;
    int32_t ctrl_key;
    int32_t test_count;
    font_test_t font_tests[max_font_tests];
    mosaic_context_t * mos_ctx;
    heap_t heap;
    mem_region_t pages;
    cJSON * json_root;
    cJSON_Env json_ctx;
} statics;

static struct {
    cg_font_context_t * ctx;
    cg_font_context_t * ctx_small;
    cg_font_context_t * ctx2;
    cg_font_context_t * ctx3;
    cg_font_context_t * ctx3_small;
    cg_font_context_t * ctx4;
} fonts;

// /* ------------------------------------------------------------------------- */
// /* ------------------------------------------------------------------------- */
static void * cjson_malloc(void * const ctx, size_t const size) {
    heap_t * const heap = (heap_t *)ctx;
    return heap_alloc(heap, size, MALLOC_TAG);
}

static void cjson_free(void * const ctx, void * const ptr) {
    if (ptr != NULL) {
        heap_t * const heap = (heap_t *)ctx;
        heap_free(heap, ptr, MALLOC_TAG);
    }
}

// File blob alloc/free
mem_region_t font_demo_alloc_blob(const char * const test_str_file) {
    const size_t test_str_file_size = get_artifact_size(sb_app_root_directory, test_str_file);
    if (test_str_file_size == 0) {
        return (mem_region_t){0};
    }

    return MEM_REGION(.ptr = heap_alloc(&statics.heap, test_str_file_size, MALLOC_TAG), .size = test_str_file_size);
}

void font_demo_free_blob(mem_region_t const test_str_blob) {
    if (test_str_blob.ptr != NULL) {
        heap_free(&statics.heap, test_str_blob.ptr, MALLOC_TAG);
    }
}

bool font_demo_parse_json(const_mem_region_t const test_str_blob) {
    statics.json_root = cJSON_Parse(&statics.json_ctx, test_str_blob);
    if (statics.json_root == NULL) {
        LOG_ERROR(TAG_FONT_DEMO, "Invalid JSON syntax");
        return false;
    }

    // Get test strings from json file
    cJSON * const font_tests = cJSON_GetObjectItem(statics.json_root, "font_tests");
    cJSON * font_test;
    statics.test_count = 0;

    // Loop through each test to parse/verify
    cJSON_ArrayForEach(font_test, font_tests) {
        cJSON * test_str = cJSON_GetObjectItem(font_test, "test_str");
        cJSON * fill = cJSON_GetObjectItem(font_test, "fill");
        cJSON * font_ctx = cJSON_GetObjectItem(font_test, "font_ctx");
        cJSON * r = cJSON_GetObjectItem(fill, "r");
        cJSON * g = cJSON_GetObjectItem(fill, "g");
        cJSON * b = cJSON_GetObjectItem(fill, "b");
        cJSON * a = cJSON_GetObjectItem(fill, "a");

        // verify values are valid
        if (!cJSON_IsNumber(r) || !cJSON_IsNumber(g) || !cJSON_IsNumber(b) || !cJSON_IsNumber(a)) {
            LOG_ERROR(TAG_FONT_DEMO, "Invalid RGBA value");
            return false;
        }
        if (!cJSON_IsNumber(font_ctx)) {
            LOG_ERROR(TAG_FONT_DEMO, "Invalid font_ctx value");
            return false;
        }
        if (!cJSON_IsString(test_str) || test_str->valuestring == NULL) {
            LOG_ERROR(TAG_FONT_DEMO, "Invalid test string");
            return false;
        }

        // store parsed values for use during rendering
        statics.font_tests[statics.test_count].color = ((cg_color_t){.r = (float)r->valuedouble, .g = (float)g->valuedouble, .b = (float)b->valuedouble, .a = (float)a->valuedouble});
        statics.font_tests[statics.test_count].test_json = test_str;

        // default values for font ctx and font multiplier
        statics.font_tests[statics.test_count].ctx = &fonts.ctx;
        statics.font_tests[statics.test_count].font_multiplier = 1.00f;

        if (font_ctx->valueint == 2) {
            statics.font_tests[statics.test_count].ctx = &fonts.ctx2;
            statics.font_tests[statics.test_count].font_multiplier = 1.25f;
        } else if (font_ctx->valueint == 1) {
            statics.font_tests[statics.test_count].ctx = &fonts.ctx_small;
        } else if (font_ctx->valueint == 3) {
            statics.font_tests[statics.test_count].ctx = &fonts.ctx3_small;
        } else if (font_ctx->valueint == 4) {
            statics.font_tests[statics.test_count].ctx = &fonts.ctx4;
        }
        statics.test_count++;
    }
    return true;
}

bool font_demo_init() {
    // Create font demo heap
    statics.pages = sb_map_pages(PAGE_ALIGN_INT(font_demo_heap_size), system_page_protect_read_write);
    TRAP_OUT_OF_MEMORY(statics.pages.ptr);
    heap_init_with_region(&statics.heap, statics.pages, 8, 0, "font-demo-heap");

    // initialize cjson context for reading in utf-8 text
    statics.json_ctx = (cJSON_Env){
        .ctx = &statics.heap,
        .callbacks = {
            .malloc = cjson_malloc,
            .free = cjson_free,
        }};

    // load json file with test cases and store data for use during test
    char asset_path_buff[2 * sb_max_path_length];
    bool success = false;
    mem_region_t test_str_blob = font_demo_alloc_blob(merlin_asset("json/font_demo_utf8.json", asset_path_buff, ARRAY_SIZE(asset_path_buff)));
    if (!test_str_blob.ptr) {
        LOG_ERROR(TAG_FONT_DEMO, "UTF-8 file not found or empty: \"%s\"", asset_path_buff);
    } else {
        if (load_artifact_data(sb_app_root_directory, test_str_blob, asset_path_buff, 0)) {
            success = font_demo_parse_json(test_str_blob.consted);
        }
    }

    font_demo_free_blob(test_str_blob);
    return success;
}

void font_demo_shutdown() {
    // Free cJSON resources
    if (statics.json_root) {
        cJSON_Delete(&statics.json_ctx, statics.json_root);
        statics.json_root = NULL;
    }
    // clean up heap resources allocated
    heap_destroy(&statics.heap, "font-demo-heap");
    sb_unmap_pages(statics.pages);
}

static bool dispatch_events_and_read_msecs(sb_window_t * const main_window, milliseconds_t * const msec_time) {
    const adk_event_t *head, *tail;
    sb_tick(&head, &tail);
    bool app_running = true;

    for (const adk_event_t * event = head; event != tail; ++event) {
        switch (event->event_data.type) {
            case adk_application_event: {
                if (event->event_data.app.event == adk_application_event_quit) {
                    app_running = false;
                }
            } break;
            case adk_window_event: {
                switch (event->event_data.win.event_data.event) {
                    case adk_window_event_close:
                        if (event->event_data.win.window.ptr == main_window) {
                            app_running = false;
                        }
                        break;
                    default:
                        break;
                }
            } break;
            case adk_stb_input_event: {
                if ((event->event_data.stb_input.repeat == 0) && (event->event_data.stb_input.stb_key == adk_stb_key_back)) {
                    app_running = false;
                }
            } break;
            case adk_gamepad_event: {
                if ((event->event_data.gamepad.event_data.event == adk_gamepad_event_button) && (event->event_data.gamepad.event_data.button_event.event == adk_gamepad_button_event_down) && (event->event_data.gamepad.event_data.button_event.button == adk_gamepad_button_b)) {
                    app_running = false;
                }
            } break;
            case adk_time_event:
                *msec_time = event->time;
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
                return app_running;
#else
                return true;
#endif
            default:
                break;
            case adk_key_event: {
                if ((event->event_data.key.event == adk_key_event_key_down) && (event->event_data.key.repeat == 0)) {
                    if (event->event_data.key.key == ' ') {
                        statics.view_atlas = !statics.view_atlas;
                    }
                    if (event->event_data.key.key == adk_key_f1) {
                        statics.atlas_squares = !statics.atlas_squares;
                    }
                    if (event->event_data.key.key == adk_key_escape) {
                        app_running = false;
                    }
                }
            } break;
            case adk_mouse_event: {
                statics.ctrl_key = event->event_data.mouse.mouse_state.mod_keys & adk_mod_control;

                statics.mouse_pos_x = event->event_data.mouse.mouse_state.x;
                statics.mouse_pos_y = event->event_data.mouse.mouse_state.y;

                if (event->event_data.mouse.event_data.event == adk_mouse_event_button) {
                    statics.mouse_dragging = event->event_data.mouse.event_data.button_event.event == adk_mouse_button_event_down;

                    if (statics.mouse_dragging) {
                        statics.mouse_start_pos_x = statics.mouse_pos_x;
                        statics.mouse_start_pos_y = statics.mouse_pos_y;
                    }
                }
            } break;
        }
    }

    // missing "time" event as last frame event
    TRAP("event journal error");
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
    return app_running;
#else
    return true;
#endif
}

static void cg_demo(const float dt) {
    static float time = 0.0f;
    time += 1.0f / 60.0f;

    cg_context_save();
    cg_context_translate((cg_vec2_t){.x = 780.0, .y = 300.0});
    cg_context_rotate((cg_rads_t){.rads = sinf(time / 10.0f) * 0.2f});
    cg_context_fill_style((cg_color_t){.r = 255, .g = 0, .b = 128, .a = 255});
    cg_context_fill_text_with_options(fonts.ctx3, (cg_vec2_t){.x = 0.0, .y = 0.0}, "Dreamcast", cg_font_fill_options_align_center);
    cg_context_restore();

    cg_context_save();
    cg_context_translate((cg_vec2_t){.x = 10.0, .y = 10.0});

    const char * text0 = "Left-top alignment, no options";
    cg_context_fill_style((cg_color_t){.r = 255, .g = 0, .b = 0, .a = 255});
    cg_context_fill_text(fonts.ctx2, (cg_vec2_t){.x = 0.0, .y = 0.0}, text0);

    const char * text1 = " | metrics test";
    const cg_font_metrics_t text_metrics = cg_context_text_measure(fonts.ctx2, text0);
    cg_context_fill_style((cg_color_t){.r = 0, .g = 128, .b = 255, .a = 255});
    cg_context_fill_text(fonts.ctx2, (cg_vec2_t){.x = text_metrics.bounds.width, .y = 0.0}, text1);

    cg_context_translate((cg_vec2_t){.x = 10.0, .y = 80.0});
    cg_context_begin_path(MALLOC_TAG);
    cg_context_rounded_rect((cg_rect_t){.x = 0.0, .y = 0.0, .width = 256.0, .height = 128.0}, 8.0, MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 128, .b = 0, .a = 255});
    cg_context_fill(MALLOC_TAG);

    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_fill_text_with_options(fonts.ctx, (cg_vec2_t){.x = 256.0 / 2, .y = 128.0 / 2}, "Centered", cg_font_fill_options_align_center);

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 180.0});
    cg_context_begin_path(MALLOC_TAG);
    cg_context_rounded_rect((cg_rect_t){.x = 0.0, .y = 0.0, .width = 256.0, .height = 128.0}, 8.0, MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 128, .b = 0, .a = 255});
    cg_context_fill(MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_fill_text_with_options(fonts.ctx, (cg_vec2_t){.x = 0.0, .y = 128.0 / 2}, "Left", cg_font_fill_options_align_left | cg_font_fill_options_align_top);

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 180.0});
    cg_context_begin_path(MALLOC_TAG);
    cg_context_rounded_rect((cg_rect_t){.x = 0.0, .y = 0.0, .width = 256.0, .height = 128.0}, 8.0, MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 128, .b = 0, .a = 255});
    cg_context_fill(MALLOC_TAG);
    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_fill_text_with_options(fonts.ctx, (cg_vec2_t){.x = 256.0, .y = 128.0 / 2}, "Right", cg_font_fill_options_align_right | cg_font_fill_options_align_bottom);

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 144.0});

    cg_context_fill_text_line(fonts.ctx, (cg_vec2_t){.x = 0.0, .y = 0.0}, "Fill Text Line: Some Text");

    cg_context_translate((cg_vec2_t){.x = 0.0, .y = 64.0});

    // Purposefully long enough to overflow the window
    cg_context_fill_text_line(fonts.ctx, (cg_vec2_t){.x = 0.0, .y = 0.0}, "Fill Text Line: What do you get when you multiply six by nine?");

    cg_context_restore();
}

static void font_demo() {
    const float mouse_pos_x = (float)statics.mouse_pos_x;
    const float mouse_pos_y = (float)statics.mouse_pos_y;
    const float mouse_start_pos_x = (float)statics.mouse_start_pos_x;
    const float mouse_start_pos_y = (float)statics.mouse_start_pos_y;

    cg_font_metrics_t metrics;
    float y = 0;

    for (int32_t i = 0; i < statics.test_count; i++) {
        cg_context_fill_style(statics.font_tests[i].color);
        metrics = cg_context_fill_text(*(statics.font_tests[i].ctx), (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, statics.font_tests[i].test_json->valuestring);
        y += metrics.bounds.height * statics.font_tests[i].font_multiplier;
        if (i == 0) {
            cg_context_stroke_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
            cg_context_stroke_rect(metrics.bounds, MALLOC_TAG);
        }
    }

    float width = mouse_pos_x - mouse_start_pos_x;
    float height = mouse_pos_y - mouse_start_pos_y;
    const float min_selection_size = 4.f;
    if (fabs(width) < min_selection_size) {
        width = width < 0.f ? -min_selection_size : min_selection_size;
    }
    if (fabs(height) < min_selection_size) {
        height = height < 0.f ? -min_selection_size : min_selection_size;
    }
    if (statics.mouse_dragging) {
        cg_context_set_line_width(1.0);
        cg_context_begin_path(MALLOC_TAG);
        cg_color_t selection_color = {.r = 50, .g = 100, .b = 190, .a = 100};
        cg_context_stroke_style(selection_color);
        cg_context_fill_style(selection_color);
        cg_context_rect((cg_rect_t){.x = mouse_start_pos_x + (width < 0.f ? width : 0.f), .y = mouse_start_pos_y + (height < 0.f ? height : 0.f), .width = (float)fabs(width), .height = (float)fabs(height)}, MALLOC_TAG);
        cg_context_stroke(MALLOC_TAG);
        cg_context_fill(MALLOC_TAG);
    }
}

void mosaic_context_glyph_raster_debug_draw(mosaic_context_t * const mosaic_ctx, const int32_t mouse_pos_x, const int32_t mouse_pos_y, const bool draw_rects);

static void display_atlas(cg_context_t * const ctx) {
    cg_context_save();
    mosaic_context_glyph_raster_debug_draw(ctx->mosaic_ctx, statics.mouse_pos_x, statics.mouse_pos_y, statics.atlas_squares);
    cg_context_restore();
}

static void cg_canvas_demo_step(cg_context_t * const ctx, float dt) {
    if (statics.view_atlas) {
        display_atlas(ctx);
    } else {
        cg_demo(dt);
        font_demo();
    }

    cg_context_fill_style((cg_color_t){.r = 128, .g = 255, .b = 100, .a = 255});
}

static const char * const adk_app_name = "Font Demo";

int font_demo_main(const int argc, const char * const * const argv) {
    adk_app_init_main_display(adk_app_name);

    statics.mos_ctx = the_app.canvas.context.mosaic_ctx; // mosaic_context_new(&the_app.cg, 1024, 1024/2);

    VERIFY_MSG(font_demo_init(), "Failed to initialize font-demo");

    cg_font_file_t * const tech_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/tech.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    // loading this file may take a decent bit of time.
    //   if the app appears stuck on a black screen swap this remote url value out with the above disk location.
    cg_font_file_t * const arial_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/arial.ttf", cg_memory_region_high, cg_font_load_opts_none, MALLOC_TAG);
    cg_font_file_t * const dj_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/dj.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);
    cg_font_file_t * const cjk_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/MXiangHeHeiSCStd-Medium.otf", cg_memory_region_high, cg_font_load_opts_none, MALLOC_TAG);

    const cg_font_file_t * const font_files[] = {tech_font, arial_font, dj_font, cjk_font};

    bool inited_font_contexts = false;
    milliseconds_t time;
    milliseconds_t last_time = {0};
    uint32_t num_frames = 0;
    milliseconds_t fps_time = {0};
    float filtered_dt = 0.0f;

    milliseconds_t runtime;
    const bool use_max_runtime = sample_get_runtime_duration(argc, argv, &runtime);

    sb_enumerate_display_modes_result_t display_mode_result;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);

    while (dispatch_events_and_read_msecs(the_app.window, &time)) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);

        int num_fonts_loaded = 0;
        for (int i = 0; i < ARRAY_SIZE(font_files); ++i) {
            const cg_font_async_load_status_e font_status = cg_get_font_load_status(font_files[i]);
            if (font_status == cg_font_async_load_complete) {
                ++num_fonts_loaded;
            }
            VERIFY(font_status >= cg_font_async_load_complete);
        }

        if ((num_fonts_loaded == ARRAY_SIZE(font_files)) && !inited_font_contexts) {
            inited_font_contexts = true;
            fonts.ctx = cg_context_create_font_context(tech_font, 50.0f, 4, MALLOC_TAG);
            fonts.ctx_small = cg_context_create_font_context(tech_font, 30.f, 4, MALLOC_TAG);
            fonts.ctx2 = cg_context_create_font_context(arial_font, 30.0f, 4, MALLOC_TAG);
            fonts.ctx3 = cg_context_create_font_context(dj_font, 120.0f, 4, MALLOC_TAG);
            fonts.ctx3_small = cg_context_create_font_context(dj_font, 60.0f, 4, MALLOC_TAG);
            fonts.ctx4 = cg_context_create_font_context(cjk_font, 30.0f, 4, MALLOC_TAG);
        }

        if (last_time.ms != 0) {
            const milliseconds_t delta_time = {time.ms - last_time.ms};
            publish_metric(metric_type_delta_time_in_ms, &delta_time, sizeof(delta_time));

            if (use_max_runtime) {
                if (runtime.ms < delta_time.ms) {
                    break;
                }
                runtime.ms -= delta_time.ms;
            }

            last_time = time;

            ++num_frames;
            fps_time.ms += delta_time.ms;

            if (fps_time.ms >= 1000) {
                const milliseconds_t ms_per_frame = {fps_time.ms / num_frames};
                LOG_ALWAYS(TAG_FONT_DEMO, "[%4d] FPS: [%dms/frame]", (ms_per_frame.ms > 0) ? 1000 / ms_per_frame.ms : 1000, ms_per_frame.ms);

                render_device_log_resource_tracking(the_app.render_device, the_app.runtime_config.renderer.render_resource_tracking.periodic_logging);

                fps_time.ms = 0;
                num_frames = 0;
            }

            RENDER_ENSURE_WRITE_CMD_STREAM(
                &the_app.render_device->default_cmd_stream,
                render_cmd_buf_write_set_display_size,
                display_mode_result.display_mode.width,
                display_mode_result.display_mode.height,
                MALLOC_TAG);

            RENDER_ENSURE_WRITE_CMD_STREAM(
                &the_app.render_device->default_cmd_stream,
                render_cmd_buf_write_set_viewport,
                0,
                0,
                display_mode_result.display_mode.width,
                display_mode_result.display_mode.height,
                MALLOC_TAG);

            cg_context_begin(delta_time);
            if (inited_font_contexts) {
                cg_canvas_demo_step(&the_app.canvas.context, filtered_dt);
            }
            cg_context_end(MALLOC_TAG);

            RENDER_ENSURE_WRITE_CMD_STREAM(
                &the_app.render_device->default_cmd_stream,
                render_cmd_buf_write_present,
                (rhi_swap_interval_t){0},
                MALLOC_TAG);

            render_device_frame(the_app.render_device);
        } else {
            last_time = time;
        }
    }

    font_demo_shutdown();

    cg_context_font_context_free(fonts.ctx, MALLOC_TAG); // test freeing with explicit free
    cg_context_font_context_free(fonts.ctx_small, MALLOC_TAG);
    cg_context_font_context_free(fonts.ctx2, MALLOC_TAG);
    cg_context_font_context_free(fonts.ctx3, MALLOC_TAG);
    cg_context_font_context_free(fonts.ctx3_small, MALLOC_TAG);
    cg_context_font_context_free(fonts.ctx4, MALLOC_TAG);

    cg_context_font_file_free(cjk_font, MALLOC_TAG);
    cg_context_font_file_free(arial_font, MALLOC_TAG);
    cg_context_font_file_free(tech_font, MALLOC_TAG);
    cg_context_font_file_free(dj_font, MALLOC_TAG);

    return 0;
}

#endif // _MERLIN_DEMOS
