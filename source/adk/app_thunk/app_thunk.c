/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
app_thunk.c

standard ADK app init thunks
*/

#include "source/adk/app_thunk/app_thunk.h"

#include "source/adk/app_thunk/watchdog.h"
#include "source/adk/cmdlets/cmdlets.h"
#include "source/adk/cncbus/cncbus_addresses.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/coredump/coredump.h"
#include "source/adk/extender/extender.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/adk_websocket_backend_selector.h"
#include "source/adk/http/websockets/websockets.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/log/private/event_logger.h"
#include "source/adk/log/private/log_p.h"
#include "source/adk/log/private/log_receiver.h"
#include "source/adk/manifest/manifest.h"
#include "source/adk/merlin/drivers/minnie/exit_codes.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/nve/video_texture_api.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/reporting/adk_reporting_sentry_options.h"
#include "source/adk/runtime/bifurcated_heap.h"
#include "source/adk/runtime/hosted_app.h"
#include "source/adk/runtime/private/events.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/telemetry/telemetry.h"

#include <limits.h>

#define TAG_APP FOURCC('A', 'P', 'P', '\0')

typedef struct bus_dispatcher_t {
    cncbus_t * bus;
    sb_thread_id_t thread_id;
    volatile bool run;
} bus_dispatcher_t;

adk_app_t the_app;

enum {
    default_thread_pool_thread_count = 1,
    default_background_tick_rate_ms_delay = 500 // 2hz
};

#ifdef _NATIVE_FFI
typedef int (*callback_fn_t)(void * call);
typedef int (*callback_v_fn_t)(void * const call, void * const a0);
typedef int (*callback_vi_fn_t)(void * call, void * a0, int a1);
typedef int (*callback_vvi_fn_t)(void * call, void * a0, void * a1, int a2);
typedef int (*callback_vvii_fn_t)(void * const call, void * const a0, void * const a1, const int a2, const int a3);
typedef int (*callback_ivi_fn_t)(void * const call, const int a0, void * const a1, const int a2);

typedef void (*callback_once_i_fn_t)(void * call, const int a0);
typedef void (*callback_once_ii_fn_t)(void * call, const int a0, const int a1);
typedef void (*callback_once_iii_fn_t)(void * call, const int a0, const int a1, const int a2);
typedef void (*callback_once_iiii_fn_t)(void * call, const int a0, const int a1, const int a2, const int a3);
typedef int (*callback_once_v_fn_t)(void * const call, void * const a0);
typedef int (*callback_once_vi_fn_t)(void * call, void * a0, int a1);

typedef struct rust_callbacks_t {
    callback_fn_t callback;
    callback_v_fn_t callback_v;
    callback_vi_fn_t callback_vi;
    callback_vvi_fn_t callback_vvi;
    callback_vvii_fn_t callback_vvii;
    callback_ivi_fn_t callback_ivi;

    callback_once_i_fn_t callback_once_i;
    callback_once_ii_fn_t callback_once_ii;
    callback_once_iii_fn_t callback_once_iii;
    callback_once_iiii_fn_t callback_once_iiii;
    callback_once_v_fn_t callback_once_v;
    callback_once_vi_fn_t callback_once_vi;

    callback_fn_t drop_callback;
} rust_callbacks_t;

static struct {
    rust_callbacks_t rust_calls;
#else
static struct {
#endif
    bus_dispatcher_t bus_dispatcher;
    int argc;
    const char * const * argv;
    bool subsystems_init; // apps may not always init subsystems, thus we may need to skip part of the shutdown processes.
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
    bool restart_requested;
#endif
    bool vid_restart_requested;
    bool vid_restarting;
    adk_app_state_e app_state;
    bool background_requested;
    bool foreground_requested;
    adk_memory_mode_e memory_mode;
    rhi_swap_interval_t swap_interval;
    bool overridden_swap_interval;

    watchdog_t watchdog;

    struct {
        int32_t max_write_bytes_per_second;
        float bytes_to_drain;
    } adk_file_rate_limiting;
    heap_t reporting_instance_heap;
} statics;

metric_time_to_first_interaction_t time_to_first_interaction;
metric_memory_footprint_t memory_footprint;

int app_main(const int argc, const char * const * const argv);

static int bus_dispatcher_thread_proc(void * const arg) {
    bus_dispatcher_t * const dispatcher = arg;

    while (dispatcher->run) {
        if (cncbus_dispatch(dispatcher->bus, cncbus_dispatch_flush) != cncbus_dispatch_ok) {
        }

        sb_thread_sleep((milliseconds_t){1});
    }

    return 0;
}

static void init_reporting(const adk_api_t * const api, const runtime_configuration_t * const config) {
#ifdef GUARD_PAGE_SUPPORT
    if (the_app.guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&statics.reporting_instance_heap, api->mmap.reporting.region.size, 8, 0, "reporting_instance_heap", the_app.guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&statics.reporting_instance_heap, api->mmap.reporting.region, 8, 0, "reporting_instance_heap");
    }

    adk_reporting_init_options_t reporting_options = {
        .heap = &statics.reporting_instance_heap,
        .httpx_client = the_app.httpx_client,
        .reporter_name = "core",
        .sentry_dsn = config->reporting.sentry_dsn,
        .send_queue_size = config->reporting.send_queue_size,
        .min_report_level = config->reporting.minimum_event_level,
    };
    the_app.reporting_instance = adk_reporting_instance_create(&reporting_options);
}

bool app_init_subsystems(const runtime_configuration_t runtime_config) {
    APP_THUNK_TRACE_PUSH_FN();
    if (statics.subsystems_init) {
        LOG_ALWAYS(TAG_APP, "App init (skipped: already initialized)");
        APP_THUNK_TRACE_POP();
        return true;
    }

    LOG_ALWAYS(TAG_APP, "App init");

    const bool _720p_hack = the_app.display_settings._720p_hack;
    ZEROMEM(&the_app);
    the_app.guard_page_mode = runtime_config.guard_page_mode;
    the_app.display_settings._720p_hack = _720p_hack;
    the_app.runtime_config = runtime_config;

    APP_THUNK_TRACE_PUSH("adk_init");
    const adk_api_t * const api = adk_init(
        statics.argc,
        statics.argv,
        the_app.guard_page_mode,
        runtime_config.memory_reservations,
        MALLOC_TAG);
    APP_THUNK_TRACE_POP();

    if (!api) {
        APP_THUNK_TRACE_POP();
        return false;
    }
    the_app.api = api;

    adk_app_metrics_init();

    APP_THUNK_TRACE_PUSH("adk_coredump_init");
    const bool did_init_core_dump = adk_coredump_init((int)runtime_config.coredump_memory_size);
    APP_THUNK_TRACE_POP();
    if (!did_init_core_dump) {
        LOG_ERROR(TAG_APP, "Failed to initialize core-dump component");
        APP_THUNK_TRACE_POP();
        return false;
    }

    APP_THUNK_TRACE_PUSH("thread_pool_init");
    thread_pool_init(
        &the_app.default_thread_pool,
        api->mmap.default_thread_pool.region,
        the_app.guard_page_mode,
        (runtime_config.thread_pool_thread_count > 0) ? runtime_config.thread_pool_thread_count : default_thread_pool_thread_count,
        "dflt_thp",
        MALLOC_TAG);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("adk_httpx_init");
    adk_httpx_init_global_certs(runtime_config.http.httpx_global_certs);
    adk_httpx_enable_http2((adk_httpx_http2_options){
        .enabled = runtime_config.http2.enabled,
        .use_multiplexing = runtime_config.http2.use_multiplexing,
        .multiplex_wait_for_existing_connection = runtime_config.http2.multiplex_wait_for_existing_connection});
    the_app.httpx_client = adk_httpx_client_create(
        the_app.api->mmap.httpx.region,
        the_app.api->mmap.httpx_fragment_buffers.region,
        runtime_config.network_pump_fragment_size,
        runtime_config.network_pump_sleep_period,
        the_app.guard_page_mode,
        adk_httpx_init_normal,
        "app-httpx");
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("init_reporting");
    init_reporting(api, &runtime_config);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("adk_curl_api_init");
    if (!adk_curl_api_init(
            the_app.api->mmap.curl.region,
            the_app.api->mmap.curl_fragment_buffers.region,
            runtime_config.network_pump_fragment_size,
            runtime_config.network_pump_sleep_period,
            the_app.guard_page_mode,
            adk_curl_http_init_normal)) {
        LOG_ERROR(TAG_APP, "Failed to initialize HTTP stack");

        adk_shutdown(MALLOC_TAG);
        APP_THUNK_TRACE_POP();
        APP_THUNK_TRACE_POP();
        return false;
    }
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("cncbus_init");
    cncbus_init(&the_app.bus, api->mmap.cncbus.region, the_app.guard_page_mode);

    statics.bus_dispatcher.bus = &the_app.bus;
    statics.bus_dispatcher.run = true;
    statics.bus_dispatcher.thread_id = sb_create_thread("m5_bus_disptchr", sb_thread_default_options, bus_dispatcher_thread_proc, &statics.bus_dispatcher, MALLOC_TAG);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("log_init");
    log_receiver_init(&the_app.bus, CNCBUS_ADDRESS_LOGGER);
    log_init(&the_app.bus, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE, runtime_config.reporting.capture_logs ? the_app.reporting_instance : NULL);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("event_logger init");
    event_logger_init(&the_app.bus, CNCBUS_ADDRESS_EVENTS);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("json_deflate_init");
    json_deflate_init(api->mmap.json_deflate.region, the_app.guard_page_mode, &the_app.default_thread_pool);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("adk_ssl_init");
    adk_mbedtls_init(api->mmap.ssl.region, the_app.guard_page_mode, MALLOC_TAG);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("adk_http_init(ws)");
    adk_http_init(adk_http_init_default_cert_path, api->mmap.http2.region, runtime_config.websocket.backend, runtime_config.websocket.config, the_app.guard_page_mode, MALLOC_TAG);
    APP_THUNK_TRACE_POP();

    APP_THUNK_TRACE_PUSH("adk_file_set_write_limit");
    adk_system_metrics_t sys_metrics;
    adk_get_system_metrics(&sys_metrics);
    statics.adk_file_rate_limiting.max_write_bytes_per_second = sys_metrics.persistent_storage_max_write_bytes_per_second;
    adk_file_set_write_limit(sys_metrics.persistent_storage_max_write_bytes_per_second);
    APP_THUNK_TRACE_POP();

    static const int num_systems = sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t);
    const uint32_t * const sizes_src = (const uint32_t *)&runtime_config.memory_reservations.low;
    memory_footprint.high_memory_size = runtime_config.memory_reservations.high.canvas;
    memory_footprint.low_memory_size = 0;

    for (int i = 0; i < num_systems; ++i) {
        memory_footprint.low_memory_size += sizes_src[i];
    }
    publish_metric(metric_type_memory_footprint, &memory_footprint, sizeof(memory_footprint));

    adk_reporting_instance_push_tag(the_app.reporting_instance, "websocket_backend", runtime_config.websocket.backend == adk_websocket_backend_http2 ? "http2" : "websocket");
    // when running as a native rust app we do not have a wasm interpreter; running demos we do this is due different path of running m5.
    if (get_active_wasm_interpreter()) {
        adk_reporting_instance_push_tag(the_app.reporting_instance, "wasm_interpreter", get_active_wasm_interpreter()->name);
    }

    render_cmd_init((render_cmd_config_t){
        .rhi_command_diffing = {
            .enabled = the_app.runtime_config.renderer.rhi_command_diffing.enabled,
            .verbose = the_app.runtime_config.renderer.rhi_command_diffing.verbose,
            .tracking = {
                .enabled = the_app.runtime_config.renderer.rhi_command_diffing.tracking.enabled,
                .buffer_size = the_app.runtime_config.renderer.rhi_command_diffing.tracking.buffer_size,
            }}});

    statics.subsystems_init = true;

    APP_THUNK_TRACE_POP();
    return true;
}

bool app_subsystem_initialized() {
    return statics.subsystems_init;
}

void app_init_main_display(const int display_index, const int display_mode_index, const char * const adk_app_name) {
    APP_THUNK_TRACE_PUSH_FN();

    sb_window_t * w = sb_init_main_display(display_index, display_mode_index, adk_app_name);

    if (!w) {
        adk_shutdown(MALLOC_TAG);
        APP_THUNK_TRACE_POP();
        return;
    }

    the_app.display_settings.curr_display = display_index;
    the_app.display_settings.curr_display_mode = display_mode_index;

    rhi_error_t * err;
    render_device_t * const render_device = create_render_device(
        the_app.api->rhi,
        w,
        &err,
        the_app.api->mmap.render_device.region,
        (int)the_app.runtime_config.renderer.device.num_cmd_buffers,
        (int)the_app.runtime_config.renderer.device.cmd_buf_size,
        1,
        the_app.guard_page_mode,
        MALLOC_TAG);

    if (err) {
        LOG_ERROR(TAG_APP, "create_render_device error: '%s'", rhi_get_error_message(err));
        rhi_error_release(err, MALLOC_TAG);
    }

    the_app.window = w;
    the_app.render_device = render_device;

    sb_enumerate_display_modes_result_t display_result;
    VERIFY(sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_result));

    cg_set_context(&the_app.canvas.context);
    cg_context_init(
        &the_app.canvas.context,
        &the_app.canvas.cg_gl,
        &the_app.default_thread_pool,
        the_app.render_device,
        (cg_context_memory_initializers_t){
            .high_mem_size = the_app.api->high_mem_reservations.canvas,
            .low_mem_region = the_app.api->mmap.canvas_low_mem.region,
            .font_scratchpad_mem = the_app.api->mmap.canvas_font_scratchpad.region,
            .initial_memory_mode = cg_memory_mode_high,
            .guard_page_mode = the_app.guard_page_mode},
        display_result.display_mode,
        the_app.runtime_config.canvas,
        the_app.display_settings._720p_hack ? cg_context_display_size_720p : cg_context_display_size_default,
#ifndef _SHIP
        (findarg("--log-canvas-image-load", statics.argc, statics.argv) != -1) || (findarg("-I", statics.argc, statics.argv) != -1) ? cg_context_image_time_logging_enabled : cg_context_image_time_logging_disabled,
#else
        cg_context_image_time_logging_disabled,
#endif
        MALLOC_TAG);

    APP_THUNK_TRACE_POP();
}

void app_shutdown_main_display(const app_display_shutdown_mode_e window) {
    if (the_app.window) {
        // Drain thread pool to ensure we aren't currently performing operations before tearing down canvas
        thread_pool_drain(&the_app.default_thread_pool);
        render_device_frame(the_app.render_device);
        flush_render_device(the_app.render_device);

        cg_gl_state_free(&the_app.canvas.cg_gl);
        cg_context_free(&the_app.canvas.context, MALLOC_TAG);

        const int ref_count = render_release(&the_app.render_device->resource, MALLOC_TAG);
        VERIFY_MSG(ref_count == 0, "Resources not full cleaned up before video restart");

        if (window == app_display_shutdown_mode_destroy_window) {
            sb_destroy_main_window();
        }
    }
}

void app_shutdown_thunk() {
    APP_THUNK_TRACE_PUSH_FN();
    if (!statics.subsystems_init) {
        LOG_ALWAYS(TAG_APP, "App shutdown (skipped: already shutdown)");
        APP_THUNK_TRACE_POP();
        return;
    }

    LOG_ALWAYS(TAG_APP, "App shutdown");

    // drain and shutdown the thread-pool
    // do this before shutting down systems to make sure
    // any enqueued tasks finish.

    thread_pool_drain(&the_app.default_thread_pool);
    thread_pool_shutdown(&the_app.default_thread_pool, MALLOC_TAG);

    if (the_app.window) {
        if (the_app.render_device) {
            cg_gl_state_free(&the_app.canvas.cg_gl);
            cg_context_free(&the_app.canvas.context, MALLOC_TAG);

            render_release(&the_app.render_device->resource, MALLOC_TAG);
            the_app.render_device = NULL;
        }
    }

    json_deflate_shutdown();

    adk_http_shutdown(MALLOC_TAG); // websockets

    // Shutdown reporting

    // reinit the log with the same initial initializers except the reporting instance,
    // so that any of the remaining shutdowns that try to log something would not report it to the reporting instance.
    log_init(&the_app.bus, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE, NULL);
    adk_reporting_instance_free(the_app.reporting_instance);
    adk_httpx_client_free(the_app.httpx_client);
    heap_destroy(&statics.reporting_instance_heap, MALLOC_TAG);

    if (get_default_runtime_configuration().http.httpx_global_certs) {
        adk_httpx_free_global_certs();
    }

    adk_curl_api_shutdown_all_handles();
    adk_curl_api_shutdown();

    adk_mbedtls_shutdown(MALLOC_TAG);

    adk_app_metrics_shutdown();

#ifdef _TELEMETRY
    telemetry_shutdown();
#endif

    // NOTE: stopping bus dispatch should occur last to ensure log message delivery
    statics.bus_dispatcher.run = false;
    sb_join_thread(statics.bus_dispatcher.thread_id);

    log_receiver_shutdown(&the_app.bus);
    event_logger_shutdown(&the_app.bus);
    cncbus_destroy(&the_app.bus);
    log_shutdown();

    adk_coredump_shutdown();

    adk_shutdown(MALLOC_TAG);

    render_cmd_shutdown();

    statics.subsystems_init = false;

    APP_THUNK_TRACE_POP();
}

static bool check_vid_restart_requested(void) {
    return statics.vid_restart_requested;
}

static bool check_vid_restarting(void) {
    return statics.vid_restarting;
}

void request_vid_restart(void) {
    if (check_vid_restarting()) {
        LOG_ALWAYS(TAG_APP, "Thunk video restart requested, but already restarting");
    } else {
        LOG_ALWAYS(TAG_APP, "Thunk video restart requested");
        statics.vid_restart_requested = true;
    }
}

void verify_wasm_call_and_halt_on_failure(const wasm_call_result_t result);

void adk_video_restart_begin(void) {
#ifdef _NVE
    nve_begin_vid_restart();
#endif // _NVE
    const wasm_call_result_t wasm_call_result = get_active_wasm_interpreter()->call_void("app_video_restart_begin");
    verify_wasm_call_and_halt_on_failure(wasm_call_result);

    app_shutdown_main_display(app_display_shutdown_mode_keep_window);
}

void adk_video_restart_end(void) {
    const wasm_call_result_t wasm_call_result = get_active_wasm_interpreter()->call_void("app_video_restart_end");
    verify_wasm_call_and_halt_on_failure(wasm_call_result);

#ifdef _NVE
    nve_end_vid_restart();
#endif //  _NVE
}

adk_app_state_e app_get_state() {
    return statics.app_state;
}

bool app_check_is_backgrounded() {
    return app_get_state() == adk_app_state_background;
}

/*
    Foreground app
*/

void app_request_foreground() {
    if (app_check_is_backgrounded()) {
        LOG_ALWAYS(TAG_APP, "Thunk foreground requested");
        statics.foreground_requested = true;
        app_start_foreground();
    } else {
        LOG_ALWAYS(TAG_APP, "Thunk foreground requested, but app is foregrounded");
    }
}

bool app_check_foreground_requested() {
    return statics.foreground_requested;
}

void app_start_foreground() {
#ifdef _NVE
    nve_end_vid_restart();
#endif
    statics.foreground_requested = false;
    statics.app_state = adk_app_state_foreground;
}

/*
Background app
*/

void app_request_background() {
    if (app_check_is_backgrounded()) {
        LOG_ALWAYS(TAG_APP, "Thunk background requested, but already backgrounded");
    } else {
        LOG_ALWAYS(TAG_APP, "Thunk background requested");
        statics.background_requested = true;
    }
}

bool app_check_background_requested() {
    return statics.background_requested;
}

void app_start_background() {
    statics.background_requested = false;
    statics.app_state = adk_app_state_background;
    // Stop rendering, Stop rendering threads, tear down rendering resources and devices
#ifdef _NVE
    nve_begin_vid_restart();
#endif // _NVE
    app_shutdown_main_display(app_display_shutdown_mode_destroy_window);
}

/*
    Restart app
*/

void thunk_request_restart() {
#ifdef APP_THUNK_IGNORE_APP_TERMINATE
    LOG_WARN(TAG_APP, "Restart not available");
#else
    LOG_ALWAYS(TAG_APP, "Thunk restart requested");
    statics.restart_requested = true;
#endif
}

#ifndef APP_THUNK_IGNORE_APP_TERMINATE
bool thunk_check_restart_requested() {
    return statics.restart_requested;
}
#endif

int adk_main(const int argc, const char * const * const argv) {
    LOG(log_level_none, TAG_APP, "startup-adk-version: %s built on %s @ %s", ADK_VERSION_STRING, __DATE__, __TIME__);

    if (!sb_preinit(argc, argv)) {
        LOG_ERROR(TAG_APP, "sb_preinit() failed");
        return merlin_exit_code_sb_preinit_failure;
    }

    const int telemetry_help = findarg("--telemetry-help", argc, argv);
    const char * const telemetry_groups = getargarg("--telemetry", argc, argv);
    const char * const telemetry_address = getargarg("--telemetry-server", argc, argv);
#ifdef _TELEMETRY
    if (telemetry_help != -1) {
        telemetry_print_help();
        return merlin_exit_code_success;
    }

    if (telemetry_address) {
        char address_buff[1024];
        strcpy_s(address_buff, ARRAY_SIZE(address_buff), telemetry_address);
        char * const port_separator = strrchr(address_buff, ':');
        VERIFY_MSG(port_separator, "telemetry (--telemetry-server) address must be specified with the format: `<address>:<port>`\nReceived: [%s]", telemetry_address);
        *port_separator = '\0';
        telemetry_init(address_buff, atoi(port_separator + 1), telemetry_groups);
    } else {
        telemetry_init("localhost", 4719, telemetry_groups);
    }
#else
    if (telemetry_help || telemetry_groups || telemetry_address) {
        LOG_ALWAYS(TAG_APP, "Telemetry disabled in build!");
    }
#endif

    TTFI_TRACE_TIME_SPAN_BEGIN(&time_to_first_interaction.main_timestamp, TIME_TO_FIRST_INTERACTION_STR " main");
    time_to_first_interaction.main_timestamp = adk_read_millisecond_clock();

    statics.argc = argc;
    statics.argv = argv;
    statics.app_state = adk_app_state_foreground;

    int exit_code = merlin_exit_code_success;

    statics.overridden_swap_interval = false;

#if !defined(_SHIP)
    const char * arg_swap_interval = getargarg("--swap-interval", statics.argc, statics.argv);
    if (!arg_swap_interval) {
        arg_swap_interval = getargarg("-n", statics.argc, statics.argv);
    }
    if (arg_swap_interval) {
        statics.swap_interval.interval = atoi(arg_swap_interval);
        statics.overridden_swap_interval = true;
        LOG_ALWAYS(TAG_APP, "overridden swap interval [%i]", statics.swap_interval.interval);
    }

    const char * cmdlet_arg = (const char *)getargarg(CMDLET_FLAG, argc, argv);

    if (cmdlet_arg != NULL) {
        // invoke a commandlet, then exit
        exit_code = cmdlet_run(argc, argv);

    } else
#endif
    {
        exit_code = app_main(argc, argv);
    }

    app_shutdown_thunk();

    return exit_code;
}

void adk_set_memory_mode(const adk_memory_mode_e memory_mode) {
    APP_THUNK_TRACE_PUSH_FN();

    if (memory_mode == statics.memory_mode) {
        APP_THUNK_TRACE_POP();
        return;
    }
    statics.memory_mode = memory_mode;
    if (memory_mode == adk_memory_mode_low) {
        thread_pool_drain(&the_app.default_thread_pool);
        flush_render_device(the_app.render_device);
        cg_context_set_memory_mode(&the_app.canvas.context, cg_memory_mode_low);
    } else {
        ASSERT(memory_mode == adk_memory_mode_high);
        cg_context_set_memory_mode(&the_app.canvas.context, cg_memory_mode_high);
    }

    APP_THUNK_TRACE_POP();
}

static void log_periodic_metrics(const milliseconds_t delta_time, const sb_display_mode_t display_mode) {
    ++the_app.fps.num_frames;
    the_app.fps.time.ms += delta_time.ms;

    if (the_app.fps.time.ms >= 1000) {
        const milliseconds_t ms_per_frame = {the_app.fps.time.ms / the_app.fps.num_frames};
        LOG_ALWAYS(TAG_APP, "[%4" PRIu32 "] FPS: [%" PRIu32 "ms/frame] (%d:%d)", (ms_per_frame.ms > 0) ? 1000 / ms_per_frame.ms : 1000, ms_per_frame.ms, display_mode.hz, statics.swap_interval.interval);

        render_cmd_log_metrics();
        render_device_log_resource_tracking(the_app.render_device, the_app.runtime_config.renderer.render_resource_tracking.periodic_logging);

        the_app.fps.time.ms = 0;
        the_app.fps.num_frames = 0;
    }
}

void adk_halt(const char * const message) {
    if (statics.watchdog.running) {
        watchdog_shutdown(&statics.watchdog);
    }

    sb_halt(message);
}

void app_event_loop(int (*tick_fn)(const uint32_t abstime, const float dt, void * arg), void * const arg) {
    milliseconds_t time = {0};
    milliseconds_t last_time = {0};
    milliseconds_t bglast_time = (milliseconds_t){adk_read_millisecond_clock().ms};

    the_app.event_head = the_app.event_tail = NULL;

    APP_THUNK_TRACE_PUSH("sb_enumerate_display_modes");
    sb_enumerate_display_modes_result_t display_mode_result = {0};
    if (the_app.window) {
        sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &display_mode_result);
    }
    APP_THUNK_TRACE_POP();

    const bool log_input_events = manifest_get_runtime_configuration()->log_input_events;

    bool did_init_back_buffer = false;

    LOG_INFO(TAG_APP, "Starting a watchdog thread");
    const runtime_configuration_t * const rt = manifest_get_runtime_configuration();
    if (rt->watchdog.enabled) {
        watchdog_start(&statics.watchdog, rt->watchdog.suspend_threshold, rt->watchdog.warning_delay_ms, rt->watchdog.fatal_delay_ms);
    }

#ifdef APP_THUNK_IGNORE_APP_TERMINATE
    // According to Leia and Vader TRCs, the app can't terminate itself
    while (true) {
#else
    statics.restart_requested = false;
    int running = 1;
    while (running) {
#endif
        if (app_check_background_requested()) {
            app_start_background();
        }

        if (app_check_is_backgrounded()) {
            const uint32_t current_time_ms = adk_read_millisecond_clock().ms;
            const uint32_t ms_elapsed = current_time_ms - bglast_time.ms;
            if (ms_elapsed < default_background_tick_rate_ms_delay) {
                const milliseconds_t sleep_for = {default_background_tick_rate_ms_delay - ms_elapsed};
                sb_thread_sleep(sleep_for);
            }
            bglast_time.ms = current_time_ms;
        }

        APP_THUNK_TRACE_PUSH_FN();

        APP_THUNK_TRACE_PUSH("app_event_loop_pretick");

        APP_THUNK_TRACE_PUSH("thread pool callbacks");
        thread_pool_run_completion_callbacks(&the_app.default_thread_pool);
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("adk_curl_run_callbacks");
        adk_curl_run_callbacks();
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("adk_http_tick");
        adk_http_tick();
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("tick_extensions");
        tick_extensions(NULL);
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("adk_reporting_tick");
        adk_reporting_tick(the_app.reporting_instance);
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("sb_tick");
        ASSERT_MSG(the_app.event_head == the_app.event_tail, "Not all events handled!");

        sb_tick(PEDANTIC_CAST(const adk_event_t **) & the_app.event_head, PEDANTIC_CAST(const adk_event_t **) & the_app.event_tail);

        ASSERT(the_app.event_head < the_app.event_tail);
        APP_THUNK_TRACE_POP();

        // last event should be time;
        {
            const adk_event_t * time_event = (the_app.event_tail - 1);
            ASSERT(time_event->event_data.type == adk_time_event);
            time = time_event->time;
            if ((last_time.ms == 0) || ((time.ms - last_time.ms) > 1000)) {
                last_time.ms = time.ms - 1;
            }
        }

        --the_app.event_tail;

        if (log_input_events) {
            APP_THUNK_TRACE_PUSH("sending events to cncbus");

            // Per convention last event is alway a time event, thus this would exclude time events
            // from sending to the cncbus.
            const size_t batch_size = the_app.event_tail - the_app.event_head;
            if (batch_size > 0) {
                cncbus_msg_t * const cncbus_msg = cncbus_msg_begin_unchecked(&the_app.bus, cncbus_msg_type_event);
                cncbus_msg_write_checked(cncbus_msg, &batch_size, sizeof(size_t));
                cncbus_msg_write_checked(cncbus_msg, the_app.event_head, (int)(sizeof(adk_event_t) * batch_size));

                cncbus_send_async(cncbus_msg, CNCBUS_INVALID_ADDRESS, CNCBUS_ADDRESS_EVENTS, CNCBUS_SUBNET_MASK_CORE, NULL);
            }

            APP_THUNK_TRACE_POP();
        }

        const milliseconds_t delta_time = {time.ms - last_time.ms};
        ASSERT(delta_time.ms <= 1000);
        APP_THUNK_TRACE_PUSH("publish fps");
        publish_metric(metric_type_delta_time_in_ms, &delta_time, sizeof(delta_time));
        APP_THUNK_TRACE_POP();

        APP_THUNK_TRACE_PUSH("file rate limit drain");
        if (statics.adk_file_rate_limiting.max_write_bytes_per_second > 0) {
            statics.adk_file_rate_limiting.bytes_to_drain += (((float)delta_time.ms / 1000) * (float)statics.adk_file_rate_limiting.max_write_bytes_per_second);
            const int32_t bytes_to_drain = (int32_t)statics.adk_file_rate_limiting.bytes_to_drain;
            if (bytes_to_drain > 0) {
                adk_file_write_limit_drain(bytes_to_drain);
            }

            statics.adk_file_rate_limiting.bytes_to_drain -= (float)bytes_to_drain;
        }
        APP_THUNK_TRACE_POP();

        last_time = time;

        log_periodic_metrics(delta_time, display_mode_result.display_mode);

#ifndef _STB_NATIVE
        if (!the_app.window) {
            APP_THUNK_TRACE_POP(); // app_event_loop_pretick
            APP_THUNK_TRACE_PUSH("app tick");
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
            running =
#endif
                tick_fn(time.ms, delta_time.ms / 1000.f, arg);
            APP_THUNK_TRACE_POP(); // app tick
            APP_THUNK_TRACE_PUSH("app_event_loop_post_tick");
        } else {
#else
        {
#endif
            if (!app_check_is_backgrounded()) {
                APP_THUNK_TRACE_PUSH("render_cmd write display & viewport");
                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_set_display_size,
                    display_mode_result.display_mode.width,
                    display_mode_result.display_mode.height,
                    MALLOC_TAG);

                RENDER_ENSURE_WRITE_CMD_STREAM(
                    &the_app.render_device->default_cmd_stream,
                    render_cmd_buf_write_set_viewport,
                    0,
                    0,
                    display_mode_result.display_mode.width,
                    display_mode_result.display_mode.height,
                    MALLOC_TAG);

                did_init_back_buffer = true;
                APP_THUNK_TRACE_POP();

                if (did_init_back_buffer) {
                    APP_THUNK_TRACE_PUSH("cg_context_begin");
                    cg_context_begin(delta_time);
                    APP_THUNK_TRACE_POP();

                    APP_THUNK_TRACE_POP(); // app_event_loop_pretick
                    APP_THUNK_TRACE_PUSH("app tick");
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
                    running =
#endif
                        tick_fn(time.ms, delta_time.ms / 1000.f, arg);
                    APP_THUNK_TRACE_POP();

                    APP_THUNK_TRACE_PUSH("app_event_loop_post_tick");
                    APP_THUNK_TRACE_PUSH("cg_context_end");
                    cg_context_end(MALLOC_TAG);
                    APP_THUNK_TRACE_POP();

                    APP_THUNK_TRACE_PUSH("render write_present & device_frame");
                    RENDER_ENSURE_WRITE_CMD_STREAM(
                        &the_app.render_device->default_cmd_stream,
                        render_cmd_buf_write_present,
                        statics.swap_interval,
                        MALLOC_TAG);

                    render_device_frame(the_app.render_device);
                    APP_THUNK_TRACE_POP();
                }
            } else {
#ifndef APP_THUNK_IGNORE_APP_TERMINATE
                running =
#endif

                    tick_fn(time.ms, delta_time.ms / 1000.f, arg);
            }
        }
        APP_THUNK_TRACE_POP(); // app_event_loop_post_tick
        APP_THUNK_TRACE_POP();

#ifndef APP_THUNK_IGNORE_APP_TERMINATE
        if (thunk_check_restart_requested()) {
            running = false;
        }
#endif

        // Sleep for the 'remainder' of the tick (when RHI command diffing is enabled)
        if (the_app.runtime_config.renderer.rhi_command_diffing.enabled && statics.swap_interval.interval > 0) {
            APP_TRACE_PUSH("frame sleep");

            const milliseconds_t tick_duration = {.ms = 1000 / (display_mode_result.display_mode.hz / statics.swap_interval.interval)};
            if (tick_duration.ms > delta_time.ms) {
                sb_thread_sleep((milliseconds_t){tick_duration.ms - delta_time.ms});
            }

            APP_TRACE_POP();
        }

        APP_TRACE_POP();
        if (rt->watchdog.enabled) {
            watchdog_tick(&statics.watchdog);
        }
        TRACE_TICK();
    }

    if (rt->watchdog.enabled) {
        watchdog_shutdown(&statics.watchdog);
    }
}

static void wasm_dump_heap_usage() {
    extern bifurcated_heap_t wasm_heap;
    heap_dump_usage(&wasm_heap.low);
    heap_dump_usage(&wasm_heap.high);
}

void app_dump_heaps(const adk_dump_heap_flags_e heaps_to_dump) {
    APP_THUNK_TRACE_PUSH_FN();

    if (heaps_to_dump & adk_dump_heap_runtime) {
        sb_platform_dump_heap_usage();
    }
    if (heaps_to_dump & adk_dump_heap_rhi_and_renderer) {
        render_dump_heap_usage(the_app.render_device);
    }
    if (heaps_to_dump & adk_dump_heap_canvas) {
        cg_context_dump_heap_usage(&the_app.canvas.context);
    }
    if (heaps_to_dump & adk_dump_heap_http_curl) {
        adk_curl_dump_heap_usage();
    }
    if (heaps_to_dump & adk_dump_heap_json_deflate) {
        json_deflate_dump_heap_usage();
    }
    if (heaps_to_dump & adk_dump_heap_http2) {
        adk_http_dump_heap_usage();
    }

    // basic hack of a check to see if we're in native vs not
    if ((heaps_to_dump & adk_dump_heap_wasm)
#ifdef _NATIVE_FFI
        && (!statics.rust_calls.drop_callback)
#endif
    ) {
        wasm_dump_heap_usage();
    }
}

void app_drop_callback(void * const closure) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.drop_callback) {
        statics.rust_calls.drop_callback(closure);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_i("app_drop_callback", (uint32_t)(uintptr_t)closure);
    verify_wasm_call_and_halt_on_failure(call_result);
}

int app_run_callback(void * const closure) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback) {
        return statics.rust_calls.callback(closure);
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_i("app_dispatch_callback", (uint32_t)(uintptr_t)closure);
    verify_wasm_call_and_halt_on_failure(call_result);
    return 0;
}

int app_run_callback_v(void * const closure, void * const a0) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_v) {
        return statics.rust_calls.callback_v(closure, a0);
    }
#endif
    uint32_t ret = 0;
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ri_ip("app_dispatch_callback_v", &ret, (uint32_t)(uintptr_t)closure, a0);
    verify_wasm_call_and_halt_on_failure(call_result);
    return ret;
}

void app_run_callback_vi(void * const closure, void * const a0, const int a1) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_vi) {
        statics.rust_calls.callback_vi(closure, a0, a1);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ipi("app_dispatch_callback_vi", (uint32_t)(uintptr_t)closure, a0, (uint32_t)a1);
    verify_wasm_call_and_halt_on_failure(call_result);
}

int app_run_callback_vvi(void * const closure, void * const a0, void * const a1, const int a2) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_vvi) {
        return statics.rust_calls.callback_vvi(closure, a0, a1, a2);
    }
#endif
    uint32_t ret = 0;
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ri_ippi("app_dispatch_callback_vvi", &ret, (uint32_t)(uintptr_t)closure, a0, a1, (uint32_t)a2);
    verify_wasm_call_and_halt_on_failure(call_result);
    return ret;
}

int app_run_callback_vvii(void * const closure, void * const a0, void * const a1, const int a2, const int a3) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_vvi) {
        return statics.rust_calls.callback_vvii(closure, a0, a1, a2, a3);
    }
#endif
    uint32_t ret = 0;
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ri_ippii("app_dispatch_callback_vvii", &ret, (uint32_t)(uintptr_t)closure, a0, a1, (uint32_t)a2, (uint32_t)a3);
    verify_wasm_call_and_halt_on_failure(call_result);
    return ret;
}

int app_run_callback_ivi(void * const closure, const int a0, void * const a1, const int a2) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_vvi) {
        return statics.rust_calls.callback_ivi(closure, a0, a1, a2);
    }
#endif
    uint32_t ret = 0;
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ri_iipi("app_dispatch_callback_ivi", &ret, (uint32_t)(uintptr_t)closure, (uint32_t)a0, a1, (uint32_t)a2);
    verify_wasm_call_and_halt_on_failure(call_result);
    return ret;
}

void app_run_callback_once_i(void * const closure, const int a0) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_i) {
        statics.rust_calls.callback_once_i(closure, a0);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ii("app_dispatch_callback_once_i", (uint32_t)(uintptr_t)closure, (uint32_t)a0);
    verify_wasm_call_and_halt_on_failure(call_result);
}

void app_run_callback_once_ii(void * const closure, const int a0, const int a1) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_ii) {
        statics.rust_calls.callback_once_ii(closure, a0, a1);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_iii("app_dispatch_callback_once_ii", (uint32_t)(uintptr_t)closure, (uint32_t)a0, (uint32_t)a1);
    verify_wasm_call_and_halt_on_failure(call_result);
}

void app_run_callback_once_iii(void * const closure, const int a0, const int a1, const int a2) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_iii) {
        statics.rust_calls.callback_once_iii(closure, a0, a1, a2);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_iiii("app_dispatch_callback_once_iii", (uint32_t)(uintptr_t)closure, (uint32_t)a0, (uint32_t)a1, (uint32_t)a2);
    verify_wasm_call_and_halt_on_failure(call_result);
}

void app_run_callback_once_iiii(void * const closure, const int a0, const int a1, const int a2, const int a3) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_iiii) {
        statics.rust_calls.callback_once_iiii(closure, a0, a1, a2, a3);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_iiiii("app_dispatch_callback_once_iiii", (uint32_t)(uintptr_t)closure, (uint32_t)a0, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3);
    verify_wasm_call_and_halt_on_failure(call_result);
}

void app_run_callback_once_v(void * const closure, void * const a0) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_v) {
        statics.rust_calls.callback_once_v(closure, a0);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ip("app_dispatch_callback_once_v", (uint32_t)(uintptr_t)closure, a0);
    verify_wasm_call_and_halt_on_failure(call_result);
}

void app_run_callback_once_vi(void * const closure, void * const a0, const int a1) {
#ifdef _NATIVE_FFI
    if (statics.rust_calls.callback_once_vi) {
        statics.rust_calls.callback_once_vi(closure, a0, a1);
        return;
    }
#endif
    const wasm_call_result_t call_result = get_active_wasm_interpreter()->call_ipi("app_dispatch_callback_once_vi", (uint32_t)(uintptr_t)closure, a0, (uint32_t)a1);
    verify_wasm_call_and_halt_on_failure(call_result);
}

bool adk_try_override_min_log_level(const char * const cli_arg_value) {
    for (uint8_t idx = 0; idx < num_log_levels; idx++) {
        const log_level_e level = (log_level_e)idx;
        if (strcasecmp(cli_arg_value, log_get_level_name(level)) == 0) {
            log_set_min_level(level);
            return true;
        }
    }

    const int level_value = atoi(cli_arg_value); // on failure returns 0 -- defaults to debug
    if (level_value >= 0 && level_value < num_log_levels) {
        log_set_min_level((log_level_e)level_value);
        return true;
    }

    return false;
}

#ifdef _NATIVE_FFI
void adk_thunk_init(const int argc, const char * const * const argv, const rust_callbacks_t * const rust_callbacks) {
    statics.argc = argc;
    statics.argv = argv;
    statics.rust_calls = *rust_callbacks;
    statics.app_state = adk_app_state_foreground;
}
#endif

int32_t adk_get_supported_refresh_rate(const int32_t starting_refresh_rate) {
    sb_enumerate_display_modes_result_t curr_mode;
    sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &curr_mode);

    int matched_hz = INT_MAX;

    sb_enumerate_display_modes_result_t mode;
    for (int ix = 0; sb_enumerate_display_modes(the_app.display_settings.curr_display, ix, &mode); ++ix) {
        if ((mode.display_mode.width == curr_mode.display_mode.width)
            && (mode.display_mode.height == curr_mode.display_mode.height)
            && (mode.display_mode.hz >= starting_refresh_rate)
            && (mode.display_mode.hz < matched_hz)) {
            matched_hz = mode.display_mode.hz;
        }
    }

    return matched_hz == INT_MAX ? 0 : matched_hz;
}

bool adk_set_refresh_rate(const int32_t refresh_rate, const int32_t video_fps) {
    if (video_fps < 0 || refresh_rate < video_fps) {
        return false;
    }
    if (refresh_rate == 0) {
        if (statics.overridden_swap_interval) {
            LOG_ALWAYS(TAG_APP, "unlimited refresh rate requested, but swap interval overridden: [%i]", statics.swap_interval.interval);
        } else {
#ifndef _SHIP
            statics.swap_interval.interval = 0;
#else
            statics.swap_interval.interval = 1;
#endif
            LOG_ALWAYS(TAG_APP, "unlimited refresh rate requested, setting swap interval to: [%i]", statics.swap_interval.interval);
        }
    } else {
        sb_enumerate_display_modes_result_t old_mode;
        sb_enumerate_display_modes(the_app.display_settings.curr_display, the_app.display_settings.curr_display_mode, &old_mode);

        if (refresh_rate != old_mode.display_mode.hz) {
            // Validate requested rate as some platforms may not have appropriate error checking
            int supported_rate = adk_get_supported_refresh_rate(refresh_rate);
            if (supported_rate != refresh_rate) {
                LOG_INFO(TAG_APP, "Unable to set refresh rate to [%i] Hz, next supported is [%i] Hz", refresh_rate, supported_rate);
                return false;
            }

            if (!sb_set_main_display_refresh_rate(refresh_rate)) {
                LOG_INFO(TAG_APP, "Unable to set refresh rate to [%i] Hz", refresh_rate);
                return false;
            }

            // Update display mode index
            int ix;
            sb_enumerate_display_modes_result_t mode;
            for (ix = 0; sb_enumerate_display_modes(the_app.display_settings.curr_display, ix, &mode); ++ix) {
                if ((mode.display_mode.hz == refresh_rate)
                    && (mode.display_mode.width == old_mode.display_mode.width)
                    && (mode.display_mode.height == old_mode.display_mode.height)) {
                    the_app.display_settings.curr_display_mode = ix;
                    break;
                }
            }
            if (ix != the_app.display_settings.curr_display_mode) {
                LOG_ERROR(TAG_APP, "error, unable to update current display mode for new refresh rate [%i] Hz", refresh_rate);
            }
        }

        // NB: If refresh rate isn't an integer multiple of video fps, low video quality is likely.  In that case, the app should choose
        // the highest supported refresh rate.
        // If refresh_rate is not an integer multiple of video_fps, closest swap interval will be calculated while not exceeding requested video_fps
        const int new_interval = video_fps ? (1 + ((refresh_rate - 1) / video_fps)) : 1;

        if (statics.overridden_swap_interval) {
            LOG_ALWAYS(TAG_APP, "refresh rate [%i] Hz, video rate [%i] fps, swap interval [%i] (overridden from %i)", refresh_rate, video_fps, statics.swap_interval.interval, new_interval);
        } else {
            statics.swap_interval.interval = new_interval;
            LOG_ALWAYS(TAG_APP, "refresh rate [%i] Hz, video rate [%i] fps, swap interval [%i]", refresh_rate, video_fps, statics.swap_interval.interval);
        }
    }

    return true;
}

void adk_log_msg(const char * const msg) {
    LOG(log_level_app, TAG_APP, "%s", msg);
}

int32_t read_events(void * const evbuffer, int32_t bufsize, int32_t sizeof_event) {
    VERIFY_MSG(sizeof_event == (int)sizeof(adk_event_t), "Rust event structure size does not match native C structure size (%d != %d)", sizeof_event, (int)sizeof(adk_event_t));
    adk_event_t * write_event = evbuffer;
    int read_count = 0;
    while ((the_app.event_head < the_app.event_tail) && (read_count < bufsize)) {
        *write_event = *the_app.event_head;
        ++write_event;
        ++the_app.event_head;
        ++read_count;
    }
    return read_count;
}

heap_metrics_t adk_get_heap_usage(const adk_heap_e heap) {
    switch (heap) {
        case adk_heap_runtime:
            return sb_platform_get_heap_metrics();
        case adk_heap_rhi_and_renderer:
            return render_get_heap_metrics(the_app.render_device);
        case adk_heap_canvas_low:
            return cg_context_get_heap_metrics_low(&the_app.canvas.context);
        case adk_heap_canvas_high:
            return cg_context_get_heap_metrics_high(&the_app.canvas.context);
        case adk_heap_http_curl:
            return adk_curl_get_heap_metrics();
        case adk_heap_http2:
            return adk_http_get_heap_metrics();
        case adk_heap_json_deflate:
            return json_deflate_get_heap_metrics();
        case adk_heap_wasm_low:
#ifdef _NATIVE_FFI
            if (statics.rust_calls.drop_callback) {
                return (heap_metrics_t){0};
            } else
#endif
            {
                extern bifurcated_heap_t wasm_heap;
                return heap_get_metrics(&wasm_heap.low);
            }
        case adk_heap_wasm_high:
#ifdef _NATIVE_FFI
            if (statics.rust_calls.drop_callback) {
                return (heap_metrics_t){0};
            } else
#endif
            {
                extern bifurcated_heap_t wasm_heap;
                return heap_get_metrics(&wasm_heap.high);
            }
    }

    TRAP("The specified heap cannot not be queried.");

    return (heap_metrics_t){0};
}

void adk_notify_app_status(const sb_app_notify_e notify) {
    APP_THUNK_TRACE_PUSH_FN();

    sb_notify_app_status(notify);
    if (notify == sb_app_notify_dismiss_system_loading_screen) {
        // at this point the app is considered 'init' from a core perspective (we're rendering pixels to the screen)
        time_to_first_interaction.dimiss_system_splash_timestamp = adk_read_millisecond_clock();
        TTFI_TRACE_TIME_SPAN_END(&time_to_first_interaction.app_init_timestamp);
        publish_metric(metric_type_time_to_first_interaction, &time_to_first_interaction, sizeof(time_to_first_interaction));
    }

    APP_THUNK_TRACE_POP();
}

native_slice_t adk_get_deeplink_buffer_bridge(
    const sb_deeplink_handle_t * const handle) {
    const const_mem_region_t region = sb_get_deeplink_buffer(handle);
    return (native_slice_t){.ptr = (uint64_t)region.adr, .size = (int32_t)region.size};
}
