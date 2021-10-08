/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
dispmanx.c

implementation details for dispmanx (windowing/display/input)
*/

#include _PCH

#include "dispmanx.h"

#include "bcm_host.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/private/display_simulated.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/sb_display.h"

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termio.h>
#include <unistd.h>

#define NO_SCALE

#define TAG_DISPMANX FOURCC('D', 'S', 'M', 'X')

enum {
    max_hardware_input_devices = 32,
    key_code_table_size = 512,
    display_hz = 60 // can't find a call for dispmanx that gives us a refresh rate, so default is 60 (best to just ignore?)
};

static struct {
    dispmanx_window_t the_window;
    int hardware_input_events[max_hardware_input_devices];
    bool using_hardware_inputs;
    simulated_display_modes_t modes;
    uint32_t display_width;
    uint32_t display_height;
    adk_keycode_e key_lookup_table[key_code_table_size];
} statics = {0};

STATIC_ASSERT(key_code_table_size > KEY_LAST);

static void init_lookup() {
    statics.key_lookup_table[KEY_SPACE] = adk_key_space;
    statics.key_lookup_table[KEY_APOSTROPHE] = adk_key_apostrophe;
    statics.key_lookup_table[KEY_COMMA] = adk_key_comma;
    statics.key_lookup_table[KEY_MINUS] = adk_key_minus;
    statics.key_lookup_table[KEY_DOT] = adk_key_period;
    statics.key_lookup_table[KEY_SLASH] = adk_key_slash;
    statics.key_lookup_table[KEY_0] = adk_key_0;
    statics.key_lookup_table[KEY_1] = adk_key_1;
    statics.key_lookup_table[KEY_2] = adk_key_2;
    statics.key_lookup_table[KEY_3] = adk_key_3;
    statics.key_lookup_table[KEY_4] = adk_key_4;
    statics.key_lookup_table[KEY_5] = adk_key_5;
    statics.key_lookup_table[KEY_6] = adk_key_6;
    statics.key_lookup_table[KEY_7] = adk_key_7;
    statics.key_lookup_table[KEY_8] = adk_key_8;
    statics.key_lookup_table[KEY_9] = adk_key_9;
    statics.key_lookup_table[KEY_SEMICOLON] = adk_key_semicolon;
    statics.key_lookup_table[KEY_EQUAL] = adk_key_equal;
    statics.key_lookup_table[KEY_A] = adk_key_a;
    statics.key_lookup_table[KEY_B] = adk_key_b;
    statics.key_lookup_table[KEY_C] = adk_key_c;
    statics.key_lookup_table[KEY_D] = adk_key_d;
    statics.key_lookup_table[KEY_E] = adk_key_e;
    statics.key_lookup_table[KEY_F] = adk_key_f;
    statics.key_lookup_table[KEY_G] = adk_key_g;
    statics.key_lookup_table[KEY_H] = adk_key_h;
    statics.key_lookup_table[KEY_I] = adk_key_i;
    statics.key_lookup_table[KEY_J] = adk_key_j;
    statics.key_lookup_table[KEY_K] = adk_key_k;
    statics.key_lookup_table[KEY_L] = adk_key_l;
    statics.key_lookup_table[KEY_M] = adk_key_m;
    statics.key_lookup_table[KEY_N] = adk_key_n;
    statics.key_lookup_table[KEY_O] = adk_key_o;
    statics.key_lookup_table[KEY_P] = adk_key_p;
    statics.key_lookup_table[KEY_Q] = adk_key_q;
    statics.key_lookup_table[KEY_R] = adk_key_r;
    statics.key_lookup_table[KEY_S] = adk_key_s;
    statics.key_lookup_table[KEY_T] = adk_key_t;
    statics.key_lookup_table[KEY_U] = adk_key_u;
    statics.key_lookup_table[KEY_V] = adk_key_v;
    statics.key_lookup_table[KEY_W] = adk_key_w;
    statics.key_lookup_table[KEY_X] = adk_key_x;
    statics.key_lookup_table[KEY_Y] = adk_key_y;
    statics.key_lookup_table[KEY_Z] = adk_key_z;
    statics.key_lookup_table[KEY_LEFTBRACE] = adk_key_left_bracket;
    statics.key_lookup_table[KEY_BACKSLASH] = adk_key_backslash;
    statics.key_lookup_table[KEY_RIGHTBRACE] = adk_key_right_bracket;
    statics.key_lookup_table[KEY_GRAVE] = adk_key_grave_accent;
    // adk_key_world_1;
    // adk_key_world_2;

    /* = function = keys = */
    statics.key_lookup_table[KEY_ESC] = adk_key_escape;
    statics.key_lookup_table[KEY_ENTER] = adk_key_enter;
    statics.key_lookup_table[KEY_TAB] = adk_key_tab;
    statics.key_lookup_table[KEY_BACKSPACE] = adk_key_backspace;
    statics.key_lookup_table[KEY_INSERT] = adk_key_insert;
    statics.key_lookup_table[KEY_DELETE] = adk_key_delete;
    statics.key_lookup_table[KEY_RIGHT] = adk_key_right;
    statics.key_lookup_table[KEY_LEFT] = adk_key_left;
    statics.key_lookup_table[KEY_DOWN] = adk_key_down;
    statics.key_lookup_table[KEY_UP] = adk_key_up;
    statics.key_lookup_table[KEY_PAGEUP] = adk_key_page_up;
    statics.key_lookup_table[KEY_PAGEDOWN] = adk_key_page_down;
    statics.key_lookup_table[KEY_HOME] = adk_key_home;
    statics.key_lookup_table[KEY_END] = adk_key_end;
    statics.key_lookup_table[KEY_CAPSLOCK] = adk_key_caps_lock;
    statics.key_lookup_table[KEY_SCROLLLOCK] = adk_key_scroll_lock;
    statics.key_lookup_table[KEY_NUMLOCK] = adk_key_num_lock;
    statics.key_lookup_table[KEY_PRINT] = adk_key_print_screen;
    statics.key_lookup_table[KEY_PAUSE] = adk_key_pause;
    statics.key_lookup_table[KEY_F1] = adk_key_f1;
    statics.key_lookup_table[KEY_F2] = adk_key_f2;
    statics.key_lookup_table[KEY_F3] = adk_key_f3;
    statics.key_lookup_table[KEY_F4] = adk_key_f4;
    statics.key_lookup_table[KEY_F5] = adk_key_f5;
    statics.key_lookup_table[KEY_F6] = adk_key_f6;
    statics.key_lookup_table[KEY_F7] = adk_key_f7;
    statics.key_lookup_table[KEY_F8] = adk_key_f8;
    statics.key_lookup_table[KEY_F9] = adk_key_f9;
    statics.key_lookup_table[KEY_F10] = adk_key_f10;
    statics.key_lookup_table[KEY_F11] = adk_key_f11;
    statics.key_lookup_table[KEY_F12] = adk_key_f12;
    statics.key_lookup_table[KEY_F13] = adk_key_f13;
    statics.key_lookup_table[KEY_F14] = adk_key_f14;
    statics.key_lookup_table[KEY_F15] = adk_key_f15;
    statics.key_lookup_table[KEY_F16] = adk_key_f16;
    statics.key_lookup_table[KEY_F17] = adk_key_f17;
    statics.key_lookup_table[KEY_F18] = adk_key_f18;
    statics.key_lookup_table[KEY_F19] = adk_key_f19;
    statics.key_lookup_table[KEY_F20] = adk_key_f20;
    statics.key_lookup_table[KEY_F21] = adk_key_f21;
    statics.key_lookup_table[KEY_F22] = adk_key_f22;
    statics.key_lookup_table[KEY_F23] = adk_key_f23;
    statics.key_lookup_table[KEY_F24] = adk_key_f24;
    // adk_key_f25;
    statics.key_lookup_table[KEY_KP0] = adk_key_kp_0;
    statics.key_lookup_table[KEY_KP1] = adk_key_kp_1;
    statics.key_lookup_table[KEY_KP2] = adk_key_kp_2;
    statics.key_lookup_table[KEY_KP3] = adk_key_kp_3;
    statics.key_lookup_table[KEY_KP4] = adk_key_kp_4;
    statics.key_lookup_table[KEY_KP5] = adk_key_kp_5;
    statics.key_lookup_table[KEY_KP6] = adk_key_kp_6;
    statics.key_lookup_table[KEY_KP7] = adk_key_kp_7;
    statics.key_lookup_table[KEY_KP8] = adk_key_kp_8;
    statics.key_lookup_table[KEY_KP9] = adk_key_kp_9;
    statics.key_lookup_table[KEY_KPDOT] = adk_key_kp_decimal;
    statics.key_lookup_table[KEY_KPSLASH] = adk_key_kp_divide;
    statics.key_lookup_table[KEY_KPASTERISK] = adk_key_kp_multiply;
    statics.key_lookup_table[KEY_KPMINUS] = adk_key_kp_subtract;
    statics.key_lookup_table[KEY_KPPLUS] = adk_key_kp_add;
    statics.key_lookup_table[KEY_KPENTER] = adk_key_kp_enter;
    statics.key_lookup_table[KEY_KPEQUAL] = adk_key_kp_equal;
    statics.key_lookup_table[KEY_LEFTSHIFT] = adk_key_left_shift;
    statics.key_lookup_table[KEY_LEFTCTRL] = adk_key_left_control;
    statics.key_lookup_table[KEY_LEFTALT] = adk_key_left_alt;
    statics.key_lookup_table[KEY_LEFTMETA] = adk_key_left_super;
    statics.key_lookup_table[KEY_RIGHTSHIFT] = adk_key_right_shift;
    statics.key_lookup_table[KEY_RIGHTCTRL] = adk_key_right_control;
    statics.key_lookup_table[KEY_RIGHTALT] = adk_key_right_alt;
    statics.key_lookup_table[KEY_RIGHTMETA] = adk_key_right_super;
    statics.key_lookup_table[KEY_MENU] = adk_key_menu;
    statics.key_lookup_table[KEY_LAST] = adk_key_last;
}

static adk_keycode_e linux_to_adk_keycode(const int system_code) {
    if (system_code > key_code_table_size) {
        return adk_key_none;
    }
    return statics.key_lookup_table[system_code];
}

static int kbhit(void) {
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);

    struct termios term;
    memcpy(&term, &original, sizeof(term));

    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    int characters_buffered = 0;
    ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);

    tcsetattr(STDIN_FILENO, TCSANOW, &original);

    return characters_buffered;
}

static void send_close() {
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = (adk_event_data_t){
            .type = adk_window_event,
            .win = (adk_window_event_t){
                .window = &statics.the_window,
                .event_data = (adk_window_event_data_t){
                    .event = adk_window_event_close,
                    .close = 0,
                }}}});
}

static void sig_handler(int signo) {
    adk_post_event_async((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = (adk_event_data_t){
            .type = adk_window_event,
            .win = (adk_window_event_t){
                .window = &statics.the_window,
                .event_data = (adk_window_event_data_t){
                    .event = adk_window_event_close,
                    .close = 0,
                }}}});
}

static void get_hardware_event_queues() {
    const char * device_path = "/dev/input/";
    FILE * fp = popen("ls /dev/input", "r");
    const int max_events_str_len = 128;
    char events_str[max_events_str_len];
    for (int i = 0; i < max_hardware_input_devices; ++i) {
        if (fgets(events_str, max_events_str_len, fp)) {
            // cull folders, and generic 'mice' event (so if we start listening on mouse inputs we don't get duplicates)
            if (strstr(events_str, "by-path") || strstr(events_str, "by-id") || strstr(events_str, "mice")) {
                continue;
            }
            events_str[strlen(events_str) - 1] = '\0';
            char buff[256] = {0};
            sprintf_s(buff, ARRAY_SIZE(buff), "%s%s", device_path, events_str);
            int fd = open(buff, O_RDONLY | O_NONBLOCK | O_NDELAY);
            statics.hardware_input_events[i] = fd;

            if (statics.hardware_input_events[i] != -1) {
                statics.using_hardware_inputs = true;
            }
        } else {
            statics.hardware_input_events[i] = -1;
        }
    }
    pclose(fp);
}

bool dispmanx_init() {
    LOG_DEBUG(TAG_DISPMANX, "dispmanx_init");
    ZEROMEM(&statics);

    // prevent linux from buffering keystrokes.
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    bcm_host_init();
    signal(SIGINT, sig_handler);

    init_lookup();
    get_hardware_event_queues();

    return true;
}

void dispmanx_shutdown() {
    LOG_DEBUG(TAG_DISPMANX, "dispmanx_shutdown");
    bcm_host_deinit();
    for (int i = 0; i < max_hardware_input_devices; ++i) {
        if (statics.hardware_input_events[i] != -1) {
            close(statics.hardware_input_events[i]);
        }
    }
}

static void key_down(const adk_keycode_e key) {
    const milliseconds_t time = adk_read_millisecond_clock();

    adk_post_event((adk_event_t){
        .time = time,
        .event_data = (adk_event_data_t){
            .type = adk_key_event,
            .key = (adk_key_event_t){
                .event = adk_key_event_key_down,
                .key = key,
                .mod_keys = 0,
                .repeat = 0}}});
}

static void key_up(const adk_keycode_e key) {
    const milliseconds_t time = adk_read_millisecond_clock();

    adk_post_event((adk_event_t){
        .time = time,
        .event_data = (adk_event_data_t){
            .type = adk_key_event,
            .key = (adk_key_event_t){
                .event = adk_key_event_key_up,
                .key = key,
                .mod_keys = 0,
                .repeat = 0}}});
}

static void handle_key(const adk_keycode_e key, bool down) {
    if (down) {
        key_down(key);
    } else {
        key_up(key);
    }
}

static void send_key(const adk_keycode_e key) {
    key_down(key);
    key_up(key);
}

void dispmanx_dispatch_events() {
    if (statics.using_hardware_inputs) {
        struct input_event input_event;
        for (int i = 0; i < max_hardware_input_devices; ++i) {
            if (statics.hardware_input_events[i] == -1) {
                continue;
            }
            while (true) {
                struct pollfd pollfd;
                pollfd.fd = statics.hardware_input_events[i];
                pollfd.events = POLLIN;
                int ready = poll(&pollfd, 1, 0);
                if (ready <= 0) {
                    break;
                }
                if ((pollfd.revents & POLLIN) == 0) {
                    break;
                }
                const ssize_t n = read(statics.hardware_input_events[i], &input_event, sizeof(input_event));
                if (n == (ssize_t)-1) {
                    ASSERT(errno != EINVAL);
                    if (errno == EINTR) {
                        continue;
                    } else {
                        break;
                    }
                } else if (n != sizeof(input_event)) {
                    errno = EIO;
                    break;
                }
                if (input_event.type == EV_KEY && input_event.value >= 0 && input_event.value <= 2) {
                    const adk_keycode_e adk_keycode = linux_to_adk_keycode(input_event.code);
                    if (adk_keycode != adk_key_none) {
                        handle_key(adk_keycode, input_event.value >= 1);
                    }
                }
            }
        }
    } else {
        int a = 0, b = 0;
        const int n = kbhit();

        if (n) {
            const int c = getchar();

            if (n == 3) {
                b = getchar();
                a = getchar();
            }

            if (c == 'q') {
                send_close();
            }

            if (c == ' ') {
                send_key(adk_key_space);
            }

            if ((c == 27) && (b == 91)) {
                if (a == 65) {
                    send_key(adk_key_up);
                }

                if (a == 66) {
                    send_key(adk_key_down);
                }

                if (a == 67) {
                    send_key(adk_key_right);
                }

                if (a == 68) {
                    send_key(adk_key_left);
                }
            }
        }
    }
}

dispmanx_window_t * dispmanx_create_window(const int w, const int h, const char * const tag) {
    LOG_DEBUG(TAG_DISPMANX, "dispmanx_create_window");
    ASSERT(!statics.the_window.display);

    VC_RECT_T dst_rect = {0};
    VC_RECT_T src_rect = {0};

    src_rect.width = w << 16;
    src_rect.height = h << 16;

    uint32_t screen_width, screen_height;
#ifdef NO_SCALE
    screen_width = w;
    screen_height = h;
#else
    VERIFY(graphics_get_display_size(0, &screen_width, &screen_height) >= 0);
#endif

    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    statics.the_window.w = w;
    statics.the_window.h = h;
    statics.the_window.display = vc_dispmanx_display_open(0);

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    VERIFY(update);

    VC_DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};
    statics.the_window.window.element = vc_dispmanx_element_add(update, statics.the_window.display, 0 /*layer*/, &dst_rect, 0 /*src*/, &src_rect, DISPMANX_PROTECTION_NONE, &alpha /*alpha*/, 0 /*clamp*/, VC_IMAGE_ROT0);
    statics.the_window.window.width = screen_width;
    statics.the_window.window.height = screen_height;

    vc_dispmanx_update_submit_sync(update);

    return &statics.the_window;
}

void dispmanx_get_window_size(dispmanx_window_t * const window, int * const w, int * const h) {
    *w = window->w;
    *h = window->h;
}

void dispmanx_close_window(dispmanx_window_t * const window, const char * const tag) {
    LOG_DEBUG(TAG_DISPMANX, "dispmanx_close_window");
    ASSERT(window == &statics.the_window);
    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    VERIFY(update);
    vc_dispmanx_element_remove(update, statics.the_window.window.element);
    vc_dispmanx_update_submit_sync(update);
    vc_dispmanx_display_close(statics.the_window.display);
    ZEROMEM(&statics.the_window);
}

static void load_file_display_modes(int window_id) {
    VERIFY(graphics_get_display_size(0, (uint32_t *)&statics.display_width, (uint32_t *)&statics.display_height) >= 0);

    const sb_display_mode_t max_screen_size = {
        .width = statics.display_width,
        .height = statics.display_height,
        .hz = display_hz};

    load_simulated_display_modes(&statics.modes, max_screen_size);
}

bool sb_set_main_display_refresh_rate(const int32_t hz) {
    return hz == display_hz; // only choice
}

bool sb_enumerate_display_modes(const int32_t display_index, const int32_t display_mode_index, sb_enumerate_display_modes_result_t * const out_results) {
    ZEROMEM(out_results);

    if (statics.modes.display_mode_count == 0) {
        load_file_display_modes(0);
    }

    if ((display_index != 0) || (display_mode_index >= statics.modes.display_mode_count) || display_mode_index < 0) {
        return false;
    }
    const sb_display_mode_t selected_mode = statics.modes.display_modes[display_mode_index];

    *out_results = (sb_enumerate_display_modes_result_t){
        .display_mode = selected_mode,
        .status = ((display_index == 0) ? sb_display_modes_primary_display : 0) | (((statics.display_height == (uint32_t)selected_mode.height) && (statics.display_width == (uint32_t)selected_mode.width)) ? sb_display_modes_current_display_mode : 0)};

    return true;
}
