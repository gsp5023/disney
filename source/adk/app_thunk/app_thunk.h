/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
app_thunk.h

standard ADK app init thunks
*/

#include "source/adk/canvas/cg.h"
#include "source/adk/cncbus/cncbus.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/manifest/manifest.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

// According to Leia and Vader TRCs, the app can't terminate itself
#if defined(_SHIP) && defined(_CONSOLE_NATIVE)
#define APP_THUNK_IGNORE_APP_TERMINATE
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adk_app_t {
    const adk_api_t * api;
    sb_window_t * window;
    render_device_t * render_device;
    adk_event_t * event_head;
    adk_event_t * event_tail;
    cncbus_t bus;
    thread_pool_t default_thread_pool;
    struct {
        cg_context_t context;
        cg_gl_state_t cg_gl;
    } canvas;
    system_guard_page_mode_e guard_page_mode;
    struct {
        int num_frames;
        milliseconds_t time;
    } fps;
    struct {
        int curr_display;
        int curr_display_mode;
        bool _720p_hack;
    } display_settings;
    adk_httpx_client_t * httpx_client;
    adk_reporting_instance_t * reporting_instance;
    runtime_configuration_t runtime_config;
} adk_app_t;

FFI_EXPORT typedef struct native_slice_t {
    uint64_t ptr;
    int32_t size;
} native_slice_t;

FFI_EXPORT
FFI_TYPE_MODULE(memory)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_dump_heap_flags_e {
    adk_dump_heap_runtime = 0x1,
    adk_dump_heap_rhi_and_renderer = 0x2,
    adk_dump_heap_canvas = 0x4,
    adk_dump_heap_http_curl = 0x8,
    adk_dump_heap_json_deflate = 0x20,
    adk_dump_heap_http2 = 0x40,
    adk_dump_heap_wasm = 0x80,

    adk_dump_heap_all = adk_dump_heap_wasm | adk_dump_heap_http2 | adk_dump_heap_json_deflate | adk_dump_heap_http_curl | adk_dump_heap_canvas | adk_dump_heap_rhi_and_renderer | adk_dump_heap_runtime,
} adk_dump_heap_flags_e;

FFI_EXPORT
FFI_TYPE_MODULE(memory)
FFI_ENUM_CLEAN_NAMES
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_query_heap_e {
    adk_heap_runtime,
    adk_heap_rhi_and_renderer,
    adk_heap_canvas_low,
    adk_heap_canvas_high,
    adk_heap_http_curl,
    adk_heap_http2,
    adk_heap_json_deflate,
    adk_heap_wasm_low,
    adk_heap_wasm_high
} adk_heap_e;

FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_memory_mode_e {
    adk_memory_mode_low = 2,
    adk_memory_mode_high = 3,
} adk_memory_mode_e;

extern adk_app_t the_app;

// used as a tag for telemetry so we can tell that the time spans are logically grouped
#define TIME_TO_FIRST_INTERACTION_STR "[TtFI]"
extern metric_time_to_first_interaction_t time_to_first_interaction;

DLL_EXPORT int adk_main(const int argc, const char * const * const argv);

bool app_init_subsystems(const runtime_configuration_t runtime_config);

bool app_subsystem_initialized(void);
void app_shutdown_thunk();

FFI_EXPORT
FFI_NAME(adk_init_main_display)
void app_init_main_display(const int32_t display_index, const int32_t display_mode_index, FFI_PTR_WASM const char * const adk_app_name);

typedef enum app_display_shutdown_mode_e {
    app_display_shutdown_mode_destroy_window,
    app_display_shutdown_mode_keep_window,
} app_display_shutdown_mode_e;

void app_shutdown_main_display(const app_display_shutdown_mode_e window);

/// Return lowest refresh rate greater than or equal to the given refresh rate that is supported by the current display at the current
/// resolution.  Can be used to check if a rate is supported and to enumerate available refresh rates at current resolution.
///
/// * `starting_refresh_rate`: The refresh rate in fps
///
/// Returns matching supported refresh rate or zero if none.
FFI_EXPORT int32_t adk_get_supported_refresh_rate(const int32_t starting_refresh_rate);

/// Set main display refresh rate and swap interval for compatibility with video content frame rate.
///
/// * `refresh_rate`: The requested display refresh rate in Hz.  Must be a supported rate greater than or equal to `video_fps`.  If not an
///    integral multiple of `video_fps`, better video quality may be achieved by using a refresh rate that is at least 2x `video_fps`.
/// * `video_fps`: Optional frame rate of video presentation, can be zero.
///
/// If `refresh_rate` is zero, sets swap interval to match current refresh rate (in non-ship builds sets unlimited swap interval).
///
/// Returns true if display refresh rate was successfuly set.
FFI_EXPORT bool adk_set_refresh_rate(const int32_t refresh_rate, const int32_t video_fps);

void app_event_loop(int (*tick_fn)(const uint32_t abstime, const float dt, void * arg), void * const arg);

FFI_EXPORT
typedef enum adk_app_state_e {
    adk_app_state_background,
    adk_app_state_foreground,
} adk_app_state_e;

// Get app state
FFI_EXPORT
FFI_NAME(adk_get_app_state)
adk_app_state_e app_get_state();

/// app_start_foreground() Will tear down the graphics system and slow sb_tick to 2hz, anyone calling
/// app_request_background() is responsible for freeing anything related to canvas before calling.
/// app_request_foreground() will stand up the graphics layer and return the sb_tick to normal.
/// Foreground
FFI_EXPORT
FFI_NAME(adk_leave_background_mode)
void app_request_foreground();

bool app_check_foreground_requested();
void app_start_foreground();

/// Background
FFI_EXPORT
FFI_NAME(adk_enter_background_mode)
void app_request_background();

bool app_check_background_requested();
void app_start_background();

/// App Restart
FFI_EXPORT
FFI_NAME(adk_app_request_restart)
void thunk_request_restart();

#ifndef APP_THUNK_IGNORE_APP_TERMINATE
bool thunk_check_restart_requested();
#endif

FFI_EXPORT FFI_NAME(adk_dump_heap_usage) void app_dump_heaps(const adk_dump_heap_flags_e heaps_to_dump);

// callbacks expect wasm ptr offsets when running wasm, otherwise native ptrs
int app_run_callback(void * const callback);
int app_run_callback_v(void * const callback, void * const a0);
void app_run_callback_vi(void * const callback, void * const a0, const int a1);
int app_run_callback_vvi(void * const callback, void * const a0, void * const a1, const int a2);
int app_run_callback_vvii(void * const callback, void * const a0, void * const a1, const int a2, const int a3);
int app_run_callback_ivi(void * const callback, const int a0, void * const a1, const int a2);

void app_run_callback_once_i(void * const callback, const int a0);
void app_run_callback_once_ii(void * const callback, const int a0, const int a1);
void app_run_callback_once_iii(void * const callback, const int a0, const int a1, const int a2);
void app_run_callback_once_iiii(void * const callback, const int a0, const int a1, const int a2, const int a3);
void app_run_callback_once_v(void * const callback, void * const a0);
void app_run_callback_once_vi(void * const callback, void * const a0, const int a1);

void app_drop_callback(void * const callback);

FFI_EXPORT void adk_set_memory_mode(const adk_memory_mode_e memory_mode);

FFI_EXPORT void adk_log_msg(FFI_PTR_WASM const char * const msg);

FFI_EXPORT int32_t read_events(FFI_PTR_WASM FFI_SLICE void * const evbuffer, int32_t bufsize, int32_t sizeof_event);

FFI_EXPORT void adk_notify_app_status(const sb_app_notify_e notify);

FFI_EXPORT FFI_NO_RUST_THUNK heap_metrics_t adk_get_heap_usage(const adk_heap_e heap);

FFI_EXPORT FFI_NAME(adk_get_deeplink_buffer) native_slice_t adk_get_deeplink_buffer_bridge(FFI_PTR_NATIVE const sb_deeplink_handle_t * const handle);

// returns a pointer to a statically allocated buffer if there is a pending wasm error/stack trace
const char * adk_get_wasm_error_and_stack_trace();
void adk_clear_wasm_error_and_stack_trace();

FFI_EXPORT FFI_NO_RUST_THUNK FFI_CAN_BE_NULL FFI_PTR_NATIVE const char * adk_get_env(
    FFI_PTR_WASM const char * const env_name);

/// Enables min_log_level override via CLI arguments in native and ffi builds.
/// Returns true is `cli_arg_value` has been successfully parsed and `min_log_level` was reset.
bool adk_try_override_min_log_level(const char * const cli_arg_value);

#ifdef _NATIVE_FFI
typedef struct rust_callbacks_t rust_callbacks_t;
void adk_thunk_init(const int argc, const char * const * const argv, const rust_callbacks_t * const rust_callbacks);
#endif

#ifdef __cplusplus
}
#endif
