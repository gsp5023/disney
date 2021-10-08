/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#ifdef _DRYDOCK

#include "impl_tracking.h"

#include <assert.h>

#define COLOR_GREEN "\033[92m"
#define COLOR_RED "\033[91m"
#define COLOR_PURPLE "\033[95m"
#define COLOR_OFF "\033[m"

// TODO: Should we use gperf to generate a perfect hash function and table here?
// But then steamboat api changes will need a regeneration of the whole table.
// This could be automated, but is it worth the added complexity?
drydock_api_entry_t g_sb_impl_table[] = {

    // sb_display.h
    {"sb_destroy_main_window", "", 0, steam_api_schrodinger},
    {"sb_enumerate_display_modes", "", 0, steam_api_schrodinger},
    {"sb_get_window_client_area", "", 0, steam_api_schrodinger},
    {"sb_init_main_display", "", 0, steam_api_schrodinger},

    // sb_file.h
    {"sb_close_directory", "", 0, steam_api_schrodinger},
    {"sb_create_directory_path", "", 0, steam_api_schrodinger},
    {"sb_delete_directory", "", 0, steam_api_schrodinger},
    {"sb_delete_file", "", 0, steam_api_schrodinger},
    {"sb_fclose", "", 0, steam_api_schrodinger},
    {"sb_feof", "", 0, steam_api_schrodinger},
    {"sb_fopen", "", 0, steam_api_schrodinger},
    {"sb_fread", "", 0, steam_api_schrodinger},
    {"sb_fseek", "", 0, steam_api_schrodinger},
    {"sb_ftell", "", 0, steam_api_schrodinger},
    {"sb_fwrite", "", 0, steam_api_schrodinger},
    {"sb_get_directory_entry_name", "", 0, steam_api_schrodinger},
    {"sb_open_directory", "", 0, steam_api_schrodinger},
    {"sb_read_directory", "", 0, steam_api_schrodinger},
    {"sb_stat", "", 0, steam_api_schrodinger},

    // sb_locale.h
    {"sb_get_locale", "", 0, steam_api_schrodinger},

    // sb_platform.h
    {"sb_notify_app_status", "", 0, steam_api_schrodinger},
    {"sb_assert_failed", "", 0, steam_api_schrodinger},
    {"sb_generate_uuid", "", 0, steam_api_schrodinger},
    {"sb_get_deeplink_buffer", "", 0, steam_api_schrodinger},
    {"sb_get_system_metrics", "", 0, steam_api_schrodinger},
    {"sb_get_time_since_epoch", "", 0, steam_api_schrodinger},
    {"sb_init", "", 0, steam_api_schrodinger},
    {"sb_map_pages", "", 0, steam_api_schrodinger},
    {"sb_on_app_load_failure", "", 0, steam_api_schrodinger},
    {"sb_platform_dump_heap_usage", "", 0, steam_api_schrodinger},
    {"sb_preinit", "", 0, steam_api_schrodinger},
    {"sb_protect_pages", "", 0, steam_api_schrodinger},
    {"sb_read_nanosecond_clock", "", 0, steam_api_schrodinger},
    {"sb_release_deeplink", "", 0, steam_api_schrodinger},
    {"sb_report_app_metrics", "", 0, steam_api_schrodinger},
    {"sb_shutdown", "", 0, steam_api_schrodinger},
    {"sb_text_to_speech", "", 0, steam_api_schrodinger},
    {"sb_tick", "", 0, steam_api_schrodinger},
    {"sb_unmap_pages", "", 0, steam_api_schrodinger},

    // sb_socket.h
    {"sb_accept_socket", "", 0, steam_api_schrodinger},
    {"sb_bind_socket", "", 0, steam_api_schrodinger},
    {"sb_close_socket", "", 0, steam_api_schrodinger},
    {"sb_connect_socket", "", 0, steam_api_schrodinger},
    {"sb_create_socket", "", 0, steam_api_schrodinger},
    {"sb_enable_blocking_socket", "", 0, steam_api_schrodinger},
    {"sb_getaddrinfo", "", 0, steam_api_schrodinger},
    {"sb_getaddrinfo_error_str", "", 0, steam_api_schrodinger},
    {"sb_getnameinfo", "", 0, steam_api_schrodinger},
    {"sb_getpeername", "", 0, steam_api_schrodinger},
    {"sb_getsockname", "", 0, steam_api_schrodinger},
    {"sb_listen_socket", "", 0, steam_api_schrodinger},
    {"sb_platform_socket_error_str", "", 0, steam_api_schrodinger},
    {"sb_shutdown_socket", "", 0, steam_api_schrodinger},
    {"sb_socket_accept_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_bind_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_connect_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_dump_heap_usage", "", 0, steam_api_schrodinger},
    {"sb_socket_listen_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_receive", "", 0, steam_api_schrodinger},
    {"sb_socket_receive_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_receive_from", "", 0, steam_api_schrodinger},
    {"sb_socket_select", "", 0, steam_api_schrodinger},
    {"sb_socket_send", "", 0, steam_api_schrodinger},
    {"sb_socket_send_error_str", "", 0, steam_api_schrodinger},
    {"sb_socket_send_to", "", 0, steam_api_schrodinger},
    {"sb_socket_set_linger", "", 0, steam_api_schrodinger},
    {"sb_socket_set_tcp_no_delay", "", 0, steam_api_schrodinger},

    // sb_thread.h
    {"sb_condition_wake_all", "", 0, steam_api_schrodinger},
    {"sb_condition_wake_one", "", 0, steam_api_schrodinger},
    {"sb_create_condition_variable", "", 0, steam_api_schrodinger},
    {"sb_create_mutex", "", 0, steam_api_schrodinger},
    {"sb_create_thread", "", 0, steam_api_schrodinger},
    {"sb_destroy_condition_variable", "", 0, steam_api_schrodinger},
    {"sb_destroy_mutex", "", 0, steam_api_schrodinger},
    {"sb_get_current_thread_id", "", 0, steam_api_schrodinger},
    {"sb_join_thread", "", 0, steam_api_schrodinger},
    {"sb_lock_mutex", "", 0, steam_api_schrodinger},
    {"sb_set_thread_priority", "", 0, steam_api_schrodinger},
    {"sb_thread_sleep", "", 0, steam_api_schrodinger},
    {"sb_thread_yield", "", 0, steam_api_schrodinger},
    {"sb_unlock_mutex", "", 0, steam_api_schrodinger},
    {"sb_wait_condition", "", 0, steam_api_schrodinger},

    // runtime.h
    {"sb_seconds_since_epoch_to_localtime", "", 0, steam_api_schrodinger},
    {"sb_unformatted_debug_write", "", 0, steam_api_schrodinger},
    {"sb_unformatted_debug_write_line", "", 0, steam_api_schrodinger},
    {"sb_vadebug_write", "", 0, steam_api_schrodinger},
    {"sb_vadebug_write_line", "", 0, steam_api_schrodinger}};

int g_sb_impl_table_count = sizeof(g_sb_impl_table) / sizeof(g_sb_impl_table[0]);

int index_of(const char * fname) {
    for (int index = 0; index < g_sb_impl_table_count; index++) {
        if (strcmp(g_sb_impl_table[index].name, fname) == 0) {
            return index;
        }
    }

    printf("%sUnexpected steamboat api \"%s\" not found in the table!%s\n", COLOR_RED, fname, COLOR_OFF);
    assert(0);

    return -1;
}

void dump_table(void) {
    int impl_count = 0;
    int unimpl_count = 0;
    int unknown_count = 0;

    for (int index = 0; index < g_sb_impl_table_count; index++) {
        drydock_api_entry_t * entry = &g_sb_impl_table[index];
        char * avail = NULL;
        char * highlight = COLOR_PURPLE;
        switch (entry->api_avail) {
            case steam_api_schrodinger:
                avail = "steam_api_schrodinger";
                highlight = COLOR_PURPLE;
                ++unknown_count;
                break;

            case steam_api_implemented:
                avail = "steam_api_implemented";
                highlight = COLOR_GREEN;
                ++impl_count;
                break;

            case steam_api_not_implemented:
                avail = "steam_api_not_implemented";
                highlight = COLOR_RED;
                ++unimpl_count;
                break;

            default:
                avail = "(unexpected)";
                ++unknown_count;
                break;
        };

        // clang-format off
        printf("Steamboat api [%s] status [%s%s%s] file [%s] line [%i]\n",
            entry->name, highlight, avail, COLOR_OFF, entry->file_name, entry->line_num);
        // clang-format on
    }

    // clang-format off
    printf(
        "\nSummary:\n"
        " %d of %d functions %simplemented%s\n"
        " %d of %d functions %snot implemented%s yet\n"
        " %d of %d functions in an %sindeterminate%s state\n",
        impl_count, g_sb_impl_table_count, COLOR_GREEN, COLOR_OFF,
        unimpl_count, g_sb_impl_table_count, COLOR_RED, COLOR_OFF,
        unknown_count, g_sb_impl_table_count, COLOR_PURPLE, COLOR_OFF);
    // clang-format on
}

#endif /* _DRYDOCK */
