/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_http_utils.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_thread.h"

#include <math.h>

static struct {
    size_t bytes_downloaded;
    int num_queued;
    int num_complete;
    int num_ticks;
    microseconds_t start_time;
    microseconds_t first_byte_time;
    microseconds_t end_time;
} statics;

typedef struct handle_info_t {
    adk_curl_handle_t * handle;
    size_t content_length;
    size_t bytes_downloaded;
} handle_info_t;

static bool on_http_header_recv(adk_curl_handle_t * const handle, const const_mem_region_t data, const adk_curl_callbacks_t * const callbacks) {
    handle_info_t * const handle_info = (handle_info_t *)callbacks->user[0];

    const const_mem_region_t value = http_parse_header_for_key("Content-Length", data);
    if (value.ptr != NULL) {
        char content_length_value_buffer[128] = {0};
        memcpy(content_length_value_buffer, value.ptr, value.size);
        handle_info->content_length = (size_t)strtol(content_length_value_buffer, NULL, 10);
    }

    return true;
}

static bool on_http_recv(adk_curl_handle_t * const handle, const const_mem_region_t data, const adk_curl_callbacks_t * const callbacks) {
    handle_info_t * const handle_info = (handle_info_t *)callbacks->user[0];
    // Report TTFB of first connection, a standard measure of server/network responsiveness
    if (statics.first_byte_time.us == 0) {
        statics.first_byte_time = adk_read_microsecond_clock();
        const double ttfb_ms = (statics.first_byte_time.us - statics.start_time.us) / 1000.0;
        debug_write_line("time to first byte = [%.2f] ms", ttfb_ms);
    }
    handle_info->bytes_downloaded += data.size;
    statics.bytes_downloaded += data.size;
    return true;
}

static void on_http_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const struct adk_curl_callbacks_t * const callbacks) {
    handle_info_t * const handle_info = (handle_info_t *)callbacks->user[0];

    VERIFY(handle_info->content_length > 0);
    if (handle_info->bytes_downloaded != handle_info->content_length) {
        debug_write_line("handle [%p], error, downloaded [%zd] bytes, Content-Length: %zd", handle_info->content_length);
    }

    ++statics.num_complete;
    adk_curl_close_handle(handle);
    handle_info->handle = NULL;
}

static int get_arg_value_or_default(const int argc, const char * const * const argv, const char * const name, const int default_value) {
    const char * const arg = getargarg(name, argc, argv);
    if (arg != NULL) {
        return atoi(arg);
    }
    return default_value;
}

#define DEFAULT_URL "https://releases.ubuntu.com/20.04.2.0/ubuntu-20.04.2.0-desktop-amd64.iso?_ga=2.148594423.1533927758.1615608737-846390805.1615608737"

int cmdlet_http_test(const int argc, const char * const * const argv) {
    if (findarg("--help", argc, argv) != -1) {
        debug_write_line("http_test - perform http bandwidth test:\n arguments:\n");
        debug_write_line("--repeat [num]: Repeats the download test 'num' times, and reports the average. Defaults to 5.");
        debug_write_line("--num_downloads [num]: Number of simultaneous http request to make. Defaults to 1.");
        debug_write_line("--tick_rate_hz [num]: Simulate the specified frame-rate. Defaults to 0 which means run as fast as possible.");
        debug_write_line("--buffer_size [num]: Receive buffer size in KiB.  Default depends on platform.");
        debug_write_line("--url [string]: Use the specified url. Defaults to \"" DEFAULT_URL "\"");
        return 0;
    }

    const int repeat_count = get_arg_value_or_default(argc, argv, "--repeat", 5);
    const int num_downloads = get_arg_value_or_default(argc, argv, "--num_downloads", 1);
    VERIFY_MSG(num_downloads > 0, "At least one download is required");
    const int tick_rate = get_arg_value_or_default(argc, argv, "--tick_rate_hz", 0);
    const int buffer_size_kib = get_arg_value_or_default(argc, argv, "--buffer_size", 0);
    const char * const url_arg = getargarg("--url", argc, argv);

    const char * const url = url_arg ? url_arg : DEFAULT_URL;

    const adk_memory_reservations_t default_reservations = adk_get_default_memory_reservations();
    const adk_api_t * const api = adk_init(argc, argv, system_guard_page_mode_enabled, default_reservations, MALLOC_TAG);

    VERIFY(adk_mbedtls_init(api->mmap.ssl.region, system_guard_page_mode_enabled, MALLOC_TAG));
    VERIFY(adk_curl_api_init(api->mmap.curl.region, api->mmap.httpx_fragment_buffers.region, 4096, 1, system_guard_page_mode_enabled, adk_curl_http_init_normal));

    handle_info_t * const handle_info = malloc(sizeof(handle_info_t) * num_downloads);
    TRAP_OUT_OF_MEMORY(handle_info);

    const uint64_t sleep_rate_us = (tick_rate > 0) ? 1000000ull / (uint64_t)tick_rate : 0;

    size_t total_bytes = 0;
    uint64_t total_time_us = 0;
    size_t max_bytes_per_tick = 0;

    for (int test_repeat = 0; test_repeat < repeat_count; ++test_repeat) {
        ZEROMEM(&statics);
        memset(handle_info, 0, sizeof(*handle_info) * num_downloads);

        for (int i = 0; i < num_downloads; ++i) {
            const adk_curl_callbacks_t callbacks = {
                .on_http_header_recv = on_http_header_recv,
                .on_http_recv = on_http_recv,
                .on_complete = on_http_complete,
                .user = {&handle_info[i], &handle_info[i], &handle_info[i]}};

            adk_curl_handle_t * const handle = adk_curl_open_handle();
            handle_info[i].handle = handle;

            adk_curl_set_opt_ptr(handle, adk_curl_opt_url, url);
            adk_curl_set_opt_ptr(handle, adk_curl_opt_user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.183 Safari/537.36");
            adk_curl_set_opt_long(handle, adk_curl_opt_verbose, 0);
            adk_curl_set_opt_long(handle, adk_curl_opt_follow_location, 1);
            if (buffer_size_kib) {
                adk_curl_set_opt_long(handle, adk_curl_opt_buffer_size, buffer_size_kib * 1024);
            }
            adk_curl_set_buffering_mode(handle, adk_curl_handle_buffer_off);
            adk_curl_async_perform(handle, callbacks);
        }

        if (test_repeat == 0) {
            debug_write_line("downloading [%d] file(s) at a [%dhz] tick rate...", num_downloads, tick_rate);
        }

        statics.start_time = adk_read_microsecond_clock();

        static const uint64_t sample_freq = 2 * 1000 * 1000;
        size_t sampled_bytes = 0;
        microseconds_t prev_sample_time = statics.start_time;
        microseconds_t next_sample_micros = (microseconds_t){statics.start_time.us + sample_freq};

        while (statics.num_complete < num_downloads) {
            const uint64_t end_us = adk_read_microsecond_clock().us + sleep_rate_us;
            size_t const bytes_downloaded = statics.bytes_downloaded;
            ++statics.num_ticks;
            adk_curl_run_callbacks();
            while (adk_read_microsecond_clock().us < end_us) {
                sb_thread_sleep((milliseconds_t){1});
            }
            size_t const bytes_per_tick = statics.bytes_downloaded - bytes_downloaded;
            if (bytes_per_tick > max_bytes_per_tick) {
                max_bytes_per_tick = bytes_per_tick;
            }

            sampled_bytes += bytes_per_tick;
            const microseconds_t sample_time = adk_read_microsecond_clock();
            if (sample_time.us >= next_sample_micros.us) {
                const uint64_t elapsed_us = sample_time.us - prev_sample_time.us;
                prev_sample_time = sample_time;
                const double elapsed_seconds = ((double)elapsed_us / 1000000.0);
                const double bytes_per_second = (double)sampled_bytes / elapsed_seconds;

                debug_write_line(
                    "downloaded [%d] bytes in [%.2f] seconds, download rate = [%.2f] KBps or [%.2f] kbps",
                    (int)sampled_bytes,
                    elapsed_seconds,
                    bytes_per_second / 1000.0,
                    bytes_per_second * 8.0 / 1000.0);

                next_sample_micros.us = sample_time.us + sample_freq;
                sampled_bytes = 0;
            }
        }

        statics.end_time = adk_read_microsecond_clock();

        const uint64_t elapsed_us = statics.end_time.us - statics.start_time.us;

        total_bytes += statics.bytes_downloaded;
        total_time_us += elapsed_us;

        const double elapsed_seconds = ((double)elapsed_us / 1000000.0);
        const double bytes_per_second = (double)statics.bytes_downloaded / elapsed_seconds;
        if (bytes_per_second > 1000) {
            debug_write_line(
                "complete [run %d][tick rate %dhz](%.2fhz measured): downloaded [%d] bytes in [%.2f] seconds, download rate = [%.2f] KBps or [%.2f] kbps, [%.0f] avg bytes/tick",
                test_repeat + 1,
                tick_rate,
                statics.num_ticks / elapsed_seconds,
                (int)statics.bytes_downloaded,
                elapsed_seconds,
                bytes_per_second / 1000.0,
                bytes_per_second * 8.0 / 1000.0,
                ceil((double)statics.bytes_downloaded / statics.num_ticks));
        } else {
            debug_write_line(
                "complete [run %d][tick rate %dhz](%.2fhz measured): downloaded [%d] bytes in [%.2f] seconds, download rate = [%.2f] Bps or [%.2f] bps, [%.0f] avg bytes/tick",
                test_repeat + 1,
                tick_rate,
                statics.num_ticks / elapsed_seconds,
                (int)statics.bytes_downloaded,
                elapsed_seconds,
                bytes_per_second,
                bytes_per_second * 8,
                ceil((double)statics.bytes_downloaded / statics.num_ticks));
        }
    }

    {
        const double elapsed_seconds = ((double)total_time_us / 1000000.0);
        const double bytes_per_second = (double)total_bytes / elapsed_seconds;
        // Note that network speeds are stated in decimal units (e.g. KBps) rather than binary units (KiBps)
        if (bytes_per_second > 1000) {
            debug_write_line("avg [%.2f] KBps or [%.2f] kbps", bytes_per_second / 1000.0, bytes_per_second * 8.0 / 1000.0);
        } else {
            debug_write_line("avg [%.2f] Bps or [%.2f] bps", bytes_per_second, bytes_per_second * 8);
        }
        debug_write_line("max [%zd] bytes/tick", bytes_per_second * 8.0 / 1000.0, max_bytes_per_tick);
        if (buffer_size_kib && ((int)max_bytes_per_tick == num_downloads * buffer_size_kib * 1024)) {
            debug_write_line("note: download speed possibly limited by receive buffer size [%zd] bytes per connection", buffer_size_kib * 1024);
        }
    }

    free(handle_info);

    adk_curl_api_shutdown();
    adk_mbedtls_shutdown(MALLOC_TAG);

    return 0;
}
