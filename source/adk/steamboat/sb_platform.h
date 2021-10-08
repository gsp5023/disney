/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_platform.h

steamboat platform functions
*/

#pragma once

#include "source/adk/runtime/runtime.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct adk_api_t;
struct adk_system_metrics_t;
struct adk_event_t;

/// Performs any initialization that must occur before initializing the platform
///
/// * `argc`: number of arguments passed into main
/// * `argv`: Pointer to the first argument
///
/// Returns true on success
EXT_EXPORT bool sb_preinit(const int argc, const char * const * const argv);

/// Initializes the platform
///
/// * `api`: The `adk_api_t` to be initialized
/// * `argc`: number of arguments passed into main
/// * `argv`: Pointer to the first argument
/// * `guard_page_mode`: How heap guard pages are handled in the system
///
EXT_EXPORT bool sb_init(struct adk_api_t * api, const int argc, const char * const * const argv, const system_guard_page_mode_e guard_page_mode);

/// Shuts down the platform
EXT_EXPORT void sb_shutdown();

/// Displays native message dialog (if available) and does not return
EXT_EXPORT void sb_halt(const char * const message);

/// Dumps internal heap allocations
EXT_EXPORT void sb_platform_dump_heap_usage();

/// Creates a page-aligned memory mapping of `size` bytes
EXT_EXPORT mem_region_t sb_map_pages(const size_t size, const system_page_protect_e protect);

/// Modify the memory protection properties of `pages`
///
/// * `pages`: The pages to modify
/// * `protect`: The protection properties
///
EXT_EXPORT void sb_protect_pages(const mem_region_t pages, const system_page_protect_e protect);

/// Release the memory mapping of the region `pages`
///
/// * `pages`: The pages to be released
///
EXT_EXPORT void sb_unmap_pages(const mem_region_t pages);

/// Abort the program after a false assertion (for use by ASSERT, TRAP, et al)
///
/// * `message`: The message describing the cause of the failure
/// * `filename`: file where the failure occurred
/// * `function`: function in which the failure occurred
/// * `line`: line on which the failure occurred
///
EXT_EXPORT void sb_assert_failed(const char * const message, const char * const filename, const char * const function, const int line);

/// Inform the system that the application launcher has failed to launch the application
/// This can occur due to an inaccessible/invalid manifest, bundle, or WASM
EXT_EXPORT void sb_on_app_load_failure();

/// Application state-changes
FFI_EXPORT FFI_NAME(adk_app_notify_e)
    FFI_ENUM_CLEAN_NAMES
    typedef enum sb_app_notify_e {
        /// A user logged out of their account.
        sb_app_notify_logged_out,
        /// A user has logged into an account that is not entitled to play content.
        sb_app_notify_logged_in_not_entitled,
        /// A user has logged into an account that is entitled to play content.
        sb_app_notify_logged_in_and_entitled,
        /// The system loading screen has been dismissed.
        sb_app_notify_dismiss_system_loading_screen
    } sb_app_notify_e;

/// Notify system of application state changes
///
/// * `notify`: State change
///
EXT_EXPORT void sb_notify_app_status(const sb_app_notify_e notify);

/// Provides information (via `out`) about the device on which the application is running
///
/// * `out`: The resulting device information
///
EXT_EXPORT void sb_get_system_metrics(FFI_PTR_WASM struct adk_system_metrics_t * const out);

/// Move the application forward a frame.
///
/// Call adk_lock_events() and process system/hid events and post them to the
/// adk event queue via adk_post_event(), and then call adk_event_swap
/// This function is called once per-application-frame.
///
/// **Do not block, wait, or perform heavy computation in this function.**
///
/// * `head`: First even processed this tick
/// * `tail`: Last event processed this tick
///
EXT_EXPORT void sb_tick(const struct adk_event_t ** const head, const struct adk_event_t ** const tail);

/// Handle referencing deeplink application data buffer
typedef struct sb_deeplink_handle_t sb_deeplink_handle_t;

/// Returns deeplink application data
///
/// * `handle`: deeplink to be translated into a memory region
///
EXT_EXPORT const_mem_region_t sb_get_deeplink_buffer(const sb_deeplink_handle_t * const handle);

/// Releases deeplink corresponding to `handle`
///
/// * `handle`: deeplink to be released
///
EXT_EXPORT FFI_EXPORT FFI_NAME(adk_release_deeplink) void sb_release_deeplink(FFI_PTR_NATIVE sb_deeplink_handle_t * const handle);

/// Time in nanoseconds
FFI_EXPORT typedef struct nanoseconds_t {
    uint64_t ns;
} nanoseconds_t;

/// monotonically increasing nanosecond clock
EXT_EXPORT FFI_EXPORT FFI_NAME(adk_read_nanosecond_clock) nanoseconds_t sb_read_nanosecond_clock();

/// The system time since the UNIX Epoch in seconds and microseconds parts
typedef struct sb_time_since_epoch_t {
    uint32_t seconds;
    uint32_t microseconds;
} sb_time_since_epoch_t;

/// Returns the system time since the UNIX Epoch in seconds and microseconds parts
EXT_EXPORT sb_time_since_epoch_t sb_get_time_since_epoch();

/// Converts given time since epoch (in seconds) into 'local' calendar time
EXT_EXPORT void sb_seconds_since_epoch_to_localtime(const time_t seconds, struct tm * const _tm);

/// Converts `text` to audio output as speech (when available)
///
/// * `text`: String to be converted to speech
///
EXT_EXPORT FFI_EXPORT FFI_NAME(adk_text_to_speech) void sb_text_to_speech(FFI_PTR_WASM const char * const text);

/// Universally unique identifier (UUID)
FFI_EXPORT
typedef struct sb_uuid_t {
    uint8_t id[16];
} sb_uuid_t;

/// Creates a new universally unique identifier (UUID)
/// see [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier)
///
/// returns the newly created UUID
///
EXT_EXPORT FFI_EXPORT FFI_NAME(adk_generate_uuid) FFI_NO_RUST_THUNK sb_uuid_t sb_generate_uuid();

/// Receives metrics from the hosted application
///
/// * `app_id`: The returned application id
/// * `app_name`: The returned UTF-8 encoded, localized application name
/// * `app_version`: The returned application version
///
EXT_EXPORT void sb_report_app_metrics(
    const char * const app_id,
    const char * const app_name,
    const char * const app_version);

FFI_EXPORT
FFI_NAME(adk_cpu_mem_status_t)
typedef struct sb_cpu_mem_status_t {
    /// 0-1 (aggregate of all cores, 1 == 100% all cores)
    float cpu_utilization;
    /// how many bytes of ram are available to the application. Total virtual memory available
    uint64_t memory_available;
    /// how much memory has been allocated. Total resident set size memory in use
    uint64_t memory_used;
} sb_cpu_mem_status_t;

/// Retrieves information on CPU and memory utilization
EXT_EXPORT FFI_EXPORT FFI_NAME(adk_get_cpu_mem_status) sb_cpu_mem_status_t sb_get_cpu_mem_status();

#ifdef __cplusplus
}
#endif
