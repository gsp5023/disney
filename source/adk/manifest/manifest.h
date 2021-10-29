/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/http/websockets/adk_websocket_backend_selector.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"

enum {
#if (defined(_STB_NATIVE) || defined(_RPI) || defined(_SHIP))
    default_guard_page_mode = system_guard_page_mode_disabled,
#else
    default_guard_page_mode = system_guard_page_mode_enabled,
#endif
};

static const char * const default_bundle_config_file_path = "bin/.config";

void manifest_init(const adk_system_metrics_t * const metrics);
void manifest_shutdown();

// Allocates from manifest heap a region large enough for the named file
mem_region_t manifest_alloc_file_blob(const char * const manifest_path);

// Allocates, from manifest heap, a region specified for a bundle config file
mem_region_t manifest_alloc_bundle_file_blob(uint64_t size);

// Frees manifest blob
void manifest_free_file_blob(mem_region_t const manifest_blob);

// Formats the default manifest URL into the given buffer
void manifest_format_default_url(char * const buf, const size_t buf_size, const char * const url_fmt);

typedef enum manifest_resource_type_e {
    manifest_resource_file,
    manifest_resource_url,
} manifest_resource_type_e;

enum {
    manifest_value_max_length = 256
};

typedef enum logging_mode_e {
    logging_disabled,
    logging_tty,
    logging_metrics,
    logging_tty_and_metrics,
} logging_mode_e;

typedef struct fetch_retry_context_t {
    uint32_t retry_max_attempts;
    milliseconds_t retry_backoff_ms;
} fetch_retry_context_t;

typedef struct runtime_configuration_canvas_gl_t {
    struct {
        uint32_t max_verts_per_vertex_bank;
        uint32_t num_vertex_banks;
        uint32_t num_meshes;
    } internal_limits;
} runtime_configuration_canvas_gl_t;

typedef struct runtime_configuration_canvas_t {
    bool enable_punchthrough_blend_mode_fix;
    struct {
        uint32_t max_states;
        uint32_t max_tessellation_steps;
    } internal_limits;
    struct {
        int32_t width;
        int32_t height;
    } font_atlas;
    struct {
        uint32_t size;
        bool enabled;
    } text_mesh_cache;
    struct {
        uint32_t working_space;
    } gzip_limits;

    runtime_configuration_canvas_gl_t gl;
} runtime_configuration_canvas_t;

typedef struct runtime_configuration_renderer_t {
    struct {
        size_t num_cmd_buffers;
        size_t cmd_buf_size;
    } device;

    struct {
        bool enabled;
        bool verbose;
        struct {
            bool enabled;
            size_t buffer_size;
        } tracking;
    } rhi_command_diffing;

    struct {
        logging_mode_e periodic_logging;
    } render_resource_tracking;
} runtime_configuration_renderer_t;

typedef struct runtime_configuration_t {
    adk_memory_reservations_t memory_reservations;
    system_guard_page_mode_e guard_page_mode;
    bool log_input_events;
    uint32_t wasm_low_memory_size;
    uint32_t wasm_high_memory_size;
    uint32_t wasm_heap_allocation_threshold;
    uint32_t network_pump_fragment_size;
    uint32_t network_pump_sleep_period;
    struct {
        bool enabled;
        uint32_t suspend_threshold;
        uint32_t warning_delay_ms;
        uint32_t fatal_delay_ms;
    } watchdog;
    uint32_t http_max_pooled_connections;
    fetch_retry_context_t bundle_fetch;
    uint32_t coredump_memory_size;
    uint8_t thread_pool_thread_count;
    struct {
        adk_websocket_backend_e backend;
        websocket_config_t config;
    } websocket;
    runtime_configuration_canvas_t canvas;
    runtime_configuration_renderer_t renderer;
    struct {
        bool capture_logs;
        adk_reporting_event_level_e minimum_event_level;
        char sentry_dsn[adk_reporting_max_string_length];
        uint32_t send_queue_size;
    } reporting;
    struct {
        bool httpx_global_certs;
    } http;
    struct {
        bool enabled;
        bool use_multiplexing;
        bool multiplex_wait_for_existing_connection;
    } http2;
} runtime_configuration_t;

typedef struct manifest_t {
    runtime_configuration_t runtime_config;
    manifest_resource_type_e resource_type;
    char resource[manifest_value_max_length];
    char interpreter[manifest_value_max_length];
    char signature[manifest_value_max_length];
} manifest_t;

// Returns runtime configuration populated with system default values
runtime_configuration_t get_default_runtime_configuration(void);

// Returns the static, manifest module owned, runtime configuration
runtime_configuration_t * manifest_get_runtime_configuration();

bool parse_bundle_config(const char * const bundle_config_file_path, runtime_configuration_t * const out_config);

// Loads wasm app through the given manifest
manifest_t manifest_parse(
    const_mem_region_t const manifest_blob);

void manifest_free_parser();

manifest_t manifest_parse_fp(
    sb_file_t * const manifest_file,
    const size_t manifest_file_content_size);

// Parses the data from a bundle's config file and overwrites the given runtime_configuration_t with the values from the config data
// Returns true on successfully parsed bundle, returns false non-valid or malformed JSON
bool bundle_config_parse_overwrite(
    const_mem_region_t const bundle_config_blob,
    runtime_configuration_t * const config);

void manifest_config_parse_overwrite(
    runtime_configuration_t * const config);

// returns true if the entire manifest is set to 0.  Functions that return manifest_t structures will return empty structures on failures
bool manifest_is_empty(const manifest_t * const manifest);

// Sets the method by which the bundle is chosen for A/B testing.  Bundles are
// selected by a weighted distribution.  Each bundle is weighted in the
// manifest with the `sample` node.  The sum of these samples is the range of
// this selector (i.e., `total).  The `select_func` argument should point to
// a function that returns a number between 0 and (`total`-1).  If
// `select_func` is NULL, the default, random selection is used.
void manifest_set_bundle_selector(uint32_t (*select_func)(uint32_t total));
// Parses manifest platform settings looking for `key` and converting to an int if present. Returns false
// if the key cannot be found or the value is not an int
bool manifest_get_platform_setting_int(const char * const platform, const char * const key, int * out_value);

// Parses manifest platform settings looking for `key` and converting to an int if present. Returns false
// if the key cannot be found or the value is not an int
bool manifest_get_platform_setting_bool(const char * const platform, const char * const key, bool * out_value);

// Parses manifest platform settings looking for `key` and converting to an int if present. Returns false
// if the key cannot be found or the value is not an int
bool manifest_get_platform_setting_double(const char * const platform, const char * const key, double * out_value);

// Parses manifest platform settings looking for `key` and converting to an int if present. Returns false
// if the key cannot be found or the value is not an int
bool manifest_get_platform_setting_string(const char * const platform, const char * const key, char ** out_value);
