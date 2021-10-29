/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_demo.c

Canvas rendering test
*/

#include "source/adk/merlin/drivers/minnie/merlin.h"

#if _MERLIN_DEMOS

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/canvas/cg.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/adk_sample_common.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/telemetry/telemetry.h"

#define TAG_CANVAS_DEMO FOURCC('C', 'V', 'D', 'M')

static const uint32_t clear_colors[5] = {
    // Black,
    0x000000FF,
    // Teal - Manually construct each component
    (0 << 24) + (255 << 16) + (255 << 8) + (255),
    // Rest created via familiar hex codes
    // Cornflower blue
    0x6495edff,
    // White
    0xFFFFFFFF,
    // Forest Green
    0x228B22FF};

static cg_font_context_t * font_ctx = NULL;
static uint8_t current_clear_color_index = 0;

static void tick_clear_color() {
    current_clear_color_index++;
    current_clear_color_index = current_clear_color_index % (sizeof(clear_colors) / sizeof(uint32_t));
    cg_context_set_clear_color(clear_colors[current_clear_color_index]);
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
            case adk_key_event: {
                if ((event->event_data.key.event == adk_key_event_key_down) && (event->event_data.key.repeat == 0) && (event->event_data.key.key == adk_key_escape)) {
                    app_running = false;
                } else if ((event->event_data.key.event == adk_key_event_key_down) && (event->event_data.key.repeat == 0) && (event->event_data.key.key == adk_key_c)) {
                    tick_clear_color();
                }
            } break;
            case adk_stb_input_event: {
                if (event->event_data.stb_input.repeat == 0) {
                    switch (event->event_data.stb_input.stb_key) {
                        case adk_stb_key_back: {
                            app_running = false;
                            break;
                        }
                        case adk_stb_key_blue: {
                            tick_clear_color();
                            break;
                        }
                        default:
                            break;
                    }
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

#define CG_TAU 6.28318530717958647692528676655900576f
#define CG_PI 3.14159265358979323846264338327950288f

typedef struct images_to_draw_t {
    cg_image_t * image;
    cg_image_t * image_gif;
    cg_image_t * image_url;
    cg_image_t * gif_url;
    cg_image_t * rating_grayscale_image;
    cg_image_t * etc1;
    cg_image_t * etc1_url;
    cg_image_t * image_bif;
    cg_image_t * image_alpha;
    cg_image_t * image_alpha_mask;
    cg_image_t * image_gnf;
} images_to_draw_t;

static void cg_canvas_demo_step(const images_to_draw_t images, const int mouse_x, const int mouse_y, const int window_width, const milliseconds_t delta_time) {
    float mx = (float)mouse_x, my = (float)mouse_y;
    (void)mx;
    (void)my;
    //const float px = clamp_float(mx / (float)window_width, 0.0f, 1.0f);
    //  const float py = CG_CLAMP(my/720.0f, 0.0f, 1.0f);
    //  printf("%f %f\n", mx, my);
    // cg_context_translate(mx, my);

    //cg_context_scale(max_float(px * 8.0f, 1.0f), max_float(px * 8.0f, 1.0f));

    cg_context_scale((cg_vec2_t){.x = 2, .y = 2});

    cg_context_draw_image(images.image, (cg_vec2_t){0});

    cg_context_set_feather(40);
    cg_context_begin_path(MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    cg_context_rounded_rect((cg_rect_t){.x = mx, .y = my, .width = 400, .height = 300}, 30, MALLOC_TAG);
    cg_context_fill_style_image((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255}, images.image);
    cg_context_fill(MALLOC_TAG);
    cg_context_set_feather(1);

    //mozilla tests

    //arcs
    cg_context_set_clip_state(cg_clip_state_enabled);
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
    cg_context_set_clip_state(cg_clip_state_disabled);

    // bubble
    cg_context_begin_path(MALLOC_TAG);
    cg_context_move_to((cg_vec2_t){.x = 75, .y = 25}, MALLOC_TAG);
    cg_context_quad_bezier_to(25, 25, 25, 62.5, MALLOC_TAG);
    cg_context_quad_bezier_to(25, 100, 50, 100, MALLOC_TAG);
    cg_context_quad_bezier_to(50, 120, 30, 125, MALLOC_TAG);
    cg_context_quad_bezier_to(60, 120, 65, 100, MALLOC_TAG);
    cg_context_quad_bezier_to(125, 100, 125, 62.5, MALLOC_TAG);
    cg_context_quad_bezier_to(125, 25, 75, 25, MALLOC_TAG);
    cg_context_fill_style_image((cg_color_t){.r = 255, .g = 255, .b = 255, .a = 255}, images.image);
    cg_context_fill_with_options(cg_path_options_concave, MALLOC_TAG);
    cg_context_set_line_width(4);
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 0, .b = 0, .a = 255});
    cg_context_stroke(MALLOC_TAG);

    // rounded rect
    cg_context_set_line_width(2.0);
    cg_context_begin_path(MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 0, .g = 255, .b = 0, .a = 255});
    cg_context_fill_style_image_hex(0xFFF, images.image_gif);
    cg_context_rounded_rect((cg_rect_t){.x = 300, .y = 25, .width = 300, .height = 100}, 20, MALLOC_TAG);
    cg_context_fill(MALLOC_TAG);
    cg_context_stroke(MALLOC_TAG);

    cg_context_save();

    cg_context_set_global_alpha(1.0f);
    cg_context_translate((cg_vec2_t){.x = 175, .y = 25});
    cg_context_fill_style((cg_color_t){.r = 125.f, .g = 125.f, .b = 30.f, .a = 255.f});
    cg_context_sdf_fill_rect_rounded((cg_rect_t){.x = -5, .y = -5, .width = 110, .height = 110}, (cg_sdf_rect_params_t){.roundness = 1000});
    cg_context_fill_style((cg_color_t){.r = 125.f, .g = 30.f, .b = 30.f, .a = 255.f});
    cg_context_scale((cg_vec2_t){1 / 2.f, 1 / 2.f});
    cg_context_sdf_fill_rect_rounded((cg_rect_t){.x = 25, .y = 50, .width = 150, .height = 100}, (cg_sdf_rect_params_t){.roundness = 1000});

    cg_context_restore();

    // face
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

    // clear
    cg_context_translate((cg_vec2_t){.x = 0, .y = 120});
    cg_context_fill_style_hex(0x09F);
    cg_context_fill_rect((cg_rect_t){.x = 25, .y = 25, .width = 100, .height = 100}, MALLOC_TAG);
    cg_context_clear_rect((cg_rect_t){.x = 45, .y = 45, .width = 60, .height = 60}, MALLOC_TAG);
    cg_context_stroke_style((cg_color_t){.r = 255, .g = 255, .b = 0, .a = 255});
    cg_context_stroke_rect((cg_rect_t){.x = 50, .y = 50, .width = 50, .height = 50}, MALLOC_TAG);

    // tris
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

    // global alpha
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

    for (int i = 0; i < 7; i++) {
        cg_context_begin_path(MALLOC_TAG);
        cg_context_arc((cg_vec2_t){.x = 75.f, .y = 75.f}, 10.f + 10.f * i, (cg_rads_t){.rads = 0.f}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_counter_clock_wise, MALLOC_TAG);
        cg_context_fill(MALLOC_TAG);
    }

    static float time = 0.0f;
    time += 0.002f;
    cg_context_save();
    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 890.0f, .y = 150.0f});
    cg_context_rotate((cg_rads_t){.rads = cg_sin(time) * 0.2f});
    cg_context_fill_style((cg_color_t){.r = 255.0f, .g = 0.0f, .b = 128.0f, .a = 255.0f});
    if (font_ctx) {
        cg_context_fill_text_with_options(font_ctx, (cg_vec2_t){.x = 0.0f, .y = 0.0f}, "Dreamcast", cg_font_fill_options_align_center);
    }
    cg_context_restore();

    cg_context_save();
    cg_context_identity();
    cg_context_set_global_alpha(1.0f);
    cg_context_translate((cg_vec2_t){.x = 0.f, .y = 500.f});
    cg_context_draw_image_9slice(images.image_url, (cg_margins_t){.left = 20, .right = 20, .top = 20, .bottom = 20}, cg_context_image_rect(images.image_url));

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 860.f, .y = 420.f});
    cg_context_draw_image(images.gif_url, (cg_vec2_t){0});
    cg_context_identity();
    cg_context_draw_image_scale(images.rating_grayscale_image, (cg_rect_t){.x = 0, .y = 0, .width = 60, .height = 60});

#ifndef _VADER

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 860.f, .y = 320.f});
    cg_rect_t etc1_rect = cg_context_image_rect(images.etc1);
    etc1_rect.width *= 0.35f;
    etc1_rect.height *= 0.35f;
    cg_context_draw_image_scale(images.etc1, etc1_rect);

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 400.f, .y = 220.f});
    cg_rect_t etc1_url_rect = cg_context_image_rect(images.etc1_url);
    etc1_url_rect.width *= 0.35f;
    etc1_url_rect.height *= 0.35f;
    cg_context_draw_image_scale(images.etc1_url, etc1_url_rect);

#endif

    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 60.f, .y = 80.f});
    const uint32_t bif_num_frames = cg_context_get_image_frame_count(images.image_bif);

    static milliseconds_t bif_frame_timer = {0};
    static uint32_t bif_frame_index = 0;
    const milliseconds_t bif_frame_delay = {1000};

    bif_frame_timer.ms += delta_time.ms;
    if (bif_frame_timer.ms > bif_frame_delay.ms) {
        bif_frame_timer.ms = 0;
        bif_frame_index = (bif_frame_index + 1) % bif_num_frames;
    }

    cg_context_set_image_frame_index(images.image_bif, bif_frame_index);
    cg_context_draw_image_scale(images.image_bif, (cg_rect_t){.x = 0, .y = 0, .width = 180, .height = 100});

#if defined(_LEIA)
    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 500.f, .y = 0.f});
    cg_rect_t gnf_rect = cg_context_image_rect(images.image_gnf);
    gnf_rect.width *= 0.35f;
    gnf_rect.height *= 0.35f;
    cg_context_draw_image_scale(images.image_gnf, gnf_rect);
#endif

    // Alpha mask test
    cg_context_identity();
    cg_context_translate((cg_vec2_t){.x = 200.f, .y = 200.f});
    const cg_rect_t alpha_mask_rect = cg_context_image_rect(images.image_alpha);
    cg_context_draw_image_rect_alpha_mask(images.image_alpha, images.image_alpha_mask, alpha_mask_rect, alpha_mask_rect);

    cg_context_restore();
}

static const char * const adk_app_name = "Canvas Demo";

int canvas_demo_main(const int argc, const char * const * const argv) {
    adk_app_init_main_display(adk_app_name);
    sb_enumerate_display_modes_result_t display_results;
    VERIFY(sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_results));

    char asset_path_buff[2 * sb_max_path_length];

    images_to_draw_t images = {
        .image = cg_context_load_image_async(merlin_asset("images/nemo.png", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
        .image_gif = cg_context_load_image_async(merlin_asset("images/starwars.gif", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high_to_low, cg_image_load_opts_none, MALLOC_TAG),
        .gif_url = cg_context_load_image_async("https://prod-ripcut-delivery.disney-plus.net/v1/rawFiles/disney/RAW_C061B00E543326DA345FBF996B4D3D76422B58A49FDEE9AD9A2664618619A8F9", cg_memory_region_high_to_low, cg_image_load_opts_none, MALLOC_TAG),
        .image_url = cg_context_load_image_async("https://lumiere-a.akamaihd.net/v1/images/hb_huludisneyplusespnbundle_logo_dpluscentered_19230_b33a7b2d.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
        .rating_grayscale_image = cg_context_load_image_async("https://prod-ripcut-delivery.disney-plus.net/v1/variant/disney/EF120E7409E77D30C4B49503418C004DFB5A94C9FDFA9E0BA2801D2FC6EC736F/scale?height=23&format=png", cg_memory_region_low, cg_image_load_opts_none, MALLOC_TAG),
#ifndef _VADER
        .etc1 = cg_context_load_image_async(merlin_asset("images/pvr-etc1-alpha.gzip", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
        .etc1_url = cg_context_load_image_async("http://delivery.sdata.bamtech.dev.us-east-1.bamgrid.net/v1/variant/disney/EE1A0C0AAEF3CCF68DA4BE5EB56C1FB6DA3EC21EA1CC9B34B6FB8EF62E187D16/scale?width=400&height=300&scalingAlgorithm=bilinear&format=pvr&library=1&texture=etc1&textureQuality=ETCFast&quality=80&compression=1", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
#endif
#if defined(_LEIA)
        .image_gnf = cg_context_load_image_async("https://qa-ripcut-delivery.disney-plus.net/v1/variant/disney/CFB0563B69DAF82FABA9ADE9AE2450DB87623DDDBB81A69C1C612D600F8A61FC/scale?width=552&library=1&format=gnf", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
#endif // _LEIA || _VADER
        .image_bif = cg_context_load_image_async(merlin_asset("images/roku-moana.bif", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
        .image_alpha = cg_context_load_image_async(merlin_asset("images/aladdin.jpeg", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG),
        .image_alpha_mask = cg_context_load_image_async(merlin_asset("images/rounded.png", asset_path_buff, ARRAY_SIZE(asset_path_buff)), cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG)

    };

    cg_context_set_image_animation_state(images.image_gif, cg_image_animation_running);
    cg_context_set_image_animation_state(images.gif_url, cg_image_animation_running);

    cg_image_t * const some_image = cg_context_load_image_async("https://lumiere-a.akamaihd.net/v1/images/hb_huludisneyplusespnbundle_logo_dpluscentered_19230_b33a7b2d.png", cg_memory_region_high, cg_image_load_opts_none, MALLOC_TAG);
    cg_context_image_free(some_image, MALLOC_TAG);

    cg_font_file_t * const dj_font = cg_context_load_font_file_async("https://dev-config-cd-dmgz.bamgrid.com/static/adk/fonts/dj.ttf", cg_memory_region_low, cg_font_load_opts_none, MALLOC_TAG);

    milliseconds_t time;
    milliseconds_t last_time = {0};
    uint32_t num_frames = 0;
    milliseconds_t fps_time = {0};

    milliseconds_t runtime;
    const bool use_max_runtime = sample_get_runtime_duration(argc, argv, &runtime);

    sb_enumerate_display_modes_result_t display_mode_result;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);

    cg_context_set_clear_color(clear_colors[current_clear_color_index]);

    sb_text_to_speech("Hello World");
    while (dispatch_events_and_read_msecs(the_app.window, &time)) {
        APP_THUNK_TRACE_PUSH_FN();
        adk_curl_run_callbacks();
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);

        if ((cg_get_font_load_status(dj_font) == cg_font_async_load_complete) && !font_ctx) {
            font_ctx = cg_context_create_font_context(dj_font, 80.0f, 4, MALLOC_TAG);
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
                LOG_ALWAYS(TAG_CANVAS_DEMO, "[%4d] FPS: [%dms/frame]", (ms_per_frame.ms > 0) ? 1000 / ms_per_frame.ms : 1000, ms_per_frame.ms);

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
            cg_canvas_demo_step(images, 0, 0, display_results.display_mode.width, delta_time);
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
        APP_THUNK_TRACE_POP();
        TRACE_TICK();
    }

    cg_context_font_context_free(font_ctx, MALLOC_TAG); // test freeing with explicit free
    cg_context_font_file_free(dj_font, MALLOC_TAG);

    cg_context_image_free(images.image_url, MALLOC_TAG);
    cg_context_image_free(images.gif_url, MALLOC_TAG);
    cg_context_image_free(images.image, MALLOC_TAG);
    cg_context_image_free(images.image_gif, MALLOC_TAG);
    cg_context_image_free(images.rating_grayscale_image, MALLOC_TAG);
    cg_context_image_free(images.etc1, MALLOC_TAG);
    cg_context_image_free(images.etc1_url, MALLOC_TAG);
    cg_context_image_free(images.image_bif, MALLOC_TAG);
    cg_context_image_free(images.image_alpha, MALLOC_TAG);
    cg_context_image_free(images.image_alpha_mask, MALLOC_TAG);
#if defined(_LEIA)
    cg_context_image_free(images.image_gnf, MALLOC_TAG);
#endif

    return 0;
}

#endif // _MERLIN_DEMOS
