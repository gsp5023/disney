/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
app.h

Application events and windows
*/

#include "events.h"
#include "source/adk/crypto/crypto.h"
#include "source/adk/steamboat/sb_display.h"

#ifdef __cplusplus
extern "C" {
#endif

// The Rendering Hardware Interface
struct rhi_api_t;
/// The Audio API
struct audio_api_t;
/// The persistent storage API
struct persistant_storage_api_t;
/// The audio video decoder
struct av_decoder_api_t;
// The domain of the artifact repository
struct artifact_domain_t;

enum {
    adk_metrics_string_max = 256
};

/// Bitmask values for image textures
FFI_EXPORT
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum adk_gpu_ready_texture_formats_e {
    /// No textures
    adk_gpu_ready_texture_format_none = 0,
    /// ETC1
    adk_gpu_ready_texture_format_etc1 = 1 << 0,
    /// BC1
    adk_gpu_ready_texture_format_bc1 = 1 << 1,
    /// BC3
    adk_gpu_ready_texture_format_bc3 = 1 << 2,
    /// Proprietary texture
    adk_gpu_ready_texture_format_vader = 1 << 3,
    FORCE_ENUM_INT32(adk_gpu_ready_texture_formats_e)
} adk_gpu_ready_texture_formats_e;

/// Device class
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_device_class_e {
    /// Desktop PC
    adk_device_class_desktop_pc = 1,
    /// Gaming Console
    adk_device_class_game_console,
    /// Set top Box
    adk_device_class_stb,
    /// TV
    adk_device_class_tv,
    /// DVR
    adk_device_class_dvr,
    /// Mobile Device
    adk_device_class_mobile,
    /// Micro single board computer
    adk_device_class_minature_sbc,
    FORCE_ENUM_INT32(adk_device_class_e)
} adk_device_class_e;

/// Information about the system running the application
/// This data, returned by `adk_get_system_metrics(...)`, MUST be complete, accurate and the Disney-approved format.
FFI_EXPORT FFI_NAME(adk_native_system_metrics_t) typedef struct adk_system_metrics_t {
    // FILLED IN BY CORE. PARTNERS TO LEAVE THIS BLANK

    /// Core version ID
    /// **FILLED IN BY CORE. PARTNERS TO LEAVE THIS BLANK**
    char core_version[adk_metrics_string_max];
    /// Deployment configuration (debug, release, ship)
    /// **FILLED IN BY CORE. PARTNERS TO LEAVE THIS BLANK**
    char config[adk_metrics_string_max];

    // FILLED IN BY PARTNER - ALL values defined and provided by Disney Partner Integration, unless indicated otherwise.

    /// Name of the device manufacturer
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **FILLED IN BY PARTNER**
    char vendor[adk_metrics_string_max];
    /// Disney assigned partner name
    /// **PROVIDED TO PARTNER BY DISNEY**
    char partner[adk_metrics_string_max];
    /// Disney assigned device name
    /// **PROVIDED TO PARTNER BY DISNEY**
    char device[adk_metrics_string_max];
    /// Device firmware version
    /// **FILLED IN BY PARTNER**
    char firmware[adk_metrics_string_max];
    /// System software (middleware) name
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **FILLED IN BY PARTNER**
    char software[adk_metrics_string_max];
    /// System software (middleware) version
    /// **FILLED IN BY PARTNER**
    char revision[adk_metrics_string_max];
    /// Type of GPU
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **FILLED IN BY PARTNER**
    char gpu[adk_metrics_string_max];
    /// Type of CPU/processor. Example: "arm32"
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **FILLED IN BY PARTNER**
    char cpu[adk_metrics_string_max];
    /// Partner assigned unique device identifier or serial number
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **FILLED IN BY PARTNER**
    crypto_hmac_hex_t device_id;
    /// Device region provided by sb_get_locale()
    /// **FILLED IN BY PARTNER**
    char device_region[adk_metrics_string_max];
    /// Tenancy
    /// MUST be `prod` for retail. Other values as-specified by Disney.
    /// **PROVIDED TO PARTNER BY DISNEY**
    char tenancy[adk_metrics_string_max];
    /// Disney assigned partner specific unique identifier
    /// **PROVIDED TO PARTNER BY DISNEY**
    char partner_guid[adk_metrics_string_max];
    /// Partner provided advertising ID. Set to NULL
    /// **Reserved - please contact Disney Partner Integration before using any value other than NULL**
    char advertising_id[adk_metrics_string_max];

    /// Size of main memory in megabytes
    /// **FILLED IN BY PARTNER -- please verify the value with Disney Partner Integration**
    int32_t main_memory_mbytes;
    /// Size of video memory in megabytes
    /// **FILLED IN BY PARTNER -- please verify the value with Disney Partner Integration**
    int32_t video_memory_mbytes;
    /// Number of hardware threads supported
    /// **FILLED IN BY PARTNER -- please verify the value with Disney Partner Integration**
    int32_t num_hardware_threads;
    /// The number of cores
    /// **FILLED IN BY PARTNER -- please verify the value with Disney Partner Integration**
    int32_t num_cores;

    /// Device class. Operator STBs must use adk_device_class_stb
    /// **PROVIDED TO PARTNER BY DISNEY**
    adk_device_class_e device_class;

    /// Bitmask of supported image textures
    /// **FILLED IN BY PARTNER**
    adk_gpu_ready_texture_formats_e gpu_ready_texture_formats;

    /* persistent storage information */

    /// total bytes available for storage by the application
    /// this is a constant allocated by the target and does not
    /// change based on how much storage is actually used by the application
    /// 0 means no limit hard limit.
    /// **FILLED IN BY PARTNER**
    int32_t persistent_storage_available_bytes;

    /// persistant storage bandwidth limit.
    /// 0 means no limit
    /// **FILLED IN BY PARTNER**
    int32_t persistent_storage_max_write_bytes_per_second;

    /* persona */

    /// Unique ID identifying which ADK application will be launched
    /// May be set to an empty string "" to run the default application
    /// or to a string provided by Disney corresponding to a specific application
    /// Must be comprised of the following characters: 'a'-'z', 'A'-'Z', '0'-'9', or '_'
    /// **PROVIDED TO PARTNER BY DISNEY**
    char persona_id[adk_metrics_string_max];

} adk_system_metrics_t;

FFI_EXPORT
FFI_RETURN(out)
FFI_NAME(adk_get_system_metrics_native)
FFI_PUB_CRATE
void adk_get_system_metrics(FFI_PTR_WASM adk_system_metrics_t * const out);

/*
===============================================================================
adk_memory_reservation_t

This structure defines the memory reservation of the entire ADK application.
At program start this layout is allocated and determines the memory usage limits
of the various ADK systems.
===============================================================================
*/

typedef struct adk_low_memory_reservations_t {
    uint32_t runtime;
    uint32_t rhi;
    uint32_t render_device;
    uint32_t bundle;
    uint32_t canvas;
    uint32_t canvas_font_scratchpad;
    uint32_t cncbus;
    uint32_t curl;
    uint32_t curl_fragment_buffers;
    uint32_t json_deflate;
    uint32_t default_thread_pool;
    uint32_t http2;
    uint32_t httpx;
    uint32_t httpx_fragment_buffers;
    uint32_t reporting;
} adk_low_memory_reservations_t;

typedef struct adk_high_memory_reservations_t {
    uint32_t canvas;
} adk_high_memory_reservations_t;

typedef struct adk_memory_reservations_t {
    adk_low_memory_reservations_t low;
    adk_high_memory_reservations_t high;
} adk_memory_reservations_t;

adk_memory_reservations_t adk_get_default_memory_reservations();

/*
===============================================================================
adk_memory_map_t

The allocated memory block of an ADK application.
===============================================================================
*/

typedef struct adk_mem_region_t {
    mem_region_t region;
#ifdef GUARD_PAGE_SUPPORT
    struct {
        mem_region_t pages;
    } internal;
#endif
} adk_mem_region_t;

/// Memory allocated by modules
typedef struct adk_memory_map_t {
    /// memory for runtime environment
    adk_mem_region_t runtime;
    /// memory for Rendering Hardware Interface
    adk_mem_region_t rhi;
    /// memory for the rendering device
    adk_mem_region_t render_device;
    /// memory for bundle file system
    adk_mem_region_t bundle;
    /// memory for rendering canvas
    adk_mem_region_t canvas_low_mem;
    /// memory canvas font sandbox
    adk_mem_region_t canvas_font_scratchpad;
    /// memory for the control bus
    adk_mem_region_t cncbus;
    /// memory for network connectivity
    adk_mem_region_t curl;
    /// memory for HTTP Fragment Buffers
    adk_mem_region_t curl_fragment_buffers;
    /// memory for JSON deflate
    adk_mem_region_t json_deflate;
    /// memory for the thread pool
    adk_mem_region_t default_thread_pool;
    /// memory for web sockets
    adk_mem_region_t http2;
    /// memory for HTTP subsystem
    adk_mem_region_t httpx;
    /// memory for HTTPX Fragment Buffers
    adk_mem_region_t httpx_fragment_buffers;
    /// memory for the reporting instance
    adk_mem_region_t reporting;
    /// For internal use
    struct {
        /// memory for pages
        mem_region_t page_memory;
    } internal;
} adk_memory_map_t;

STATIC_ASSERT((sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t)) == ((sizeof(adk_memory_map_t) - sizeof(mem_region_t)) / sizeof(adk_mem_region_t)));

///  ADK context structure
typedef struct adk_api_t {
    /// Pointer to Rendering Hardware Interface
    struct rhi_api_t * rhi;
    /// Pointer to the audio API
    struct audio_api_t * audio;
    /// Pointer to the persistent storage API
    struct persistant_storage_api_t * p_store;
    /// Pointer to the audio/video decoder
    struct av_decoder_api_t * av_decoder;
    /// The memory allocating per module
    adk_memory_map_t mmap;
    /// maximum memory allocations during high memory usage
    adk_high_memory_reservations_t high_mem_reservations;
#ifndef _STB_NATIVE
    /// Information about runtime environment
    struct {
        /// True = running without a monitor
        bool headless;
    } runtime_flags;
#endif
} adk_api_t;

/*
===============================================================================
adk_init

Initialize the ADK.

Returns a pointer to the ADK api table for use by the m5 application.
===============================================================================
*/

const adk_api_t * adk_init(const int argc, const char * const * const argv, const system_guard_page_mode_e guard_page_mode, const adk_memory_reservations_t memory_reservations, const char * const tag);

/*
===============================================================================
adk_shutdown

Shutdown the ADK.
===============================================================================
*/

void adk_shutdown(const char * const tag);

/*
===============================================================================
adk_halt

Halt the ADK.
===============================================================================
*/

void adk_halt(const char * const message);

#ifndef _STB_NATIVE

/*
===============================================================================
adk_get_gamepad_state

Gets the current gamepad state. If the gamepad is disconnected NULL is returned.
NOTE: Do not assume this will not return NULL when processsing gamepad events:
events by definition are from a prior time and the gamepad may have since been
disconnected.
===============================================================================
*/

const adk_gamepad_state_t * adk_get_gamepad_state(const int index);
#endif

#ifdef __cplusplus
}
#endif
