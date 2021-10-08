/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

FFI_THUNK(0x10, void, nve_enable_debug_logging, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_nve_enable_debug_logging();
})

FFI_THUNK(0x10, void, nve_init, (FFI_WASM_INTERPRETER_STATE runtime), {
    _wamr_thunk_nve_init();
})

FFI_THUNK(0x104, void, nve_event_filtering_enabled, (FFI_WASM_INTERPRETER_STATE runtime, const int32_t is_enabled), {
    _wamr_thunk_nve_event_filtering_enabled(is_enabled);
})

FFI_THUNK(0x108, void, nve_m5_callback_manager_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    _wamr_thunk_nve_m5_callback_manager_release(callback_manager_ptr);
})

FFI_THUNK(0x108, void, nve_m5_challenge_generated_listener_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) challenge_generated_listener_ptr), {
    _wamr_thunk_nve_m5_challenge_generated_listener_release(challenge_generated_listener_ptr);
})

FFI_THUNK(0x108, void, nve_m5_operation_complete_listener_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    _wamr_thunk_nve_m5_operation_complete_listener_release(operation_complete_listener_ptr);
})

FFI_THUNK(0x108, void, nve_m5_view_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view_ptr), {
    _wamr_thunk_nve_m5_view_release(view_ptr);
})

FFI_THUNK(0x108, void, nve_media_player_event_buffer_unbind, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(nve_handle_t) handle), {
    _wamr_thunk_nve_media_player_event_buffer_unbind(handle);
})

FFI_THUNK(0x108, void, nve_media_player_pause, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    _wamr_thunk_nve_media_player_pause(player_ptr);
})

FFI_THUNK(0x108, void, nve_media_player_play, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    _wamr_thunk_nve_media_player_play(player_ptr);
})

FFI_THUNK(0x108, void, nve_media_player_stop, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    _wamr_thunk_nve_media_player_stop(player_ptr);
})

FFI_THUNK(0x108, void, nve_media_resource_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) media_resource_ptr), {
    _wamr_thunk_nve_media_resource_release(media_resource_ptr);
})

FFI_THUNK(0x108, void, nve_payload_ref_add, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) payload_ptr), {
    _wamr_thunk_nve_payload_ref_add(payload_ptr);
})

FFI_THUNK(0x108, void, nve_payload_ref_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) payload_ptr), {
    _wamr_thunk_nve_payload_ref_release(payload_ptr);
})

FFI_THUNK(0x108, void, nve_release_psdk_ref, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) psdk_shared_ptr), {
    _wamr_thunk_nve_release_psdk_ref(psdk_shared_ptr);
})

FFI_THUNK(0x1084, void, nve_media_player_event_buffer_bind, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(nve_handle_t) handle, uint32_t event_buffer_capacity), {
    _wamr_thunk_nve_media_player_event_buffer_bind(handle, event_buffer_capacity);
})

FFI_THUNK(0x1084, void, nve_text_visibility_set, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, const int32_t cc_visibility), {
    _wamr_thunk_nve_text_visibility_set(player_ptr, cc_visibility);
})

FFI_THUNK(0x10844, void, nve_view_set_pos, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view, int32_t x, int32_t y), {
    _wamr_thunk_nve_view_set_pos(view, x, y);
})

FFI_THUNK(0x10844, void, nve_view_set_size, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view, int32_t width, int32_t height), {
    _wamr_thunk_nve_view_set_size(view, width, height);
})

FFI_THUNK(0x1088, void, nve_dispatcher_bind_media_player_events, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    _wamr_thunk_nve_dispatcher_bind_media_player_events(dispatcher_ptr, media_player_ptr);
})

FFI_THUNK(0x1088, void, nve_dispatcher_unbind_media_player_events, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    _wamr_thunk_nve_dispatcher_unbind_media_player_events(dispatcher_ptr, media_player_ptr);
})

FFI_THUNK(0x1088, void, nve_media_player_set_view, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) view_ptr), {
    _wamr_thunk_nve_media_player_set_view(player_ptr, view_ptr);
})

FFI_THUNK(0x10888, void, nve_media_player_replace_resource, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) resource_ptr, FFI_NATIVE_PTR(void *) config_ptr), {
    _wamr_thunk_nve_media_player_replace_resource(player_ptr, resource_ptr, config_ptr);
})

FFI_THUNK(0x108A, void, nve_drm_error_text_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) operation_complete_ptr, FFI_WASM_PTR const out_drm_error), {
    _wamr_thunk_nve_drm_error_text_get(operation_complete_ptr, out_drm_error);
})

FFI_THUNK(0x108A, void, nve_media_player_buffered_range_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    _wamr_thunk_nve_media_player_buffered_range_get(player_ptr, out_range);
})

FFI_THUNK(0x108A, void, nve_media_player_event_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const payload), {
    _wamr_thunk_nve_media_player_event_get(handle, payload);
})

FFI_THUNK(0x108A, void, nve_media_player_get_abr_params, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_media_player_get_abr_params(player_ptr, params);
})

FFI_THUNK(0x108A, void, nve_media_player_get_buffer_params, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_media_player_get_buffer_params(player_ptr, params);
})

FFI_THUNK(0x108A, void, nve_media_player_get_text_style, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    _wamr_thunk_nve_media_player_get_text_style(player_ptr, cc_style);
})

FFI_THUNK(0x108A, void, nve_media_player_playback_range_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    _wamr_thunk_nve_media_player_playback_range_get(player_ptr, out_range);
})

FFI_THUNK(0x108A, void, nve_media_player_set_abr_params, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_media_player_set_abr_params(player_ptr, params);
})

FFI_THUNK(0x108A, void, nve_media_player_set_buffer_params, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_media_player_set_buffer_params(player_ptr, params);
})

FFI_THUNK(0x108A, void, nve_media_player_set_text_style, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    _wamr_thunk_nve_media_player_set_text_style(player_ptr, cc_style);
})

FFI_THUNK(0x108A, void, nve_mpi_config_subscribe_tag, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) config_ptr, FFI_WASM_PTR const tag_name), {
    _wamr_thunk_nve_mpi_config_subscribe_tag(config_ptr, tag_name);
})

FFI_THUNK(0x108AA, void, nve_m5_challenge_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) challenge_generated_event_payload_ptr, FFI_WASM_PTR const out_buffer_ptr, FFI_WASM_PTR const out_size_ptr), {
    _wamr_thunk_nve_m5_challenge_get(challenge_generated_event_payload_ptr, out_buffer_ptr, out_size_ptr);
})

FFI_THUNK(0x108AAAA, void, nve_m5_cenc_init_data_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size, FFI_WASM_PTR const method, FFI_WASM_PTR const tag), {
    _wamr_thunk_nve_m5_cenc_init_data_get(handle, out_ptr, out_size, method, tag);
})

FFI_THUNK(0x108D, void, nve_media_player_seek, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, double position_ms), {
    _wamr_thunk_nve_media_player_seek(player_ptr, position_ms);
})

FFI_THUNK(0x108F, void, nve_media_player_prepare_to_play, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, float position), {
    _wamr_thunk_nve_media_player_prepare_to_play(player_ptr, position);
})

FFI_THUNK(0x10A, void, nve_abr_params_init, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_abr_params_init(params);
})

FFI_THUNK(0x10A, void, nve_buffer_params_init, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_buffer_params_init(params);
})

FFI_THUNK(0x10A, void, nve_set_local_storage_path, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const storage_path), {
    _wamr_thunk_nve_set_local_storage_path(storage_path);
})

FFI_THUNK(0x10A, void, nve_text_format_init, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const params), {
    _wamr_thunk_nve_text_format_init(params);
})

FFI_THUNK(0x10A, void, nve_version_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const out_version), {
    _wamr_thunk_nve_version_get(out_version);
})

FFI_THUNK(0x408, FFI_ENUM(nve_network_request_state_e), nve_network_request_get_state, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle), {
    return _wamr_thunk_nve_network_request_get_state(request_handle);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_release(player_ptr);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_reset, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_reset(player_ptr);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_restore, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_restore(player_ptr);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_suspend, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_suspend(player_ptr);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_execute, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle), {
    return _wamr_thunk_nve_network_request_execute(request_handle);
})

FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_release, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle), {
    return _wamr_thunk_nve_network_request_release(request_handle);
})

FFI_THUNK(0x408, int32_t, nve_audio_num_tracks_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_audio_num_tracks_get(player_ptr);
})

FFI_THUNK(0x408, int32_t, nve_metadata_num_keys_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) metadata_ptr), {
    return _wamr_thunk_nve_metadata_num_keys_get(metadata_ptr);
})

FFI_THUNK(0x408, int32_t, nve_profile_num_profiles_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_profile_num_profiles_get(player_ptr);
})

FFI_THUNK(0x408, int32_t, nve_text_has_tracks, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_text_has_tracks(player_ptr);
})

FFI_THUNK(0x408, int32_t, nve_text_num_tracks_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_text_num_tracks_get(player_ptr);
})

FFI_THUNK(0x408, int32_t, nve_text_visibility_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_text_visibility_get(player_ptr);
})

FFI_THUNK(0x408, int32_t, nve_view_get_height, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view), {
    return _wamr_thunk_nve_view_get_height(view);
})

FFI_THUNK(0x408, int32_t, nve_view_get_width, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view), {
    return _wamr_thunk_nve_view_get_width(view);
})

FFI_THUNK(0x408, int32_t, nve_view_get_x, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view), {
    return _wamr_thunk_nve_view_get_x(view);
})

FFI_THUNK(0x408, int32_t, nve_view_get_y, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) view), {
    return _wamr_thunk_nve_view_get_y(view);
})

FFI_THUNK(0x408, uint32_t, nve_network_request_result_code_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle), {
    return _wamr_thunk_nve_network_request_result_code_get(request_handle);
})

FFI_THUNK(0x4084, int32_t, nve_audio_track_set, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    return _wamr_thunk_nve_audio_track_set(player_ptr, track_id);
})

FFI_THUNK(0x4084, int32_t, nve_text_track_set, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    return _wamr_thunk_nve_text_track_set(player_ptr, track_id);
})

FFI_THUNK(0x4084A, int32_t, nve_audio_track_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    return _wamr_thunk_nve_audio_track_get(player_ptr, track_id, out_track);
})

FFI_THUNK(0x4084A, int32_t, nve_metadata_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) metadata_ptr, int32_t metadata_index, FFI_WASM_PTR const out_metadata), {
    return _wamr_thunk_nve_metadata_get(metadata_ptr, metadata_index, out_metadata);
})

FFI_THUNK(0x4084A, int32_t, nve_profile_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, int32_t profile_index, FFI_WASM_PTR const out_profile), {
    return _wamr_thunk_nve_profile_get(player_ptr, profile_index, out_profile);
})

FFI_THUNK(0x4084A, int32_t, nve_text_track_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    return _wamr_thunk_nve_text_track_get(player_ptr, track_id, out_track);
})

FFI_THUNK(0x4088, int32_t, nve_metadata_event_capture, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) metadata_ptr, FFI_NATIVE_PTR(void *) event_ptr), {
    return _wamr_thunk_nve_metadata_event_capture(metadata_ptr, event_ptr);
})

FFI_THUNK(0x408844, int32_t, nve_text_set_custom_font, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void * const) font_bytes, uint32_t len, uint32_t uncompressed_len), {
    return _wamr_thunk_nve_text_set_custom_font(player_ptr, font_bytes, len, uncompressed_len);
})

FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_media_player_item_network_configuration_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const config), {
    return _wamr_thunk_nve_media_player_item_network_configuration_get(player_ptr, config);
})

FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_network_request_error_data_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_error_data), {
    return _wamr_thunk_nve_network_request_error_data_get(request_handle, out_error_data);
})

FFI_THUNK(0x408A, int32_t, nve_audio_current_track_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    return _wamr_thunk_nve_audio_current_track_get(player_ptr, out_track);
})

FFI_THUNK(0x408A, int32_t, nve_current_profile_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_profile), {
    return _wamr_thunk_nve_current_profile_get(player_ptr, out_profile);
})

FFI_THUNK(0x408A, int32_t, nve_load_information_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) event_ptr, FFI_WASM_PTR const out_load_information), {
    return _wamr_thunk_nve_load_information_get(event_ptr, out_load_information);
})

FFI_THUNK(0x408A, int32_t, nve_text_current_track_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    return _wamr_thunk_nve_text_current_track_get(player_ptr, out_track);
})

FFI_THUNK(0x408A4, FFI_ENUM(nve_result_e), nve_network_request_payload_set, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const data_ptr, uint32_t data_size), {
    return _wamr_thunk_nve_network_request_payload_set(request_handle, data_ptr, data_size);
})

FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_generate_challenge, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return _wamr_thunk_nve_drm_manager_generate_challenge(drm_manager, bytes, len, listener);
})

FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_certificate_bytes, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return _wamr_thunk_nve_drm_manager_store_certificate_bytes(drm_manager, bytes, len, listener);
})

FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_license_bytes, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return _wamr_thunk_nve_drm_manager_store_license_bytes(drm_manager, bytes, len, listener);
})

FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_body_data_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    return _wamr_thunk_nve_network_request_body_data_get(request_handle, out_ptr, out_size);
})

FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_data_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    return _wamr_thunk_nve_network_request_header_data_get(request_handle, out_ptr, out_size);
})

FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_set, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const header_name, FFI_WASM_PTR const header_data), {
    return _wamr_thunk_nve_network_request_header_set(request_handle, header_name, header_data);
})

FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_capabilities_get, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const caps), {
    return _wamr_thunk_nve_capabilities_get(caps);
})

FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_initialize_drm, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const payload), {
    return _wamr_thunk_nve_initialize_drm(payload);
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_m5_callback_manager_create, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_nve_m5_callback_manager_create();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_metadata_create, (FFI_WASM_INTERPRETER_STATE runtime), {
    return _wamr_thunk_nve_metadata_create();
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_dispatcher_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    return _wamr_thunk_nve_dispatcher_create(callback_manager_ptr);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_m5_operation_complete_listener_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_m5_operation_complete_listener_create(player_ptr);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) dispatcher_ptr), {
    return _wamr_thunk_nve_media_player_create(dispatcher_ptr);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_drm_manager, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_get_drm_manager(player_ptr);
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_view, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_get_view(player_ptr);
})

FFI_THUNK(0x8084444, FFI_NATIVE_PTR(void *), nve_view_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(struct cg_context_t * const) ctx, int32_t x, int32_t y, int32_t w, int32_t h), {
    return _wamr_thunk_nve_view_create(ctx, x, y, w, h);
})

FFI_THUNK(0x80888, FFI_NATIVE_PTR(void *), nve_m5_challenge_generated_listener_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) drm_manager_ptr, FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    return _wamr_thunk_nve_m5_challenge_generated_listener_create(player_ptr, drm_manager_ptr, operation_complete_listener_ptr);
})

FFI_THUNK(0x80A, FFI_NATIVE_PTR(void *), nve_mpi_config_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const network_configuration), {
    return _wamr_thunk_nve_mpi_config_create(network_configuration);
})

FFI_THUNK(0x80A4, FFI_NATIVE_PTR(void *), nve_network_request_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const url, FFI_ENUM(nve_network_method_e) method), {
    return _wamr_thunk_nve_network_request_create(url, method);
})

FFI_THUNK(0x80A48, FFI_NATIVE_PTR(void *), nve_media_resource_create, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const url, FFI_ENUM(nve_media_resource_type_e) media_resource_type, FFI_NATIVE_PTR(void *) metadata_ptr), {
    return _wamr_thunk_nve_media_resource_create(url, media_resource_type, metadata_ptr);
})

FFI_THUNK(0xD08, double, nve_media_player_position, (FFI_WASM_INTERPRETER_STATE runtime, FFI_NATIVE_PTR(void *) player_ptr), {
    return _wamr_thunk_nve_media_player_position(player_ptr);
})

