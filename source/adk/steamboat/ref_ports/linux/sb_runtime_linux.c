/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_runtime_linux.c

steamboat platform for linux
*/

#include _PCH

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include "source/adk/log/log.h"

#include <assert.h>
#include <uuid/uuid.h>
// used for sysconf and associated _SC_* constants
#include <unistd.h>

#pragma pop_macro("NDEBUG")

#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/private/glfw/glfw_support.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_system_metrics.h"

#define TAG_RT_LINUX FOURCC('R', 'T', 'L', 'X')

enum {
    metrics_bytes_per_mbytes = 1024 * 1024
};

static struct
{
    GLFWwindow * main_win;
} statics;

extern const char * sb_persona_id;

bool __sb_posix_init_platform(adk_api_t * api, const int argc, const char * const * const argv) {
    if (!api->runtime_flags.headless) {
        if (!init_glfw()) {
            LOG_ERROR(TAG_RT_LINUX, "init_glfw() failed.");
            return false;
        }
    }

    return true;
}

void __sb_posix_shutdown_platform() {
    glfwTerminate();
}

void __sb_posix_get_device_id(adk_system_metrics_t * const out);

void sb_get_system_metrics(adk_system_metrics_t * const out) {
    ZEROMEM(out);

    // partner and partner_guid must be valid AWS S3 bucket names
    VERIFY(0 == strcpy_s(out->vendor, ARRAY_SIZE(out->vendor), SB_METRICS_VENDOR));
    VERIFY(0 == strcpy_s(out->partner, ARRAY_SIZE(out->partner), SB_METRICS_PARTNER));
    VERIFY(0 == strcpy_s(out->device, ARRAY_SIZE(out->device), SB_METRICS_DEVICE));
    VERIFY(0 == strcpy_s(out->software, ARRAY_SIZE(out->software), SB_METRICS_SOFTWARE));
    VERIFY(0 == strcpy_s(out->revision, ARRAY_SIZE(out->revision), SB_METRICS_REVISION));
    VERIFY(0 == strcpy_s(out->gpu, ARRAY_SIZE(out->gpu), SB_METRICS_GPU));
    VERIFY(0 == strcpy_s(out->partner_guid, ARRAY_SIZE(out->partner_guid), SB_METRICS_PARTNER_GUID));
    VERIFY(0 == strcpy_s(out->advertising_id, ARRAY_SIZE(out->advertising_id), SB_METRICS_ADVERTISING_ID));
    VERIFY(0 == strcpy_s(out->device_region, ARRAY_SIZE(out->device_region), SB_METRICS_REGION));
    VERIFY(0 == strcpy_s(out->firmware, ARRAY_SIZE(out->firmware), SB_METRICS_FIRMWARE));

    VERIFY(0 == strcpy_s(out->cpu, ARRAY_SIZE(out->cpu), SB_METRICS_CPU));

    __sb_posix_get_device_id(out);

    out->device_class = sb_metrics_device_class;

    out->num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    out->main_memory_mbytes = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)) / metrics_bytes_per_mbytes;
    out->video_memory_mbytes = sb_metrics_video_memory_mbytes;
    out->num_hardware_threads = sysconf(_SC_NPROCESSORS_ONLN);

    out->gpu_ready_texture_formats = sb_metrics_texture_format;

    out->persistent_storage_available_bytes = sb_metrics_persistent_storage_available_bytes;
    out->persistent_storage_max_write_bytes_per_second = sb_metrics_persistent_storage_max_write_bps;

#ifdef _SHIP
    VERIFY(0 == strcpy_s(out->tenancy, ARRAY_SIZE(out->tenancy), "prod"));
#else
    VERIFY(0 == strcpy_s(out->tenancy, ARRAY_SIZE(out->tenancy), "dev"));
#endif

    // If the persona id was set at the command line using the --persona parameter, use that value.
    // Otherwise, set the persona id to an empty string, indicating the default persona will be run.
    if (sb_persona_id) {
        VERIFY(0 == strcpy_s(out->persona_id, ARRAY_SIZE(out->persona_id), sb_persona_id));
    } else {
        VERIFY(0 == strcpy_s(out->persona_id, ARRAY_SIZE(out->persona_id), ""));
    }
}

sb_window_t * sb_init_main_display(const int display_index, const int display_mode_index, const char * const title) {
    ASSERT(statics.main_win == NULL);

    sb_enumerate_display_modes_result_t display_results;
    VERIFY_MSG(sb_enumerate_display_modes(display_index, display_mode_index, &display_results), "Received invalid display index or display mode index when initing main display.");

    int monitor_count;
    GLFWmonitor ** const monitors = glfwGetMonitors(&monitor_count);
    ASSERT((display_index < monitor_count) && (display_index >= 0));

    statics.main_win = create_glfw_window(
        display_results.display_mode.width,
        display_results.display_mode.height,
        title,
        (display_results.status & sb_display_modes_primary_display && display_results.status & sb_display_modes_current_display_mode) ? monitors[display_index] : NULL);

    return (sb_window_t *)statics.main_win;
}

bool sb_set_main_display_refresh_rate(const int32_t hz) {
    if (statics.main_win == NULL) {
        return false; // not initialized
    }

    // Get main window's monitor, only available if window is in full-screen mode
    GLFWmonitor * const monitor = glfwGetWindowMonitor(statics.main_win);
    if (!monitor) {
        return false; // error or not in full-screen, can't change refresh rate
    }

    glfwSetWindowMonitor(statics.main_win, monitor, 0, 0, 0, 0, hz);

    return true;
}

void sb_destroy_main_window() {
    if (statics.main_win) {
        glfwDestroyWindow((GLFWwindow *)statics.main_win);
        statics.main_win = NULL;
    }
}

void sb_get_window_client_area(sb_window_t * const window, int * const out_width, int * const out_height) {
    glfwGetWindowSize((GLFWwindow *)window, out_width, out_height);
}

void sb_tick(const adk_event_t ** const head, const adk_event_t ** const tail) {
    if (glfw_restart_requested()) {
        glfw_restart_acknowledge();

        void adk_video_restart_begin(void);
        void adk_video_restart_end(void);

        adk_video_restart_begin();
        glfwDestroyWindow(statics.main_win);
        statics.main_win = NULL;
        adk_video_restart_end();
    }

    adk_lock_events();
    // NOTE: the event system here can be journaled to a file
    // and then played back. Since time is part of the event queue
    // it is possible to reconstruct identical inputs.
    glfwPollEvents();
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = (adk_event_data_t){
            .type = adk_time_event,
            .unused = 0,
        }});
    adk_get_events_swap_and_unlock(head, tail);
}

void sb_notify_app_status(const sb_app_notify_e notify) {
    // App will call this function signaling the various states/requested changes present in `sb_app_notify_e`
    // On receiving a `sb_app_notify_dismiss_system_loading_screen`, this function is responsible for removing the system's "loading" screen.
    // The other variants of `sb_app_notify_e` are informational notifications to the system and do not require any action with respect to the application.
}

/*
=======================================
DEBUG
=======================================
*/

void sb_assert_failed(const char * const message, const char * const filename, const char * const function, const int line) {
    __assert_fail(message, filename, line, function);
}

/*
=======================================
Text-to-Speech
=======================================
*/
void sb_text_to_speech(const char * const text) {
    ASSERT(text);
    ASSERT(text[0]);
    // App will call this function with UTF-8-encoded text for the TTS implementation to vocalize/play.
    // Implement if text to speech is desired or a requirement for your platform.
}

/*
=======================================
UUID
=======================================
*/

sb_uuid_t sb_generate_uuid() {
    sb_uuid_t uuid;
    uuid_generate(uuid.id);
    return uuid;
}
