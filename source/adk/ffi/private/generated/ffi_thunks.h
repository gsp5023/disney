/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_ad_end, (const int32_t session_id), {
    adk_analytics_ad_end(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_ad_end, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_ad_start, (const int32_t session_id), {
    adk_analytics_ad_start(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_ad_start, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10AA, void, adk_analytics_add_event_to_dictionary, (FFI_WASM_PTR const key, FFI_WASM_PTR const event_name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(key), "key cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(event_name), "event_name cannot be NULL");
    adk_analytics_add_event_to_dictionary(FFI_PIN_WASM_PTR(key), FFI_PIN_WASM_PTR(event_name));
})
#else
FFI_THUNK(0x10AA, void, adk_analytics_add_event_to_dictionary, (FFI_WASM_PTR const key, FFI_WASM_PTR const event_name), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x1048, void, adk_analytics_attach_player, (const int32_t session_id, FFI_NATIVE_PTR(void *) player), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(void *, player), "player cannot be NULL");
    adk_analytics_attach_player(session_id, FFI_GET_NATIVE_PTR(void *, player));
})
#else
FFI_THUNK(0x1048, void, adk_analytics_attach_player, (const int32_t session_id, FFI_NATIVE_PTR(void *) player), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_cleanup_session, (const int32_t session_id), {
    adk_analytics_cleanup_session(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_cleanup_session, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10, void, adk_analytics_create_dictionary, (), {
    adk_analytics_create_dictionary();
})
#else
FFI_THUNK(0x10, void, adk_analytics_create_dictionary, (), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_create_session, (), {
    return adk_analytics_create_session();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_create_session, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10, void, adk_analytics_destroy_dictionary, (), {
    adk_analytics_destroy_dictionary();
})
#else
FFI_THUNK(0x10, void, adk_analytics_destroy_dictionary, (), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_detach_player, (const int32_t session_id), {
    adk_analytics_detach_player(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_detach_player, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_asset_name, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_asset_name());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_asset_name, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x804, FFI_NATIVE_PTR(void *), adk_analytics_get_attached_player, (const int32_t session_id), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_attached_player(session_id));
})
#else
FFI_THUNK(0x804, FFI_NATIVE_PTR(void *), adk_analytics_get_attached_player, (const int32_t session_id), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_buffer_length, (), {
    return adk_analytics_get_buffer_length();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_buffer_length, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_default_bitrate, (), {
    return adk_analytics_get_default_bitrate();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_default_bitrate, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_cdn, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_default_cdn());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_cdn, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_resource, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_default_resource());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_default_resource, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_brand, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_device_brand());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_brand, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_manufacturer, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_device_manufacturer());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_manufacturer, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_model, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_device_model());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_model, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, FFI_ENUM(Device), adk_analytics_get_device_type, (), {
    return (FFI_ENUM(Device))adk_analytics_get_device_type();
})
#else
FFI_THUNK(0x40, FFI_ENUM(Device), adk_analytics_get_device_type, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_version, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_device_version());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_device_version, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_duration, (), {
    return adk_analytics_get_duration();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_duration, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_enable_player_state_inference, (), {
    return adk_analytics_get_enable_player_state_inference();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_enable_player_state_inference, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_name, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_framework_name());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_name, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_version, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_framework_version());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_framework_version, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_getway_url, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_getway_url());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_getway_url, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, uint32_t, adk_analytics_get_heartbeat_interval, (), {
    return adk_analytics_get_heartbeat_interval();
})
#else
FFI_THUNK(0x40, uint32_t, adk_analytics_get_heartbeat_interval, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_is_live, (), {
    return adk_analytics_get_is_live();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_is_live, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, FFI_ENUM(adk_analytics_log_e), adk_analytics_get_log_level, (), {
    return (FFI_ENUM(adk_analytics_log_e))adk_analytics_get_log_level();
})
#else
FFI_THUNK(0x40, FFI_ENUM(adk_analytics_log_e), adk_analytics_get_log_level, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_min_buffer_length, (), {
    return adk_analytics_get_min_buffer_length();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_min_buffer_length, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_name, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_operating_system_name());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_name, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_version, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_operating_system_version());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_operating_system_version, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, int32_t, adk_analytics_get_playahead_time, (), {
    return adk_analytics_get_playahead_time();
})
#else
FFI_THUNK(0x40, int32_t, adk_analytics_get_playahead_time, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_player_name, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_player_name());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_player_name, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40, uint32_t, adk_analytics_get_player_poll_interval, (), {
    return adk_analytics_get_player_poll_interval();
})
#else
FFI_THUNK(0x40, uint32_t, adk_analytics_get_player_poll_interval, (), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_type, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_player_type());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_type, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_version, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_player_version());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_analytics_get_player_version, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0xF0, float, adk_analytics_get_rendered_framerate, (), {
    return adk_analytics_get_rendered_framerate();
})
#else
FFI_THUNK(0xF0, float, adk_analytics_get_rendered_framerate, (), {
    return 0.0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_stream_url, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_stream_url());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_stream_url, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80A, FFI_NATIVE_PTR(const char *), adk_analytics_get_tag, (FFI_WASM_PTR const key), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(key), "key cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_analytics_get_tag(FFI_PIN_WASM_PTR(key)));
})
#else
FFI_THUNK(0x80A, FFI_NATIVE_PTR(const char *), adk_analytics_get_tag, (FFI_WASM_PTR const key), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_viewer_id, (), {
    return FFI_SET_NATIVE_PTR(adk_analytics_get_viewer_id());
})
#else
FFI_THUNK(0x80, FFI_NATIVE_PTR(char *), adk_analytics_get_viewer_id, (), {
    return FFI_SET_NATIVE_PTR(NULL);
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_init_client, (FFI_WASM_PTR const consumer_key), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(consumer_key), "consumer_key cannot be NULL");
    adk_analytics_init_client(FFI_PIN_WASM_PTR(consumer_key));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_init_client, (FFI_WASM_PTR const consumer_key), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_notify_network_connection_type, (FFI_WASM_PTR const _type), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(_type), "_type cannot be NULL");
    adk_analytics_notify_network_connection_type(FFI_PIN_WASM_PTR(_type));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_notify_network_connection_type, (FFI_WASM_PTR const _type), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10F, void, adk_analytics_notify_network_signal_strength, (const float strength), {
    adk_analytics_notify_network_signal_strength(strength);
})
#else
FFI_THUNK(0x10F, void, adk_analytics_notify_network_signal_strength, (const float strength), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_notify_network_wifi_link_encryption, (FFI_WASM_PTR const _type), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(_type), "_type cannot be NULL");
    adk_analytics_notify_network_wifi_link_encryption(FFI_PIN_WASM_PTR(_type));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_notify_network_wifi_link_encryption, (FFI_WASM_PTR const _type), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_remove_session, (int32_t session_id), {
    adk_analytics_remove_session(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_remove_session, (int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104F, void, adk_analytics_seek_end, (const int32_t session_id, const float player_seek_end_pos), {
    adk_analytics_seek_end(session_id, player_seek_end_pos);
})
#else
FFI_THUNK(0x104F, void, adk_analytics_seek_end, (const int32_t session_id, const float player_seek_end_pos), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_seek_start, (const int32_t session_id), {
    adk_analytics_seek_start(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_seek_start, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x40A, int32_t, adk_analytics_send_event, (FFI_WASM_PTR const event_name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(event_name), "event_name cannot be NULL");
    return adk_analytics_send_event(FFI_PIN_WASM_PTR(event_name));
})
#else
FFI_THUNK(0x40A, int32_t, adk_analytics_send_event, (FFI_WASM_PTR const event_name), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x404A, int32_t, adk_analytics_send_session_event, (const int32_t session_id, FFI_WASM_PTR const event_name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(event_name), "event_name cannot be NULL");
    return adk_analytics_send_session_event(session_id, FFI_PIN_WASM_PTR(event_name));
})
#else
FFI_THUNK(0x404A, int32_t, adk_analytics_send_session_event, (const int32_t session_id, FFI_WASM_PTR const event_name), {
    return 0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104A4, void, adk_analytics_session_report_error, (const int32_t session_id, FFI_WASM_PTR const error_msg, const int32_t is_fatal), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(error_msg), "error_msg cannot be NULL");
    adk_analytics_session_report_error(session_id, FFI_PIN_WASM_PTR(error_msg), is_fatal ? true : false);
})
#else
FFI_THUNK(0x104A4, void, adk_analytics_session_report_error, (const int32_t session_id, FFI_WASM_PTR const error_msg, const int32_t is_fatal), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_asset_name, (FFI_WASM_PTR const name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    adk_analytics_set_asset_name(FFI_PIN_WASM_PTR(name));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_asset_name, (FFI_WASM_PTR const name), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_buffering, (const int32_t session_id), {
    adk_analytics_set_buffering(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_buffering, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_default_bitrate, (const int32_t bitrate), {
    adk_analytics_set_default_bitrate(bitrate);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_default_bitrate, (const int32_t bitrate), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_default_cdn, (FFI_WASM_PTR const name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    adk_analytics_set_default_cdn(FFI_PIN_WASM_PTR(name));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_default_cdn, (FFI_WASM_PTR const name), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_default_resource, (FFI_WASM_PTR const name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    adk_analytics_set_default_resource(FFI_PIN_WASM_PTR(name));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_default_resource, (FFI_WASM_PTR const name), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_device_brand, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_device_brand(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_device_brand, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_device_manufacturer, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_device_manufacturer(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_device_manufacturer, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_device_model, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_device_model(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_device_model, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_device_type, (FFI_ENUM(const Device) value), {
    ASSERT_MSG((Device_Unknown == (FFI_ENUM(const Device))value) || (Device_Desktop == (FFI_ENUM(const Device))value) || (Device_Console == (FFI_ENUM(const Device))value) || (Device_Settop == (FFI_ENUM(const Device))value) || (Device_Mobile == (FFI_ENUM(const Device))value) || (Device_Tablet == (FFI_ENUM(const Device))value) || (Device_SmartTV == (FFI_ENUM(const Device))value), "Argument must be one of [Device_Unknown, Device_Desktop, Device_Console, Device_Settop, Device_Mobile, Device_Tablet, Device_SmartTV]");
    adk_analytics_set_device_type((FFI_ENUM(const Device))value);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_device_type, (FFI_ENUM(const Device) value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_device_version, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_device_version(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_device_version, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_duration, (const int32_t duration), {
    adk_analytics_set_duration(duration);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_duration, (const int32_t duration), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_enable_player_state_inference, (const int32_t state), {
    adk_analytics_set_enable_player_state_inference(state ? true : false);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_enable_player_state_inference, (const int32_t state), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_framework_name, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_framework_name(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_framework_name, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_framework_version, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_framework_version(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_framework_version, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_gateway_url, (FFI_WASM_PTR const url), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    adk_analytics_set_gateway_url(FFI_PIN_WASM_PTR(url));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_gateway_url, (FFI_WASM_PTR const url), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_heartbeat_interval, (const uint32_t interval), {
    adk_analytics_set_heartbeat_interval(interval);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_heartbeat_interval, (const uint32_t interval), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_is_live, (const int32_t live), {
    adk_analytics_set_is_live(live ? true : false);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_is_live, (const int32_t live), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_log_level, (FFI_ENUM(const adk_analytics_log_e) level), {
    ASSERT_MSG((log_none == (FFI_ENUM(const adk_analytics_log_e))level) || (log_error == (FFI_ENUM(const adk_analytics_log_e))level) || (log_warn == (FFI_ENUM(const adk_analytics_log_e))level) || (log_info == (FFI_ENUM(const adk_analytics_log_e))level) || (log_debug == (FFI_ENUM(const adk_analytics_log_e))level), "Argument must be one of [log_none, log_error, log_warn, log_info, log_debug]");
    adk_analytics_set_log_level((FFI_ENUM(const adk_analytics_log_e))level);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_log_level, (FFI_ENUM(const adk_analytics_log_e) level), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_name, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_operating_system_name(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_name, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_version, (FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_operating_system_version(FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_operating_system_version, (FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_paused, (const int32_t session_id), {
    adk_analytics_set_paused(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_paused, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_player_name, (FFI_WASM_PTR const name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    adk_analytics_set_player_name(FFI_PIN_WASM_PTR(name));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_player_name, (FFI_WASM_PTR const name), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_player_poll_interval, (const uint32_t interval), {
    adk_analytics_set_player_poll_interval(interval);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_player_poll_interval, (const uint32_t interval), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_playing, (const int32_t session_id), {
    adk_analytics_set_playing(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_playing, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x1044, void, adk_analytics_set_session_bitrate, (const int32_t session_id, const int32_t bitrate), {
    adk_analytics_set_session_bitrate(session_id, bitrate);
})
#else
FFI_THUNK(0x1044, void, adk_analytics_set_session_bitrate, (const int32_t session_id, const int32_t bitrate), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x1044, void, adk_analytics_set_session_duration, (const int32_t session_id, const int32_t duration), {
    adk_analytics_set_session_duration(session_id, duration);
})
#else
FFI_THUNK(0x1044, void, adk_analytics_set_session_duration, (const int32_t session_id, const int32_t duration), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x1044, void, adk_analytics_set_session_encoded_framerate, (const int32_t session_id, const int32_t framerate), {
    adk_analytics_set_session_encoded_framerate(session_id, framerate);
})
#else
FFI_THUNK(0x1044, void, adk_analytics_set_session_encoded_framerate, (const int32_t session_id, const int32_t framerate), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x1044AA, void, adk_analytics_set_session_stream, (const int32_t session_id, const int32_t bitrate_kbps, FFI_WASM_PTR const cdn, FFI_WASM_PTR const resource), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(cdn), "cdn cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(resource), "resource cannot be NULL");
    adk_analytics_set_session_stream(session_id, bitrate_kbps, FFI_PIN_WASM_PTR(cdn), FFI_PIN_WASM_PTR(resource));
})
#else
FFI_THUNK(0x1044AA, void, adk_analytics_set_session_stream, (const int32_t session_id, const int32_t bitrate_kbps, FFI_WASM_PTR const cdn, FFI_WASM_PTR const resource), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10444, void, adk_analytics_set_session_video_size, (const int32_t session_id, const int32_t width, const int32_t height), {
    adk_analytics_set_session_video_size(session_id, width, height);
})
#else
FFI_THUNK(0x10444, void, adk_analytics_set_session_video_size, (const int32_t session_id, const int32_t width, const int32_t height), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_set_stopped, (const int32_t session_id), {
    adk_analytics_set_stopped(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_set_stopped, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_stream_url, (FFI_WASM_PTR const url), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    adk_analytics_set_stream_url(FFI_PIN_WASM_PTR(url));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_stream_url, (FFI_WASM_PTR const url), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10AA, void, adk_analytics_set_tag, (FFI_WASM_PTR const key, FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(key), "key cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_analytics_set_tag(FFI_PIN_WASM_PTR(key), FFI_PIN_WASM_PTR(value));
})
#else
FFI_THUNK(0x10AA, void, adk_analytics_set_tag, (FFI_WASM_PTR const key, FFI_WASM_PTR const value), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x10A, void, adk_analytics_set_viewer_id, (FFI_WASM_PTR const id), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(id), "id cannot be NULL");
    adk_analytics_set_viewer_id(FFI_PIN_WASM_PTR(id));
})
#else
FFI_THUNK(0x10A, void, adk_analytics_set_viewer_id, (FFI_WASM_PTR const id), {
    (void)0;
})
#endif // _LEIA

#ifdef _LEIA
FFI_THUNK(0x104, void, adk_analytics_update_content_info, (const int32_t session_id), {
    adk_analytics_update_content_info(session_id);
})
#else
FFI_THUNK(0x104, void, adk_analytics_update_content_info, (const int32_t session_id), {
    (void)0;
})
#endif // _LEIA

FFI_THUNK(0x10, void, adk_app_request_restart, (), {
    thunk_request_restart();
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_screenshot_t *), adk_capture_screenshot, (), {
    return FFI_SET_NATIVE_PTR(ffi_capture_screenshot());
})

FFI_THUNK(0x10AA, void, adk_coredump_add_data, (FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_coredump_add_data(FFI_PIN_WASM_PTR(name), FFI_PIN_WASM_PTR(value));
})

FFI_THUNK(0x10AA, void, adk_coredump_add_data_public, (FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    adk_coredump_add_data_public(FFI_PIN_WASM_PTR(name), FFI_PIN_WASM_PTR(value));
})

FFI_THUNK(0x40A, uint32_t, adk_crc_str_32, (FFI_WASM_PTR const str), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(str), "str cannot be NULL");
    return crc_str_32(FFI_PIN_WASM_PTR(str));
})

FFI_THUNK(0x404A, int32_t, adk_create_directory_path, (FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const input_path), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(input_path), "input_path cannot be NULL");
    return adk_create_directory_path((FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(input_path));
})

FFI_THUNK(0x108AAA, void, adk_curl_async_perform_raw, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const on_http_recv_header_ptr, FFI_WASM_PTR const on_http_recv_ptr, FFI_WASM_PTR const on_complete_ptr), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    adk_curl_async_perform_raw(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), FFI_WASM_PTR_OFFSET(on_http_recv_header_ptr), FFI_WASM_PTR_OFFSET(on_http_recv_ptr), FFI_WASM_PTR_OFFSET(on_complete_ptr));
})

FFI_THUNK(0x108, void, adk_curl_close_handle, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    adk_curl_close_handle(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle));
})

FFI_THUNK(0x408, FFI_ENUM(adk_curl_handle_buffer_mode_e), adk_curl_get_buffering_mode, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    return (FFI_ENUM(adk_curl_handle_buffer_mode_e))adk_curl_get_buffering_mode(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle));
})

FFI_THUNK(0x108A, void, adk_curl_get_http_body, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, native_slice_t);
    *(native_slice_t *)FFI_PIN_WASM_PTR(ret_val) = adk_curl_get_http_body_bridge(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle));
})

FFI_THUNK(0x108A, void, adk_curl_get_http_header, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, native_slice_t);
    *(native_slice_t *)FFI_PIN_WASM_PTR(ret_val) = adk_curl_get_http_header_bridge(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle));
})

FFI_THUNK(0x4084A, FFI_ENUM(adk_curl_result_e), adk_curl_get_info_i32, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_info_e) info, FFI_WASM_PTR const out), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out, int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out), "out cannot be NULL");
    return (FFI_ENUM(adk_curl_result_e))adk_curl_get_info_i32(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), (FFI_ENUM(const adk_curl_info_e))info, FFI_PIN_WASM_PTR(out));
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_curl_handle_t *), adk_curl_open_handle, (), {
    return FFI_SET_NATIVE_PTR(adk_curl_open_handle());
})

FFI_THUNK(0x1084, void, adk_curl_set_buffering_mode, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_handle_buffer_mode_e) buffer_mode), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    adk_curl_set_buffering_mode(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), (FFI_ENUM(const adk_curl_handle_buffer_mode_e))buffer_mode);
})

FFI_THUNK(0x10844, void, adk_curl_set_opt_i32, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, int32_t arg), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    adk_curl_set_opt_long(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), (FFI_ENUM(const adk_curl_option_e))opt, (const long)arg);
})

FFI_THUNK(0x1084A, void, adk_curl_set_opt_ptr, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, FFI_WASM_PTR const arg), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg), "arg cannot be NULL");
    adk_curl_set_opt_ptr(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), (FFI_ENUM(const adk_curl_option_e))opt, FFI_PIN_WASM_PTR(arg));
})

FFI_THUNK(0x10848, void, adk_curl_set_opt_slist, (FFI_NATIVE_PTR(adk_curl_handle_t * const) handle, FFI_ENUM(const adk_curl_option_e) opt, FFI_NATIVE_PTR(adk_curl_slist_t * const) arg), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), "handle cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_slist_t * const, arg), "arg cannot be NULL");
    adk_curl_set_opt_slist(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, handle), (FFI_ENUM(const adk_curl_option_e))opt, FFI_GET_NATIVE_PTR(adk_curl_slist_t * const, arg));
})

FFI_THUNK(0x808A, FFI_NATIVE_PTR(adk_curl_slist_t *), adk_curl_slist_append, (FFI_NATIVE_PTR(adk_curl_slist_t *) list, FFI_WASM_PTR const sz), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(sz), "sz cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_curl_slist_append(FFI_GET_NATIVE_PTR(adk_curl_slist_t *, list), FFI_PIN_WASM_PTR(sz)));
})

FFI_THUNK(0x108, void, adk_curl_slist_free_all, (FFI_NATIVE_PTR(adk_curl_slist_t * const) list), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_slist_t * const, list), "list cannot be NULL");
    adk_curl_slist_free_all(FFI_GET_NATIVE_PTR(adk_curl_slist_t * const, list));
})

FFI_THUNK(0x404A, FFI_ENUM(sb_directory_delete_error_e), adk_delete_directory, (FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const subpath), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(subpath), "subpath cannot be NULL");
    return (FFI_ENUM(sb_directory_delete_error_e))adk_delete_directory((FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(subpath));
})

FFI_THUNK(0x404A, int32_t, adk_delete_file, (FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(filename), "filename cannot be NULL");
    return adk_delete_file((FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(filename));
})

FFI_THUNK(0x104, void, adk_dump_heap_usage, (FFI_ENUM(const adk_dump_heap_flags_e) heaps_to_dump), {
    app_dump_heaps((FFI_ENUM(const adk_dump_heap_flags_e))heaps_to_dump);
})

FFI_THUNK(0x10, void, adk_enter_background_mode, (), {
    app_request_background();
})

FFI_THUNK(0x4044A, int32_t, adk_enumerate_display_modes, (const int32_t display_index, const int32_t display_mode_index, FFI_WASM_PTR const out_results), {
    FFI_ASSERT_ALIGNED_WASM_PTR(out_results, sb_enumerate_display_modes_result_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_results), "out_results cannot be NULL");
    return sb_enumerate_display_modes(display_index, display_mode_index, FFI_PIN_WASM_PTR(out_results));
})

FFI_THUNK(0x408, int32_t, adk_fclose, (FFI_NATIVE_PTR(sb_file_t * const) file), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(sb_file_t * const, file), "file cannot be NULL");
    return adk_fclose(FFI_GET_NATIVE_PTR(sb_file_t * const, file));
})

FFI_THUNK(0x408, int32_t, adk_feof, (FFI_NATIVE_PTR(sb_file_t * const) file), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(sb_file_t * const, file), "file cannot be NULL");
    return adk_feof(FFI_GET_NATIVE_PTR(sb_file_t * const, file));
})

FFI_THUNK(0x804AA, FFI_NATIVE_PTR(sb_file_t *), adk_fopen, (FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const path, FFI_WASM_PTR const mode), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(path), "path cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(mode), "mode cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_fopen((FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(path), FFI_PIN_WASM_PTR(mode)));
})

FFI_THUNK(0x40A448, int32_t, adk_fread, (FFI_WASM_PTR const buffer, int32_t elem_size, int32_t elem_count, FFI_NATIVE_PTR(sb_file_t * const) file), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(sb_file_t * const, file), "file cannot be NULL");
    return (int32_t)adk_fread(FFI_PIN_WASM_PTR(buffer), (const size_t)elem_size, (const size_t)elem_count, FFI_GET_NATIVE_PTR(sb_file_t * const, file));
})

FFI_THUNK(0x10A448A, void, adk_fwrite, (FFI_WASM_PTR const buffer, int32_t elem_size, int32_t elem_count, FFI_NATIVE_PTR(sb_file_t * const) file, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(sb_file_t * const, file), "file cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, adk_fwrite_result_t);
    *(adk_fwrite_result_t *)FFI_PIN_WASM_PTR(ret_val) = adk_fwrite(FFI_PIN_WASM_PTR(buffer), (const size_t)elem_size, (const size_t)elem_count, FFI_GET_NATIVE_PTR(sb_file_t * const, file));
})

FFI_THUNK(0x10A, void, adk_generate_uuid, (FFI_WASM_PTR const ret_val), {
    *(sb_uuid_t *)FFI_PIN_WASM_PTR(ret_val) = sb_generate_uuid();
})

FFI_THUNK(0x40, FFI_ENUM(adk_app_state_e), adk_get_app_state, (), {
    return (FFI_ENUM(adk_app_state_e))app_get_state();
})

FFI_THUNK(0x10A, void, adk_get_cpu_mem_status, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, sb_cpu_mem_status_t);
    *(sb_cpu_mem_status_t *)FFI_PIN_WASM_PTR(ret_val) = sb_get_cpu_mem_status();
})

FFI_THUNK(0x108A, void, adk_get_deeplink_buffer, (FFI_NATIVE_PTR(const sb_deeplink_handle_t * const) handle, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const sb_deeplink_handle_t * const, handle), "handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, native_slice_t);
    *(native_slice_t *)FFI_PIN_WASM_PTR(ret_val) = adk_get_deeplink_buffer_bridge(FFI_GET_NATIVE_PTR(const sb_deeplink_handle_t * const, handle));
})

FFI_THUNK(0x80A, FFI_NATIVE_PTR(const char *), adk_get_env, (FFI_WASM_PTR const env_name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(env_name), "env_name cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_get_env(FFI_PIN_WASM_PTR(env_name)));
})

FFI_THUNK(0x10A, void, adk_get_locale, (FFI_WASM_PTR const ret_val), {
    *(sb_locale_t *)FFI_PIN_WASM_PTR(ret_val) = sb_get_locale();
})

FFI_THUNK(0x80, uint64_t, adk_get_milliseconds_since_epoch, (), {
    return adk_get_milliseconds_since_epoch();
})

FFI_THUNK(0x404, int32_t, adk_get_supported_refresh_rate, (const int32_t starting_refresh_rate), {
    return adk_get_supported_refresh_rate(starting_refresh_rate);
})

FFI_THUNK(0x10A, void, adk_get_system_metrics_native, (FFI_WASM_PTR const out), {
    FFI_ASSERT_ALIGNED_WASM_PTR(out, adk_system_metrics_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out), "out cannot be NULL");
    adk_get_system_metrics(FFI_PIN_WASM_PTR(out));
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(const char *), adk_get_wasm_call_stack, (), {
    return FFI_SET_NATIVE_PTR(adk_get_wasm_call_stack());
})

FFI_THUNK(0x10A, void, adk_get_wasm_heap_usage, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, heap_metrics_t);
    *(heap_metrics_t *)FFI_PIN_WASM_PTR(ret_val) = adk_get_wasm_heap_usage();
})

FFI_THUNK(0x808AA, FFI_NATIVE_PTR(adk_http_header_list_t *), adk_http_append_header_list, (FFI_NATIVE_PTR(adk_http_header_list_t *) list, FFI_WASM_PTR const name, FFI_WASM_PTR const value), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(name), "name cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(value), "value cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_http_append_header_list(FFI_GET_NATIVE_PTR(adk_http_header_list_t *, list), FFI_PIN_WASM_PTR(name), FFI_PIN_WASM_PTR(value), MALLOC_TAG));
})

FFI_THUNK(0x10A, void, adk_http_set_proxy, (FFI_WASM_PTR const proxy), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(proxy), "proxy cannot be NULL");
    adk_http_set_proxy(FFI_PIN_WASM_PTR(proxy));
})

FFI_THUNK(0x10A, void, adk_http_set_socks, (FFI_WASM_PTR const socks), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(socks), "socks cannot be NULL");
    adk_http_set_socks(FFI_PIN_WASM_PTR(socks));
})

FFI_THUNK(0x108, void, adk_httpx_client_free, (FFI_NATIVE_PTR(adk_httpx_client_t * const) client), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_client_t * const, client), "client cannot be NULL");
    adk_httpx_client_free(FFI_GET_NATIVE_PTR(adk_httpx_client_t * const, client));
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(adk_httpx_client_t *), adk_httpx_client_new, (), {
    return FFI_SET_NATIVE_PTR(adk_httpx_client_new());
})

FFI_THUNK(0x8084A, FFI_NATIVE_PTR(adk_httpx_request_t *), adk_httpx_client_request, (FFI_NATIVE_PTR(const adk_httpx_client_t * const) client, FFI_ENUM(const adk_httpx_method_e) method, FFI_WASM_PTR const url), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_client_t * const, client), "client cannot be NULL");
    ASSERT_MSG((adk_httpx_method_get == (FFI_ENUM(const adk_httpx_method_e))method) || (adk_httpx_method_post == (FFI_ENUM(const adk_httpx_method_e))method) || (adk_httpx_method_put == (FFI_ENUM(const adk_httpx_method_e))method) || (adk_httpx_method_patch == (FFI_ENUM(const adk_httpx_method_e))method) || (adk_httpx_method_delete == (FFI_ENUM(const adk_httpx_method_e))method) || (adk_httpx_method_head == (FFI_ENUM(const adk_httpx_method_e))method), "Argument must be one of [adk_httpx_method_get, adk_httpx_method_post, adk_httpx_method_put, adk_httpx_method_patch, adk_httpx_method_delete, adk_httpx_method_head]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_httpx_client_request_const(FFI_GET_NATIVE_PTR(const adk_httpx_client_t * const, client), (FFI_ENUM(const adk_httpx_method_e))method, FFI_PIN_WASM_PTR(url)));
})

FFI_THUNK(0x408, int32_t, adk_httpx_client_tick, (FFI_NATIVE_PTR(const adk_httpx_client_t * const) client), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_client_t * const, client), "client cannot be NULL");
    return adk_httpx_client_tick_const(FFI_GET_NATIVE_PTR(const adk_httpx_client_t * const, client));
})

FFI_THUNK(0x108A4, void, adk_httpx_request_set_body, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const body, int32_t body_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(body), "body cannot be NULL");
    adk_httpx_request_set_body(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), FFI_PIN_WASM_PTR(body), (const size_t)body_size);
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_follow_location, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, const int32_t follow_location), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    adk_httpx_request_set_follow_location(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), follow_location ? true : false);
})

FFI_THUNK(0x108A, void, adk_httpx_request_set_header, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const header), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(header), "header cannot be NULL");
    adk_httpx_request_set_header(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), FFI_PIN_WASM_PTR(header));
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_preferred_receive_buffer_size, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, uint32_t preferred_receive_buffer_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    adk_httpx_request_set_preferred_receive_buffer_size(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), preferred_receive_buffer_size);
})

FFI_THUNK(0x1088, void, adk_httpx_request_set_timeout, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, uint64_t timeout), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    adk_httpx_request_set_timeout(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), timeout);
})

FFI_THUNK(0x1084, void, adk_httpx_request_set_verbose, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request, const int32_t verbose), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    adk_httpx_request_set_verbose(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), verbose ? true : false);
})

FFI_THUNK(0x108, void, adk_httpx_response_free, (FFI_NATIVE_PTR(adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_response_t * const, response), "response cannot be NULL");
    adk_httpx_response_free(FFI_GET_NATIVE_PTR(adk_httpx_response_t * const, response));
})

FFI_THUNK(0x408A4, uint32_t, adk_httpx_response_get_body_copy, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response, FFI_WASM_PTR const buffer, uint32_t buffer_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    return (uint32_t)adk_httpx_response_get_body_copy(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), FFI_PIN_WASM_PTR(buffer), (const size_t)buffer_size);
})

FFI_THUNK(0x408, uint32_t, adk_httpx_response_get_body_size, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return (uint32_t)adk_httpx_response_get_body_size(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response));
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(const char *), adk_httpx_response_get_error, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_httpx_response_get_error(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response)));
})

FFI_THUNK(0x408A4, uint32_t, adk_httpx_response_get_headers_copy, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response, FFI_WASM_PTR const buffer, uint32_t buffer_size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    return (uint32_t)adk_httpx_response_get_headers_copy(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), FFI_PIN_WASM_PTR(buffer), (const size_t)buffer_size);
})

FFI_THUNK(0x408, uint32_t, adk_httpx_response_get_headers_size, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return (uint32_t)adk_httpx_response_get_headers_size(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response));
})

FFI_THUNK(0x808, int64_t, adk_httpx_response_get_response_code, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return adk_httpx_response_get_response_code(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response));
})

FFI_THUNK(0x408, FFI_ENUM(adk_httpx_result_e), adk_httpx_response_get_result, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return (FFI_ENUM(adk_httpx_result_e))adk_httpx_response_get_result(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response));
})

FFI_THUNK(0x408, FFI_ENUM(adk_future_status_e), adk_httpx_response_get_status, (FFI_NATIVE_PTR(const adk_httpx_response_t * const) response), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response), "response cannot be NULL");
    return (FFI_ENUM(adk_future_status_e))adk_httpx_response_get_status(FFI_GET_NATIVE_PTR(const adk_httpx_response_t * const, response));
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(adk_httpx_response_t *), adk_httpx_send, (FFI_NATIVE_PTR(adk_httpx_request_t * const) request), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_httpx_send(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request)));
})

FFI_THUNK(0x1044A, void, adk_init_main_display, (const int32_t display_index, const int32_t display_mode_index, FFI_WASM_PTR const adk_app_name), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(adk_app_name), "adk_app_name cannot be NULL");
    app_init_main_display(display_index, display_mode_index, FFI_PIN_WASM_PTR(adk_app_name));
})

FFI_THUNK(0x10, void, adk_leave_background_mode, (), {
    app_request_foreground();
})

FFI_THUNK(0x804A, FFI_NATIVE_PTR(adk_screenshot_t *), adk_load_screenshot, (FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(filename), "filename cannot be NULL");
    return FFI_SET_NATIVE_PTR(ffi_load_screenshot((FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(filename)));
})

FFI_THUNK(0x10A, void, adk_log_msg, (FFI_WASM_PTR const msg), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(msg), "msg cannot be NULL");
    adk_log_msg(FFI_PIN_WASM_PTR(msg));
})

FFI_THUNK(0x104, void, adk_notify_app_status, (FFI_ENUM(const sb_app_notify_e) notify), {
    ASSERT_MSG((sb_app_notify_logged_out == (FFI_ENUM(const sb_app_notify_e))notify) || (sb_app_notify_logged_in_not_entitled == (FFI_ENUM(const sb_app_notify_e))notify) || (sb_app_notify_logged_in_and_entitled == (FFI_ENUM(const sb_app_notify_e))notify) || (sb_app_notify_dismiss_system_loading_screen == (FFI_ENUM(const sb_app_notify_e))notify), "Argument must be one of [sb_app_notify_logged_out, sb_app_notify_logged_in_not_entitled, sb_app_notify_logged_in_and_entitled, sb_app_notify_dismiss_system_loading_screen]");
    adk_notify_app_status((FFI_ENUM(const sb_app_notify_e))notify);
})

FFI_THUNK(0x10A, void, adk_rand_create_generator, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, adk_rand_generator_t);
    *(adk_rand_generator_t *)FFI_PIN_WASM_PTR(ret_val) = adk_rand_create_generator();
})

FFI_THUNK(0x108A, void, adk_rand_create_generator_with_seed, (const uint64_t seed, FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, adk_rand_generator_t);
    *(adk_rand_generator_t *)FFI_PIN_WASM_PTR(ret_val) = adk_rand_create_generator_with_seed(seed);
})

FFI_THUNK(0x80A, uint64_t, adk_rand_next, (FFI_WASM_PTR const generator), {
    FFI_ASSERT_ALIGNED_WASM_PTR(generator, adk_rand_generator_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(generator), "generator cannot be NULL");
    return adk_rand_next(FFI_PIN_WASM_PTR(generator));
})

FFI_THUNK(0x10A, void, adk_read_microsecond_clock, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, microseconds_t);
    *(microseconds_t *)FFI_PIN_WASM_PTR(ret_val) = adk_read_microsecond_clock();
})

FFI_THUNK(0x10A, void, adk_read_millisecond_clock, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, milliseconds_t);
    *(milliseconds_t *)FFI_PIN_WASM_PTR(ret_val) = adk_read_millisecond_clock();
})

FFI_THUNK(0x10A, void, adk_read_nanosecond_clock, (FFI_WASM_PTR const ret_val), {
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, nanoseconds_t);
    *(nanoseconds_t *)FFI_PIN_WASM_PTR(ret_val) = sb_read_nanosecond_clock();
})

FFI_THUNK(0x108, void, adk_release_deeplink, (FFI_NATIVE_PTR(sb_deeplink_handle_t * const) handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(sb_deeplink_handle_t * const, handle), "handle cannot be NULL");
    sb_release_deeplink(FFI_GET_NATIVE_PTR(sb_deeplink_handle_t * const, handle));
})

FFI_THUNK(0x108, void, adk_release_screenshot, (FFI_NATIVE_PTR(adk_screenshot_t * const) screenshot), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_screenshot_t * const, screenshot), "screenshot cannot be NULL");
    ffi_release_screenshot(FFI_GET_NATIVE_PTR(adk_screenshot_t * const, screenshot));
})

FFI_THUNK(0x10AAA, void, adk_report_app_metrics, (FFI_WASM_PTR const app_id, FFI_WASM_PTR const app_name, FFI_WASM_PTR const app_version), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(app_id), "app_id cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(app_name), "app_name cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(app_version), "app_version cannot be NULL");
    adk_app_metrics_report(FFI_PIN_WASM_PTR(app_id), FFI_PIN_WASM_PTR(app_name), FFI_PIN_WASM_PTR(app_version));
})

FFI_THUNK(0x10844A, void, adk_save_screenshot, (FFI_NATIVE_PTR(const adk_screenshot_t * const) screenshot, FFI_ENUM(const image_save_file_type_e) file_type, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, screenshot), "screenshot cannot be NULL");
    ASSERT_MSG((image_save_png == (FFI_ENUM(const image_save_file_type_e))file_type) || (image_save_tga == (FFI_ENUM(const image_save_file_type_e))file_type), "Argument must be one of [image_save_png, image_save_tga]");
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(filename), "filename cannot be NULL");
    ffi_save_screenshot(FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, screenshot), (FFI_ENUM(const image_save_file_type_e))file_type, (FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(filename));
})

FFI_THUNK(0x40884, int32_t, adk_screenshot_compare, (FFI_NATIVE_PTR(const adk_screenshot_t * const) testcase_screenshot, FFI_NATIVE_PTR(const adk_screenshot_t * const) baseline_screenshot, const int32_t image_tolerance), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, testcase_screenshot), "testcase_screenshot cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, baseline_screenshot), "baseline_screenshot cannot be NULL");
    return adk_screenshot_compare(FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, testcase_screenshot), FFI_GET_NATIVE_PTR(const adk_screenshot_t * const, baseline_screenshot), image_tolerance);
})

FFI_THUNK(0x1088444A, void, adk_screenshot_dump_deltas, (FFI_NATIVE_PTR(adk_screenshot_t * const) testcase_screenshot, FFI_NATIVE_PTR(adk_screenshot_t * const) baseline_screenshot, const int32_t image_tolerance, FFI_ENUM(const image_save_file_type_e) file_type, FFI_ENUM(const sb_file_directory_e) directory, FFI_WASM_PTR const filename_prefix), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_screenshot_t * const, testcase_screenshot), "testcase_screenshot cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_screenshot_t * const, baseline_screenshot), "baseline_screenshot cannot be NULL");
    ASSERT_MSG((image_save_png == (FFI_ENUM(const image_save_file_type_e))file_type) || (image_save_tga == (FFI_ENUM(const image_save_file_type_e))file_type), "Argument must be one of [image_save_png, image_save_tga]");
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))directory) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))directory), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(filename_prefix), "filename_prefix cannot be NULL");
    adk_screenshot_dump_deltas(FFI_GET_NATIVE_PTR(adk_screenshot_t * const, testcase_screenshot), FFI_GET_NATIVE_PTR(adk_screenshot_t * const, baseline_screenshot), image_tolerance, (FFI_ENUM(const image_save_file_type_e))file_type, (FFI_ENUM(const sb_file_directory_e))directory, FFI_PIN_WASM_PTR(filename_prefix));
})

FFI_THUNK(0x104, void, adk_set_memory_mode, (FFI_ENUM(const adk_memory_mode_e) memory_mode), {
    ASSERT_MSG((adk_memory_mode_low == (FFI_ENUM(const adk_memory_mode_e))memory_mode) || (adk_memory_mode_high == (FFI_ENUM(const adk_memory_mode_e))memory_mode), "Argument must be one of [adk_memory_mode_low, adk_memory_mode_high]");
    adk_set_memory_mode((FFI_ENUM(const adk_memory_mode_e))memory_mode);
})

FFI_THUNK(0x4044, int32_t, adk_set_refresh_rate, (const int32_t refresh_rate, const int32_t video_fps), {
    return adk_set_refresh_rate(refresh_rate, video_fps);
})

FFI_THUNK(0x104AA, void, adk_stat, (FFI_ENUM(const sb_file_directory_e) mount_point, FFI_WASM_PTR const subpath, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG((sb_app_root_directory == (FFI_ENUM(const sb_file_directory_e))mount_point) || (sb_app_config_directory == (FFI_ENUM(const sb_file_directory_e))mount_point) || (sb_app_cache_directory == (FFI_ENUM(const sb_file_directory_e))mount_point), "Argument must be one of [sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory]");
    ASSERT_MSG(FFI_PIN_WASM_PTR(subpath), "subpath cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, sb_stat_result_t);
    *(sb_stat_result_t *)FFI_PIN_WASM_PTR(ret_val) = adk_stat((FFI_ENUM(const sb_file_directory_e))mount_point, FFI_PIN_WASM_PTR(subpath));
})

FFI_THUNK(0x10A, void, adk_text_to_speech, (FFI_WASM_PTR const text), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    sb_text_to_speech(FFI_PIN_WASM_PTR(text));
})

FFI_THUNK(0x408A, FFI_ENUM(adk_websocket_message_type_e), adk_websocket_begin_read_bridge, (FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle, FFI_WASM_PTR const out_buffer), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), "ws_handle cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(out_buffer, native_slice_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(out_buffer), "out_buffer cannot be NULL");
    return (FFI_ENUM(adk_websocket_message_type_e))adk_websocket_begin_read_bridge(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), FFI_PIN_WASM_PTR(out_buffer));
})

FFI_THUNK(0x108, void, adk_websocket_close, (FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), "ws_handle cannot be NULL");
    adk_websocket_close(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), MALLOC_TAG);
})

FFI_THUNK(0x80AA8AA, FFI_NATIVE_PTR(adk_websocket_handle_t *), adk_websocket_create_raw, (FFI_WASM_PTR const url, FFI_WASM_PTR const supported_protocols, FFI_NATIVE_PTR(adk_http_header_list_t * const) header_list, FFI_WASM_PTR const success_callback, FFI_WASM_PTR const error_callback), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(url), "url cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(supported_protocols), "supported_protocols cannot be NULL");
    return FFI_SET_NATIVE_PTR(adk_websocket_create_raw(FFI_PIN_WASM_PTR(url), FFI_PIN_WASM_PTR(supported_protocols), FFI_GET_NATIVE_PTR(adk_http_header_list_t * const, header_list), FFI_WASM_PTR_OFFSET(success_callback), FFI_WASM_PTR_OFFSET(error_callback)));
})

FFI_THUNK(0x108, void, adk_websocket_end_read, (FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), "ws_handle cannot be NULL");
    adk_websocket_end_read(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), MALLOC_TAG);
})

FFI_THUNK(0x408, FFI_ENUM(adk_websocket_status_e), adk_websocket_get_status, (FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), "ws_handle cannot be NULL");
    return (FFI_ENUM(adk_websocket_status_e))adk_websocket_get_status(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle));
})

FFI_THUNK(0x408A44AA, FFI_ENUM(adk_websocket_status_e), adk_websocket_send_raw, (FFI_NATIVE_PTR(adk_websocket_handle_t * const) ws_handle, FFI_WASM_PTR const ptr, const int32_t size, FFI_ENUM(const adk_websocket_message_type_e) message_type, FFI_WASM_PTR const success_callback, FFI_WASM_PTR const error_callback), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), "ws_handle cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(ptr), "ptr cannot be NULL");
    ASSERT_MSG((adk_websocket_message_error == (FFI_ENUM(const adk_websocket_message_type_e))message_type) || (adk_websocket_message_none == (FFI_ENUM(const adk_websocket_message_type_e))message_type) || (adk_websocket_message_binary == (FFI_ENUM(const adk_websocket_message_type_e))message_type) || (adk_websocket_message_text == (FFI_ENUM(const adk_websocket_message_type_e))message_type), "Argument must be one of [adk_websocket_message_error, adk_websocket_message_none, adk_websocket_message_binary, adk_websocket_message_text]");
    return (FFI_ENUM(adk_websocket_status_e))adk_websocket_send_raw(FFI_GET_NATIVE_PTR(adk_websocket_handle_t * const, ws_handle), FFI_PIN_WASM_PTR(ptr), size, (FFI_ENUM(const adk_websocket_message_type_e))message_type, FFI_WASM_PTR_OFFSET(success_callback), FFI_WASM_PTR_OFFSET(error_callback));
})

FFI_THUNK(0x10AFAA4, void, cg_context_arc, (FFI_WASM_PTR const pos, const float radius, FFI_WASM_PTR const start, FFI_WASM_PTR const end, FFI_ENUM(const cg_rotation_e) rotation), {
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(start, cg_rads_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(end, cg_rads_t);
    ASSERT_MSG((cg_rotation_counter_clock_wise == (FFI_ENUM(const cg_rotation_e))rotation) || (cg_rotation_clock_wise == (FFI_ENUM(const cg_rotation_e))rotation), "Argument must be one of [cg_rotation_counter_clock_wise, cg_rotation_clock_wise]");
    cg_context_arc(*(cg_vec2_t *)FFI_PIN_WASM_PTR(pos), radius, *(cg_rads_t *)FFI_PIN_WASM_PTR(start), *(cg_rads_t *)FFI_PIN_WASM_PTR(end), (FFI_ENUM(const cg_rotation_e))rotation, MALLOC_TAG);
})

FFI_THUNK(0x10AAF, void, cg_context_arc_to, (FFI_WASM_PTR const pos1, FFI_WASM_PTR const pos2, const float radius), {
    FFI_ASSERT_ALIGNED_WASM_PTR(pos1, cg_vec2_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(pos2, cg_vec2_t);
    cg_context_arc_to(*(cg_vec2_t *)FFI_PIN_WASM_PTR(pos1), *(cg_vec2_t *)FFI_PIN_WASM_PTR(pos2), radius, MALLOC_TAG);
})

FFI_THUNK(0x10, void, cg_context_begin_path, (), {
    cg_context_begin_path(MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_blit_video_frame, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_blit_video_frame(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect));
})

FFI_THUNK(0x10A, void, cg_context_clear_rect, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_clear_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect), MALLOC_TAG);
})

FFI_THUNK(0x10, void, cg_context_close_path, (), {
    cg_context_close_path(MALLOC_TAG);
})

FFI_THUNK(0x808F4, FFI_NATIVE_PTR(cg_font_context_t *), cg_context_create_font_context, (FFI_NATIVE_PTR(cg_font_file_t * const) cg_font, const float size, const int32_t tab_space_multiplier), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_file_t * const, cg_font), "cg_font cannot be NULL");
    return FFI_SET_NATIVE_PTR(cg_context_create_font_context(FFI_GET_NATIVE_PTR(cg_font_file_t * const, cg_font), size, tab_space_multiplier, MALLOC_TAG));
})

FFI_THUNK(0x108A, void, cg_context_draw_image, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const pos), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    cg_context_draw_image(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), *(cg_vec2_t *)FFI_PIN_WASM_PTR(pos));
})

FFI_THUNK(0x108AA, void, cg_context_draw_image_9slice, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const margin, FFI_WASM_PTR const dst), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(margin, cg_margins_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(dst, cg_rect_t);
    cg_context_draw_image_9slice(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), *(cg_margins_t *)FFI_PIN_WASM_PTR(margin), *(cg_rect_t *)FFI_PIN_WASM_PTR(dst));
})

FFI_THUNK(0x108AA, void, cg_context_draw_image_rect, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const src, FFI_WASM_PTR const dst), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(src, cg_rect_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(dst, cg_rect_t);
    cg_context_draw_image_rect(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), *(cg_rect_t *)FFI_PIN_WASM_PTR(src), *(cg_rect_t *)FFI_PIN_WASM_PTR(dst));
})

FFI_THUNK(0x1088AA, void, cg_context_draw_image_rect_alpha_mask, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_NATIVE_PTR(const cg_image_t * const) mask, FFI_WASM_PTR const src, FFI_WASM_PTR const dst), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, mask), "mask cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(src, cg_rect_t);
    FFI_ASSERT_ALIGNED_WASM_PTR(dst, cg_rect_t);
    cg_context_draw_image_rect_alpha_mask(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), FFI_GET_NATIVE_PTR(const cg_image_t * const, mask), *(cg_rect_t *)FFI_PIN_WASM_PTR(src), *(cg_rect_t *)FFI_PIN_WASM_PTR(dst));
})

FFI_THUNK(0x108A, void, cg_context_draw_image_scale, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const rect), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_draw_image_scale(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), *(cg_rect_t *)FFI_PIN_WASM_PTR(rect));
})

FFI_THUNK(0x10, void, cg_context_end_path, (), {
    cg_context_end_path(MALLOC_TAG);
})

FFI_THUNK(0xF0, float, cg_context_feather, (), {
    return cg_context_feather();
})

FFI_THUNK(0x10, void, cg_context_fill, (), {
    cg_context_fill(MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_fill_rect, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_fill_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect), MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_fill_style, (FFI_WASM_PTR const color), {
    FFI_ASSERT_ALIGNED_WASM_PTR(color, cg_color_t);
    cg_context_fill_style(*(cg_color_t *)FFI_PIN_WASM_PTR(color));
})

FFI_THUNK(0x104, void, cg_context_fill_style_hex, (const int32_t color), {
    cg_context_fill_style_hex(color);
})

FFI_THUNK(0x10A8, void, cg_context_fill_style_image, (FFI_WASM_PTR const color, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    FFI_ASSERT_ALIGNED_WASM_PTR(color, cg_color_t);
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    cg_context_fill_style_image(*(cg_color_t *)FFI_PIN_WASM_PTR(color), FFI_GET_NATIVE_PTR(const cg_image_t * const, image));
})

FFI_THUNK(0x1048, void, cg_context_fill_style_image_hex, (const int32_t color, FFI_NATIVE_PTR(const cg_image_t * const) image), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    cg_context_fill_style_image_hex(color, FFI_GET_NATIVE_PTR(const cg_image_t * const, image));
})

FFI_THUNK(0x108AAA, void, cg_context_fill_text, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const pos, FFI_WASM_PTR const text, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_font_metrics_t);
    *(cg_font_metrics_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_fill_text(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), *(cg_vec2_t *)FFI_PIN_WASM_PTR(pos), FFI_PIN_WASM_PTR(text));
})

FFI_THUNK(0x108AFFAA4A, void, cg_context_fill_text_block_with_options, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text_rect, const float text_scroll_offset, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_WASM_PTR const optional_ellipses, FFI_ENUM(const cg_text_block_options_e) options, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(text_rect, cg_rect_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_font_metrics_t);
    *(cg_font_metrics_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_fill_text_block_with_options(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), *(cg_rect_t *)FFI_PIN_WASM_PTR(text_rect), text_scroll_offset, extra_line_spacing, FFI_PIN_WASM_PTR(text), FFI_PIN_WASM_PTR(optional_ellipses), (FFI_ENUM(const cg_text_block_options_e))options);
})

FFI_THUNK(0x108AA4A, void, cg_context_fill_text_with_options, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const pos, FFI_WASM_PTR const text, FFI_ENUM(const cg_font_fill_options_e) options, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_font_metrics_t);
    *(cg_font_metrics_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_fill_text_with_options(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), *(cg_vec2_t *)FFI_PIN_WASM_PTR(pos), FFI_PIN_WASM_PTR(text), (FFI_ENUM(const cg_font_fill_options_e))options);
})

FFI_THUNK(0x104, void, cg_context_fill_with_options, (FFI_ENUM(const cg_path_options_e) options), {
    ASSERT_MSG((cg_path_options_none == (FFI_ENUM(const cg_path_options_e))options) || (cg_path_options_concave == (FFI_ENUM(const cg_path_options_e))options) || (cg_path_options_no_fethering == (FFI_ENUM(const cg_path_options_e))options), "Argument must be one of [cg_path_options_none, cg_path_options_concave, cg_path_options_no_fethering]");
    cg_context_fill_with_options((FFI_ENUM(const cg_path_options_e))options, MALLOC_TAG);
})

FFI_THUNK(0x10, void, cg_context_font_clear_glyph_cache, (), {
    cg_context_font_clear_glyph_cache();
})

FFI_THUNK(0x108, void, cg_context_font_context_free, (FFI_NATIVE_PTR(cg_font_context_t *) font), {
    cg_context_font_context_free(FFI_GET_NATIVE_PTR(cg_font_context_t *, font), MALLOC_TAG);
})

FFI_THUNK(0x108, void, cg_context_font_file_free, (FFI_NATIVE_PTR(cg_font_file_t * const) font), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_file_t * const, font), "font cannot be NULL");
    cg_context_font_file_free(FFI_GET_NATIVE_PTR(cg_font_file_t * const, font), MALLOC_TAG);
})

FFI_THUNK(0x108A, void, cg_context_font_precache_glyphs, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const characters), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(characters), "characters cannot be NULL");
    cg_context_font_precache_glyphs(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), FFI_PIN_WASM_PTR(characters));
})

FFI_THUNK(0x108F, void, cg_context_font_set_virtual_size, (FFI_NATIVE_PTR(cg_font_context_t *) font_ctx, float size), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t *, font_ctx), "font_ctx cannot be NULL");
    cg_context_font_set_virtual_size(FFI_GET_NATIVE_PTR(cg_font_context_t *, font_ctx), size);
})

FFI_THUNK(0xF0, float, cg_context_get_alpha_test_threshold, (), {
    return cg_context_get_alpha_test_threshold();
})

FFI_THUNK(0x408, uint32_t, cg_context_get_image_frame_count, (FFI_NATIVE_PTR(const cg_image_t * const) cg_image), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, cg_image), "cg_image cannot be NULL");
    return cg_context_get_image_frame_count(FFI_GET_NATIVE_PTR(const cg_image_t * const, cg_image));
})

FFI_THUNK(0x40, FFI_ENUM(cg_blend_mode_e), cg_context_get_punchthrough_blend_mode, (), {
    return (FFI_ENUM(cg_blend_mode_e))cg_context_get_punchthrough_blend_mode();
})

FFI_THUNK(0xF08FFA4, float, cg_context_get_text_block_height, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, const float line_width, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_ENUM(const cg_text_block_options_e) options), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    return cg_context_get_text_block_height(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), line_width, extra_line_spacing, FFI_PIN_WASM_PTR(text), (FFI_ENUM(const cg_text_block_options_e))options);
})

FFI_THUNK(0x108AFFA4A, void, cg_context_get_text_block_page_offsets, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text_rect, const float scroll_offset, const float extra_line_spacing, FFI_WASM_PTR const text, FFI_ENUM(const cg_text_block_options_e) options, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(text_rect, cg_rect_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_text_block_page_offsets_t);
    *(cg_text_block_page_offsets_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_get_text_block_page_offsets(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), *(cg_rect_t *)FFI_PIN_WASM_PTR(text_rect), scroll_offset, extra_line_spacing, FFI_PIN_WASM_PTR(text), (FFI_ENUM(const cg_text_block_options_e))options);
})

FFI_THUNK(0xF0, float, cg_context_global_alpha, (), {
    return cg_context_global_alpha();
})

FFI_THUNK(0x10, void, cg_context_identity, (), {
    cg_context_identity();
})

FFI_THUNK(0x108, void, cg_context_image_free, (FFI_NATIVE_PTR(cg_image_t * const) image), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_image_t * const, image), "image cannot be NULL");
    cg_context_image_free(FFI_GET_NATIVE_PTR(cg_image_t * const, image), MALLOC_TAG);
})

FFI_THUNK(0x108A, void, cg_context_image_rect, (FFI_NATIVE_PTR(const cg_image_t * const) image, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_rect_t);
    *(cg_rect_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_image_rect(FFI_GET_NATIVE_PTR(const cg_image_t * const, image));
})

FFI_THUNK(0x10844, void, cg_context_image_set_repeat, (FFI_NATIVE_PTR(cg_image_t * const) image, const int32_t repeat_x, const int32_t repeat_y), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_image_t * const, image), "image cannot be NULL");
    cg_context_image_set_repeat(FFI_GET_NATIVE_PTR(cg_image_t * const, image), repeat_x ? true : false, repeat_y ? true : false);
})

FFI_THUNK(0x10A, void, cg_context_line_to, (FFI_WASM_PTR const pos), {
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    cg_context_line_to(*(cg_vec2_t *)FFI_PIN_WASM_PTR(pos), MALLOC_TAG);
})

FFI_THUNK(0x80A44, FFI_NATIVE_PTR(cg_font_file_t *), cg_context_load_font_file_async, (FFI_WASM_PTR const filepath, FFI_ENUM(const cg_memory_region_e) memory_region, FFI_ENUM(const cg_font_load_opts_e) font_load_opts), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(filepath), "filepath cannot be NULL");
    ASSERT_MSG((cg_memory_region_high == (FFI_ENUM(const cg_memory_region_e))memory_region) || (cg_memory_region_low == (FFI_ENUM(const cg_memory_region_e))memory_region) || (cg_memory_region_high_to_low == (FFI_ENUM(const cg_memory_region_e))memory_region), "Argument must be one of [cg_memory_region_high, cg_memory_region_low, cg_memory_region_high_to_low]");
    return FFI_SET_NATIVE_PTR(cg_context_load_font_file_async(FFI_PIN_WASM_PTR(filepath), (FFI_ENUM(const cg_memory_region_e))memory_region, (FFI_ENUM(const cg_font_load_opts_e))font_load_opts, MALLOC_TAG));
})

FFI_THUNK(0x80A44, FFI_NATIVE_PTR(cg_image_t *), cg_context_load_image_async, (FFI_WASM_PTR const file_location, FFI_ENUM(const cg_memory_region_e) memory_region, FFI_ENUM(const cg_image_load_opts_e) image_load_opts), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(file_location), "file_location cannot be NULL");
    ASSERT_MSG((cg_memory_region_high == (FFI_ENUM(const cg_memory_region_e))memory_region) || (cg_memory_region_low == (FFI_ENUM(const cg_memory_region_e))memory_region) || (cg_memory_region_high_to_low == (FFI_ENUM(const cg_memory_region_e))memory_region), "Argument must be one of [cg_memory_region_high, cg_memory_region_low, cg_memory_region_high_to_low]");
    return FFI_SET_NATIVE_PTR(cg_context_load_image_async(FFI_PIN_WASM_PTR(file_location), (FFI_ENUM(const cg_memory_region_e))memory_region, (FFI_ENUM(const cg_image_load_opts_e))image_load_opts, MALLOC_TAG));
})

FFI_THUNK(0x10A, void, cg_context_move_to, (FFI_WASM_PTR const pos), {
    FFI_ASSERT_ALIGNED_WASM_PTR(pos, cg_vec2_t);
    cg_context_move_to(*(cg_vec2_t *)FFI_PIN_WASM_PTR(pos), MALLOC_TAG);
})

FFI_THUNK(0x80844, FFI_NATIVE_PTR(const cg_pattern_t *), cg_context_pattern, (FFI_NATIVE_PTR(const cg_image_t * const) image, const int32_t repeat_x, const int32_t repeat_y), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    return FFI_SET_NATIVE_PTR(cg_context_pattern(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), repeat_x ? true : false, repeat_y ? true : false));
})

FFI_THUNK(0x10FFFF, void, cg_context_quad_bezier_to, (const float cpx, const float cpy, const float x, const float y), {
    cg_context_quad_bezier_to(cpx, cpy, x, y, MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_rect, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect), MALLOC_TAG);
})

FFI_THUNK(0x10, void, cg_context_restore, (), {
    cg_context_restore();
})

FFI_THUNK(0x10A, void, cg_context_rotate, (FFI_WASM_PTR const angle), {
    FFI_ASSERT_ALIGNED_WASM_PTR(angle, cg_rads_t);
    cg_context_rotate(*(cg_rads_t *)FFI_PIN_WASM_PTR(angle));
})

FFI_THUNK(0x10AF, void, cg_context_rounded_rect, (FFI_WASM_PTR const rect, const float radius), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_rounded_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect), radius, MALLOC_TAG);
})

FFI_THUNK(0x10, void, cg_context_save, (), {
    cg_context_save();
})

FFI_THUNK(0x10A, void, cg_context_scale, (FFI_WASM_PTR const scale), {
    FFI_ASSERT_ALIGNED_WASM_PTR(scale, cg_vec2_t);
    cg_context_scale(*(cg_vec2_t *)FFI_PIN_WASM_PTR(scale));
})

FFI_THUNK(0x10F, void, cg_context_set_alpha_test_threshold, (const float threshold), {
    cg_context_set_alpha_test_threshold(threshold);
})

FFI_THUNK(0x10A, void, cg_context_set_clip_rect, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_set_clip_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect));
})

FFI_THUNK(0x104, void, cg_context_set_clip_state, (FFI_ENUM(const cg_clip_state_e) clip_state), {
    ASSERT_MSG((cg_clip_state_enabled == (FFI_ENUM(const cg_clip_state_e))clip_state) || (cg_clip_state_disabled == (FFI_ENUM(const cg_clip_state_e))clip_state), "Argument must be one of [cg_clip_state_enabled, cg_clip_state_disabled]");
    cg_context_set_clip_state((FFI_ENUM(const cg_clip_state_e))clip_state);
})

FFI_THUNK(0x10F, void, cg_context_set_feather, (const float feather), {
    cg_context_set_feather(feather);
})

FFI_THUNK(0x108A, void, cg_context_set_font_context_missing_glyph_indicator, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const missing_glyph_indicator), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(missing_glyph_indicator), "missing_glyph_indicator cannot be NULL");
    cg_context_set_font_context_missing_glyph_indicator(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), FFI_PIN_WASM_PTR(missing_glyph_indicator));
})

FFI_THUNK(0x10F, void, cg_context_set_global_alpha, (const float alpha), {
    cg_context_set_global_alpha(alpha);
})

FFI_THUNK(0x10A, void, cg_context_set_global_missing_glyph_indicator, (FFI_WASM_PTR const missing_glyph_indicator), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(missing_glyph_indicator), "missing_glyph_indicator cannot be NULL");
    cg_context_set_global_missing_glyph_indicator(FFI_PIN_WASM_PTR(missing_glyph_indicator));
})

FFI_THUNK(0x1084, void, cg_context_set_image_animation_state, (FFI_NATIVE_PTR(cg_image_t * const) image, FFI_ENUM(const cg_image_animation_state_e) image_animation_state), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_image_t * const, image), "image cannot be NULL");
    ASSERT_MSG((cg_image_animation_stopped == (FFI_ENUM(const cg_image_animation_state_e))image_animation_state) || (cg_image_animation_running == (FFI_ENUM(const cg_image_animation_state_e))image_animation_state) || (cg_image_animation_restart == (FFI_ENUM(const cg_image_animation_state_e))image_animation_state), "Argument must be one of [cg_image_animation_stopped, cg_image_animation_running, cg_image_animation_restart]");
    cg_context_set_image_animation_state(FFI_GET_NATIVE_PTR(cg_image_t * const, image), (FFI_ENUM(const cg_image_animation_state_e))image_animation_state);
})

FFI_THUNK(0x1084, void, cg_context_set_image_frame_index, (FFI_NATIVE_PTR(cg_image_t * const) cg_image, const uint32_t image_index), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_image_t * const, cg_image), "cg_image cannot be NULL");
    cg_context_set_image_frame_index(FFI_GET_NATIVE_PTR(cg_image_t * const, cg_image), image_index);
})

FFI_THUNK(0x10F, void, cg_context_set_line_width, (const float width), {
    cg_context_set_line_width(width);
})

FFI_THUNK(0x104, void, cg_context_set_punchthrough_blend_mode, (FFI_ENUM(const cg_blend_mode_e) blend_mode), {
    ASSERT_MSG((cg_blend_mode_src_alpha_rgb == (FFI_ENUM(const cg_blend_mode_e))blend_mode) || (cg_blend_mode_src_alpha_all == (FFI_ENUM(const cg_blend_mode_e))blend_mode) || (cg_blend_mode_blit == (FFI_ENUM(const cg_blend_mode_e))blend_mode) || (cg_blend_mode_alpha_test == (FFI_ENUM(const cg_blend_mode_e))blend_mode), "Argument must be one of [cg_blend_mode_src_alpha_rgb, cg_blend_mode_src_alpha_all, cg_blend_mode_blit, cg_blend_mode_alpha_test]");
    cg_context_set_punchthrough_blend_mode((FFI_ENUM(const cg_blend_mode_e))blend_mode);
})

FFI_THUNK(0x10, void, cg_context_stroke, (), {
    cg_context_stroke(MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_stroke_rect, (FFI_WASM_PTR const rect), {
    FFI_ASSERT_ALIGNED_WASM_PTR(rect, cg_rect_t);
    cg_context_stroke_rect(*(cg_rect_t *)FFI_PIN_WASM_PTR(rect), MALLOC_TAG);
})

FFI_THUNK(0x10A, void, cg_context_stroke_style, (FFI_WASM_PTR const color), {
    FFI_ASSERT_ALIGNED_WASM_PTR(color, cg_color_t);
    cg_context_stroke_style(*(cg_color_t *)FFI_PIN_WASM_PTR(color));
})

FFI_THUNK(0x104, void, cg_context_stroke_with_options, (FFI_ENUM(const cg_path_options_e) options), {
    ASSERT_MSG((cg_path_options_none == (FFI_ENUM(const cg_path_options_e))options) || (cg_path_options_concave == (FFI_ENUM(const cg_path_options_e))options) || (cg_path_options_no_fethering == (FFI_ENUM(const cg_path_options_e))options), "Argument must be one of [cg_path_options_none, cg_path_options_concave, cg_path_options_no_fethering]");
    cg_context_stroke_with_options((FFI_ENUM(const cg_path_options_e))options, MALLOC_TAG);
})

FFI_THUNK(0x108AA, void, cg_context_text_measure, (FFI_NATIVE_PTR(cg_font_context_t * const) font_ctx, FFI_WASM_PTR const text, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), "font_ctx cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(text), "text cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, cg_font_metrics_t);
    *(cg_font_metrics_t *)FFI_PIN_WASM_PTR(ret_val) = cg_context_text_measure(FFI_GET_NATIVE_PTR(cg_font_context_t * const, font_ctx), FFI_PIN_WASM_PTR(text));
})

FFI_THUNK(0x10A, void, cg_context_translate, (FFI_WASM_PTR const translation), {
    FFI_ASSERT_ALIGNED_WASM_PTR(translation, cg_vec2_t);
    cg_context_translate(*(cg_vec2_t *)FFI_PIN_WASM_PTR(translation));
})

FFI_THUNK(0x80, FFI_NATIVE_PTR(cg_context_t *), cg_get_context, (), {
    return FFI_SET_NATIVE_PTR(cg_get_context());
})

FFI_THUNK(0x408, FFI_ENUM(cg_font_async_load_status_e), cg_get_font_load_status, (FFI_NATIVE_PTR(const cg_font_file_t * const) cg_font), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_font_file_t * const, cg_font), "cg_font cannot be NULL");
    return (FFI_ENUM(cg_font_async_load_status_e))cg_get_font_load_status(FFI_GET_NATIVE_PTR(const cg_font_file_t * const, cg_font));
})

FFI_THUNK(0x408, FFI_ENUM(cg_image_async_load_status_e), cg_get_image_load_status, (FFI_NATIVE_PTR(const cg_image_t * const) image), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    return (FFI_ENUM(cg_image_async_load_status_e))cg_get_image_load_status(FFI_GET_NATIVE_PTR(const cg_image_t * const, image));
})

FFI_THUNK(0x408, int32_t, cg_get_image_ripcut_error_code, (FFI_NATIVE_PTR(const cg_image_t * const) image), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const cg_image_t * const, image), "image cannot be NULL");
    return cg_get_image_ripcut_error_code(FFI_GET_NATIVE_PTR(const cg_image_t * const, image));
})

FFI_THUNK(0x108, void, cg_set_context, (FFI_NATIVE_PTR(cg_context_t * const) ctx), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(cg_context_t * const, ctx), "ctx cannot be NULL");
    cg_set_context(FFI_GET_NATIVE_PTR(cg_context_t * const, ctx));
})

FFI_THUNK(0x408, int32_t, cstrlen, (FFI_NATIVE_PTR(const char * const) str), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const char * const, str), "str cannot be NULL");
    return cstrlen(FFI_GET_NATIVE_PTR(const char * const, str));
})

FFI_THUNK(0x10A4A4A4444A, void, json_deflate_async, (FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_WASM_PTR const json_data, uint32_t json_data_length, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(schema_layout), "schema_layout cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(json_data), "json_data cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG((json_deflate_parse_target_wasm == (FFI_ENUM(const json_deflate_parse_target_e))target) || (json_deflate_parse_target_native == (FFI_ENUM(const json_deflate_parse_target_e))target), "Argument must be one of [json_deflate_parse_target_wasm, json_deflate_parse_target_native]");
    json_deflate_async(FFI_PIN_WASM_PTR(schema_layout), schema_layout_length, FFI_PIN_WASM_PTR(json_data), json_data_length, FFI_PIN_WASM_PTR(buffer), buffer_size, expected_size, schema_hash, (FFI_ENUM(const json_deflate_parse_target_e))target, FFI_WASM_PTR_OFFSET(on_complete));
})

FFI_THUNK(0x10A48A4444A, void, json_deflate_from_http_async, (FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_NATIVE_PTR(adk_curl_handle_t * const) http, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(schema_layout), "schema_layout cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, http), "http cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG((json_deflate_parse_target_wasm == (FFI_ENUM(const json_deflate_parse_target_e))target) || (json_deflate_parse_target_native == (FFI_ENUM(const json_deflate_parse_target_e))target), "Argument must be one of [json_deflate_parse_target_wasm, json_deflate_parse_target_native]");
    json_deflate_from_http_async(FFI_PIN_WASM_PTR(schema_layout), schema_layout_length, FFI_GET_NATIVE_PTR(adk_curl_handle_t * const, http), FFI_PIN_WASM_PTR(buffer), buffer_size, expected_size, schema_hash, (FFI_ENUM(const json_deflate_parse_target_e))target, FFI_WASM_PTR_OFFSET(on_complete));
})

FFI_THUNK(0x108, void, json_deflate_http_future_drop, (FFI_NATIVE_PTR(json_deflate_http_future_t * const) future), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(json_deflate_http_future_t * const, future), "future cannot be NULL");
    json_deflate_http_future_drop(FFI_GET_NATIVE_PTR(json_deflate_http_future_t * const, future));
})

FFI_THUNK(0x808, FFI_NATIVE_PTR(adk_httpx_response_t *), json_deflate_http_future_get_response, (FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future), "future cannot be NULL");
    return FFI_SET_NATIVE_PTR(json_deflate_http_future_get_response(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future)));
})

FFI_THUNK(0x108A, void, json_deflate_http_future_get_result, (FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future, FFI_WASM_PTR const ret_val), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future), "future cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(ret_val, json_deflate_parse_result_t);
    *(json_deflate_parse_result_t *)FFI_PIN_WASM_PTR(ret_val) = json_deflate_http_future_get_result(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future));
})

FFI_THUNK(0x408, FFI_ENUM(adk_future_status_e), json_deflate_http_future_get_status, (FFI_NATIVE_PTR(const json_deflate_http_future_t * const) future), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future), "future cannot be NULL");
    return (FFI_ENUM(adk_future_status_e))json_deflate_http_future_get_status(FFI_GET_NATIVE_PTR(const json_deflate_http_future_t * const, future));
})

FFI_THUNK(0x10A484A4444A, void, json_deflate_native_async, (FFI_WASM_PTR const schema_layout, const uint32_t schema_layout_length, FFI_NATIVE_PTR(const uint8_t * const) json_data, uint32_t json_data_length, FFI_WASM_PTR const buffer, const uint32_t buffer_size, const uint32_t expected_size, const uint32_t schema_hash, FFI_ENUM(const json_deflate_parse_target_e) target, FFI_WASM_PTR const on_complete), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(schema_layout), "schema_layout cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const uint8_t * const, json_data), "json_data cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG((json_deflate_parse_target_wasm == (FFI_ENUM(const json_deflate_parse_target_e))target) || (json_deflate_parse_target_native == (FFI_ENUM(const json_deflate_parse_target_e))target), "Argument must be one of [json_deflate_parse_target_wasm, json_deflate_parse_target_native]");
    json_deflate_native_async(FFI_PIN_WASM_PTR(schema_layout), schema_layout_length, FFI_GET_NATIVE_PTR(const uint8_t * const, json_data), json_data_length, FFI_PIN_WASM_PTR(buffer), buffer_size, expected_size, schema_hash, (FFI_ENUM(const json_deflate_parse_target_e))target, FFI_WASM_PTR_OFFSET(on_complete));
})

FFI_THUNK(0x80A88A8444, FFI_NATIVE_PTR(json_deflate_http_future_t *), json_deflate_parse_httpx_async, (FFI_WASM_PTR const schema_layout, uint64_t schema_layout_size, FFI_NATIVE_PTR(adk_httpx_request_t * const) request, FFI_WASM_PTR const buffer, uint64_t buffer_size, FFI_ENUM(const json_deflate_parse_target_e) target, const uint32_t expected_size, const uint32_t schema_hash), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(schema_layout), "schema_layout cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), "request cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG((json_deflate_parse_target_wasm == (FFI_ENUM(const json_deflate_parse_target_e))target) || (json_deflate_parse_target_native == (FFI_ENUM(const json_deflate_parse_target_e))target), "Argument must be one of [json_deflate_parse_target_wasm, json_deflate_parse_target_native]");
    return FFI_SET_NATIVE_PTR(json_deflate_parse_httpx_async(FFI_PIN_WASM_PTR(schema_layout), (const size_t)schema_layout_size, FFI_GET_NATIVE_PTR(adk_httpx_request_t * const, request), FFI_PIN_WASM_PTR(buffer), (const size_t)buffer_size, (FFI_ENUM(const json_deflate_parse_target_e))target, expected_size, schema_hash));
})

FFI_THUNK(0x108A8A8444, void, json_deflate_parse_httpx_resize, (FFI_NATIVE_PTR(json_deflate_http_future_t * const) future, FFI_WASM_PTR const schema_layout, uint64_t schema_layout_size, FFI_WASM_PTR const buffer, uint64_t buffer_size, FFI_ENUM(const json_deflate_parse_target_e) target, const uint32_t expected_size, const uint32_t schema_hash), {
    ASSERT_MSG(FFI_GET_NATIVE_PTR(json_deflate_http_future_t * const, future), "future cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(schema_layout), "schema_layout cannot be NULL");
    ASSERT_MSG(FFI_PIN_WASM_PTR(buffer), "buffer cannot be NULL");
    ASSERT_MSG((json_deflate_parse_target_wasm == (FFI_ENUM(const json_deflate_parse_target_e))target) || (json_deflate_parse_target_native == (FFI_ENUM(const json_deflate_parse_target_e))target), "Argument must be one of [json_deflate_parse_target_wasm, json_deflate_parse_target_native]");
    json_deflate_parse_httpx_resize(FFI_GET_NATIVE_PTR(json_deflate_http_future_t * const, future), FFI_PIN_WASM_PTR(schema_layout), (const size_t)schema_layout_size, FFI_PIN_WASM_PTR(buffer), (const size_t)buffer_size, (FFI_ENUM(const json_deflate_parse_target_e))target, expected_size, schema_hash);
})

FFI_THUNK(0x10A844, void, memcpy_n2r, (FFI_WASM_PTR const dst, FFI_NATIVE_PTR(const void * const) src, const int32_t offset, const int32_t num_bytes), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(dst), "dst cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const void * const, src), "src cannot be NULL");
    memcpy_n2r(FFI_PIN_WASM_PTR(dst), FFI_GET_NATIVE_PTR(const void * const, src), offset, num_bytes);
})

FFI_THUNK(0x40A44, int32_t, read_events, (FFI_WASM_PTR const evbuffer, int32_t bufsize, int32_t sizeof_event), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(evbuffer), "evbuffer cannot be NULL");
    return read_events(FFI_PIN_WASM_PTR(evbuffer), bufsize, sizeof_event);
})

FFI_THUNK(0x40A48, int32_t, strcpy_n2r, (FFI_WASM_PTR const dst, const int32_t dst_buff_size, FFI_NATIVE_PTR(const char * const) src), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(dst), "dst cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const char * const, src), "src cannot be NULL");
    return strcpy_n2r(FFI_PIN_WASM_PTR(dst), dst_buff_size, FFI_GET_NATIVE_PTR(const char * const, src));
})

FFI_THUNK(0x40A484, int32_t, strcpy_n2r_upto, (FFI_WASM_PTR const dst, const int32_t dst_buff_size, FFI_NATIVE_PTR(const char * const) src, const int32_t max_copy_len), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(dst), "dst cannot be NULL");
    ASSERT_MSG(FFI_GET_NATIVE_PTR(const char * const, src), "src cannot be NULL");
    return strcpy_n2r_upto(FFI_PIN_WASM_PTR(dst), dst_buff_size, FFI_GET_NATIVE_PTR(const char * const, src), max_copy_len);
})

