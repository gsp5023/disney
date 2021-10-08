/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

FFI_THUNK(0x10, void, adk_analytics_create_dictionary, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_adk_analytics_create_dictionary();
})

FFI_THUNK(0x10, void, adk_analytics_destroy_dictionary, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_adk_analytics_destroy_dictionary();
})

FFI_THUNK(0x10, void, adk_app_request_restart, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_adk_app_request_restart();
})

FFI_THUNK(0x10, void, adk_enter_background_mode, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_adk_enter_background_mode();
})

FFI_THUNK(0x10, void, adk_leave_background_mode, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_adk_leave_background_mode();
})

FFI_THUNK(0x10, void, cg_context_begin_path, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_begin_path();
})

FFI_THUNK(0x10, void, cg_context_close_path, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_close_path();
})

FFI_THUNK(0x10, void, cg_context_end_path, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_end_path();
})

FFI_THUNK(0x10, void, cg_context_fill, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_fill();
})

FFI_THUNK(0x10, void, cg_context_font_clear_glyph_cache, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_font_clear_glyph_cache();
})

FFI_THUNK(0x10, void, cg_context_identity, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_identity();
})

FFI_THUNK(0x10, void, cg_context_restore, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_restore();
})

FFI_THUNK(0x10, void, cg_context_save, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_save();
})

FFI_THUNK(0x10, void, cg_context_stroke, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_cg_context_stroke();
})

FFI_THUNK(0x104, void, adk_analytics_ad_end, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_ad_end(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_ad_start, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_ad_start(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_cleanup_session, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_cleanup_session(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_detach_player, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_detach_player(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_remove_session, (FFI_WASM_INTERPRETER_STATE runtime, int32_t session_id), {
    _wamr_thunk_adk_analytics_remove_session(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_seek_start, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_seek_start(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_set_buffering, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_set_buffering(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_set_default_bitrate, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t bitrate), {
    _wamr_thunk_adk_analytics_set_default_bitrate(bitrate);
})

FFI_THUNK(0x104, void, adk_analytics_set_device_type, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const Device) value), {
    _wamr_thunk_adk_analytics_set_device_type(value);
})

FFI_THUNK(0x104, void, adk_analytics_set_duration, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t duration), {
    _wamr_thunk_adk_analytics_set_duration(duration);
})

FFI_THUNK(0x104, void, adk_analytics_set_enable_player_state_inference, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t state), {
    _wamr_thunk_adk_analytics_set_enable_player_state_inference(state);
})

FFI_THUNK(0x104, void, adk_analytics_set_heartbeat_interval, (FFI_WASM_INTERPRETER_STATE runtime, const uint32_t interval), {
    _wamr_thunk_adk_analytics_set_heartbeat_interval(interval);
})

FFI_THUNK(0x104, void, adk_analytics_set_is_live, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t live), {
    _wamr_thunk_adk_analytics_set_is_live(live);
})

FFI_THUNK(0x104, void, adk_analytics_set_log_level, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const adk_analytics_log_e) level), {
    _wamr_thunk_adk_analytics_set_log_level(level);
})

FFI_THUNK(0x104, void, adk_analytics_set_paused, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_set_paused(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_set_player_poll_interval, (FFI_WASM_INTERPRETER_STATE runtime, const uint32_t interval), {
    _wamr_thunk_adk_analytics_set_player_poll_interval(interval);
})

FFI_THUNK(0x104, void, adk_analytics_set_playing, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_set_playing(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_set_stopped, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_set_stopped(session_id);
})

FFI_THUNK(0x104, void, adk_analytics_update_content_info, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    _wamr_thunk_adk_analytics_update_content_info(session_id);
})

FFI_THUNK(0x104, void, adk_dump_heap_usage, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const adk_dump_heap_flags_e) heaps_to_dump), {
    _wamr_thunk_adk_dump_heap_usage(heaps_to_dump);
})

FFI_THUNK(0x104, void, adk_notify_app_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_app_notify_e) notify), {
    _wamr_thunk_adk_notify_app_status(notify);
})

FFI_THUNK(0x104, void, adk_set_memory_mode, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const adk_memory_mode_e) memory_mode), {
    _wamr_thunk_adk_set_memory_mode(memory_mode);
})

FFI_THUNK(0x104, void, cg_context_fill_style_hex, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t color), {
    _wamr_thunk_cg_context_fill_style_hex(color);
})

FFI_THUNK(0x104, void, cg_context_fill_with_options, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const cg_path_options_e) options), {
    _wamr_thunk_cg_context_fill_with_options(options);
})

FFI_THUNK(0x104, void, cg_context_set_clip_state, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const cg_clip_state_e) clip_state), {
    _wamr_thunk_cg_context_set_clip_state(clip_state);
})

FFI_THUNK(0x104, void, cg_context_set_punchthrough_blend_mode, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const cg_blend_mode_e) blend_mode), {
    _wamr_thunk_cg_context_set_punchthrough_blend_mode(blend_mode);
})

FFI_THUNK(0x104, void, cg_context_stroke_with_options, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const cg_path_options_e) options), {
    _wamr_thunk_cg_context_stroke_with_options(options);
})

FFI_THUNK(0x1044, void, adk_analytics_set_session_bitrate, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const int32_t bitrate), {
    _wamr_thunk_adk_analytics_set_session_bitrate(session_id, bitrate);
})

FFI_THUNK(0x1044, void, adk_analytics_set_session_duration, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const int32_t duration), {
    _wamr_thunk_adk_analytics_set_session_duration(session_id, duration);
})

FFI_THUNK(0x1044, void, adk_analytics_set_session_encoded_framerate, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const int32_t framerate), {
    _wamr_thunk_adk_analytics_set_session_encoded_framerate(session_id, framerate);
})

FFI_THUNK(0x10444, void, adk_analytics_set_session_video_size, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const int32_t width, const int32_t height), {
    _wamr_thunk_adk_analytics_set_session_video_size(session_id, width, height);
})

FFI_THUNK(0x1044A, void, adk_init_main_display, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t display_index, const int32_t display_mode_index, FFI_WASM_PTR const adk_app_name), {
    _wamr_thunk_adk_init_main_display(display_index, display_mode_index, adk_app_name);
})

FFI_THUNK(0x1044AA, void, adk_analytics_set_session_stream, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const int32_t bitrate_kbps, FFI_WASM_PTR const cdn, FFI_WASM_PTR const resource), {
    _wamr_thunk_adk_analytics_set_session_stream(session_id, bitrate_kbps, cdn, resource);
})

FFI_THUNK(0x1048, void, adk_analytics_attach_player, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, FFI_NATIVE_PTR(void *) player), {
    _wamr_thunk_adk_analytics_attach_player(session_id, player);
})

FFI_THUNK(0x1048, void, cg_context_fill_style_image_hex, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t color, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    _wamr_thunk_cg_context_fill_style_image_hex(color, image);
})

FFI_THUNK(0x104A4, void, adk_analytics_session_report_error, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, FFI_WASM_PTR const error_msg, const int32_t is_fatal), {
    _wamr_thunk_adk_analytics_session_report_error(session_id, error_msg, is_fatal);
})

FFI_THUNK(0x104AA, void, adk_stat, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) mount_point, FFI_WASM_PTR const subpath, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_stat(mount_point, subpath, ret_val);
})

FFI_THUNK(0x104F, void, adk_analytics_seek_end, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, const float player_seek_end_pos), {
    _wamr_thunk_adk_analytics_seek_end(session_id, player_seek_end_pos);
})

FFI_THUNK(0x108, void, adk_curl_close_handle, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle), {
    _wamr_thunk_adk_curl_close_handle(handle);
})

FFI_THUNK(0x108, void, adk_curl_slist_free_all, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_slist_t * const) list), {
    _wamr_thunk_adk_curl_slist_free_all(list);
})

FFI_THUNK(0x108, void, adk_httpx_client_free, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_client_t * const) client), {
    _wamr_thunk_adk_httpx_client_free(client);
})

FFI_THUNK(0x108, void, adk_httpx_response_free, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_response_t * const) response), {
    _wamr_thunk_adk_httpx_response_free(response);
})

FFI_THUNK(0x108, void, adk_release_deeplink, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(sb_deeplink_handle_t * const) handle), {
    _wamr_thunk_adk_release_deeplink(handle);
})

FFI_THUNK(0x108, void, adk_release_screenshot, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_screenshot_t * const) screenshot), {
    _wamr_thunk_adk_release_screenshot(screenshot);
})

FFI_THUNK(0x108, void, adk_websocket_close, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    _wamr_thunk_adk_websocket_close(ws_handle);
})

FFI_THUNK(0x108, void, adk_websocket_end_read, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    _wamr_thunk_adk_websocket_end_read(ws_handle);
})

FFI_THUNK(0x108, void, cg_context_font_context_free, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t *) font), {
    _wamr_thunk_cg_context_font_context_free(font);
})

FFI_THUNK(0x108, void, cg_context_font_file_free, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_file_t * const) font), {
    _wamr_thunk_cg_context_font_file_free(font);
})

FFI_THUNK(0x108, void, cg_context_image_free, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_image_t * const) image), {
    _wamr_thunk_cg_context_image_free(image);
})

FFI_THUNK(0x108, void, cg_set_context, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_context_t * const) ctx), {
    _wamr_thunk_cg_set_context(ctx);
})

FFI_THUNK(0x108, void, json_deflate_http_future_drop, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(json_deflate_http_future_t * const) future), {
    _wamr_thunk_json_deflate_http_future_drop(future);
})

FFI_THUNK(0x1084, void, adk_curl_set_buffering_mode, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_handle_buffer_mode_e) buffer_mode), {
    _wamr_thunk_adk_curl_set_buffering_mode(handle, buffer_mode);
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_follow_location, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, const int32_t follow_location), {
    _wamr_thunk_adk_httpx_request_set_follow_location(request, follow_location);
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_preferred_receive_buffer_size, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, uint32_t preferred_receive_buffer_size), {
    _wamr_thunk_adk_httpx_request_set_preferred_receive_buffer_size(request, preferred_receive_buffer_size);
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_verbose, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, const int32_t verbose), {
    _wamr_thunk_adk_httpx_request_set_verbose(request, verbose);
})

FFI_THUNK(0x1084, void, cg_context_set_image_animation_state, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_image_t * const) image, FFI_ENUM(const cg_image_animation_state_e) image_animation_state), {
    _wamr_thunk_cg_context_set_image_animation_state(image, image_animation_state);
})

FFI_THUNK(0x1084, void, cg_context_set_image_frame_index, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_image_t * const) cg_image, const uint32_t image_index), {
    _wamr_thunk_cg_context_set_image_frame_index(cg_image, image_index);
})

FFI_THUNK(0x10844, void, adk_curl_set_opt_i32, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, int32_t arg), {
    _wamr_thunk_adk_curl_set_opt_i32(handle, opt, arg);
})

FFI_THUNK(0x10844, void, cg_context_image_set_repeat, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_image_t * const) image, const int32_t repeat_x, const int32_t repeat_y), {
    _wamr_thunk_cg_context_image_set_repeat(image, repeat_x, repeat_y);
})

FFI_THUNK(0x10844A, void, adk_save_screenshot, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_screenshot_t * const) screenshot, FFI_ENUM(const image_save_file_type_e) file_type, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    _wamr_thunk_adk_save_screenshot(screenshot, file_type, directory, filename);
})

FFI_THUNK(0x10848, void, adk_curl_set_opt_slist, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, FFI_NATIVE_PTR(adk_curl_slist_t * const) arg), {
    _wamr_thunk_adk_curl_set_opt_slist(handle, opt, arg);
})

FFI_THUNK(0x1084A, void, adk_curl_set_opt_ptr, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, FFI_WASM_PTR const arg), {
    _wamr_thunk_adk_curl_set_opt_ptr(handle, opt, arg);
})

FFI_THUNK(0x1088, void, adk_httpx_request_set_timeout, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, uint64_t timeout), {
    _wamr_thunk_adk_httpx_request_set_timeout(request, timeout);
})

FFI_THUNK(0x1088444A, void, adk_screenshot_dump_deltas, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_screenshot_t * const) testcase_screenshot, FFI_NATIVE_PTR(adk_screenshot_t * const) baseline_screenshot, const int32_t image_tolerance, FFI_ENUM(const image_save_file_type_e) file_type, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename_prefix), {
    _wamr_thunk_adk_screenshot_dump_deltas(testcase_screenshot, baseline_screenshot, image_tolerance, file_type, directory, filename_prefix);
})

FFI_THUNK(0x1088AA, void, cg_context_draw_image_rect_alpha_mask, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_NATIVE_PTR(const cg_image_t * const) mask, FFI_WASM_PTR const src, FFI_WASM_PTR const dst), {
    _wamr_thunk_cg_context_draw_image_rect_alpha_mask(image, mask, src, dst);
})

FFI_THUNK(0x108A, void, adk_curl_get_http_body, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_curl_get_http_body(handle, ret_val);
})

FFI_THUNK(0x108A, void, adk_curl_get_http_header, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_curl_get_http_header(handle, ret_val);
})

FFI_THUNK(0x108A, void, adk_get_deeplink_buffer, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const sb_deeplink_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_get_deeplink_buffer(handle, ret_val);
})

FFI_THUNK(0x108A, void, adk_httpx_request_set_header, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const header), {
    _wamr_thunk_adk_httpx_request_set_header(request, header);
})

FFI_THUNK(0x108A, void, adk_rand_create_generator_with_seed, (FFI_WASM_INTERPRETER_STATE runtime, const uint64_t seed, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_rand_create_generator_with_seed(seed, ret_val);
})

FFI_THUNK(0x108A, void, cg_context_draw_image, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const pos), {
    _wamr_thunk_cg_context_draw_image(image, pos);
})

FFI_THUNK(0x108A, void, cg_context_draw_image_scale, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_draw_image_scale(image, rect);
})

FFI_THUNK(0x108A, void, cg_context_font_precache_glyphs, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const characters), {
    _wamr_thunk_cg_context_font_precache_glyphs(font_ctx, characters);
})

FFI_THUNK(0x108A, void, cg_context_image_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_image_rect(image, ret_val);
})

FFI_THUNK(0x108A, void, cg_context_set_font_context_missing_glyph_indicator, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const missing_glyph_indicator), {
    _wamr_thunk_cg_context_set_font_context_missing_glyph_indicator(font_ctx, missing_glyph_indicator);
})

FFI_THUNK(0x108A, void, json_deflate_http_future_get_result, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_json_deflate_http_future_get_result(future, ret_val);
})

FFI_THUNK(0x108A4, void, adk_httpx_request_set_body, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const body, int32_t body_size), {
    _wamr_thunk_adk_httpx_request_set_body(request, body, body_size);
})

FFI_THUNK(0x108A8A8444, void, json_deflate_parse_httpx_resize, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(json_deflate_http_future_t * const) future, FFI_WASM_PTR const schema_layout, uint64_t schema_layout_size, FFI_WASM_PTR const buffer, uint64_t buffer_size, FFI_ENUM(const json_deflate_parse_target_e) target, const uint32_t expected_size, const uint32_t schema_hash), {
    _wamr_thunk_json_deflate_parse_httpx_resize(future, schema_layout, schema_layout_size, buffer, buffer_size, target, expected_size, schema_hash);
})

FFI_THUNK(0x108AA, void, cg_context_draw_image_9slice, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const margin, FFI_WASM_PTR const dst), {
    _wamr_thunk_cg_context_draw_image_9slice(image, margin, dst);
})

FFI_THUNK(0x108AA, void, cg_context_draw_image_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const src, FFI_WASM_PTR const dst), {
    _wamr_thunk_cg_context_draw_image_rect(image, src, dst);
})

FFI_THUNK(0x108AA, void, cg_context_text_measure, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_text_measure(font_ctx, text, ret_val);
})

FFI_THUNK(0x108AA4A, void, cg_context_fill_text_with_options, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const pos, FFI_WASM_PTR const text, FFI_ENUM(const cg_font_fill_options_e) options, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_fill_text_with_options(font_ctx, pos, text, options, ret_val);
})

FFI_THUNK(0x108AAA, void, adk_curl_async_perform_raw, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const on_http_recv_header_ptr, FFI_WASM_PTR const on_http_recv_ptr, FFI_WASM_PTR const on_complete_ptr), {
    _wamr_thunk_adk_curl_async_perform_raw(handle, on_http_recv_header_ptr, on_http_recv_ptr, on_complete_ptr);
})

FFI_THUNK(0x108AAA, void, cg_context_fill_text, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const pos, FFI_WASM_PTR const text, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_fill_text(font_ctx, pos, text, ret_val);
})

FFI_THUNK(0x108AFFA4A, void, cg_context_get_text_block_page_offsets, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text_rect, const float scroll_offset, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_ENUM(const cg_text_block_options_e) options, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_get_text_block_page_offsets(font_ctx, text_rect, scroll_offset, extra_line_spacing, text, options, ret_val);
})

FFI_THUNK(0x108AFFAA4A, void, cg_context_fill_text_block_with_options, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text_rect, const float text_scroll_offset, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_WASM_PTR const optional_ellipses, FFI_ENUM(const cg_text_block_options_e) options, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_cg_context_fill_text_block_with_options(font_ctx, text_rect, text_scroll_offset, extra_line_spacing, text, optional_ellipses, options, ret_val);
})

FFI_THUNK(0x108F, void, cg_context_font_set_virtual_size, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t *) font_ctx, float size), {
    _wamr_thunk_cg_context_font_set_virtual_size(font_ctx, size);
})

FFI_THUNK(0x10A, void, adk_analytics_init_client, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const consumer_key), {
    _wamr_thunk_adk_analytics_init_client(consumer_key);
})

FFI_THUNK(0x10A, void, adk_analytics_notify_network_connection_type, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const _type), {
    _wamr_thunk_adk_analytics_notify_network_connection_type(_type);
})

FFI_THUNK(0x10A, void, adk_analytics_notify_network_wifi_link_encryption, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const _type), {
    _wamr_thunk_adk_analytics_notify_network_wifi_link_encryption(_type);
})

FFI_THUNK(0x10A, void, adk_analytics_set_asset_name, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name), {
    _wamr_thunk_adk_analytics_set_asset_name(name);
})

FFI_THUNK(0x10A, void, adk_analytics_set_default_cdn, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name), {
    _wamr_thunk_adk_analytics_set_default_cdn(name);
})

FFI_THUNK(0x10A, void, adk_analytics_set_default_resource, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name), {
    _wamr_thunk_adk_analytics_set_default_resource(name);
})

FFI_THUNK(0x10A, void, adk_analytics_set_device_brand, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_device_brand(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_device_manufacturer, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_device_manufacturer(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_device_model, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_device_model(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_device_version, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_device_version(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_framework_name, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_framework_name(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_framework_version, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_framework_version(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_gateway_url, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const url), {
    _wamr_thunk_adk_analytics_set_gateway_url(url);
})

FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_name, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_operating_system_name(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_version, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_operating_system_version(value);
})

FFI_THUNK(0x10A, void, adk_analytics_set_player_name, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name), {
    _wamr_thunk_adk_analytics_set_player_name(name);
})

FFI_THUNK(0x10A, void, adk_analytics_set_stream_url, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const url), {
    _wamr_thunk_adk_analytics_set_stream_url(url);
})

FFI_THUNK(0x10A, void, adk_analytics_set_viewer_id, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const id), {
    _wamr_thunk_adk_analytics_set_viewer_id(id);
})

FFI_THUNK(0x10A, void, adk_generate_uuid, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_generate_uuid(ret_val);
})

FFI_THUNK(0x10A, void, adk_get_cpu_mem_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_get_cpu_mem_status(ret_val);
})

FFI_THUNK(0x10A, void, adk_get_locale, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_get_locale(ret_val);
})

FFI_THUNK(0x10A, void, adk_get_system_metrics_native, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const out), {
    _wamr_thunk_adk_get_system_metrics_native(out);
})

FFI_THUNK(0x10A, void, adk_get_wasm_heap_usage, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_get_wasm_heap_usage(ret_val);
})

FFI_THUNK(0x10A, void, adk_http_set_proxy, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const proxy), {
    _wamr_thunk_adk_http_set_proxy(proxy);
})

FFI_THUNK(0x10A, void, adk_http_set_socks, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const socks), {
    _wamr_thunk_adk_http_set_socks(socks);
})

FFI_THUNK(0x10A, void, adk_log_msg, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const msg), {
    _wamr_thunk_adk_log_msg(msg);
})

FFI_THUNK(0x10A, void, adk_rand_create_generator, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_rand_create_generator(ret_val);
})

FFI_THUNK(0x10A, void, adk_read_microsecond_clock, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_read_microsecond_clock(ret_val);
})

FFI_THUNK(0x10A, void, adk_read_millisecond_clock, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_read_millisecond_clock(ret_val);
})

FFI_THUNK(0x10A, void, adk_read_nanosecond_clock, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_read_nanosecond_clock(ret_val);
})

FFI_THUNK(0x10A, void, adk_text_to_speech, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const text), {
    _wamr_thunk_adk_text_to_speech(text);
})

FFI_THUNK(0x10A, void, cg_context_blit_video_frame, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_blit_video_frame(rect);
})

FFI_THUNK(0x10A, void, cg_context_clear_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_clear_rect(rect);
})

FFI_THUNK(0x10A, void, cg_context_fill_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_fill_rect(rect);
})

FFI_THUNK(0x10A, void, cg_context_fill_style, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const color), {
    _wamr_thunk_cg_context_fill_style(color);
})

FFI_THUNK(0x10A, void, cg_context_line_to, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const pos), {
    _wamr_thunk_cg_context_line_to(pos);
})

FFI_THUNK(0x10A, void, cg_context_move_to, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const pos), {
    _wamr_thunk_cg_context_move_to(pos);
})

FFI_THUNK(0x10A, void, cg_context_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_rect(rect);
})

FFI_THUNK(0x10A, void, cg_context_rotate, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const angle), {
    _wamr_thunk_cg_context_rotate(angle);
})

FFI_THUNK(0x10A, void, cg_context_scale, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const scale), {
    _wamr_thunk_cg_context_scale(scale);
})

FFI_THUNK(0x10A, void, cg_context_set_clip_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_set_clip_rect(rect);
})

FFI_THUNK(0x10A, void, cg_context_set_global_missing_glyph_indicator, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const missing_glyph_indicator), {
    _wamr_thunk_cg_context_set_global_missing_glyph_indicator(missing_glyph_indicator);
})

FFI_THUNK(0x10A, void, cg_context_stroke_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect), {
    _wamr_thunk_cg_context_stroke_rect(rect);
})

FFI_THUNK(0x10A, void, cg_context_stroke_style, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const color), {
    _wamr_thunk_cg_context_stroke_style(color);
})

FFI_THUNK(0x10A, void, cg_context_translate, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const translation), {
    _wamr_thunk_cg_context_translate(translation);
})

FFI_THUNK(0x10A448A, void, adk_fwrite, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const buffer, int32_t elem_size, int32_t elem_count, FFI_NATIVE_PTR(sb_file_t * const) file, FFI_WASM_PTR const ret_val), {
    _wamr_thunk_adk_fwrite(buffer, elem_size, elem_count, file, ret_val);
})

FFI_THUNK(0x10A484A4444A, void, json_deflate_native_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_NATIVE_PTR(const uint8_t * const) json_data, uint32_t json_data_length, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    _wamr_thunk_json_deflate_native_async(schema_layout, schema_layout_length, json_data, json_data_length, buffer, buffer_size, expected_size, schema_hash, target, on_complete);
})

FFI_THUNK(0x10A48A4444A, void, json_deflate_from_http_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_NATIVE_PTR(adk_curl_handle_t * const) http, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    _wamr_thunk_json_deflate_from_http_async(schema_layout, schema_layout_length, http, buffer, buffer_size, expected_size, schema_hash, target, on_complete);
})

FFI_THUNK(0x10A4A4A4444A, void, json_deflate_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_WASM_PTR const json_data, uint32_t json_data_length, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    _wamr_thunk_json_deflate_async(schema_layout, schema_layout_length, json_data, json_data_length, buffer, buffer_size, expected_size, schema_hash, target, on_complete);
})

FFI_THUNK(0x10A8, void, cg_context_fill_style_image, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const color, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    _wamr_thunk_cg_context_fill_style_image(color, image);
})

FFI_THUNK(0x10A844, void, memcpy_n2r, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const dst, FFI_NATIVE_PTR(const void * const) src, const int32_t offset, const int32_t num_bytes), {
    _wamr_thunk_memcpy_n2r(dst, src, offset, num_bytes);
})

FFI_THUNK(0x10AA, void, adk_analytics_add_event_to_dictionary, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const key, FFI_WASM_PTR const event_name), {
    _wamr_thunk_adk_analytics_add_event_to_dictionary(key, event_name);
})

FFI_THUNK(0x10AA, void, adk_analytics_set_tag, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const key, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_analytics_set_tag(key, value);
})

FFI_THUNK(0x10AA, void, adk_coredump_add_data, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_coredump_add_data(name, value);
})

FFI_THUNK(0x10AA, void, adk_coredump_add_data_public, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    _wamr_thunk_adk_coredump_add_data_public(name, value);
})

FFI_THUNK(0x10AAA, void, adk_report_app_metrics, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const app_id, FFI_WASM_PTR const app_name, FFI_WASM_PTR const app_version), {
    _wamr_thunk_adk_report_app_metrics(app_id, app_name, app_version);
})

FFI_THUNK(0x10AAF, void, cg_context_arc_to, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const pos1, FFI_WASM_PTR const pos2, const float radius), {
    _wamr_thunk_cg_context_arc_to(pos1, pos2, radius);
})

FFI_THUNK(0x10AF, void, cg_context_rounded_rect, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const rect, const float radius), {
    _wamr_thunk_cg_context_rounded_rect(rect, radius);
})

FFI_THUNK(0x10AFAA4, void, cg_context_arc, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const pos, const float radius, FFI_WASM_PTR const start, FFI_WASM_PTR const end, FFI_ENUM(const cg_rotation_e) rotation), {
    _wamr_thunk_cg_context_arc(pos, radius, start, end, rotation);
})

FFI_THUNK(0x10F, void, adk_analytics_notify_network_signal_strength, (FFI_WASM_INTERPRETER_STATE runtime, const float strength), {
    _wamr_thunk_adk_analytics_notify_network_signal_strength(strength);
})

FFI_THUNK(0x10F, void, cg_context_set_alpha_test_threshold, (FFI_WASM_INTERPRETER_STATE runtime, const float threshold), {
    _wamr_thunk_cg_context_set_alpha_test_threshold(threshold);
})

FFI_THUNK(0x10F, void, cg_context_set_feather, (FFI_WASM_INTERPRETER_STATE runtime, const float feather), {
    _wamr_thunk_cg_context_set_feather(feather);
})

FFI_THUNK(0x10F, void, cg_context_set_global_alpha, (FFI_WASM_INTERPRETER_STATE runtime, const float alpha), {
    _wamr_thunk_cg_context_set_global_alpha(alpha);
})

FFI_THUNK(0x10F, void, cg_context_set_line_width, (FFI_WASM_INTERPRETER_STATE runtime, const float width), {
    _wamr_thunk_cg_context_set_line_width(width);
})

FFI_THUNK(0x10FFFF, void, cg_context_quad_bezier_to, (FFI_WASM_INTERPRETER_STATE runtime, const float cpx, const float cpy, const float x, const float y), {
    _wamr_thunk_cg_context_quad_bezier_to(cpx, cpy, x, y);
})

FFI_THUNK(0x40, FFI_ENUM(Device), adk_analytics_get_device_type, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_device_type();
})

FFI_THUNK(0x40, FFI_ENUM(adk_analytics_log_e), adk_analytics_get_log_level, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_log_level();
})

FFI_THUNK(0x40, FFI_ENUM(adk_app_state_e), adk_get_app_state, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_get_app_state();
})

FFI_THUNK(0x40, FFI_ENUM(cg_blend_mode_e), cg_context_get_punchthrough_blend_mode, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_cg_context_get_punchthrough_blend_mode();
})

FFI_THUNK(0x40, int32_t, adk_analytics_create_session, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_create_session();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_buffer_length, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_buffer_length();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_default_bitrate, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_default_bitrate();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_duration, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_duration();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_enable_player_state_inference, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_enable_player_state_inference();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_is_live, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_is_live();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_min_buffer_length, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_min_buffer_length();
})

FFI_THUNK(0x40, int32_t, adk_analytics_get_playahead_time, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_playahead_time();
})

FFI_THUNK(0x40, uint32_t, adk_analytics_get_heartbeat_interval, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_heartbeat_interval();
})

FFI_THUNK(0x40, uint32_t, adk_analytics_get_player_poll_interval, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_player_poll_interval();
})

FFI_THUNK(0x404, int32_t, adk_get_supported_refresh_rate, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t starting_refresh_rate), {
    return _wamr_thunk_adk_get_supported_refresh_rate(starting_refresh_rate);
})

FFI_THUNK(0x4044, int32_t, adk_set_refresh_rate, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t refresh_rate, const int32_t video_fps), {
    return _wamr_thunk_adk_set_refresh_rate(refresh_rate, video_fps);
})

FFI_THUNK(0x4044A, int32_t, adk_enumerate_display_modes, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t display_index, const int32_t display_mode_index, FFI_WASM_PTR const out_results), {
    return _wamr_thunk_adk_enumerate_display_modes(display_index, display_mode_index, out_results);
})

FFI_THUNK(0x404A, FFI_ENUM(sb_directory_delete_error_e), adk_delete_directory, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const subpath), {
    return _wamr_thunk_adk_delete_directory(directory, subpath);
})

FFI_THUNK(0x404A, int32_t, adk_analytics_send_session_event, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id, FFI_WASM_PTR const event_name), {
    return _wamr_thunk_adk_analytics_send_session_event(session_id, event_name);
})

FFI_THUNK(0x404A, int32_t, adk_create_directory_path, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const input_path), {
    return _wamr_thunk_adk_create_directory_path(directory, input_path);
})

FFI_THUNK(0x404A, int32_t, adk_delete_file, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    return _wamr_thunk_adk_delete_file(directory, filename);
})

FFI_THUNK(0x408, FFI_ENUM(adk_curl_handle_buffer_mode_e), adk_curl_get_buffering_mode, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle), {
    return _wamr_thunk_adk_curl_get_buffering_mode(handle);
})

FFI_THUNK(0x408, FFI_ENUM(adk_future_status_e), adk_httpx_response_get_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_status(response);
})

FFI_THUNK(0x408, FFI_ENUM(adk_future_status_e), json_deflate_http_future_get_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future), {
    return _wamr_thunk_json_deflate_http_future_get_status(future);
})

FFI_THUNK(0x408, FFI_ENUM(adk_httpx_result_e), adk_httpx_response_get_result, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_result(response);
})

FFI_THUNK(0x408, FFI_ENUM(adk_websocket_status_e), adk_websocket_get_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    return _wamr_thunk_adk_websocket_get_status(ws_handle);
})

FFI_THUNK(0x408, FFI_ENUM(cg_font_async_load_status_e), cg_get_font_load_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_font_file_t * const) cg_font), {
    return _wamr_thunk_cg_get_font_load_status(cg_font);
})

FFI_THUNK(0x408, FFI_ENUM(cg_image_async_load_status_e), cg_get_image_load_status, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    return _wamr_thunk_cg_get_image_load_status(image);
})

FFI_THUNK(0x408, int32_t, adk_fclose, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(sb_file_t * const) file), {
    return _wamr_thunk_adk_fclose(file);
})

FFI_THUNK(0x408, int32_t, adk_feof, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(sb_file_t * const) file), {
    return _wamr_thunk_adk_feof(file);
})

FFI_THUNK(0x408, int32_t, adk_httpx_client_tick, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_client_t * const) client), {
    return _wamr_thunk_adk_httpx_client_tick(client);
})

FFI_THUNK(0x408, int32_t, cg_get_image_ripcut_error_code, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    return _wamr_thunk_cg_get_image_ripcut_error_code(image);
})

FFI_THUNK(0x408, int32_t, cstrlen, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const char * const) str), {
    return _wamr_thunk_cstrlen(str);
})

FFI_THUNK(0x408, uint32_t, adk_httpx_response_get_body_size, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_body_size(response);
})

FFI_THUNK(0x408, uint32_t, adk_httpx_response_get_headers_size, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_headers_size(response);
})

FFI_THUNK(0x408, uint32_t, cg_context_get_image_frame_count, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) cg_image), {
    return _wamr_thunk_cg_context_get_image_frame_count(cg_image);
})

FFI_THUNK(0x4084A, FFI_ENUM(adk_curl_result_e), adk_curl_get_info_i32, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_info_e) info, FFI_WASM_PTR const out), {
    return _wamr_thunk_adk_curl_get_info_i32(handle, info, out);
})

FFI_THUNK(0x40884, int32_t, adk_screenshot_compare, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_screenshot_t * const) testcase_screenshot, FFI_NATIVE_PTR(const adk_screenshot_t * const) baseline_screenshot, const int32_t image_tolerance), {
    return _wamr_thunk_adk_screenshot_compare(testcase_screenshot, baseline_screenshot, image_tolerance);
})

FFI_THUNK(0x408A, FFI_ENUM(adk_websocket_message_type_e), adk_websocket_begin_read_bridge, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle, FFI_WASM_PTR const out_buffer), {
    return _wamr_thunk_adk_websocket_begin_read_bridge(ws_handle, out_buffer);
})

FFI_THUNK(0x408A4, uint32_t, adk_httpx_response_get_body_copy, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response, FFI_WASM_PTR const buffer, uint32_t buffer_size), {
    return _wamr_thunk_adk_httpx_response_get_body_copy(response, buffer, buffer_size);
})

FFI_THUNK(0x408A4, uint32_t, adk_httpx_response_get_headers_copy, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response, FFI_WASM_PTR const buffer, uint32_t buffer_size), {
    return _wamr_thunk_adk_httpx_response_get_headers_copy(response, buffer, buffer_size);
})

FFI_THUNK(0x408A44AA, FFI_ENUM(adk_websocket_status_e), adk_websocket_send_raw, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle, FFI_WASM_PTR const ptr, const int32_t size, FFI_ENUM(const adk_websocket_message_type_e) message_type, FFI_WASM_PTR const success_callback, FFI_WASM_PTR const error_callback), {
    return _wamr_thunk_adk_websocket_send_raw(ws_handle, ptr, size, message_type, success_callback, error_callback);
})

FFI_THUNK(0x40A, int32_t, adk_analytics_send_event, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const event_name), {
    return _wamr_thunk_adk_analytics_send_event(event_name);
})

FFI_THUNK(0x40A, uint32_t, adk_crc_str_32, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const str), {
    return _wamr_thunk_adk_crc_str_32(str);
})

FFI_THUNK(0x40A44, int32_t, read_events, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const evbuffer, int32_t bufsize, int32_t sizeof_event), {
    return _wamr_thunk_read_events(evbuffer, bufsize, sizeof_event);
})

FFI_THUNK(0x40A448, int32_t, adk_fread, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const buffer, int32_t elem_size, int32_t elem_count, FFI_NATIVE_PTR(sb_file_t * const) file), {
    return _wamr_thunk_adk_fread(buffer, elem_size, elem_count, file);
})

FFI_THUNK(0x40A48, int32_t, strcpy_n2r, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const dst, const int32_t dst_buff_size, FFI_NATIVE_PTR(const char * const) src), {
    return _wamr_thunk_strcpy_n2r(dst, dst_buff_size, src);
})

FFI_THUNK(0x40A484, int32_t, strcpy_n2r_upto, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const dst, const int32_t dst_buff_size, FFI_NATIVE_PTR(const char * const) src, const int32_t max_copy_len), {
    return _wamr_thunk_strcpy_n2r_upto(dst, dst_buff_size, src, max_copy_len);
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_curl_handle_t *), adk_curl_open_handle, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_curl_open_handle();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_httpx_client_t *), adk_httpx_client_new, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_httpx_client_new();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_screenshot_t *), adk_capture_screenshot, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_capture_screenshot();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(cg_context_t *), cg_get_context, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_cg_get_context();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_asset_name, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_asset_name();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_cdn, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_default_cdn();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_resource, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_default_resource();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_getway_url, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_getway_url();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_player_name, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_player_name();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_stream_url, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_stream_url();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_viewer_id, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_viewer_id();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_brand, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_device_brand();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_manufacturer, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_device_manufacturer();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_model, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_device_model();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_version, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_device_version();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_name, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_framework_name();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_version, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_framework_version();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_name, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_operating_system_name();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_version, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_operating_system_version();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_type, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_player_type();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_version, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_player_version();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_get_wasm_call_stack, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_get_wasm_call_stack();
})

FFI_THUNK(0x80, uint64_t, adk_get_milliseconds_since_epoch, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_get_milliseconds_since_epoch();
})

FFI_THUNK(0x804, FFI_NATIVE_PTR(void *), adk_analytics_get_attached_player, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t session_id), {
    return _wamr_thunk_adk_analytics_get_attached_player(session_id);
})

FFI_THUNK(0x804A, FFI_NATIVE_PTR(adk_screenshot_t *), adk_load_screenshot, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    return _wamr_thunk_adk_load_screenshot(directory, filename);
})

FFI_THUNK(0x804AA, FFI_NATIVE_PTR(sb_file_t *), adk_fopen, (FFI_WASM_INTERPRETER_STATE runtime, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const path, FFI_WASM_PTR const mode), {
    return _wamr_thunk_adk_fopen(directory, path, mode);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(adk_httpx_response_t *), adk_httpx_send, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_httpx_request_t * const) request), {
    return _wamr_thunk_adk_httpx_send(request);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(adk_httpx_response_t *), json_deflate_http_future_get_response, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future), {
    return _wamr_thunk_json_deflate_http_future_get_response(future);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(const char *), adk_httpx_response_get_error, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_error(response);
})

FFI_THUNK(0x808, int64_t, adk_httpx_response_get_response_code, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    return _wamr_thunk_adk_httpx_response_get_response_code(response);
})

FFI_THUNK(0x80844, FFI_NATIVE_PTR(const cg_pattern_t *), cg_context_pattern, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const cg_image_t * const) image, const int32_t repeat_x, const int32_t repeat_y), {
    return _wamr_thunk_cg_context_pattern(image, repeat_x, repeat_y);
})

FFI_THUNK(0x8084A, FFI_NATIVE_PTR(adk_httpx_request_t *), adk_httpx_client_request, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(const adk_httpx_client_t * const) client, FFI_ENUM(const adk_httpx_method_e) method, FFI_WASM_PTR const url), {
    return _wamr_thunk_adk_httpx_client_request(client, method, url);
})

FFI_THUNK(0x808A, FFI_NATIVE_PTR(adk_curl_slist_t *), adk_curl_slist_append, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_curl_slist_t *) list, FFI_WASM_PTR const sz), {
    return _wamr_thunk_adk_curl_slist_append(list, sz);
})

FFI_THUNK(0x808AA, FFI_NATIVE_PTR(adk_http_header_list_t *), adk_http_append_header_list, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(adk_http_header_list_t *) list, FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    return _wamr_thunk_adk_http_append_header_list(list, name, value);
})

FFI_THUNK(0x808F4, FFI_NATIVE_PTR(cg_font_context_t *), cg_context_create_font_context, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_file_t * const) cg_font, const float size, const int32_t tab_space_multiplier), {
    return _wamr_thunk_cg_context_create_font_context(cg_font, size, tab_space_multiplier);
})

FFI_THUNK(0x80A, FFI_NATIVE_PTR(const char *), adk_analytics_get_tag, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const key), {
    return _wamr_thunk_adk_analytics_get_tag(key);
})

FFI_THUNK(0x80A, FFI_NATIVE_PTR(const char *), adk_get_env, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const env_name), {
    return _wamr_thunk_adk_get_env(env_name);
})

FFI_THUNK(0x80A, uint64_t, adk_rand_next, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const generator), {
    return _wamr_thunk_adk_rand_next(generator);
})

FFI_THUNK(0x80A44, FFI_NATIVE_PTR(cg_font_file_t *), cg_context_load_font_file_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const filepath, FFI_ENUM(const cg_memory_region_e) memory_region, FFI_ENUM(const cg_font_load_opts_e) font_load_opts), {
    return _wamr_thunk_cg_context_load_font_file_async(filepath, memory_region, font_load_opts);
})

FFI_THUNK(0x80A44, FFI_NATIVE_PTR(cg_image_t *), cg_context_load_image_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const file_location, FFI_ENUM(const cg_memory_region_e) memory_region, FFI_ENUM(const cg_image_load_opts_e) image_load_opts), {
    return _wamr_thunk_cg_context_load_image_async(file_location, memory_region, image_load_opts);
})

FFI_THUNK(0x80A88A8444, FFI_NATIVE_PTR(json_deflate_http_future_t *), json_deflate_parse_httpx_async, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const schema_layout, uint64_t schema_layout_size, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const buffer, uint64_t buffer_size, FFI_ENUM(const json_deflate_parse_target_e) target, const uint32_t expected_size, const uint32_t schema_hash), {
    return _wamr_thunk_json_deflate_parse_httpx_async(schema_layout, schema_layout_size, request, buffer, buffer_size, target, expected_size, schema_hash);
})

FFI_THUNK(0x80AA8AA, FFI_NATIVE_PTR(adk_websocket_handle_t *), adk_websocket_create_raw, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const url, FFI_WASM_PTR const supported_protocols, FFI_NATIVE_PTR(adk_http_header_list_t * const) header_list, FFI_WASM_PTR const success_callback, FFI_WASM_PTR const error_callback), {
    return _wamr_thunk_adk_websocket_create_raw(url, supported_protocols, header_list, success_callback, error_callback);
})

FFI_THUNK(0xF0, float, adk_analytics_get_rendered_framerate, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_adk_analytics_get_rendered_framerate();
})

FFI_THUNK(0xF0, float, cg_context_feather, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_cg_context_feather();
})

FFI_THUNK(0xF0, float, cg_context_get_alpha_test_threshold, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_cg_context_get_alpha_test_threshold();
})

FFI_THUNK(0xF0, float, cg_context_global_alpha, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_cg_context_global_alpha();
})

FFI_THUNK(0xF08FFA4, float, cg_context_get_text_block_height, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, const float line_width, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_ENUM(const cg_text_block_options_e) options), {
    return _wamr_thunk_cg_context_get_text_block_height(font_ctx, line_width, extra_line_spacing, text, options);
})

