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

#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "wayland-client.h"
#include "wayland-egl.h"
#include "simpleshell-client-protocol.h"

#define TAG_RT_STUB FOURCC('S', 'T', 'U', 'B')

#define RT_STUB_VERBOSE_SB_TICK 0
#define RT_STUB_VERBOSE_SB_GET_WINDOW_CLIENT_AREA 0
#define UNUSED(x) ((void)x)

enum {
    metrics_bytes_per_mbytes = 1024 * 1024
};

enum {
    window_width = 1920,
    window_height = 1080
};

typedef struct _AppCtx
{
   struct wl_display *display;
   struct wl_registry *registry;
   struct wl_shm *shm;
   struct wl_compositor *compositor;
   struct wl_simple_shell *shell;
   struct wl_seat *seat;
   struct wl_keyboard *keyboard;
   struct wl_pointer *pointer;
   struct wl_touch *touch;
   struct wl_surface *surface;
   struct wl_output *output;
   struct wl_egl_window *native;
   struct wl_callback *frameCallback;

   struct xkb_context *xkbCtx;
   struct xkb_keymap *xkbKeymap;
   struct xkb_state *xkbState;
   xkb_mod_index_t modAlt;
   xkb_mod_index_t modCtrl;

   int disneyPendingKeyEvent;

   bool haveMode;
   int planeX;
   int planeY;
   int planeWidth;
   int planeHeight;

   uint32_t surfaceIdOther;
   uint32_t surfaceIdCurrent;
   float surfaceOpacity;
   float surfaceZOrder;
   bool surfaceVisible;
   int surfaceX;
   int surfaceY;
   int surfaceWidth;
   int surfaceHeight;

   int surfaceDX;
   int surfaceDY;
   int surfaceDWidth;
   int surfaceDHeight;   
} AppCtx;

static struct
{
    void * main_win;
    struct {
        int w, h;
    } display_mode;

#if defined(MWSTUB_EXAMPLE_X11_OPENGLES)
    Display * x_display;
#endif
    AppCtx* ctx;
} statics;


extern const char * sb_persona_id;

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version);
static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name);

static const struct wl_registry_listener registryListener =
{
   registryHandleGlobal,
   registryHandleGlobalRemove
};

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

    AppCtx* ctx = (AppCtx*)malloc(sizeof(AppCtx));
    memset(ctx, 0, sizeof(AppCtx));

    struct wl_display* display = wl_display_connect(NULL);
    LOG_DEBUG(TAG_RT_STUB, "wl_display_connect: display = %p", display);

    struct wl_registry* registry = wl_display_get_registry(display);
    LOG_DEBUG(TAG_RT_STUB, "wl_display_get_registry: registry = %p", registry);

    ctx->display = display;
    ctx->registry = registry;
    wl_registry_add_listener(registry, &registryListener, ctx);

    wl_display_roundtrip(ctx->display);

    statics.ctx = ctx;

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
    uint64_t membytes = sysconf(_SC_PHYS_PAGES);
    membytes *= sysconf(_SC_PAGESIZE);
    membytes /= metrics_bytes_per_mbytes;
    out->main_memory_mbytes = membytes;
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

    AppCtx* ctx = statics.ctx;
    ctx->surface = wl_compositor_create_surface(ctx->compositor);
    ctx->native = wl_egl_window_create(ctx->surface, window_width, window_height);
    ctx->planeWidth = window_width;
    ctx->planeHeight = window_height;

    statics.main_win = ctx->native;
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

    if ((display_index != 0) || (display_mode_index != 0)) {
        LOG_DEBUG(TAG_RT_STUB, "invalid display and/or mode index query");
        return false;
    }

    LOG_DEBUG(TAG_RT_STUB, "display_mode_index %d display_index %d", display_mode_index, display_index);
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

    wl_display_dispatch_pending(statics.ctx->display);
    if (statics.ctx->disneyPendingKeyEvent != 0) {
/*
        adk_post_event((adk_event_t){
            .time = adk_read_millisecond_clock(),
            .event_data = (adk_event_data_t){
                .type = adk_stb_rich_input_event,
                .stb_rich_input.stb_key = statics.ctx->disneyPendingKeyEvent,
                .stb_rich_input.event = adk_key_event_key_down,
                .stb_rich_input.repeat = 0
            }});

        adk_post_event((adk_event_t){
            .time = adk_read_millisecond_clock(),
            .event_data = (adk_event_data_t){
                .type = adk_stb_rich_input_event,
                .stb_rich_input.stb_key = statics.ctx->disneyPendingKeyEvent,
                .stb_rich_input.event = adk_key_event_key_up,
                .stb_rich_input.repeat = 0
            }});
*/
        adk_post_event((adk_event_t){
            .time = adk_read_millisecond_clock(),
            .event_data = (adk_event_data_t){
                .type = adk_stb_input_event,
                .stb_input.stb_key = statics.ctx->disneyPendingKeyEvent,
                .stb_input.repeat = 0
            }});
        LOG_DEBUG(TAG_RT_STUB, "PostKey Event %d", statics.ctx->disneyPendingKeyEvent);
        statics.ctx->disneyPendingKeyEvent = 0;
    } 
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

static void shmFormat(void *data, struct wl_shm *wl_shm, uint32_t format)
{
   AppCtx *ctx = (AppCtx*)data;
   LOG_DEBUG(TAG_RT_STUB, "shm format: %X", format);
}

struct wl_shm_listener shmListener = {
   shmFormat
};

static void keyboardKeymap( void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size )
{
   AppCtx *ctx= (AppCtx*)data;

   if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
   {
      void *map= mmap( 0, size, PROT_READ, MAP_SHARED, fd, 0 );
      if ( map != MAP_FAILED )
      {
         if ( !ctx->xkbCtx )
         {
            ctx->xkbCtx= xkb_context_new( XKB_CONTEXT_NO_FLAGS );
         }
         else
         {
            LOG_ERROR(TAG_RT_STUB, "error: xkb_context_new failed");
         }
         if ( ctx->xkbCtx )
         {
            if ( ctx->xkbKeymap )
            {
               xkb_keymap_unref( ctx->xkbKeymap );
               ctx->xkbKeymap= 0;
            }
            ctx->xkbKeymap= xkb_keymap_new_from_string( ctx->xkbCtx, (char*)map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
            if ( !ctx->xkbKeymap )
            {
               LOG_ERROR(TAG_RT_STUB, "error: xkb_keymap_new_from_string failed");
            }
            if ( ctx->xkbState )
            {
               xkb_state_unref( ctx->xkbState );
               ctx->xkbState= 0;
            }
            ctx->xkbState= xkb_state_new( ctx->xkbKeymap );
            if ( !ctx->xkbState )
            {
               LOG_ERROR(TAG_RT_STUB, "error: xkb_state_new failed");
            }
            if ( ctx->xkbKeymap )
            {
               ctx->modAlt= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_ALT );
               ctx->modCtrl= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_CTRL );
            }
         }
         munmap( map, size );
      }
   }

   close( fd );
}

static void keyboardEnter( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                           struct wl_surface *surface, struct wl_array *keys )
{
   UNUSED(data);
   UNUSED(keyboard);
   UNUSED(serial);
   UNUSED(keys);

   LOG_DEBUG(TAG_RT_STUB, "keyboard enter surface %p", surface );
}

static void keyboardLeave( void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface )
{
   UNUSED(data);
   UNUSED(keyboard);
   UNUSED(serial);

   LOG_DEBUG(TAG_RT_STUB,"keyboard leave surface %p", surface );
}

static void keyboardKey( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                         uint32_t time, uint32_t key, uint32_t state )
{
   AppCtx *ctx= (AppCtx*)data;
   UNUSED(keyboard);
   UNUSED(serial);
   xkb_keycode_t keyCode;
   uint32_t sym;

   if (ctx->xkbState){
      // As per wayland protocol for XKB_V1 map, we must add 8 to the key code
        keyCode = key+8;

        sym = xkb_state_key_get_one_sym( ctx->xkbState, keyCode );
        int ctrl = 0;
        int alt = 0;

        if ( xkb_state_mod_index_is_active( ctx->xkbState, ctx->modCtrl, XKB_STATE_MODS_DEPRESSED) == 1 ){
            ctrl= 1;
        }

        if ( xkb_state_mod_index_is_active( ctx->xkbState, ctx->modAlt, XKB_STATE_MODS_DEPRESSED) == 1 ) {
           alt= 1;
        }

        LOG_DEBUG(TAG_RT_STUB, "keyboardKey: sym %X state %s ctrl %d alt %d time %u",
            sym, (state == WL_KEYBOARD_KEY_STATE_PRESSED ? "Down" : "Up"), ctrl, alt, time);

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
            switch(sym) {
                case 0xFF51:
                    ctx->disneyPendingKeyEvent = adk_stb_key_left;
                    break;
                case 0xFF52:
                    ctx->disneyPendingKeyEvent = adk_stb_key_up;
                    break;
                case 0xFF53:
                    ctx->disneyPendingKeyEvent = adk_stb_key_right;
                    break;
                case 0xFF54:
                    ctx->disneyPendingKeyEvent = adk_stb_key_down;
                    break;
                case 0xFF0D:
                    ctx->disneyPendingKeyEvent = adk_stb_key_select;
                    break;
                case 0x30:
                    ctx->disneyPendingKeyEvent = adk_stb_key_zero;
                    break;    
                case 0x31 ... 0x39:
                    ctx->disneyPendingKeyEvent = adk_stb_key_one + (sym - 0x31);
                    break;
                case 0xFF08:
                    ctx->disneyPendingKeyEvent = adk_stb_key_back;
                    break;
                case 0xFF1B:
                    ctx->disneyPendingKeyEvent = adk_stb_key_menu;
                    break;
                default:
                    ctx->disneyPendingKeyEvent = 0;
                    break;
            }
        }

    //   if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    //         processInput( ctx, sym );
    //   }
   }
}

static void keyboardModifiers( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                               uint32_t mods_depressed, uint32_t mods_latched,
                               uint32_t mods_locked, uint32_t group )
{
   AppCtx *ctx= (AppCtx*)data;
   if (ctx->xkbState){
      xkb_state_update_mask(ctx->xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
   }
}

static void keyboardRepeatInfo( void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay )
{
   UNUSED(data);
   UNUSED(keyboard);
   UNUSED(rate);
   UNUSED(delay);
}

static const struct wl_keyboard_listener keyboardListener= {
   keyboardKeymap,
   keyboardEnter,
   keyboardLeave,
   keyboardKey,
   keyboardModifiers,
   keyboardRepeatInfo
};


static void seatCapabilities( void *data, struct wl_seat *seat, uint32_t capabilities )
{
   AppCtx *ctx = (AppCtx*)data;
   LOG_DEBUG(TAG_RT_STUB, "seat %p caps: %X", seat, capabilities );

   if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD){
      LOG_DEBUG(TAG_RT_STUB, "  seat has keyboard");
      ctx->keyboard= wl_seat_get_keyboard( ctx->seat );
      LOG_DEBUG(TAG_RT_STUB, "  keyboard %p", ctx->keyboard );
      wl_keyboard_add_listener( ctx->keyboard, &keyboardListener, ctx );
   }
}

static void seatName( void *data, struct wl_seat *seat, const char *name )
{
   AppCtx *ctx = (AppCtx*)data;
   LOG_DEBUG(TAG_RT_STUB, "seat %p name: %s\n", seat, name);
}

static const struct wl_seat_listener seatListener = {
   seatCapabilities,
   seatName
};

static void outputGeometry( void *data, struct wl_output *output, int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height, int32_t subpixel,
                            const char *make, const char *model, int32_t transform )
{
   UNUSED(data);
   UNUSED(output);
   UNUSED(x);
   UNUSED(y);
   UNUSED(physical_width);
   UNUSED(physical_height);
   UNUSED(subpixel);
   UNUSED(make);
   UNUSED(model);
   UNUSED(transform);
}

static void outputMode( void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh )
{
   AppCtx *ctx = (AppCtx*)data;

   if (flags & WL_OUTPUT_MODE_CURRENT) {
      ctx->haveMode = true;
      if ((width != ctx->planeWidth) || (height != ctx->planeHeight)) {
         ctx->planeWidth = width;
         ctx->planeHeight = height;
         LOG_DEBUG(TAG_RT_STUB, "outputMode: resize egl window to (%d,%d)", ctx->planeWidth, ctx->planeHeight);
        //  resizeSurface(ctx, 0, 0, ctx->planeWidth, ctx->planeHeight);
      }
   }
}

static void outputDone( void *data, struct wl_output *output )
{
   UNUSED(data);
   UNUSED(output);
}

static void outputScale( void *data, struct wl_output *output, int32_t factor )
{
   UNUSED(data);
   UNUSED(output);
   UNUSED(factor);
}

static const struct wl_output_listener outputListener = {
   outputGeometry,
   outputMode,
   outputDone,
   outputScale
};

static void shellSurfaceId(void *data,
                           struct wl_simple_shell *wl_simple_shell,
                           struct wl_surface *surface,
                           uint32_t surfaceId)
{
   AppCtx *ctx = (AppCtx*)data;
   char name[32];

   sprintf( name, "disneyplus-surface-%x", surfaceId );
   LOG_DEBUG(TAG_RT_STUB, "shell: surface created: %p id %x", surface, surfaceId);
   wl_simple_shell_set_name(ctx->shell, surfaceId, name);
}

static void shellSurfaceCreated(void *data,
                                struct wl_simple_shell *wl_simple_shell,
                                uint32_t surfaceId,
                                const char *name)
{
   AppCtx *ctx = (AppCtx*)data;

   LOG_DEBUG(TAG_RT_STUB, "shell: surface created: %x name: %s", surfaceId, name);
   ctx->surfaceIdOther= ctx->surfaceIdCurrent;
   ctx->surfaceIdCurrent= surfaceId;
   wl_simple_shell_get_status( ctx->shell, ctx->surfaceIdCurrent );
   LOG_DEBUG(TAG_RT_STUB, "shell: surfaceCurrent: %x surfaceOther: %x", ctx->surfaceIdCurrent, ctx->surfaceIdOther);
}

static void shellSurfaceDestroyed(void *data,
                                  struct wl_simple_shell *wl_simple_shell,
                                  uint32_t surfaceId,
                                  const char *name)
{
   AppCtx *ctx = (AppCtx*)data;

   LOG_DEBUG(TAG_RT_STUB, "shell: surface destroyed: %x name: %s", surfaceId, name);

   if (ctx->surfaceIdCurrent == surfaceId) {
      ctx->surfaceIdCurrent= ctx->surfaceIdOther;
      ctx->surfaceIdOther= 0;
      wl_simple_shell_get_status( ctx->shell, ctx->surfaceIdCurrent );
   }

   if (ctx->surfaceIdOther == surfaceId) {
      ctx->surfaceIdOther= 0;
   }

   LOG_DEBUG(TAG_RT_STUB, "shell: surfaceCurrent: %x surfaceOther: %x", ctx->surfaceIdCurrent, ctx->surfaceIdOther);
}

static void shellSurfaceStatus(void *data,
                               struct wl_simple_shell *wl_simple_shell,
                               uint32_t surfaceId,
                               const char *name,
                               uint32_t visible,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               wl_fixed_t opacity,
                               wl_fixed_t zorder)
{
   AppCtx *ctx = (AppCtx*)data;

   LOG_DEBUG(TAG_RT_STUB, "shell: surface: %x name: %s", surfaceId, name);
   LOG_DEBUG(TAG_RT_STUB, "shell: position (%d,%d,%d,%d) visible %d opacity %f zorder %f",
           x, y, width, height, visible, wl_fixed_to_double(opacity), wl_fixed_to_double(zorder) );

   ctx->surfaceVisible= visible;
   ctx->surfaceX= x;
   ctx->surfaceY= y;
   ctx->surfaceWidth= width;
   ctx->surfaceHeight= height;
   ctx->surfaceOpacity= wl_fixed_to_double(opacity);
   ctx->surfaceZOrder= wl_fixed_to_double(zorder);
}

static void shellGetSurfacesDone(void *data,
                                 struct wl_simple_shell *wl_simple_shell)
{
   AppCtx *ctx = (AppCtx*)data;
   LOG_DEBUG(TAG_RT_STUB, "shell: get all surfaces done");
}

static const struct wl_simple_shell_listener shellListener =
{
   shellSurfaceId,
   shellSurfaceCreated,
   shellSurfaceDestroyed,
   shellSurfaceStatus,
   shellGetSurfacesDone
};

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version)
{
   AppCtx *ctx = (AppCtx*)data;
   int len;

   LOG_DEBUG(TAG_RT_STUB, "westeros-test: registry: id %d interface (%s) version %d", id, interface, version);

   len= strlen(interface);
   if ((len == 6) && !strncmp(interface, "wl_shm", len)) {
      ctx->shm= (struct wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
      LOG_DEBUG(TAG_RT_STUB, "shm %p", ctx->shm);
      wl_shm_add_listener(ctx->shm, &shmListener, ctx);
   }
   else if ((len == 13) && !strncmp(interface, "wl_compositor", len)) {
      ctx->compositor= (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
      LOG_DEBUG(TAG_RT_STUB, "compositor %p", ctx->compositor);
   }
   else if ((len == 7) && !strncmp(interface, "wl_seat", len)) {
      ctx->seat = (struct wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 4);
      LOG_DEBUG(TAG_RT_STUB, "seat %p", ctx->seat);
      wl_seat_add_listener(ctx->seat, &seatListener, ctx);
   }
   else if ((len == 9) && !strncmp(interface, "wl_output", len)) {
      ctx->output = (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
      LOG_DEBUG(TAG_RT_STUB, "output %p", ctx->output);
      wl_output_add_listener(ctx->output, &outputListener, ctx);
      wl_display_roundtrip(ctx->display);
   }
   else if ((len == 15) && !strncmp(interface, "wl_simple_shell", len)) {
        ctx->shell = (struct wl_simple_shell*)wl_registry_bind(registry, id, &wl_simple_shell_interface, 1);
        LOG_DEBUG(TAG_RT_STUB, "shell %p", ctx->shell );
        wl_simple_shell_add_listener(ctx->shell, &shellListener, ctx);
   }
}

static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name)
{
}