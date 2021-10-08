/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
hello_cube.c

Basic OGLES2 app that draws a rotating square on the screen.
*/

#include "source/adk/merlin/drivers/minnie/merlin.h"

#if _MERLIN_DEMOS

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/adk_sample_common.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/steamboat/sb_platform.h"

#include <math.h>

#define TAG_CUBE FOURCC('C', 'U', 'B', 'E')

#include "source/shaders/compiled/hello_cube.frag.gen.h"
#include "source/shaders/compiled/hello_cube.vert.gen.h"

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

static const char * const adk_app_name = "HelloCube";

int hello_cube_main(const int argc, const char * const * const argv) {
    adk_app_init_main_display(adk_app_name);
    sb_notify_app_status(sb_app_notify_dismiss_system_loading_screen);

    milliseconds_t time;
    uint32_t ticks;

#define CUBE_SCALE 0.5

    static const float verts[8][4] = {
        {-CUBE_SCALE, CUBE_SCALE, 0, 1}, /* upper left */
        {1, 0, 0, 1}, /* red */
        {-CUBE_SCALE, -CUBE_SCALE, 0, 1}, /* lower left */
        {0, 1, 0, 1}, /* green */
        {CUBE_SCALE, -CUBE_SCALE, 0, 1}, /* lower right */
        {0, 0, 1, 1}, /* blue */
        {CUBE_SCALE, CUBE_SCALE, 0, 1}, /* upper right */
        {0, 1, 1, 1}, /* yellow */
    };

    static const const_mem_region_t vertex_buffer = {
        {.ptr = verts},
        .size = sizeof(verts)};

    static const uint16_t indices[6] = {
        /* two ccw wound triangles */
        0,
        1,
        2,
        0,
        2,
        3};

    static const const_mem_region_t indices_buffer = {
        {.ptr = indices},
        .size = sizeof(indices)};

    static const rhi_index_layout_desc_t index_layout = {
        .format = rhi_vertex_index_format_uint16,
        .usage = rhi_usage_default};

    static const rhi_vertex_element_desc_t vertex_elements[] = {
        {.offset = 0,
         .count = 4,
         .format = rhi_vertex_element_format_float,
         .semantic = rhi_program_input_semantic_position0},
        {.offset = 16,
         .count = 4,
         .format = rhi_vertex_element_format_float,
         .semantic = rhi_program_input_semantic_color0}};

    static const rhi_vertex_layout_desc_t channel_layouts[] = {
        {.elements = vertex_elements,
         .num_elements = ARRAY_SIZE(vertex_elements),
         .stride = sizeof(float) * 8,
         .usage = rhi_usage_default}};

    static const rhi_mesh_data_layout_desc_t mesh_data_layout_desc = {
        .indices = &index_layout,
        .channels = channel_layouts,
        .num_channels = ARRAY_SIZE(channel_layouts)};

    r_program_t * const program = render_create_program_from_binary(the_app.render_device, hello_cube_vert_program, hello_cube_frag_program, MALLOC_TAG);

    // go ahead and flush right here to see if this compiles without errors
    flush_render_device(the_app.render_device);
    if (program->error) {
        LOG_ERROR(0, rhi_get_error_message(program->error));
        TRAP("shader program error");
    }

    // create render-states
    r_rasterizer_state_t * const rasterizer_state = render_create_rasterizer_state(
        the_app.render_device,
        (rhi_rasterizer_state_desc_t){
            .cull_mode = rhi_cull_face_none,
            .scissor_test = false},
        MALLOC_TAG);

    r_depth_stencil_state_t * const depth_stencil_state = render_create_depth_stencil_state(
        the_app.render_device,
        (rhi_depth_stencil_state_desc_t){
            .depth_test = false,
            .depth_write_mask = false,
            .depth_test_func = rhi_compare_always,
            .stencil_test = false,
            .stencil_read_mask = 0xff,
            .stencil_write_mask = 0xff,
            .stencil_front = (rhi_stencil_op_desc_t){
                .stencil_fail_op = rhi_stencil_op_keep,
                .depth_fail_op = rhi_stencil_op_keep,
                .stencil_depth_pass_op = rhi_stencil_op_keep,
                .stencil_test_func = rhi_compare_always},
            .stencil_back = (rhi_stencil_op_desc_t){.stencil_fail_op = rhi_stencil_op_keep, .depth_fail_op = rhi_stencil_op_keep, .stencil_depth_pass_op = rhi_stencil_op_keep, .stencil_test_func = rhi_compare_always}},
        MALLOC_TAG);

    r_blend_state_t * const blend_state = render_create_blend_state(
        the_app.render_device,
        (rhi_blend_state_desc_t){
            .blend_enable = false,
            .src_blend = rhi_blend_one,
            .dst_blend = rhi_blend_zero,
            .blend_op = rhi_blend_op_add,
            .src_blend_alpha = rhi_blend_one,
            .dst_blend_alpha = rhi_blend_zero,
            .blend_op_alpha = rhi_blend_op_add,
            .blend_color = {1, 1, 1, 1},
            .color_write_mask = rhi_color_write_mask_all},
        MALLOC_TAG);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        &rasterizer_state->rasterizer_state,
        &depth_stencil_state->depth_stencil_state,
        &blend_state->blend_state,
        0,
        MALLOC_TAG);

    rasterizer_state->resource.fence = depth_stencil_state->resource.fence = blend_state->resource.fence = render_get_cmd_stream_fence(&the_app.render_device->default_cmd_stream);

    // mesh data layout describes vertex data format

    r_mesh_data_layout_t * const mesh_data_layout = render_create_mesh_data_layout(
        the_app.render_device,
        mesh_data_layout_desc,
        MALLOC_TAG);

    // create the mesh
    r_mesh_t * const mesh = render_create_mesh(
        the_app.render_device,
        (rhi_mesh_data_init_indirect_t){
            .indices = indices_buffer,
            .channels = &vertex_buffer,
            .num_channels = 1,
            .num_indices = 6},
        mesh_data_layout,
        MALLOC_TAG);

    // create uniform buffer to hold mvp matrix
    r_uniform_buffer_t * const mvp_ub = render_create_uniform_buffer(
        the_app.render_device,
        (rhi_uniform_buffer_desc_t){
            .size = 16 * sizeof(float),
            .usage = rhi_usage_dynamic},
        NULL,
        MALLOC_TAG);

    // bind the uniform buffer to mvp
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_write_set_uniform_binding,
        (rhi_uniform_binding_indirect_t){
            .buffer = &mvp_ub->uniform_buffer,
            .index = rhi_program_uniform_mvp},
        MALLOC_TAG);

    // set the program
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &the_app.render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &program->program,
        MALLOC_TAG);

    ticks = 0;
    uint32_t num_frames = 0;
    milliseconds_t fps_time = {0};

    milliseconds_t runtime;
    const bool use_max_runtime = sample_get_runtime_duration(argc, argv, &runtime);

    sb_enumerate_display_modes_result_t display_mode_result;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);

    if (dispatch_events_and_read_msecs(the_app.window, &time)) {
        milliseconds_t last_time = time;
        while (dispatch_events_and_read_msecs(the_app.window, &time)) {
            const milliseconds_t delta_time = {time.ms - last_time.ms};
            publish_metric(metric_type_delta_time_in_ms, &delta_time, sizeof(delta_time));

            if (use_max_runtime) {
                if (runtime.ms < delta_time.ms) {
                    break;
                }
                runtime.ms -= delta_time.ms;
            }

            last_time = time;
            if (delta_time.ms > 0) {
                float s, c;
                float mvp[16];
                float z;
                float aspect = 1;

                ZEROMEM(&mvp);

                ticks += delta_time.ms;

                ++num_frames;
                fps_time.ms += delta_time.ms;

                if (fps_time.ms >= 1000) {
                    const milliseconds_t ms_per_frame = {fps_time.ms / num_frames};
                    LOG_ALWAYS(TAG_CUBE, "[%4d] FPS: [%dms/frame]", (ms_per_frame.ms > 0) ? 1000 / ms_per_frame.ms : 1000, ms_per_frame.ms);

                    fps_time.ms = 0;
                    num_frames = 0;
                }

                z = ticks / 10.f * DEG2RAD;
                s = sinf(z);
                c = cosf(z);

                mvp[0] = c * aspect;
                mvp[1] = s;
                mvp[4] = -s * aspect;
                mvp[5] = c;
                mvp[10] = 1;
                mvp[15] = 1;

                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_set_display_size,
                    display_mode_result.display_mode.width,
                    display_mode_result.display_mode.height,
                    MALLOC_TAG);

                // upload mvp
                render_cmd_stream_upload_uniform_buffer_data(
                    &the_app.render_device->default_cmd_stream,
                    mvp_ub,
                    CONST_MEM_REGION(.ptr = mvp, .size = sizeof(mvp)),
                    0,
                    MALLOC_TAG);

                // set viewport
                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_set_viewport,
                    0,
                    0,
                    display_mode_result.display_mode.width,
                    display_mode_result.display_mode.height,
                    MALLOC_TAG);

                // clear screen
                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_clear_screen_cds,
                    (render_clear_color_t){
                        .r = 0, .g = 0, .b = 0, .a = 0},
                    (render_clear_depth_t){
                        .depth = FLT_MAX},
                    (render_clear_stencil_t){
                        .stencil = 0},
                    MALLOC_TAG);

                // render mesh into back buffer
                struct {
                    int elem_count;
                    rhi_mesh_t ** mesh;
                } * draw_single_mesh;

                render_cmd_stream_blocking_latch_cmd_buf(&the_app.render_device->default_cmd_stream);
                render_mark_cmd_buf(the_app.render_device->default_cmd_stream.buf);
                draw_single_mesh = render_cmd_buf_unchecked_alloc(the_app.render_device->default_cmd_stream.buf, 8, sizeof(*draw_single_mesh));

                if (!draw_single_mesh || !render_cmd_buf_write_draw_indirect(the_app.render_device->default_cmd_stream.buf, (rhi_draw_params_indirect_t){.mesh_list = PEDANTIC_CAST(rhi_mesh_t * const * const *) & draw_single_mesh->mesh, .idx_ofs = NULL, .elm_counts = &draw_single_mesh->elem_count, .num_meshes = 1, .mode = rhi_triangles}, MALLOC_TAG)) {
                    render_unwind_cmd_buf(the_app.render_device->default_cmd_stream.buf);
                    render_flush_cmd_stream(&the_app.render_device->default_cmd_stream, render_no_wait);
                    render_cmd_stream_blocking_latch_cmd_buf(&the_app.render_device->default_cmd_stream);
                    draw_single_mesh = render_cmd_buf_alloc(the_app.render_device->default_cmd_stream.buf, 8, sizeof(*draw_single_mesh));
                    TRAP_OUT_OF_MEMORY(draw_single_mesh);
                    VERIFY(render_cmd_buf_write_draw_indirect(
                        the_app.render_device->default_cmd_stream.buf,
                        (rhi_draw_params_indirect_t){
                            .mesh_list = PEDANTIC_CAST(rhi_mesh_t * const * const *) & draw_single_mesh->mesh,
                            .idx_ofs = NULL,
                            .elm_counts = &draw_single_mesh->elem_count,
                            .num_meshes = 1,
                            .mode = rhi_triangles},
                        MALLOC_TAG));
                }

                draw_single_mesh->elem_count = 6;
                draw_single_mesh->mesh = &mesh->mesh;

                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_present,
                    (rhi_swap_interval_t){0},
                    MALLOC_TAG);

                render_device_frame(the_app.render_device);
            }
        }
    }

    LOG_ALWAYS(TAG_CUBE, "Hello Cube Exiting");

    render_release(&rasterizer_state->resource, MALLOC_TAG);
    render_release(&depth_stencil_state->resource, MALLOC_TAG);
    render_release(&blend_state->resource, MALLOC_TAG);
    render_release(&program->resource, MALLOC_TAG);
    render_release(&mesh_data_layout->resource, MALLOC_TAG);
    render_release(&mesh->resource, MALLOC_TAG);
    render_release(&mvp_ub->resource, MALLOC_TAG);
    return 0;
}

#endif // _MERLIN_DEMOS
