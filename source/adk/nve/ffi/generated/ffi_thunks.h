/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

#ifdef _NVE
FFI_THUNK(0x10A, void, nve_abr_params_init, (FFI_WASM_PTR const params), {
    FFI_ASSERT_ALIGNED_WASM_PTR(params, nve_abr_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_abr_params_init(FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x10A, void, nve_abr_params_init, (FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, int32_t, nve_audio_current_track_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_track, nve_audio_track_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_track), "out_track cannot be NULL");
    return nve_audio_current_track_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(out_track));
})
#else
FFI_THUNK(0x408A, int32_t, nve_audio_current_track_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_audio_num_tracks_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_audio_num_tracks_get(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_audio_num_tracks_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084A, int32_t, nve_audio_track_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_track, nve_audio_track_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_track), "out_track cannot be NULL");
    return nve_audio_track_get(FFI_GET_NATIVE_PTR(void *, player_ptr), track_id, FFI_PIN_WASM_PTR(out_track));
})
#else
FFI_THUNK(0x4084A, int32_t, nve_audio_track_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084, int32_t, nve_audio_track_set, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_audio_track_set(FFI_GET_NATIVE_PTR(void *, player_ptr), track_id);
})
#else
FFI_THUNK(0x4084, int32_t, nve_audio_track_set, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10A, void, nve_buffer_params_init, (FFI_WASM_PTR const params), {
    FFI_ASSERT_ALIGNED_WASM_PTR(params, nve_buffer_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_buffer_params_init(FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x10A, void, nve_buffer_params_init, (FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_capabilities_get, (FFI_WASM_PTR const caps), {
    FFI_ASSERT_ALIGNED_WASM_PTR(caps, nve_capabilities_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(caps), "caps cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_capabilities_get(FFI_PIN_WASM_PTR(caps));
})
#else
FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_capabilities_get, (FFI_WASM_PTR const caps), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, int32_t, nve_current_profile_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_profile), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_profile, nve_profile_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_profile), "out_profile cannot be NULL");
    return nve_current_profile_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(out_profile));
})
#else
FFI_THUNK(0x408A, int32_t, nve_current_profile_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_profile), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x1088, void, nve_dispatcher_bind_media_player_events, (FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, media_player_ptr), "media_player_ptr cannot be NULL");
    nve_dispatcher_bind_media_player_events(FFI_GET_NATIVE_PTR(void *, dispatcher_ptr), FFI_GET_NATIVE_PTR(void *, media_player_ptr));
})
#else
FFI_THUNK(0x1088, void, nve_dispatcher_bind_media_player_events, (FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_dispatcher_create, (FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, callback_manager_ptr), "callback_manager_ptr cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_dispatcher_create(FFI_GET_NATIVE_PTR(void *, callback_manager_ptr)));
})
#else
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_dispatcher_create, (FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x1088, void, nve_dispatcher_unbind_media_player_events, (FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, media_player_ptr), "media_player_ptr cannot be NULL");
    nve_dispatcher_unbind_media_player_events(FFI_GET_NATIVE_PTR(void *, dispatcher_ptr), FFI_GET_NATIVE_PTR(void *, media_player_ptr));
})
#else
FFI_THUNK(0x1088, void, nve_dispatcher_unbind_media_player_events, (FFI_NATIVE_PTR(void *) dispatcher_ptr, FFI_NATIVE_PTR(void *) media_player_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_drm_error_text_get, (FFI_NATIVE_PTR(void *) operation_complete_ptr, FFI_WASM_PTR const out_drm_error), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, operation_complete_ptr), "operation_complete_ptr cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_drm_error), "out_drm_error cannot be NULL");
    nve_drm_error_text_get(FFI_GET_NATIVE_PTR(void *, operation_complete_ptr), FFI_PIN_WASM_PTR(out_drm_error));
})
#else
FFI_THUNK(0x108A, void, nve_drm_error_text_get, (FFI_NATIVE_PTR(void *) operation_complete_ptr, FFI_WASM_PTR const out_drm_error), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_generate_challenge, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, drm_manager), "drm_manager cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(bytes), "bytes cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, listener), "listener cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_drm_manager_generate_challenge(FFI_GET_NATIVE_PTR(void *, drm_manager), FFI_PIN_WASM_PTR(bytes), len, FFI_GET_NATIVE_PTR(void *, listener));
})
#else
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_generate_challenge, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_certificate_bytes, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, drm_manager), "drm_manager cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(bytes), "bytes cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, listener), "listener cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_drm_manager_store_certificate_bytes(FFI_GET_NATIVE_PTR(void *, drm_manager), FFI_PIN_WASM_PTR(bytes), len, FFI_GET_NATIVE_PTR(void *, listener));
})
#else
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_certificate_bytes, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_license_bytes, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, drm_manager), "drm_manager cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(bytes), "bytes cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, listener), "listener cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_drm_manager_store_license_bytes(FFI_GET_NATIVE_PTR(void *, drm_manager), FFI_PIN_WASM_PTR(bytes), len, FFI_GET_NATIVE_PTR(void *, listener));
})
#else
FFI_THUNK(0x408A48, FFI_ENUM(nve_result_e), nve_drm_manager_store_license_bytes, (FFI_NATIVE_PTR(void *) drm_manager, FFI_WASM_PTR const bytes, uint32_t len, FFI_NATIVE_PTR(void *) listener), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10, void, nve_enable_debug_logging, (), {
    nve_enable_debug_logging();
})
#else
FFI_THUNK(0x10, void, nve_enable_debug_logging, (), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x104, void, nve_event_filtering_enabled, (const int32_t is_enabled), {
    nve_event_filtering_enabled(is_enabled ? true : false);
})
#else
FFI_THUNK(0x104, void, nve_event_filtering_enabled, (const int32_t is_enabled), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10, void, nve_init, (), {
    nve_init();
})
#else
FFI_THUNK(0x10, void, nve_init, (), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_initialize_drm, (FFI_WASM_PTR const payload), {
    FFI_ASSERT_ALIGNED_WASM_PTR(payload, nve_drm_init_payload_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(payload), "payload cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_initialize_drm(FFI_PIN_WASM_PTR(payload));
})
#else
FFI_THUNK(0x40A, FFI_ENUM(nve_result_e), nve_initialize_drm, (FFI_WASM_PTR const payload), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, int32_t, nve_load_information_get, (FFI_NATIVE_PTR(void *) event_ptr, FFI_WASM_PTR const out_load_information), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, event_ptr), "event_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_load_information, nve_load_information_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_load_information), "out_load_information cannot be NULL");
    return nve_load_information_get(FFI_GET_NATIVE_PTR(void *, event_ptr), FFI_PIN_WASM_PTR(out_load_information));
})
#else
FFI_THUNK(0x408A, int32_t, nve_load_information_get, (FFI_NATIVE_PTR(void *) event_ptr, FFI_WASM_PTR const out_load_information), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_m5_callback_manager_create, (), {
    return FFI_SET_NATIVE_PTR(nve_m5_callback_manager_create());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_m5_callback_manager_create, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_m5_callback_manager_release, (FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, callback_manager_ptr), "callback_manager_ptr cannot be NULL");
    nve_m5_callback_manager_release(FFI_GET_NATIVE_PTR(void *, callback_manager_ptr));
})
#else
FFI_THUNK(0x108, void, nve_m5_callback_manager_release, (FFI_NATIVE_PTR(void *) callback_manager_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108AAAA, void, nve_m5_cenc_init_data_get, (FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size, FFI_WASM_PTR const method, FFI_WASM_PTR const tag), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(nve_handle_t, handle), "handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_ptr), "out_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_size, int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_size), "out_size cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(method, nve_drm_method_e);
    ASSERT_MSG(FFI_PIN_WASM_PTR(method), "method cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(tag, nve_drm_key_tag_e);
    ASSERT_MSG(FFI_PIN_WASM_PTR(tag), "tag cannot be NULL");
    nve_m5_cenc_init_data_get(FFI_GET_NATIVE_PTR(nve_handle_t, handle), FFI_PIN_WASM_PTR(out_ptr), FFI_PIN_WASM_PTR(out_size), FFI_PIN_WASM_PTR(method), FFI_PIN_WASM_PTR(tag));
})
#else
FFI_THUNK(0x108AAAA, void, nve_m5_cenc_init_data_get, (FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size, FFI_WASM_PTR const method, FFI_WASM_PTR const tag), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80888, FFI_NATIVE_PTR(void *), nve_m5_challenge_generated_listener_create, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) drm_manager_ptr, FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, operation_complete_listener_ptr), "operation_complete_listener_ptr cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_m5_challenge_generated_listener_create(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_GET_NATIVE_PTR(void *, drm_manager_ptr), FFI_GET_NATIVE_PTR(void *, operation_complete_listener_ptr)));
})
#else
FFI_THUNK(0x80888, FFI_NATIVE_PTR(void *), nve_m5_challenge_generated_listener_create, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) drm_manager_ptr, FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_m5_challenge_generated_listener_release, (FFI_NATIVE_PTR(void *) challenge_generated_listener_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, challenge_generated_listener_ptr), "challenge_generated_listener_ptr cannot be NULL");
    nve_m5_challenge_generated_listener_release(FFI_GET_NATIVE_PTR(void *, challenge_generated_listener_ptr));
})
#else
FFI_THUNK(0x108, void, nve_m5_challenge_generated_listener_release, (FFI_NATIVE_PTR(void *) challenge_generated_listener_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108AA, void, nve_m5_challenge_get, (FFI_NATIVE_PTR(void *) challenge_generated_event_payload_ptr, FFI_WASM_PTR const out_buffer_ptr, FFI_WASM_PTR const out_size_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, challenge_generated_event_payload_ptr), "challenge_generated_event_payload_ptr cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_buffer_ptr), "out_buffer_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_size_ptr, int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_size_ptr), "out_size_ptr cannot be NULL");
    nve_m5_challenge_get(FFI_GET_NATIVE_PTR(void *, challenge_generated_event_payload_ptr), FFI_PIN_WASM_PTR(out_buffer_ptr), FFI_PIN_WASM_PTR(out_size_ptr));
})
#else
FFI_THUNK(0x108AA, void, nve_m5_challenge_get, (FFI_NATIVE_PTR(void *) challenge_generated_event_payload_ptr, FFI_WASM_PTR const out_buffer_ptr, FFI_WASM_PTR const out_size_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_m5_operation_complete_listener_create, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_m5_operation_complete_listener_create(FFI_GET_NATIVE_PTR(void *, player_ptr)));
})
#else
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_m5_operation_complete_listener_create, (FFI_NATIVE_PTR(void *) player_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_m5_operation_complete_listener_release, (FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, operation_complete_listener_ptr), "operation_complete_listener_ptr cannot be NULL");
    nve_m5_operation_complete_listener_release(FFI_GET_NATIVE_PTR(void *, operation_complete_listener_ptr));
})
#else
FFI_THUNK(0x108, void, nve_m5_operation_complete_listener_release, (FFI_NATIVE_PTR(void *) operation_complete_listener_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_m5_view_release, (FFI_NATIVE_PTR(void *) view_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view_ptr), "view_ptr cannot be NULL");
    nve_m5_view_release(FFI_GET_NATIVE_PTR(void *, view_ptr));
})
#else
FFI_THUNK(0x108, void, nve_m5_view_release, (FFI_NATIVE_PTR(void *) view_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_buffered_range_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_range, nve_range_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_range), "out_range cannot be NULL");
    nve_media_player_buffered_range_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(out_range));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_buffered_range_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_create, (FFI_NATIVE_PTR(void *) dispatcher_ptr), {
    return FFI_SET_NATIVE_PTR(nve_media_player_create(FFI_GET_NATIVE_PTR(void *, dispatcher_ptr)));
})
#else
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_create, (FFI_NATIVE_PTR(void *) dispatcher_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x1084, void, nve_media_player_event_buffer_bind, (FFI_NATIVE_PTR(nve_handle_t) handle, uint32_t event_buffer_capacity), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(nve_handle_t, handle), "handle cannot be NULL");
    nve_media_player_event_buffer_bind(FFI_GET_NATIVE_PTR(nve_handle_t, handle), event_buffer_capacity);
})
#else
FFI_THUNK(0x1084, void, nve_media_player_event_buffer_bind, (FFI_NATIVE_PTR(nve_handle_t) handle, uint32_t event_buffer_capacity), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_media_player_event_buffer_unbind, (FFI_NATIVE_PTR(nve_handle_t) handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(nve_handle_t, handle), "handle cannot be NULL");
    nve_media_player_event_buffer_unbind(FFI_GET_NATIVE_PTR(nve_handle_t, handle));
})
#else
FFI_THUNK(0x108, void, nve_media_player_event_buffer_unbind, (FFI_NATIVE_PTR(nve_handle_t) handle), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_event_get, (FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const payload), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(nve_handle_t, handle), "handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(payload, nve_event_payload_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(payload), "payload cannot be NULL");
    nve_media_player_event_get(FFI_GET_NATIVE_PTR(nve_handle_t, handle), FFI_PIN_WASM_PTR(payload));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_event_get, (FFI_NATIVE_PTR(nve_handle_t) handle, FFI_WASM_PTR const payload), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_get_abr_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(params, nve_abr_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_media_player_get_abr_params(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_get_abr_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_get_buffer_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(params, nve_buffer_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_media_player_get_buffer_params(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_get_buffer_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_drm_manager, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_media_player_get_drm_manager(FFI_GET_NATIVE_PTR(void *, player_ptr)));
})
#else
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_drm_manager, (FFI_NATIVE_PTR(void *) player_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_get_text_style, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(cc_style, nve_text_format_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(cc_style), "cc_style cannot be NULL");
    nve_media_player_get_text_style(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(cc_style));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_get_text_style, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_view, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_media_player_get_view(FFI_GET_NATIVE_PTR(void *, player_ptr)));
})
#else
FFI_THUNK(0x808, FFI_NATIVE_PTR(void *), nve_media_player_get_view, (FFI_NATIVE_PTR(void *) player_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_media_player_item_network_configuration_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const config), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(config, nve_network_configuration_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(config), "config cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_media_player_item_network_configuration_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(config));
})
#else
FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_media_player_item_network_configuration_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const config), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_media_player_pause, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_media_player_pause(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x108, void, nve_media_player_pause, (FFI_NATIVE_PTR(void *) player_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_media_player_play, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_media_player_play(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x108, void, nve_media_player_play, (FFI_NATIVE_PTR(void *) player_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_playback_range_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_range, nve_range_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_range), "out_range cannot be NULL");
    nve_media_player_playback_range_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(out_range));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_playback_range_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_range), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0xD08, double, nve_media_player_position, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_media_player_position(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0xD08, double, nve_media_player_position, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0.0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108F, void, nve_media_player_prepare_to_play, (FFI_NATIVE_PTR(void *) player_ptr, float position), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_media_player_prepare_to_play(FFI_GET_NATIVE_PTR(void *, player_ptr), position);
})
#else
FFI_THUNK(0x108F, void, nve_media_player_prepare_to_play, (FFI_NATIVE_PTR(void *) player_ptr, float position), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_release, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_media_player_release(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_release, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10888, void, nve_media_player_replace_resource, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) resource_ptr, FFI_NATIVE_PTR(void *) config_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, resource_ptr), "resource_ptr cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, config_ptr), "config_ptr cannot be NULL");
    nve_media_player_replace_resource(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_GET_NATIVE_PTR(void *, resource_ptr), FFI_GET_NATIVE_PTR(void *, config_ptr));
})
#else
FFI_THUNK(0x10888, void, nve_media_player_replace_resource, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) resource_ptr, FFI_NATIVE_PTR(void *) config_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_reset, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_media_player_reset(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_reset, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_restore, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_media_player_restore(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_restore, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108D, void, nve_media_player_seek, (FFI_NATIVE_PTR(void *) player_ptr, double position_ms), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_media_player_seek(FFI_GET_NATIVE_PTR(void *, player_ptr), position_ms);
})
#else
FFI_THUNK(0x108D, void, nve_media_player_seek, (FFI_NATIVE_PTR(void *) player_ptr, double position_ms), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_set_abr_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(params, const nve_abr_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_media_player_set_abr_params(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_set_abr_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_set_buffer_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(params, const nve_buffer_params_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_media_player_set_buffer_params(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_set_buffer_params, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_media_player_set_text_style, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(cc_style, const nve_text_format_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(cc_style), "cc_style cannot be NULL");
    nve_media_player_set_text_style(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(cc_style));
})
#else
FFI_THUNK(0x108A, void, nve_media_player_set_text_style, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const cc_style), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x1088, void, nve_media_player_set_view, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) view_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view_ptr), "view_ptr cannot be NULL");
    nve_media_player_set_view(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_GET_NATIVE_PTR(void *, view_ptr));
})
#else
FFI_THUNK(0x1088, void, nve_media_player_set_view, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void *) view_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_media_player_stop, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_media_player_stop(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x108, void, nve_media_player_stop, (FFI_NATIVE_PTR(void *) player_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_suspend, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_media_player_suspend(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_media_player_suspend, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80A48, FFI_NATIVE_PTR(void *), nve_media_resource_create, (FFI_WASM_PTR const url, FFI_ENUM(nve_media_resource_type_e) media_resource_type, FFI_NATIVE_PTR(void *) metadata_ptr), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    ASSERT_MSG((HDS == (FFI_ENUM(nve_media_resource_type_e))media_resource_type) || (HLS == (FFI_ENUM(nve_media_resource_type_e))media_resource_type) || (DASH == (FFI_ENUM(nve_media_resource_type_e))media_resource_type) || (Custom == (FFI_ENUM(nve_media_resource_type_e))media_resource_type) || (Unknown == (FFI_ENUM(nve_media_resource_type_e))media_resource_type) || (ISOBMFF == (FFI_ENUM(nve_media_resource_type_e))media_resource_type), "Argument must be one of [HDS, HLS, DASH, Custom, Unknown, ISOBMFF]");
    return FFI_SET_NATIVE_PTR(nve_media_resource_create(FFI_PIN_WASM_PTR(url), (FFI_ENUM(nve_media_resource_type_e))media_resource_type, FFI_GET_NATIVE_PTR(void *, metadata_ptr)));
})
#else
FFI_THUNK(0x80A48, FFI_NATIVE_PTR(void *), nve_media_resource_create, (FFI_WASM_PTR const url, FFI_ENUM(nve_media_resource_type_e) media_resource_type, FFI_NATIVE_PTR(void *) metadata_ptr), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_media_resource_release, (FFI_NATIVE_PTR(void *) media_resource_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, media_resource_ptr), "media_resource_ptr cannot be NULL");
    nve_media_resource_release(FFI_GET_NATIVE_PTR(void *, media_resource_ptr));
})
#else
FFI_THUNK(0x108, void, nve_media_resource_release, (FFI_NATIVE_PTR(void *) media_resource_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_metadata_create, (), {
    return FFI_SET_NATIVE_PTR(nve_metadata_create());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(void *), nve_metadata_create, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4088, int32_t, nve_metadata_event_capture, (FFI_NATIVE_PTR(void *) metadata_ptr, FFI_NATIVE_PTR(void *) event_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, metadata_ptr), "metadata_ptr cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, event_ptr), "event_ptr cannot be NULL");
    return nve_metadata_event_capture(FFI_GET_NATIVE_PTR(void *, metadata_ptr), FFI_GET_NATIVE_PTR(void *, event_ptr));
})
#else
FFI_THUNK(0x4088, int32_t, nve_metadata_event_capture, (FFI_NATIVE_PTR(void *) metadata_ptr, FFI_NATIVE_PTR(void *) event_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084A, int32_t, nve_metadata_get, (FFI_NATIVE_PTR(void *) metadata_ptr, int32_t metadata_index, FFI_WASM_PTR const out_metadata), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, metadata_ptr), "metadata_ptr cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_metadata), "out_metadata cannot be NULL");
    return nve_metadata_get(FFI_GET_NATIVE_PTR(void *, metadata_ptr), metadata_index, FFI_PIN_WASM_PTR(out_metadata));
})
#else
FFI_THUNK(0x4084A, int32_t, nve_metadata_get, (FFI_NATIVE_PTR(void *) metadata_ptr, int32_t metadata_index, FFI_WASM_PTR const out_metadata), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_metadata_num_keys_get, (FFI_NATIVE_PTR(void *) metadata_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, metadata_ptr), "metadata_ptr cannot be NULL");
    return nve_metadata_num_keys_get(FFI_GET_NATIVE_PTR(void *, metadata_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_metadata_num_keys_get, (FFI_NATIVE_PTR(void *) metadata_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80A, FFI_NATIVE_PTR(void *), nve_mpi_config_create, (FFI_WASM_PTR const network_configuration), {
    FFI_ASSERT_ALIGNED_WASM_PTR(network_configuration, nve_network_configuration_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(network_configuration), "network_configuration cannot be NULL");
    return FFI_SET_NATIVE_PTR(nve_mpi_config_create(FFI_PIN_WASM_PTR(network_configuration)));
})
#else
FFI_THUNK(0x80A, FFI_NATIVE_PTR(void *), nve_mpi_config_create, (FFI_WASM_PTR const network_configuration), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108A, void, nve_mpi_config_subscribe_tag, (FFI_NATIVE_PTR(void *) config_ptr, FFI_WASM_PTR const tag_name), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, config_ptr), "config_ptr cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(tag_name), "tag_name cannot be NULL");
    nve_mpi_config_subscribe_tag(FFI_GET_NATIVE_PTR(void *, config_ptr), FFI_PIN_WASM_PTR(tag_name));
})
#else
FFI_THUNK(0x108A, void, nve_mpi_config_subscribe_tag, (FFI_NATIVE_PTR(void *) config_ptr, FFI_WASM_PTR const tag_name), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_body_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_ptr), "out_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_size, int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_size), "out_size cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_body_data_get(FFI_GET_NATIVE_PTR(void *, request_handle), FFI_PIN_WASM_PTR(out_ptr), FFI_PIN_WASM_PTR(out_size));
})
#else
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_body_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x80A4, FFI_NATIVE_PTR(void *), nve_network_request_create, (FFI_WASM_PTR const url, FFI_ENUM(nve_network_method_e) method), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    ASSERT_MSG((nve_network_method_get == (FFI_ENUM(nve_network_method_e))method) || (nve_network_method_post == (FFI_ENUM(nve_network_method_e))method), "Argument must be one of [nve_network_method_get, nve_network_method_post]");
    return FFI_SET_NATIVE_PTR(nve_network_request_create(FFI_PIN_WASM_PTR(url), (FFI_ENUM(nve_network_method_e))method));
})
#else
FFI_THUNK(0x80A4, FFI_NATIVE_PTR(void *), nve_network_request_create, (FFI_WASM_PTR const url, FFI_ENUM(nve_network_method_e) method), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_network_request_error_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_error_data), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_error_data), "out_error_data cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_error_data_get(FFI_GET_NATIVE_PTR(void *, request_handle), FFI_PIN_WASM_PTR(out_error_data));
})
#else
FFI_THUNK(0x408A, FFI_ENUM(nve_result_e), nve_network_request_error_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_error_data), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_execute, (FFI_NATIVE_PTR(void *) request_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_execute(FFI_GET_NATIVE_PTR(void *, request_handle));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_execute, (FFI_NATIVE_PTR(void *) request_handle), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_network_request_state_e), nve_network_request_get_state, (FFI_NATIVE_PTR(void *) request_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    return (FFI_ENUM(nve_network_request_state_e))nve_network_request_get_state(FFI_GET_NATIVE_PTR(void *, request_handle));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_network_request_state_e), nve_network_request_get_state, (FFI_NATIVE_PTR(void *) request_handle), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_ptr), "out_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_size, int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_size), "out_size cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_header_data_get(FFI_GET_NATIVE_PTR(void *, request_handle), FFI_PIN_WASM_PTR(out_ptr), FFI_PIN_WASM_PTR(out_size));
})
#else
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_data_get, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const out_ptr, FFI_WASM_PTR const out_size), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_set, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const header_name, FFI_WASM_PTR const header_data), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(header_name), "header_name cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(header_data), "header_data cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_header_set(FFI_GET_NATIVE_PTR(void *, request_handle), FFI_PIN_WASM_PTR(header_name), FFI_PIN_WASM_PTR(header_data));
})
#else
FFI_THUNK(0x408AA, FFI_ENUM(nve_result_e), nve_network_request_header_set, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const header_name, FFI_WASM_PTR const header_data), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A4, FFI_ENUM(nve_result_e), nve_network_request_payload_set, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const data_ptr, uint32_t data_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(data_ptr), "data_ptr cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_payload_set(FFI_GET_NATIVE_PTR(void *, request_handle), FFI_PIN_WASM_PTR(data_ptr), data_size);
})
#else
FFI_THUNK(0x408A4, FFI_ENUM(nve_result_e), nve_network_request_payload_set, (FFI_NATIVE_PTR(void *) request_handle, FFI_WASM_PTR const data_ptr, uint32_t data_size), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_release, (FFI_NATIVE_PTR(void *) request_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    return (FFI_ENUM(nve_result_e))nve_network_request_release(FFI_GET_NATIVE_PTR(void *, request_handle));
})
#else
FFI_THUNK(0x408, FFI_ENUM(nve_result_e), nve_network_request_release, (FFI_NATIVE_PTR(void *) request_handle), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, uint32_t, nve_network_request_result_code_get, (FFI_NATIVE_PTR(void *) request_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, request_handle), "request_handle cannot be NULL");
    return nve_network_request_result_code_get(FFI_GET_NATIVE_PTR(void *, request_handle));
})
#else
FFI_THUNK(0x408, uint32_t, nve_network_request_result_code_get, (FFI_NATIVE_PTR(void *) request_handle), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_payload_ref_add, (FFI_NATIVE_PTR(void *) payload_ptr), {
    nve_payload_ref_add(FFI_GET_NATIVE_PTR(void *, payload_ptr));
})
#else
FFI_THUNK(0x108, void, nve_payload_ref_add, (FFI_NATIVE_PTR(void *) payload_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_payload_ref_release, (FFI_NATIVE_PTR(void *) payload_ptr), {
    nve_payload_ref_release(FFI_GET_NATIVE_PTR(void *, payload_ptr));
})
#else
FFI_THUNK(0x108, void, nve_payload_ref_release, (FFI_NATIVE_PTR(void *) payload_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084A, int32_t, nve_profile_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t profile_index, FFI_WASM_PTR const out_profile), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_profile, nve_profile_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_profile), "out_profile cannot be NULL");
    return nve_profile_get(FFI_GET_NATIVE_PTR(void *, player_ptr), profile_index, FFI_PIN_WASM_PTR(out_profile));
})
#else
FFI_THUNK(0x4084A, int32_t, nve_profile_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t profile_index, FFI_WASM_PTR const out_profile), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_profile_num_profiles_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_profile_num_profiles_get(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_profile_num_profiles_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x108, void, nve_release_psdk_ref, (FFI_NATIVE_PTR(void *) psdk_shared_ptr), {
    nve_release_psdk_ref(FFI_GET_NATIVE_PTR(void *, psdk_shared_ptr));
})
#else
FFI_THUNK(0x108, void, nve_release_psdk_ref, (FFI_NATIVE_PTR(void *) psdk_shared_ptr), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10A, void, nve_set_local_storage_path, (FFI_WASM_PTR const storage_path), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(storage_path), "storage_path cannot be NULL");
    nve_set_local_storage_path(FFI_PIN_WASM_PTR(storage_path));
})
#else
FFI_THUNK(0x10A, void, nve_set_local_storage_path, (FFI_WASM_PTR const storage_path), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408A, int32_t, nve_text_current_track_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_track, nve_text_track_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_track), "out_track cannot be NULL");
    return nve_text_current_track_get(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_PIN_WASM_PTR(out_track));
})
#else
FFI_THUNK(0x408A, int32_t, nve_text_current_track_get, (FFI_NATIVE_PTR(void *) player_ptr, FFI_WASM_PTR const out_track), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10A, void, nve_text_format_init, (FFI_WASM_PTR const params), {
    FFI_ASSERT_ALIGNED_WASM_PTR(params, nve_text_format_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(params), "params cannot be NULL");
    nve_text_format_init(FFI_PIN_WASM_PTR(params));
})
#else
FFI_THUNK(0x10A, void, nve_text_format_init, (FFI_WASM_PTR const params), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_text_has_tracks, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_text_has_tracks(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_text_has_tracks, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_text_num_tracks_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_text_num_tracks_get(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_text_num_tracks_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408844, int32_t, nve_text_set_custom_font, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void * const) font_bytes, uint32_t len, uint32_t uncompressed_len), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_text_set_custom_font(FFI_GET_NATIVE_PTR(void *, player_ptr), FFI_GET_NATIVE_PTR(void * const, font_bytes), len, uncompressed_len);
})
#else
FFI_THUNK(0x408844, int32_t, nve_text_set_custom_font, (FFI_NATIVE_PTR(void *) player_ptr, FFI_NATIVE_PTR(void * const) font_bytes, uint32_t len, uint32_t uncompressed_len), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084A, int32_t, nve_text_track_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_track, nve_text_track_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_track), "out_track cannot be NULL");
    return nve_text_track_get(FFI_GET_NATIVE_PTR(void *, player_ptr), track_id, FFI_PIN_WASM_PTR(out_track));
})
#else
FFI_THUNK(0x4084A, int32_t, nve_text_track_get, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id, FFI_WASM_PTR const out_track), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x4084, int32_t, nve_text_track_set, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_text_track_set(FFI_GET_NATIVE_PTR(void *, player_ptr), track_id);
})
#else
FFI_THUNK(0x4084, int32_t, nve_text_track_set, (FFI_NATIVE_PTR(void *) player_ptr, int32_t track_id), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_text_visibility_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    return nve_text_visibility_get(FFI_GET_NATIVE_PTR(void *, player_ptr));
})
#else
FFI_THUNK(0x408, int32_t, nve_text_visibility_get, (FFI_NATIVE_PTR(void *) player_ptr), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x1084, void, nve_text_visibility_set, (FFI_NATIVE_PTR(void *) player_ptr, const int32_t cc_visibility), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player_ptr), "player_ptr cannot be NULL");
    nve_text_visibility_set(FFI_GET_NATIVE_PTR(void *, player_ptr), cc_visibility ? true : false);
})
#else
FFI_THUNK(0x1084, void, nve_text_visibility_set, (FFI_NATIVE_PTR(void *) player_ptr, const int32_t cc_visibility), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10A, void, nve_version_get, (FFI_WASM_PTR const out_version), {
    FFI_ASSERT_ALIGNED_WASM_PTR(out_version, nve_version_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_version), "out_version cannot be NULL");
    nve_version_get(FFI_PIN_WASM_PTR(out_version));
})
#else
FFI_THUNK(0x10A, void, nve_version_get, (FFI_WASM_PTR const out_version), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x8084444, FFI_NATIVE_PTR(void *), nve_view_create, (FFI_NATIVE_PTR(struct cg_context_t * const) ctx, int32_t x, int32_t y, int32_t w, int32_t h), {
    return FFI_SET_NATIVE_PTR(nve_m5_view_create(FFI_GET_NATIVE_PTR(struct cg_context_t * const, ctx), x, y, w, h));
})
#else
FFI_THUNK(0x8084444, FFI_NATIVE_PTR(void *), nve_view_create, (FFI_NATIVE_PTR(struct cg_context_t * const) ctx, int32_t x, int32_t y, int32_t w, int32_t h), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_view_get_height, (FFI_NATIVE_PTR(void *) view), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    return nve_m5_view_get_height(FFI_GET_NATIVE_PTR(void *, view));
})
#else
FFI_THUNK(0x408, int32_t, nve_view_get_height, (FFI_NATIVE_PTR(void *) view), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_view_get_width, (FFI_NATIVE_PTR(void *) view), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    return nve_m5_view_get_width(FFI_GET_NATIVE_PTR(void *, view));
})
#else
FFI_THUNK(0x408, int32_t, nve_view_get_width, (FFI_NATIVE_PTR(void *) view), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_view_get_x, (FFI_NATIVE_PTR(void *) view), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    return nve_m5_view_get_x(FFI_GET_NATIVE_PTR(void *, view));
})
#else
FFI_THUNK(0x408, int32_t, nve_view_get_x, (FFI_NATIVE_PTR(void *) view), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x408, int32_t, nve_view_get_y, (FFI_NATIVE_PTR(void *) view), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    return nve_m5_view_get_y(FFI_GET_NATIVE_PTR(void *, view));
})
#else
FFI_THUNK(0x408, int32_t, nve_view_get_y, (FFI_NATIVE_PTR(void *) view), {
    return 0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10844, void, nve_view_set_pos, (FFI_NATIVE_PTR(void *) view, int32_t x, int32_t y), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    nve_m5_view_set_position(FFI_GET_NATIVE_PTR(void *, view), x, y);
})
#else
FFI_THUNK(0x10844, void, nve_view_set_pos, (FFI_NATIVE_PTR(void *) view, int32_t x, int32_t y), {
    (void)0;
})
#endif // _NVE

#ifdef _NVE
FFI_THUNK(0x10844, void, nve_view_set_size, (FFI_NATIVE_PTR(void *) view, int32_t width, int32_t height), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, view), "view cannot be NULL");
    nve_m5_view_set_size(FFI_GET_NATIVE_PTR(void *, view), width, height);
})
#else
FFI_THUNK(0x10844, void, nve_view_set_size, (FFI_NATIVE_PTR(void *) view, int32_t width, int32_t height), {
    (void)0;
})
#endif // _NVE

