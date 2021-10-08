/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * adk_analytics.h
 * analytics wrapper for m5
 * ==========================================================================*/
#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FFI_EXPORT_LEIA FFI_EXPORT FFI_IMPLEMENT_IF_DEF(_LEIA)

FFI_EXPORT
typedef enum adk_analytics_log_e {
    log_none = 0,
    log_error = 1,
    log_warn = 2,
    log_info = 3,
    log_debug = 4
} adk_analytics_log_e;

FFI_EXPORT
typedef enum Device {
    Device_Unknown = 0,
    Device_Desktop = 1,
    Device_Console = 2,
    Device_Settop = 3,
    Device_Mobile = 4,
    Device_Tablet = 5,
    Device_SmartTV = 6
} Device;

typedef struct adk_analytics_platform_meta_data_t {
    /// Brand of the device.
    ///
    /// Ex: "iPhone", "Samsung SmartTV"
    const char * device_brand;

    /// Manufacturer of the device.
    ///
    /// Ex: "Samsung", "Apple"
    const char * device_manufacturer;

    /// Model of the device.
    ///
    /// Ex: "iPhone 6 Plus", "HTC One", "Roku 3", "Samsung SmartTV 2015"
    const char * device_model;

    /// Type of the device. Case sensitive.
    Device device_type;

    /// Version of the device.
    ///
    /// Usually low-level information pertaining to the hardware.
    /// Ex: "DTP-BP-0869-34"
    const char * device_version;

    /// Name of the main framework used by your application, if applicable.
    ///
    /// It can be the name of application framework itself, or the name of the player framework used for video playback.
    /// Ex: "AVFoundation", "OSMF"
    const char * framework_name;

    /// Version of the framework used by your application, if applicable.
    ///
    /// Ex: "alpha12", "4.28.4433"
    const char * framework_version;

    /// Name of the operating system used by the device, in uppercase.
    ///
    /// Ex: "WINDOWS", "LINUX", "IOS", "MAC", ANDROID".
    /// If unknown, an approximation can be used.
    /// Ex: "ROKU", "PLAYSTATION".
    const char * operating_system_name;

    /// Version of the operating system used by the device.
    ///
    /// Ex: "10.10.1", "8.1", "T-INFOLINK2012-1012"
    const char * operating_system_version;

} adk_analytics_platform_meta_data_t;

void adk_analytics_init(const mem_region_t region);

void adk_analytics_destroy();

///*****************************
///Analytics Settings
///*****************************
FFI_EXPORT_LEIA
void adk_analytics_set_gateway_url(FFI_PTR_WASM const char * url);

/// interval in ms
FFI_EXPORT_LEIA
void adk_analytics_set_heartbeat_interval(const uint32_t interval);

/// interval in ms
FFI_EXPORT_LEIA
void adk_analytics_set_player_poll_interval(const uint32_t interval);

FFI_EXPORT_LEIA
void adk_analytics_set_log_level(const adk_analytics_log_e level);

FFI_EXPORT_LEIA
void adk_analytics_set_enable_player_state_inference(const bool state);

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_getway_url();

FFI_EXPORT_LEIA
uint32_t adk_analytics_get_heartbeat_interval();

FFI_EXPORT_LEIA
uint32_t adk_analytics_get_player_poll_interval();

FFI_EXPORT_LEIA
adk_analytics_log_e adk_analytics_get_log_level();

FFI_EXPORT_LEIA
bool adk_analytics_get_enable_player_state_inference();

///*****************************
///Analytics Content
///*****************************

///After calling adk_analytics_set_xxxx, call update for changes to happen in session with given id
FFI_EXPORT_LEIA
void adk_analytics_update_content_info(const int32_t session_id);

FFI_EXPORT_LEIA
void adk_analytics_set_asset_name(FFI_PTR_WASM const char * name);

FFI_EXPORT_LEIA
void adk_analytics_set_default_cdn(FFI_PTR_WASM const char * name);

FFI_EXPORT_LEIA
void adk_analytics_set_default_resource(FFI_PTR_WASM const char * name);

FFI_EXPORT_LEIA
void adk_analytics_set_default_bitrate(const int32_t bitrate);

FFI_EXPORT_LEIA
void adk_analytics_set_viewer_id(FFI_PTR_WASM const char * id);

FFI_EXPORT_LEIA
void adk_analytics_set_player_name(FFI_PTR_WASM const char * name);

FFI_EXPORT_LEIA
void adk_analytics_set_stream_url(FFI_PTR_WASM const char * url);

FFI_EXPORT_LEIA
void adk_analytics_set_is_live(const bool live);

FFI_EXPORT_LEIA
void adk_analytics_set_duration(const int32_t duration);

FFI_EXPORT_LEIA
void adk_analytics_set_tag(FFI_PTR_WASM const char * key, FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_asset_name();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_default_cdn();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_default_resource();

FFI_EXPORT_LEIA
int32_t adk_analytics_get_default_bitrate();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_viewer_id();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_player_name();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE char * adk_analytics_get_stream_url();

FFI_EXPORT_LEIA
bool adk_analytics_get_is_live();

FFI_EXPORT_LEIA
int32_t adk_analytics_get_duration();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_tag(FFI_PTR_WASM const char * key);

///*****************************
///Analytics Metadata
///*****************************
FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_device_brand();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_device_manufacturer();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_device_model();

FFI_EXPORT_LEIA
Device adk_analytics_get_device_type();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_device_version();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_framework_name();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_framework_version();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_operating_system_name();

FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_operating_system_version();

FFI_EXPORT_LEIA
void adk_analytics_set_device_brand(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_device_manufacturer(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_device_model(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_device_type(const Device value);

FFI_EXPORT_LEIA
void adk_analytics_set_device_version(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_framework_name(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_framework_version(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_operating_system_name(FFI_PTR_WASM const char * value);

FFI_EXPORT_LEIA
void adk_analytics_set_operating_system_version(FFI_PTR_WASM const char * value);

///*****************************
///Analytics Client / Session
///*****************************

FFI_EXPORT_LEIA
void adk_analytics_init_client(FFI_PTR_WASM const char * consumer_key);

FFI_EXPORT_LEIA
int32_t adk_analytics_create_session();

FFI_EXPORT_LEIA
void adk_analytics_remove_session(int32_t session_id);

FFI_EXPORT_LEIA
void adk_analytics_cleanup_session(const int32_t session_id);

// Player hooks
FFI_EXPORT_LEIA
void adk_analytics_attach_player(const int32_t session_id, FFI_PTR_NATIVE void * player);

FFI_EXPORT_LEIA
int32_t adk_analytics_get_playahead_time();
FFI_EXPORT_LEIA
int32_t adk_analytics_get_buffer_length();
FFI_EXPORT_LEIA
float adk_analytics_get_rendered_framerate();
FFI_EXPORT_LEIA
int32_t adk_analytics_get_min_buffer_length();
FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_player_type();
FFI_EXPORT_LEIA
FFI_PTR_NATIVE const char * adk_analytics_get_player_version();
FFI_EXPORT_LEIA
FFI_PTR_NATIVE void * adk_analytics_get_attached_player(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_detach_player(const int32_t session_id);

// Report player state
FFI_EXPORT_LEIA
void adk_analytics_set_buffering(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_set_playing(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_set_paused(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_set_stopped(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_seek_start(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_seek_end(const int32_t session_id, const float player_seek_end_pos);

// Report Ads
FFI_EXPORT_LEIA
void adk_analytics_ad_start(const int32_t session_id);
FFI_EXPORT_LEIA
void adk_analytics_ad_end(const int32_t session_id);

// Report data when available
FFI_EXPORT_LEIA
void adk_analytics_set_session_bitrate(const int32_t session_id, const int32_t bitrate);
FFI_EXPORT_LEIA
void adk_analytics_set_session_stream(const int32_t session_id, const int32_t bitrate_kbps, FFI_PTR_WASM const char * cdn, FFI_PTR_WASM const char * resource);
FFI_EXPORT_LEIA
void adk_analytics_set_session_duration(const int32_t session_id, const int32_t duration);
FFI_EXPORT_LEIA
void adk_analytics_set_session_encoded_framerate(const int32_t session_id, const int32_t framerate);
FFI_EXPORT_LEIA
void adk_analytics_set_session_video_size(const int32_t session_id, const int32_t width, const int32_t height);

FFI_EXPORT_LEIA
void adk_analytics_create_dictionary();

FFI_EXPORT_LEIA
void adk_analytics_destroy_dictionary();

FFI_EXPORT_LEIA
void adk_analytics_add_event_to_dictionary(FFI_PTR_WASM const char * key, FFI_PTR_WASM const char * event_name);

// Misc session related calls
FFI_EXPORT_LEIA
int32_t adk_analytics_send_session_event(const int32_t session_id, FFI_PTR_WASM const char * event_name);

// Misc non-session related calls
FFI_EXPORT_LEIA
int32_t adk_analytics_send_event(FFI_PTR_WASM const char * event_name);

// Update Analytics with our Connection type
FFI_EXPORT_LEIA
void adk_analytics_notify_network_connection_type(FFI_PTR_WASM const char * _type);

// Update Analytics with our network signal strength
FFI_EXPORT_LEIA
void adk_analytics_notify_network_signal_strength(const float strength);

// Update Analytics with our wifi link encryption type ("WEP", WEP2" etc)
FFI_EXPORT_LEIA
void adk_analytics_notify_network_wifi_link_encryption(FFI_PTR_WASM const char * _type);

FFI_EXPORT_LEIA
void adk_analytics_session_report_error(const int32_t session_id, FFI_PTR_WASM const char * error_msg, const bool is_fatal);

#undef FFI_EXPORT_LEIA

#ifdef __cplusplus
}
#endif
