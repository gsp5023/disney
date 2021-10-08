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

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/canvas/private/cg_font.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/adk_sample_common.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_platform.h"

#include <math.h>

#define TAG_FONT_DEMO FOURCC('F', 'O', 'N', 'T')

static struct {
    bool mouse_dragging;
    bool view_atlas;
    bool atlas_squares;
    int32_t mouse_pos_x;
    int32_t mouse_pos_y;
    int32_t mouse_start_pos_x;
    int32_t mouse_start_pos_y;
    int32_t ctrl_key;
    mosaic_context_t * mos_ctx;
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
    cg_context_restore();
}

static void font_demo() {
    const float mouse_pos_x = (float)statics.mouse_pos_x;
    const float mouse_pos_y = (float)statics.mouse_pos_y;
    const float mouse_start_pos_x = (float)statics.mouse_start_pos_x;
    const float mouse_start_pos_y = (float)statics.mouse_start_pos_y;

    const char * const text =
        "This test file can help you examine, how your UTF-8 decoder handles\n"
        "various types of correct, malformed, or otherwise interesting UTF-8\n"
        "sequences. This file is not meant to be a conformance test. It does\n"
        "not prescribe any particular outcome. Therefore, there is no way to\n"
        "'pass' or 'fail' this test file, even though the text does suggest a\n"
        "preferable decoder behaviour at some places. Its aim is, instead, to\n"
        "help you think about and test the behaviour of your UTF-8 decoder on\n"
        "systematic collection of unusual inputs. Experience so far suggests\n"
        "that most first-time authors of UTF-8 decoders find at least one\n"
        "serious problem in their decoder using this file.\n";

    float y = 0;
    cg_context_fill_style((cg_color_t){.r = 255, .g = 200, .b = 190, .a = 255});

    cg_font_metrics_t metrics = cg_context_fill_text(fonts.ctx_small, (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, (char *)text);
    y += metrics.bounds.height;
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255});
    cg_context_stroke_rect(metrics.bounds, MALLOC_TAG);

    cg_context_fill_style((cg_color_t){.r = 55, .g = 200, .b = 190, .a = 255});
    metrics = cg_context_fill_text(fonts.ctx2, (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, "There should be no such thing as boring mathematics. -Edsger Dijkstra");
    y += metrics.bounds.height * 1.25f;

    cg_context_fill_style((cg_color_t){.r = 255, .g = 20, .b = 190, .a = 255});
    metrics = cg_context_fill_text(fonts.ctx3_small, (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, "arcadefire *33");
    y += metrics.bounds.height;

    cg_context_fill_style((cg_color_t){.r = 255, .g = 255, .b = 190, .a = 255});
    metrics = cg_context_fill_text(fonts.ctx4, (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, "加勒比海盗：鬼盗船魔咒");
    y += metrics.bounds.height;

    cg_context_fill_style((cg_color_t){.r = 50, .g = 100, .b = 190, .a = 255});
    const char * const test = "Shoulders of Giants ― Isaac Newton ♜ ♞ ♝ ♛";
    metrics = cg_context_fill_text(fonts.ctx4, (cg_vec2_t){.x = mouse_pos_x, .y = mouse_pos_y + y}, test);
    y += metrics.bounds.height;

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

            if (inited_font_contexts) {
                cg_context_begin(delta_time);
                cg_canvas_demo_step(&the_app.canvas.context, filtered_dt);
                cg_context_end(MALLOC_TAG);
            }

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
