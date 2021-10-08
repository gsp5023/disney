/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
splash.c

Simple rendering of a single cached splash-screen image
*/

#include "source/adk/splash/splash.h"

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/canvas/cg.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/adk_sample_common.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_platform.h"

#define TAG_SPLASH FOURCC('S', 'P', 'L', 'S')

static bool dispatch_events_and_read_msecs(const sb_window_t * const main_window, milliseconds_t * const msec_time) {
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
            case adk_window_event:
                switch (event->event_data.win.event_data.event) {
                    case adk_window_event_close:
                        if (event->event_data.win.window.ptr == main_window) {
                            app_running = false;
                        }
                    default:
                        break;
                }
                break;
            case adk_key_event: {
                if ((event->event_data.key.event == adk_key_event_key_down) && (event->event_data.key.repeat == 0) && (event->event_data.key.key == adk_key_escape)) {
                    app_running = false;
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

static void splash_step(const cg_image_t * image, const int window_width, const int window_height, const milliseconds_t delta_time) {
    cg_context_draw_image_scale(image, (cg_rect_t){.width = (float)window_width, .height = (float)window_height});
}

void splash_main(splash_screen_contents_t splash_screen) {
    adk_app_init_main_display(splash_screen.app_name);

    sb_enumerate_display_modes_result_t display_results;
    VERIFY(sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_results));

    splash_screen.image = cg_context_load_image_async(splash_screen.image_path, cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);

    milliseconds_t time;
    milliseconds_t last_time = {0};
    const milliseconds_t start_time = adk_read_millisecond_clock();
    bool first_frame = true;

    sb_enumerate_display_modes_result_t display_mode_result;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);

    while (dispatch_events_and_read_msecs(the_app.window, &time)) {
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);

        if (last_time.ms != 0) {
            const milliseconds_t delta_time = {time.ms - last_time.ms};
            last_time = time;

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
            const bool splash_is_loaded = cg_get_image_load_status(splash_screen.image) == cg_image_async_load_complete;
            splash_step(splash_screen.image, display_results.display_mode.width, display_results.display_mode.height, delta_time);
            cg_context_end(MALLOC_TAG);

            // Enable vsync to avoid eating up CPU time
            RENDER_ENSURE_WRITE_CMD_STREAM(
                &the_app.render_device->default_cmd_stream,
                render_cmd_buf_write_present,
                (rhi_swap_interval_t){1},
                MALLOC_TAG);

            render_device_frame(the_app.render_device);

            if (first_frame) {
                if (splash_is_loaded) {
                    first_frame = false;
                    flush_render_device(the_app.render_device);
                    sb_notify_app_status(sb_app_notify_dismiss_system_loading_screen);
                } else {
                    const milliseconds_t cur_time = adk_read_millisecond_clock();
                    if ((cur_time.ms - start_time.ms) > 5000) {
                        adk_halt(splash_screen.fallback_error_message);
                    }
                }
            }
        } else {
            last_time = time;
        }
    }

    cg_context_image_free(splash_screen.image, MALLOC_TAG);
}
