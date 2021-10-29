/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_runtime_posix_stub.c

steamboat platform stub for POSIX based OpenGL ES capable GPU
*/

#include _PCH

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <assert.h>
#pragma pop_macro("NDEBUG")

#include "source/adk/log/log.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/ref_ports/drydock/impl_tracking.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_system_metrics.h"

#include <signal.h>
// used for sysconf and associated _SC_* constants
#include <unistd.h>

#if defined(MWSTUB_EXAMPLE_X11_OPENGLES)
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif // defined(MWSTUB_EXAMPLE_X11_OPENGLES)

#define TAG_RT_STUB FOURCC('S', 'T', 'U', 'B')

#define RT_STUB_VERBOSE_SB_TICK 0
#define RT_STUB_VERBOSE_SB_GET_WINDOW_CLIENT_AREA 0

enum {
    metrics_bytes_per_mbytes = 1024 * 1024
};

enum {
    window_width = 1280,
    window_height = 720
};

static struct
{
    void * main_win;
    struct {
        int w, h;
    } display_mode;

#if defined(MWSTUB_EXAMPLE_X11_OPENGLES)
    Display * x_display;
#endif
} statics;

extern const char * sb_persona_id;

static void sig_handler(int signo) {
    /*
     * PARTNER MW EXAMPLE:
     *   1) signalling MW application management events to the ADK sub-system by
     *      mapping MW events -> ADK type events
     */
    LOG_DEBUG(TAG_RT_STUB, "!!! posting ADK window close event request !!!");
    adk_post_event_async((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_window_event,
            .win = (adk_window_event_t){
                .window = statics.main_win,
                .event_data = (adk_window_event_data_t){
                    adk_window_event_close,
                }}}});
}

void __sb_posix_get_device_id(adk_system_metrics_t * const out);

bool __sb_posix_init_platform(adk_api_t * api, const int argc, const char * const * const argv) {
    /*
     * PARTNER TODO:
     *
     *   NOTE: assumed used as part of execution sequence:
     *     ./source/adk/steamboat/ref_ports/sb_runtime_posix.c->sb_init()
     *      |
     *     \|/
     *     __sb_posix_init_platform()
     *
     *   1) perform middleware (MW) use initialization essentials
     *   2) perform middleware (MW) specific human interface device (HID) input access and use initialization, i.e. for infrared (IR), Bluetooth, etc. type devices
     *   3) if required perform other middleware (MW) specific initializations and/or error handling initialization
     *
     *      Example:
     *          ...
     *          mw_initPlatform();
     *          mw_initHidAccess();
     *          mw_initOther();
     *          ...
     *          signal(SIGINT, sig_handler);
     *
     *          return true;
     */
    LOG_DEBUG(TAG_RT_STUB, "initialize MW essentials");
    LOG_DEBUG(TAG_RT_STUB, "initialize MW HID access");
    LOG_DEBUG(TAG_RT_STUB, "initialize MW other access");

    LOG_DEBUG(TAG_RT_STUB, "setup custom handler function for 'SIGINT'/'program interrupt'");
    signal(SIGINT, sig_handler);

    return true;
}

void __sb_posix_shutdown_platform() {
    /*
     * PARTNER TODO:
     *
     *   NOTE: assumed used as part of execution sequence:
     *     ./source/adk/steamboat/ref_ports/sb_runtime_posix.c->sb_shutdown()
     *      |
     *     \|/
     *     __sb_posix_shutdown_platform()
     *
     *   1) perform middleware (MW) specific human interface device (HID) access and use termination
     *   2) perform other middleware (MW) specific access and use termination items
     *   2) perform middleware (MW) access and use termination
     *
     *      Example:
     *          ...
     *          mw_terminateHidDeviceAccess();
     *          mw_terminateOther();
     *          mw_terminatePlatform();
     *          ...
     */
    LOG_DEBUG(TAG_RT_STUB, "terminate MW HID access");
    LOG_DEBUG(TAG_RT_STUB, "terminate MW other");
    LOG_DEBUG(TAG_RT_STUB, "terminate MW access essentials");
}

void sb_get_system_metrics(adk_system_metrics_t * const out) {
    LOG_DEBUG(TAG_RT_STUB, "retrieve device display information");

    VERIFY(0 == strcpy_s(out->vendor, ARRAY_SIZE(out->vendor), SB_METRICS_VENDOR));
    VERIFY(0 == strcpy_s(out->device, ARRAY_SIZE(out->device), SB_METRICS_DEVICE));
    VERIFY(0 == strcpy_s(out->software, ARRAY_SIZE(out->software), SB_METRICS_SOFTWARE));
    VERIFY(0 == strcpy_s(out->revision, ARRAY_SIZE(out->revision), SB_METRICS_REVISION));
    VERIFY(0 == strcpy_s(out->advertising_id, ARRAY_SIZE(out->advertising_id), SB_METRICS_ADVERTISING_ID));
    VERIFY(0 == strcpy_s(out->device_region, ARRAY_SIZE(out->device_region), SB_METRICS_REGION));
    VERIFY(0 == strcpy_s(out->firmware, ARRAY_SIZE(out->firmware), SB_METRICS_FIRMWARE));

    __sb_posix_get_device_id(out);

    VERIFY(0 == strcpy_s(out->cpu, ARRAY_SIZE(out->cpu), SB_METRICS_CPU));

    VERIFY(0 == strcpy_s(out->gpu, ARRAY_SIZE(out->gpu), SB_METRICS_GPU));

    out->num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    out->main_memory_mbytes = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)) / metrics_bytes_per_mbytes;
    out->video_memory_mbytes = sb_metrics_video_memory_mbytes;
    out->num_hardware_threads = sysconf(_SC_NPROCESSORS_ONLN);

    out->device_class = sb_metrics_device_class;

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
    /*
     * PARTNER TODO:
     *  1) retrieve display device information using middleware (MW) APIs
     *  2) extract and store display width + height information in 'statics.display_mode.<w,h>' entries
     *  3) if required, register with the graphics/window management sub-system of the middleware (MW)
     *  4) create and initialize a graphics window to be the size of entire display area
     *  5) store the created window information for re-use in 'statics.main_win' entry
     *
     *      Example:
     *          ...
     *          mw_stDisplaySettings graphics_settings;
     *          mw_getDisplaySettings(&graphics_settings);
     *
     *          statics.display_mode.w = graphics_settings.width;
     *          statics.display_mode.h = graphics_settings.height;
     *
     *          mw_stWindowInfo win_info;
     *          mw_registerGraphicsSubsystem(&win_info);
     *          ...
     *          win_info.x = 0;
     *          win_info.y = 0;
     *          win_info.width = statics.display_mode.w;
     *          win_info.height = statics.display_mode.h;
     *          ...
     *          statics.main_win = mw_createNativeWindow(&win_info);
     *          ...
     */

    NOT_IMPLEMENTED_EX;

#if !defined(MWSTUB_EXAMPLE_X11_OPENGLES)
    LOG_DEBUG(TAG_RT_STUB, "retrieve device display information");
    statics.display_mode.w = window_width;
    statics.display_mode.h = window_height;
    LOG_DEBUG(TAG_RT_STUB, "identified display (W x H): %d x %d", statics.display_mode.w, statics.display_mode.h);
    LOG_DEBUG(TAG_RT_STUB, "register with the graphics/window management sub-system");
    LOG_DEBUG(TAG_RT_STUB, "create and initialize a graphics window");
    statics.main_win = (void *)0xD0D0CACA;
    VERIFY(statics.main_win);
#else // defined(MWSTUB_EXAMPLE_X11_OPENGLES)
    {
        Display * x_display = NULL;
        Window root, win;
        XSetWindowAttributes swa;

        LOG_DEBUG(TAG_RT_STUB, "register with the X11 graphics/window management sub-system");
        x_display = XOpenDisplay(NULL);
        if (x_display == NULL) {
            LOG_DEBUG(TAG_RT_STUB, "cannot connect to x server");
            return NULL;
        }
        root = DefaultRootWindow(x_display);
        statics.x_display = x_display;

        LOG_DEBUG(TAG_RT_STUB, "using display (W x H): %d x %d", statics.display_mode.w, statics.display_mode.h);
        statics.display_mode.w = window_width;
        statics.display_mode.h = window_height;

        LOG_DEBUG(TAG_RT_STUB, "create and initialize a graphics window");
        swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;
        win = XCreateWindow(
            x_display, root, 0, 0, window_width, window_height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa);
        XMapWindow(x_display, win);
        XFlush(x_display);
        statics.main_win = (void *)win;
        VERIFY(statics.main_win);
    }
#endif // !defined(MWSTUB_EXAMPLE_X11_OPENGLES)

    LOG_DEBUG(TAG_RT_STUB, "created mw native window '0x%X", statics.main_win);
    return (sb_window_t *)statics.main_win;
}

bool sb_set_main_display_refresh_rate(const int32_t hz) {
    /*
     * PARTNER TODO:
     *  1) determine current monitor, if multiple monitors are supported
     *  2) determine if refresh rate can be changed (for example, is window in full screen mode?)
     *  3) set current monitor's refresh rate to specified rate
     */

    NOT_IMPLEMENTED_EX;

    return true;
}

void sb_destroy_main_window() {
    /*
     * PARTNER TODO:
     *  1) destroy graphics window previously created by 'sb_init_main_display' and identified by 'statics.main_win' entry
     *  2) clean-up 'statics.main_win' entry
     *
     *      Example:
     *          ...
     *          mw_destroyNativeWindow(statics.main_win);
     *          statics.main_win = NULL;
     *          ...
     */

    NOT_IMPLEMENTED_EX;

    LOG_DEBUG(TAG_RT_STUB, "destroy mw native window '", statics.main_win);

#if defined(MWSTUB_EXAMPLE_X11_OPENGLES)
    if (statics.x_display) {
        XDestroyWindow(statics.x_display, (Window)statics.main_win);
        XCloseDisplay(statics.x_display);
        statics.x_display = NULL;
    }
#endif // defined(MWSTUB_EXAMPLE_X11_OPENGLES)

    statics.main_win = NULL;
    statics.display_mode.w = 0;
    statics.display_mode.h = 0;
}

void sb_get_window_client_area(sb_window_t * const window, int * const out_width, int * const out_height) {
    /*
     * PARTNER TODO:
     *  1) return window client area created and used via middleware
     *
     *      Example:
     *          ...
     *          *out_width = statics.display_mode.w;
     *          *out_height = statics.display_mode.h;
     *          ...
     */

    NOT_IMPLEMENTED_EX;

#if (RT_STUB_VERBOSE_SB_GET_WINDOW_CLIENT_AREA)
    LOG_DEBUG(TAG_RT_STUB, "returning mw client window (W x H): (%d x %d)", statics.display_mode.w, statics.display_mode.h);
#endif
    *out_width = statics.display_mode.w;
    *out_height = statics.display_mode.h;
}

bool sb_enumerate_display_modes(const int32_t display_index, const int32_t display_mode_index, sb_enumerate_display_modes_result_t * const out_results) {
    /*
     * PARTNER TODO
     *  fill display mode description for the specified display and display mode.
     *
     *     NOTE: ADK is searching for primary display
     *
     *      Example:
     *          ...
     *          if ((display_index > 0) || (display_mode_index > 0)) {
     *              return false;
     *          }
     *
     *         *out_results = (sb_enumerate_display_modes_result_t){
     *              .display_mode = {
     *                  .width = window_width,
     *                  .height = window_height,
     *                  .hz = 60},
     *              .status = sb_display_modes_primary_display | sb_display_modes_current_display_mode };
     *
     *         return true;
     */

    NOT_IMPLEMENTED_EX;

    if ((display_index > 0) || (display_mode_index > 0)) {
        LOG_DEBUG(TAG_RT_STUB, "invalid display and/or mode index query");
        return false;
    }

    LOG_DEBUG(TAG_RT_STUB, "storing display (W x H x Hz): (%d x %d @ %d Hz)", window_width, window_height, 60);
    LOG_DEBUG(TAG_RT_STUB, "indicating primary display");
    LOG_DEBUG(TAG_RT_STUB, "indicating is current display mode");
    LOG_DEBUG(TAG_RT_STUB, "indicating successful query");

    *out_results = (sb_enumerate_display_modes_result_t){
        .display_mode = {
            .width = window_width,
            .height = window_height,
            .hz = 60},
        .status = sb_display_modes_primary_display | sb_display_modes_current_display_mode};

    return true;
}

void sb_tick(const adk_event_t ** const head, const adk_event_t ** const tail) {
    /*
     * PARTNER TODO
     *  1) handle rendering device change
     *  2) call 'adk_lock_events()'
     *  3) convert middleware events to ADK compatible events and insert into ADK event queue
     *  4) insert 'adk_time_event' event to end of the queue
     *  5) call 'adk_get_events_swap_and_unlock()'
     *
     *      Example:
     *          adk_lock_events();
     *          ...
     *          mw_getSystemEvents(&pEvents);
     *          partner_convertMWEventsToADKEvents(pEvents);
     *          ...
     *          adk_post_event((adk_event_t){
     *              .time = adk_read_millisecond_clock()
     *              .event_data = (adk_event_data_t) {
     *                  .type = adk_time_event,
     *                  .unused = 0,
     *              }
     *          });
     *          adk_get_events_swap_and_unlock(head, tail);
     */

    NOT_IMPLEMENTED_EX;

    adk_lock_events();

#if (RT_STUB_VERBOSE_SB_TICK)
    LOG_DEBUG(TAG_RT_STUB, "convert MW events to ADK events");
#endif

    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = (adk_event_data_t){
            .type = adk_time_event,
            .unused = 0,
        }});
    adk_get_events_swap_and_unlock(head, tail);
}

void sb_assert_failed(const char * const message, const char * const filename, const char * const function, const int line) {
    /*
     * PARTNER TODO
     *  1) map parameters to middleware (MW) implementation of POSIX assert()
     *
     *      Example:
     *          __assert_fail(message, filename, line, function);
     */

    NOT_IMPLEMENTED_EX;

    LOG_DEBUG(TAG_RT_STUB, "triggering posix 'assert()'");
    __assert_fail(message, filename, line, function);
}

void sb_notify_app_status(const sb_app_notify_e notify) {
    /*
     * PARTNER TODO
     *  1) notify middleware of application state changes such as user login,
     *     login without entitelement, logout, dismiss system loading screen,
     *     etc.
     *
     *      Example:
     *          ...
     *          switch (notify)
     *          {
     *          case sb_app_notify_dismiss_system_loading_screen:
     *              mw_notifyApplicationStart();
     *              break;
     *          default:
     *              break;
     *          }
     *          ...
     */

    NOT_IMPLEMENTED_EX;

    switch (notify) {
        case sb_app_notify_logged_out:
            LOG_DEBUG(TAG_RT_STUB, "notify mw -> 'sb_app_notify_logged_out'");
            break;
        case sb_app_notify_logged_in_not_entitled:
            LOG_DEBUG(TAG_RT_STUB, "notify mw -> 'sb_app_notify_logged_in_not_entitled'");
            break;
        case sb_app_notify_logged_in_and_entitled:
            LOG_DEBUG(TAG_RT_STUB, "notify mw -> 'sb_app_notify_logged_in_and_entitled'");
            break;
        case sb_app_notify_dismiss_system_loading_screen:
            LOG_DEBUG(TAG_RT_STUB, "notify mw -> 'sb_app_notify_dismiss_system_loading_screen'");
            break;
        default:
            break;
    }
}

void sb_text_to_speech(const char * const text) {
    /*
     * PARTNER TODO
     *  1) convert text to audio output as speech
     *
     *      Example:
     *          ...
     *          mw_convertToSpeech(text);
     *          ...
     */

    NOT_IMPLEMENTED_EX;

    ASSERT(text);
    ASSERT(text[0]);
    LOG_DEBUG(TAG_RT_STUB, "mw requested to convert to speech '%s'", text);
}

sb_uuid_t sb_generate_uuid() {
    /*
     * PARTNER TODO
     *  1) creates a new universally unique identifier (UUID)
     *
     *      Example:
     *          uuid_generate(uuid);
     */

    NOT_IMPLEMENTED_EX;

    sb_uuid_t uuid = {0};
    char * const uuid_b = (char *)uuid.id;

    // example, hard code a version 4 UUID value ed8d8255-abfd-418c-9124-0dcdf5d86dad
    LOG_DEBUG(TAG_RT_STUB, "using version 4 UUID 'ed8d8255-abfd-418c-9124-0dcdf5d86dad'");
    *(uint32_t *)(uuid_b + 0) = 0xed8d8255;
    *(uint16_t *)(uuid_b + 4) = 0xabfd;
    *(uint16_t *)(uuid_b + 6) = 0x418c;
    *(uint16_t *)(uuid_b + 8) = 0x9124;
    *(uint16_t *)(uuid_b + 10) = 0x0dcd;
    *(uint32_t *)(uuid_b + 12) = 0xf5d86dad;

    return uuid;
}
