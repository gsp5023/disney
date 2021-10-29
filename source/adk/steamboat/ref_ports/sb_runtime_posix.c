/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_runtime_posix.c

steamboat for posix
*/

#include _PCH

#include "source/adk/log/log.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_locale.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"
#include "source/adk/telemetry/telemetry.h"

#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// just used for getting the device ID, use sb_socket_t functions instead of bsd sockets directly.
#include <sys/socket.h>
// reorder barrier (needs to before linux/if.h)?
#ifndef _VADER
#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#endif
#include <stdio.h>

#define TAG_RT_IX FOURCC('R', 'T', 'I', 'X')

#if defined(NEXUS_PLATFORM) && (NEXUS_PLATFORM_VERSION_MAJOR >= 19)
#define HAS_PTHREAD_SET_NAME
#endif

enum {
    max_path = 260
};

/* extern */ int sys_page_size;
/* extern */ sb_thread_id_t sb_main_thread_id;
/* extern */ const char * sb_persona_id;

void __init_adk_events();
void __shutdown_adk_events();

bool __sb_posix_init_platform(adk_api_t * api, const int argc, const char * const * const argv);
void __sb_posix_shutdown_platform();

typedef enum network_connection_status_e {
    network_connection_status_disconnected,
    network_connection_status_connected,
    network_connection_status_unknown
} network_connection_status_e;

static struct {
    bool init;
    pthread_mutex_t mutex;
    heap_t heap;
    crypto_hmac_hex_t device_id;
    network_connection_status_e network_status;
} statics;

void * sb_runtime_heap_alloc(const size_t size, const char * const tag) {
    ASSERT(statics.init);
    pthread_mutex_lock(&statics.mutex);
    void * const p = heap_alloc(&statics.heap, size, tag);
    pthread_mutex_unlock(&statics.mutex);
    return p;
}

void sb_runtime_heap_free(void * const p, const char * const tag) {
    ASSERT(statics.init);
    pthread_mutex_lock(&statics.mutex);
    heap_free(&statics.heap, p, tag);
    pthread_mutex_unlock(&statics.mutex);
}

/*
===============================================================================
System page memory allocation
===============================================================================
*/

static int get_va_protect_flags(const system_page_protect_e protect) {
    switch (protect) {
        case system_page_protect_no_access:
            return PROT_NONE;
        case system_page_protect_read_only:
            return PROT_READ;
        default:
            ASSERT(protect == system_page_protect_read_write);
            return PROT_READ | PROT_WRITE;
    }
}

mem_region_t sb_map_pages(const size_t size, const system_page_protect_e protect) {
    ASSERT_PAGE_ALIGNED(size);
    void * const ptr = mmap(NULL, size, get_va_protect_flags(protect), MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (ptr != (void *)-1) {
        return (mem_region_t){
            {{{.ptr = ptr},
              .size = size}}};
    }
    return (mem_region_t){0};
}

static bool is_wireless(const char * const ifname) {
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return false;
    }

    struct iwreq pwrq = {0};
    strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);
    const bool has_wireless = ioctl(sock, SIOCGIWNAME, &pwrq) != -1;
    close(sock);
    return has_wireless;
}

/*
  Assigns currently used network iface name to `out_iface_name` and returns its length.
  Returns `-1`: in case of an error
           `0`: if there's no default interface informatin found in the routes file
           iface_name_length: on success
 */
static int32_t resolve_default_network_iface_name(char out_iface_name[]) {
    FILE * const route_file_handle = fopen("/proc/net/route", "r");
    if (!route_file_handle) {
        LOG_WARN(TAG_RT_IX, "Failed to read /proc/net/route file, this file is needed to get network information");
        return -1;
    }

    enum { max_line_length = 128 };
    char line[max_line_length] = {0};

    // Ignore first line with table headers
    if (!fgets(line, max_line_length, route_file_handle)) {
        LOG_WARN(TAG_RT_IX, "Failed to read first line from /proc/net/route file, this file is needed to get network information");
        fclose(route_file_handle);
        return -1;
    }

    ASSERT_MSG(strncmp("Iface\tDestination", line, 17) == 0, "/proc/net/route has unexpected format");

    while (fgets(line, max_line_length, route_file_handle) != NULL) {
        if (line[0] == '\n') {
            continue;
        }

        /* We are interested in first two columns from the /proc/net/route file: Iface and Destination.
         * If the table contains a default routing rule we'll grab the first one, which seem to be the one
         * that Linux would use to route the request and parse the interface name assigned to it.
         *
         * On both Desktop and RPi's that were checked, plugging / unplugging ethernet cable updates the routing
         * table respectively.
         */

        const int32_t iface_length = strchr(line, '\t') - line;

        const char * destination_column = line + iface_length + 1; // 1 for space
        const size_t destination_length = strchr(destination_column, '\t') - destination_column;

        if (!strncmp("00000000", destination_column, destination_length)) {
            strncpy(out_iface_name, line, iface_length);
            fclose(route_file_handle);
            return iface_length;
        }
    }

    fclose(route_file_handle);

    return 0;
}

sb_network_type_e sb_get_network_type() {
    char iface_name[64] = {0};
    if (resolve_default_network_iface_name(iface_name) <= 0) {
        LOG_ERROR(TAG_RT_IX, "Can't detected network's type due: couldn't resolve default network iface name");
        return sb_network_type_unknown;
    }

    sb_network_type_e type = is_wireless(iface_name) ? sb_network_type_wireless : sb_network_type_ethernet;
    return type;
}

static network_connection_status_e get_network_connection_status() {
    char iface_name[64] = {0};
    if (resolve_default_network_iface_name(iface_name) <= 0) {
        LOG_ERROR(TAG_RT_IX, "Can't process network status: couldn't resolve default network iface name");
        return network_connection_status_unknown;
    }

    char carrier_path[sb_max_path_length] = {0};
    sprintf_s(carrier_path, sb_max_path_length, "/sys/class/net/%s/carrier", iface_name);

    FILE * const carrier_file_handle = fopen(carrier_path, "r");
    if (!carrier_file_handle) {
        LOG_WARN(TAG_RT_IX, "Failed to read the %s file, this file is used to check the default network connection status", carrier_path);
        return network_connection_status_unknown;
    }

    char link_status_line[8] = {0}; // This would be a single number 0 or 1
    if (!fgets(link_status_line, 8, carrier_file_handle)) {
        LOG_WARN(TAG_RT_IX, "Failed to read the %s file content (first line), it's used to check the default network connection status", carrier_path);
        fclose(carrier_file_handle);
        return network_connection_status_unknown;
    }

    fclose(carrier_file_handle);

    switch (link_status_line[0]) {
        case '0': {
            return network_connection_status_disconnected;
        }
        case '1': {
            return network_connection_status_connected;
        }
        default: {
            return network_connection_status_unknown;
        }
    }
}

void process_network_status() {
    const milliseconds_t time = adk_read_millisecond_clock();

    const network_connection_status_e current_network_status = get_network_connection_status();
    switch (current_network_status) {
        case network_connection_status_disconnected: {
            if (statics.network_status == network_connection_status_connected) {
                statics.network_status = network_connection_status_disconnected;

                adk_post_event((adk_event_t){
                    .time = time,
                    .event_data = (adk_event_data_t){
                        .type = adk_application_event,
                        {.app = (adk_application_event_t){
                             .event = adk_application_event_link_down}}}});
            }
            break;
        }
        case network_connection_status_connected: {
            if (statics.network_status == network_connection_status_disconnected) {
                statics.network_status = network_connection_status_connected;

                adk_post_event((adk_event_t){
                    .time = time,
                    .event_data = (adk_event_data_t){
                        .type = adk_application_event,
                        {.app = (adk_application_event_t){
                             .event = adk_application_event_link_up}}}});
            }
            break;
        }
        case network_connection_status_unknown: {
            LOG_ERROR(TAG_RT_IX, "Failed to get an updated network status");
            break;
        }
    }
}

void sb_protect_pages(const mem_region_t pages, const system_page_protect_e protect) {
    ASSERT_PAGE_ALIGNED(pages.ptr);
    ASSERT_PAGE_ALIGNED(pages.size);

    VERIFY(mprotect(pages.ptr, pages.size, get_va_protect_flags(protect)) == 0);
}

void sb_unmap_pages(const mem_region_t pages) {
    ASSERT_PAGE_ALIGNED(pages.ptr);
    ASSERT_PAGE_ALIGNED(pages.size);

    munmap(pages.ptr, pages.size);
}

/*
===============================================================================
Application
===============================================================================
*/

static crypto_hmac_hex_t get_device_id_hash() {
    enum { mac_address_length = 6 };
    char mac_address[mac_address_length] = {0};

#ifdef _MTV
    FILE * const fp = popen("/data/busybox/busybox ip route", "r");
#else
    FILE * const fp = popen("ip route", "r");
#endif //_MTV
    VERIFY_MSG(fp, "Failed to open command to read default network interface");
    char buff[256] = {0};
    char * adapter_name = NULL;
    while (fgets(buff, ARRAY_SIZE(buff), fp)) {
        if (strstr(buff, "default")) {
            adapter_name = strstr(buff, "dev") + 4;
            *strstr(adapter_name, " ") = 0;
            break;
        }
    }
    VERIFY(adapter_name);
    pclose(fp);

    struct ifreq s;
    const int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy_s(s.ifr_name, ARRAY_SIZE(s.ifr_name), adapter_name);
    ASSERT(ARRAY_SIZE(s.ifr_hwaddr.sa_data) > mac_address_length);
    if (ioctl(sock, SIOCGIFHWADDR, &s) == 0) {
        memcpy(mac_address, s.ifr_hwaddr.sa_data, mac_address_length);
    }

    if (sock != -1) {
        close(sock);
    }

    return crypto_compute_hmac_hex(CONST_MEM_REGION(mac_address, ARRAY_SIZE(mac_address)));
}

bool sb_preinit(const int argc, const char * const * const argv) {
    ZEROMEM(&statics);
    ASSERT(sb_main_thread_id == 0);

    pthread_mutex_init(&statics.mutex, NULL);

    sb_main_thread_id = sb_get_current_thread_id();
    sys_page_size = getpagesize();

    char cwd[max_path + 1];
    VERIFY(getcwd(cwd, ARRAY_SIZE(cwd)));

    char directory_buff[sb_max_path_length];
    adk_set_directory_path(sb_app_root_directory, cwd);

#ifdef _DEV
#define APP_SUB_DIRECTORY "build/"
#else
#define APP_SUB_DIRECTORY ""
#endif

    sprintf_s(directory_buff, ARRAY_SIZE(directory_buff), "%s/" APP_SUB_DIRECTORY "config", cwd);
    adk_set_directory_path(sb_app_config_directory, directory_buff);

    sprintf_s(directory_buff, ARRAY_SIZE(directory_buff), "%s/" APP_SUB_DIRECTORY "cache", cwd);
    adk_set_directory_path(sb_app_cache_directory, directory_buff);

    sb_create_directory_path(sb_app_config_directory, "");
    sb_create_directory_path(sb_app_cache_directory, "");

    sb_persona_id = getargarg("--persona", argc, argv);

    statics.device_id = get_device_id_hash();

    return true;
}

bool sb_init(adk_api_t * api, const int argc, const char * const * const argv, const system_guard_page_mode_e guard_page_mode) {
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&statics.heap, api->mmap.runtime.region.size, 8, 0, "sb_runtime_posix_heap", guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&statics.heap, api->mmap.runtime.region, 0, 0, "sb_runtime_posix_heap");
    }

    statics.init = true;
    __init_adk_events();

    if (!__sb_posix_init_platform(api, argc, argv)) {
        return false;
    }

    extern struct rhi_api_t rhi_api_gl;
    api->rhi = &rhi_api_gl;

    sb_deeplink_handle_t * const deeplink_arg = (sb_deeplink_handle_t *)getargarg("--deeplink", argc, argv);
    if (deeplink_arg) {
        adk_event_t deeplink_event = {
            .time = adk_read_millisecond_clock(),
            .event_data = (adk_event_data_t){
                .type = adk_deeplink_event,
                {.deeplink = {
                     .handle = deeplink_arg}}}};
        adk_post_event_async(deeplink_event);
    }

    return true;
}

void sb_shutdown() {
    __sb_posix_shutdown_platform();
    __shutdown_adk_events();

#ifndef _SHIP
    heap_debug_print_leaks(&statics.heap);
#endif
    heap_destroy(&statics.heap, MALLOC_TAG);
    pthread_mutex_destroy(&statics.mutex);
    statics.init = false;
    sb_main_thread_id = 0;
}

void sb_halt(const char * const message) {
    LOG_ALWAYS(TAG_RT_IX, "sb_halt: %s", message);
    while (true) {
        sb_thread_sleep((milliseconds_t){UINT32_MAX});
    }
    // never return
}

void sb_platform_dump_heap_usage() {
    pthread_mutex_lock(&statics.mutex);
    heap_dump_usage(&statics.heap);
    pthread_mutex_unlock(&statics.mutex);
}

heap_metrics_t sb_platform_get_heap_metrics() {
    pthread_mutex_lock(&statics.mutex);
    const heap_metrics_t metrics = heap_get_metrics(&statics.heap);
    pthread_mutex_unlock(&statics.mutex);
    return metrics;
}

void __sb_posix_get_device_id(adk_system_metrics_t * const out) {
    out->device_id = statics.device_id;
}

void sb_on_app_load_failure() {
}

typedef struct stat_info_t {
    float uptime_seconds;
    unsigned long program_ticks_1;
    unsigned long program_ticks_2;
    long child_ticks_1;
    long child_ticks_2;
    unsigned long long start_ticks;
    unsigned long vm_size;
} stat_info_t;

static bool read_stat_info(stat_info_t * info) {
    if (access("/proc/uptime", R_OK) == 0 && access("/proc/self/stat", R_OK) == 0) {
        FILE * fp;
        fp = fopen("/proc/uptime", "r");
        int ret = fscanf(fp, "%f", &info->uptime_seconds);
        fclose(fp);
        if (ret == EOF || ret < 1) {
            return false;
        }

        fp = fopen("/proc/self/stat", "r");
        ret = fscanf(fp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu %ld %ld %*s %*s %*s %*s %llu %lu", &info->program_ticks_1, &info->program_ticks_2, &info->child_ticks_1, &info->child_ticks_2, &info->start_ticks, &info->vm_size);
        fclose(fp);
        if (ret == EOF || ret < 5) {
            return false;
        }

        return true;
    }

    return false;
}

static float get_cpu_utilization(const stat_info_t info) {
    uint64_t program_cpu_ticks = (info.child_ticks_1 + info.child_ticks_2 + info.program_ticks_1 + info.program_ticks_2);
    float program_uptime_ticks = (info.uptime_seconds * sysconf(_SC_CLK_TCK)) - info.start_ticks;
    return program_cpu_ticks / program_uptime_ticks;
}

static uint64_t get_memory_used() {
    struct rusage rusage;
    if (!getrusage(RUSAGE_SELF, &rusage)) {
        return rusage.ru_maxrss * 1024; // maximum resident set size (kb)
    }

    return 0;
}

static uint64_t get_memory_available(const stat_info_t info) {
    return info.vm_size;
}

sb_cpu_mem_status_t sb_get_cpu_mem_status() {
    stat_info_t info = {0};
    if (!read_stat_info(&info)) {
        return (sb_cpu_mem_status_t){0};
    }

    return (sb_cpu_mem_status_t){
        .cpu_utilization = get_cpu_utilization(info),
        .memory_available = get_memory_available(info),
        .memory_used = get_memory_used()};

    //TODO: figure out nve memory usage for brcm
}

/*
===============================================================================
System clocks
===============================================================================
*/

nanoseconds_t sb_read_nanosecond_clock() {
    struct timespec spec;
    VERIFY(clock_gettime(CLOCK_MONOTONIC, &spec) == 0);
    enum { sec_to_ns = 1000 * 1000 * 1000 };
    return (nanoseconds_t){((uint64_t)spec.tv_sec * (uint64_t)sec_to_ns) + spec.tv_nsec};
}

sb_time_since_epoch_t sb_get_time_since_epoch() {
    struct timespec spec;
    VERIFY(clock_gettime(CLOCK_REALTIME, &spec) == 0);
    const sb_time_since_epoch_t time_since_epoch = {.seconds = (uint32_t)spec.tv_sec, .microseconds = (uint32_t)spec.tv_nsec / 1000};
    return time_since_epoch;
}

void sb_seconds_since_epoch_to_localtime(const time_t seconds, struct tm * const _tm) {
    localtime_r(&seconds, _tm);
}

/*
===============================================================================
Mutex
===============================================================================
*/

sb_mutex_t * sb_create_mutex(const char * const tag) {
    pthread_mutex_t * const mutex = sb_runtime_heap_alloc(sizeof(pthread_mutex_t), tag);
    pthread_mutex_init(mutex, NULL);
    return (sb_mutex_t *)mutex;
}

void sb_destroy_mutex(sb_mutex_t * const mutex, const char * const tag) {
    pthread_mutex_destroy((pthread_mutex_t *)mutex);
    sb_runtime_heap_free(mutex, tag);
}

void sb_lock_mutex(sb_mutex_t * const mutex) {
    pthread_mutex_lock((pthread_mutex_t *)mutex);
}

void sb_unlock_mutex(sb_mutex_t * const mutex) {
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

/*
===============================================================================
Condition Variable
===============================================================================
*/

sb_condition_variable_t * sb_create_condition_variable(const char * const tag) {
    pthread_cond_t * const cnd = sb_runtime_heap_alloc(sizeof(pthread_cond_t), tag);
    pthread_condattr_t attr = {0};
    pthread_condattr_init(&attr);

    // https://man7.org/linux/man-pages/man3/pthread_cond_wait.3p.html
    // The condition variable shall have a clock attribute which
    // specifies the clock that shall be used to measure the time
    // specified by the abstime argument.
    const int err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    ASSERT_MSG(!err, "Error setting clock: %d\n", err);

    pthread_cond_init(cnd, &attr);
    pthread_condattr_destroy(&attr);

    return (sb_condition_variable_t *)cnd;
}

void sb_destroy_condition_variable(sb_condition_variable_t * cnd, const char * const tag) {
    sb_runtime_heap_free(cnd, tag);
}

void sb_condition_wake_one(sb_condition_variable_t * cnd) {
    pthread_cond_signal((pthread_cond_t *)cnd);
}

void sb_condition_wake_all(sb_condition_variable_t * cnd) {
    pthread_cond_broadcast((pthread_cond_t *)cnd);
}

bool sb_wait_condition(sb_condition_variable_t * cnd, sb_mutex_t * mutex, const milliseconds_t timeout) {
    if (timeout.ms == sb_timeout_infinite.ms) {
        return (0 == pthread_cond_wait((pthread_cond_t *)cnd, (pthread_mutex_t *)mutex));
    } else {
        struct timespec ts = {0};
        const lldiv_t div_result = lldiv(timeout.ms, 1000);
        VERIFY(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
        ts.tv_sec += (uint32_t)div_result.quot;
        ts.tv_nsec += 1000000 * (uint32_t)div_result.rem;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
        return (0 == pthread_cond_timedwait((pthread_cond_t *)cnd, (pthread_mutex_t *)mutex, &ts));
    }
}

/*
===============================================================================
Threads
===============================================================================
*/

sb_thread_id_t sb_get_current_thread_id() {
    return (sb_thread_id_t)pthread_self();
}

typedef struct pthread_args {
    int (*func_ptr)(void *);
    void * arg;
    char name[16];
} pthread_args_t;

static void * pthread_proc(void * args) {
    const pthread_args_t thread_args = *(pthread_args_t *)(args);
    TRACE_NAME_THREAD(thread_args.name);
    sb_runtime_heap_free(args, MALLOC_TAG);
#ifndef HAS_PTHREAD_SET_NAME
    prctl(PR_SET_NAME, thread_args.name);
#endif
    return (void *)(uintptr_t)thread_args.func_ptr(thread_args.arg);
}

static int thread_priority_to_sched(const sb_thread_priority_e priority) {
    switch (priority) {
        case sb_thread_priority_low:
            return SCHED_OTHER;
        case sb_thread_priority_normal:
            return SCHED_OTHER;
        case sb_thread_priority_high:
            return SCHED_RR;
        default:
            ASSERT(priority == sb_thread_priority_realtime);
            return SCHED_FIFO;
    }
}

sb_thread_id_t sb_create_thread(const char * name, const sb_thread_options_t options, int (*const thread_proc)(void * arg), void * const arg, const char * const tag) {
    ASSERT(name);
    ASSERT(strlen(name) < 16); // http://man7.org/linux/man-pages/man2/prctl.2.html PR_SET_NAME only takes 16 chars including null
    ASSERT(thread_proc);

    pthread_t thread = 0;
    pthread_args_t * ptr = sb_runtime_heap_alloc(sizeof(pthread_args_t), MALLOC_TAG);
    *ptr = (pthread_args_t){.func_ptr = thread_proc, .arg = arg};
    strcpy_s(ptr->name, sizeof(ptr->name), name);

    pthread_attr_t * attr_ptr = NULL;
    pthread_attr_t attr;
    if ((options.priority != sb_thread_priority_normal) || (options.stack_size != 0) || (options.detached == true)) {
        pthread_attr_init(&attr);
        if (options.stack_size > 0) {
            pthread_attr_setstacksize(&attr, options.stack_size);
        }
        if (options.priority != sb_thread_priority_normal) {
            struct sched_param param;
            pthread_attr_getschedparam(&attr, &param);
            param.sched_priority = sched_get_priority_max(thread_priority_to_sched(options.priority));
            pthread_attr_setschedparam(&attr, &param);
        }
        if (options.detached == true) {
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        }
        attr_ptr = &attr;
    }

    const int create_thread_err = pthread_create(&thread, attr_ptr, pthread_proc, ptr);
    VERIFY(create_thread_err == 0);

    if (attr_ptr != NULL) {
        pthread_attr_destroy(attr_ptr);
    }

#ifdef HAS_PTHREAD_SET_NAME
    pthread_setname_np(thread, name);
#endif

    return (sb_thread_id_t)thread;
}

bool sb_set_thread_priority(const sb_thread_id_t id, const sb_thread_priority_e priority) {
    struct sched_param param = {0};
    if (priority == sb_thread_priority_high || priority == sb_thread_priority_realtime) {
        param.sched_priority = sched_get_priority_max(thread_priority_to_sched(priority));
    }

    return pthread_setschedparam((pthread_t)id, thread_priority_to_sched(priority), &param) == 0;
}

void sb_join_thread(const sb_thread_id_t id) {
    pthread_join((pthread_t)id, NULL);
}

void sb_thread_sleep(const milliseconds_t time) {
    const time_t seconds = time.ms / 1000;
    const long nanos = (time.ms - (seconds * 1000)) * 1000 * 1000;

    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = nanos;

    // Older systems do not support clock_nanosleep(), however nanosleep() should still behave
    // as expected, since it should not be affected by any clock changes:
    // https://man7.org/linux/man-pages/man2/nanosleep.2.html#NOTES
    nanosleep(&ts, NULL);
}

void sb_unformatted_debug_write(const char * const msg) {
    ASSERT(msg);
    fputs(msg, stderr);
}

void sb_unformatted_debug_write_line(const char * const msg) {
    ASSERT(msg);
    fputs(msg, stderr);
    fputs("\n", stderr);
}

void sb_vadebug_write(const char * const msg, va_list args) {
    vfprintf(stderr, msg, args);
}

void sb_vadebug_write_line(const char * const msg, va_list args) {
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
}

/*
=======================================
Locale
=======================================
*/

enum {
    locale_name_max_length_cstr = 256
};

sb_locale_t sb_get_locale() {
    const char * curr_locale = setlocale(LC_ALL, NULL);
    VERIFY_MSG(curr_locale, "setlocale returned NULL");
    size_t str_len = strlen(curr_locale);

    // POSIX, and C as a locale are very nearly the same as en_US.utf8 -- they differ slightly, but only just.
    if (str_len <= 1 || strcmp(curr_locale, "C") == 0 || strcmp(curr_locale, "POSIX") == 0) {
        sb_locale_t locale = {0};
        VERIFY(strcpy_s(locale.language, ARRAY_SIZE(locale.language), "en") == 0);
        VERIFY(strcpy_s(locale.region, ARRAY_SIZE(locale.region), "US") == 0);
        return locale;
    }

    char locale_buff[locale_name_max_length_cstr];

    VERIFY(strcpy_s(locale_buff, ARRAY_SIZE(locale_buff), curr_locale) == 0);

    const char * language_str = &locale_buff[0];
    const char * region_str = NULL;

    for (size_t i = 0; i < str_len; ++i) {
        if (locale_buff[i] == '_' || locale_buff[i] == '.') {
            locale_buff[i] = '\0';
            if (!region_str) {
                region_str = &locale_buff[i + 1];
            }
        }
    }

    VERIFY_MSG(region_str, "Could not find a region in locale: %s", curr_locale);

    sb_locale_t locale;
    VERIFY(strcpy_s(locale.language, ARRAY_SIZE(locale.language), language_str) == 0);
    VERIFY(strcpy_s(locale.region, ARRAY_SIZE(locale.region), region_str) == 0);

    return locale;
}

/*
=======================================
DEEPLINK
=======================================
*/

/*
=======================================
sb_get_deeplink_buffer
Reference implementation for posix platforms.
Handle is expected to be pointing to an cstr. Posix platforms pass through
the "--deeplink" command line argument parameter as the handle.
=======================================
*/

const_mem_region_t sb_get_deeplink_buffer(const sb_deeplink_handle_t * const handle) {
    ASSERT(handle);
    return CONST_MEM_REGION(
            .ptr = handle,
            .size = handle ? strlen((const char *)handle) : 0);
}

void sb_release_deeplink(sb_deeplink_handle_t * const handle) {
    ASSERT(handle);
    // App will call this function when it has no further use for the specified deeplink.
    // Any required cleanup should be performed here.
    // This reference implementation retrieves deeplinks via command line and does not allocate memory, so no cleanup is required.
}

int64_t sb_get_localtime_offset() {
    time_t current_time = time(NULL);
    const struct tm * const info = localtime(&current_time);
    if (info == NULL) {
        LOG_ERROR(TAG_RT_IX, "Couldn't get user's timezone offset, 'localtime' call failed with: %s", strerror(errno));
        return 0;
    }

    int64_t offset = info->tm_gmtoff; // returns seconds
    return offset;
}
