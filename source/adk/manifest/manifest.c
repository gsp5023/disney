/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 manifest.c

 App runner for m5 (wasm) applications
*/

#include "manifest.h"

#include "extern/cjson/cJSON.h"
#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/file/file.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/crc.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/rand_gen.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef _MANIFEST_TRACE
#include "source/adk/telemetry/telemetry.h"
#define MANIFEST_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define MANIFEST_TRACE_POP() TRACE_POP()
#else
#define MANIFEST_TRACE_PUSH_FN()
#define MANIFEST_TRACE_POP()
#endif

#define MANIFEST_TAG FOURCC('M', 'N', 'F', 'T')

// Helper to handle manifest errors.  Example:
//     ON_ERROR(node == NULL, return -1, "name %s not found", name);
//
#define ON_ERROR(_expr, _action, ...)             \
    do {                                          \
        if (!!(_expr)) {                          \
            LOG_ERROR(MANIFEST_TAG, __VA_ARGS__); \
            _action;                              \
        }                                         \
    } while (false)

// Constants
//
#define DECL_CONST_STR(STR) static const char c_##STR[] = #STR;

DECL_CONST_STR(manifest_heap_tag);
DECL_CONST_STR(v1);
DECL_CONST_STR(options);
DECL_CONST_STR(rules);
DECL_CONST_STR(file);
DECL_CONST_STR(bundle);
DECL_CONST_STR(sample);
DECL_CONST_STR(url);
DECL_CONST_STR(interpreter);
DECL_CONST_STR(signature);
DECL_CONST_STR(runtime_config);
DECL_CONST_STR(sys_params);
DECL_CONST_STR(memory_reservations);
DECL_CONST_STR(thread_pool_thread_count);
#ifndef _SHIP
DECL_CONST_STR(guard_page_mode);
#endif
DECL_CONST_STR(wasm_memory_size);
DECL_CONST_STR(log_input_events);
DECL_CONST_STR(network_pump_fragment_size);
DECL_CONST_STR(network_pump_sleep_period_ms);
DECL_CONST_STR(watchdog);
DECL_CONST_STR(low);
DECL_CONST_STR(high);
DECL_CONST_STR(bundle_fetch);
DECL_CONST_STR(retry_max_attempts);
DECL_CONST_STR(retry_backoff_ms);
DECL_CONST_STR(coredump_memory_size);
DECL_CONST_STR(platform_settings);

#undef DECL_CONST_STR

enum {
    manifest_heap_size = 512 * 1024,
    num_sys_props = 11
};

// File-scope variables
static struct {
    const adk_system_metrics_t * metrics;

    heap_t heap;
    mem_region_t pages;

    cJSON_Env json_ctx;
    cJSON * manifest_config_json;
    cJSON * json_root;

    runtime_configuration_t runtime_config;
    uint32_t (*select_bundle_fn)(const uint32_t total);
} statics;

// Name and value of a system property
typedef struct sys_prop_t {
    const char * name;
    uint16_t val_offs; // value's offset within adk_system_metrics_t
} sys_prop_t;

// Set of all system properties
typedef struct sys_props_t {
    sys_prop_t prop[num_sys_props];
} sys_props_t;

// Returns system properties, ordered for binary search
static sys_props_t get_sys_props() {
#define INIT_PROP(IX, NAME) [(IX)] = {.name = #NAME, .val_offs = offsetof(adk_system_metrics_t, NAME)}

    sys_props_t sys_props = {
        .prop = {
            // Keep in ascending order by name
            INIT_PROP(0, config),
            INIT_PROP(1, core_version),
            INIT_PROP(2, cpu),
            INIT_PROP(3, device),
            INIT_PROP(4, device_id),
            INIT_PROP(5, device_region),
            INIT_PROP(6, firmware),
            INIT_PROP(7, gpu),
            INIT_PROP(8, revision),
            INIT_PROP(9, software),
            INIT_PROP(10, vendor),
        }};

#undef INIT_PROP

    return sys_props;
}

// Finds given system property name by binary search
static int find_sys_prop(const sys_props_t * const props, const char * const name) {
    // exclusive search bounds: lbound < ix < ubound
    int lbound = -1, ubound = num_sys_props;
    int dist = num_sys_props;
    int ix = dist >> 1;

    while (dist > 0) {
        const int cmp = strcasecmp(name, props->prop[ix].name);
        if (cmp > 0) {
            lbound = ix;
            dist = (ubound - lbound) >> 1;
            ix += dist;
        } else if (cmp < 0) {
            ubound = ix;
            dist = (ubound - lbound) >> 1;
            ix -= dist;
        } else {
            return ix;
        }
    }

    return -1; // not found
}

static cJSON * parse_bundle_config_json(const char * const bundle_config_file_path) {
    mem_region_t bundle_config_blob = manifest_alloc_file_blob(bundle_config_file_path);

    cJSON * root = NULL;
    if (bundle_config_blob.ptr) {
        if (load_artifact_data(sb_app_root_directory, bundle_config_blob, bundle_config_file_path, 0)) {
            root = cJSON_Parse(&statics.json_ctx, CONST_MEM_REGION(.ptr = bundle_config_blob.ptr, .size = bundle_config_blob.size));
            if (root == NULL) {
                LOG_WARN(MANIFEST_TAG, "Could not parse bundle configuration file '%s'.", bundle_config_file_path);
            }
        }
        manifest_free_file_blob(bundle_config_blob);
        return root;
    }

    LOG_DEBUG(MANIFEST_TAG, "Could not find bundle configuration file '%s'.", bundle_config_file_path);

    return NULL;
}

// Returns system property value, given properties and index
static inline const char * get_sys_prop_val(const sys_props_t * const props, const int ix) {
    return (const char *)statics.metrics + props->prop[ix].val_offs;
}

static void * cjson_malloc(void * const ctx, size_t const size) {
    heap_t * const heap = (heap_t *)ctx;
    return heap_alloc(heap, size, MALLOC_TAG);
}

static void cjson_free(void * const ctx, void * const ptr) {
    if (ptr != NULL) {
        heap_t * const heap = (heap_t *)ctx;
        heap_free(heap, ptr, MALLOC_TAG);
    }
}

// pick a random number ranging from 0 to `total` - 1
static uint32_t select_random(const uint32_t total) {
    // pick a number in the distribution range
    const microseconds_t us = adk_read_microsecond_clock();
    const int seed = (int)((us.us >> 32) ^ us.us);
    srand(seed);
    return (total > 0) ? rand() % total : 0;
}

// pick a random number based on the device id so a device will always read the same roll.
static uint32_t select_random_seed_with_device_id(const uint32_t total) {
    adk_rand_generator_t rand_gen = adk_rand_create_generator_with_seed(crc_64_ecma((unsigned char *)&statics.metrics->device_id.bytes, (size_t)ARRAY_SIZE(statics.metrics->device_id.bytes)));
    return (total > 0) ? adk_rand_next(&rand_gen) % total : 0;
}

static void init_selector_function(void) {
    if (!statics.select_bundle_fn) {
        statics.select_bundle_fn = &select_random_seed_with_device_id;
    }
}

// Init/shutdown
//
void manifest_init(const adk_system_metrics_t * const metrics) {
    MANIFEST_TRACE_PUSH_FN();

    statics.metrics = metrics;

    // Create manifest heap
    statics.pages = sb_map_pages(PAGE_ALIGN_INT(manifest_heap_size), system_page_protect_read_write);
    TRAP_OUT_OF_MEMORY(statics.pages.ptr);
    heap_init_with_region(&statics.heap, statics.pages, 8, 0, c_manifest_heap_tag);

    statics.json_ctx = (cJSON_Env){
        .ctx = &statics.heap,
        .callbacks = {
            .malloc = cjson_malloc,
            .free = cjson_free,
        }};

    init_selector_function();

    MANIFEST_TRACE_POP();
}

void manifest_shutdown() {
    MANIFEST_TRACE_PUSH_FN();

    manifest_free_parser();

    heap_destroy(&statics.heap, c_manifest_heap_tag);
    sb_unmap_pages(statics.pages);

    MANIFEST_TRACE_POP();
}

mem_region_t manifest_alloc_file_blob(const char * const manifest_path) {
    MANIFEST_TRACE_PUSH_FN();

    const size_t manifest_size = get_artifact_size(sb_app_root_directory, manifest_path);
    if (!manifest_size) {
        MANIFEST_TRACE_POP();
        return (mem_region_t){0}; // not found or empty
    }

    const mem_region_t region = MEM_REGION(.ptr = heap_alloc(&statics.heap, manifest_size, MALLOC_TAG), .size = manifest_size);
    MANIFEST_TRACE_POP();
    return region;
}

void manifest_free_file_blob(mem_region_t const manifest_blob) {
    heap_free(&statics.heap, manifest_blob.ptr, MALLOC_TAG);
}

void manifest_format_default_url(char * const buf, const size_t buf_size, const char * const url_fmt) {
    VERIFY_MSG(statics.metrics->partner[0] && statics.metrics->partner_guid[0], "Partner configuration is missing, please check the corresponding 'persona.json' file for proper values");

    sprintf_s(buf, buf_size, url_fmt, statics.metrics->partner, statics.metrics->partner_guid);
}

runtime_configuration_t * manifest_get_runtime_configuration() {
    return &statics.runtime_config;
}

static const char warning_msg[] = "expected %s, found '%s'";
static const char not_found_err_msg[] = "'%s' invalid or missing";

// Returns named member of JSON object IFF it's a non-empty string
static const char * get_json_str_mem(cJSON * const obj, const char * const mem_name) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const mem = cJSON_GetObjectItem(obj, mem_name);
    if (!mem) {
        MANIFEST_TRACE_POP();
        return NULL; // not found, possibly optional
    }

    const char * const val = cJSON_GetStringValue(mem);
    ON_ERROR(
        !val || !*val, do {
            MANIFEST_TRACE_POP();
            return NULL;
        } while (false),
        warning_msg,
        "JSON string",
        mem_name);

    MANIFEST_TRACE_POP();
    return val;
}

static void read_interpreter(cJSON * const obj, manifest_t * const manifest) {
    MANIFEST_TRACE_PUSH_FN();

    const char * interpreter = get_json_str_mem(obj, c_interpreter);
    if (interpreter) {
        strcpy_s(manifest->interpreter, ARRAY_SIZE(manifest->interpreter), interpreter);
    }

    MANIFEST_TRACE_POP();
}

// Evaluates system property rule, returning `true` if all options matched the system properties
static bool evaluate_system_properties(cJSON * const rules_obj, const sys_props_t * const sys_props) {
    MANIFEST_TRACE_PUSH_FN();
    cJSON * prop_item;
    cJSON_ArrayForEach(prop_item, rules_obj) {
        const int sys_prop_ix = find_sys_prop(sys_props, prop_item->string);
        if (sys_prop_ix < 0) {
            LOG_WARN(MANIFEST_TAG, "'%s' not a valid system property name", prop_item->string);
            continue; // ignore, not a prop name
        }

        const char * const sys_prop_val = get_sys_prop_val(sys_props, sys_prop_ix);
        const char * reqd_val;

        // If the item is an array go through the list of items.
        if (cJSON_IsArray(prop_item)) {
            bool found = false;
            cJSON * prop_list_item;
            cJSON_ArrayForEach(prop_list_item, prop_item) {
                ON_ERROR(!cJSON_IsString(prop_list_item), continue, warning_msg, "string", prop_list_item->string);
                reqd_val = cJSON_GetStringValue(prop_list_item);
                if (!reqd_val || (strcasecmp(reqd_val, sys_prop_val) != 0)) {
                    LOG_DEBUG(MANIFEST_TAG, "rule did not match for '%s': '%s' != '%s'", prop_item->string, reqd_val, sys_prop_val);
                } else {
                    found = true;
                    break;
                }
            }
            if (!found) {
                break;
            }
        } else {
            ON_ERROR(!cJSON_IsString(prop_item), continue, warning_msg, "string", prop_item->string);
            reqd_val = cJSON_GetStringValue(prop_item);
            if (!reqd_val || (strcasecmp(reqd_val, sys_prop_val) != 0)) {
                LOG_DEBUG(MANIFEST_TAG, "rule did not match for '%s': '%s' != '%s'", prop_item->string, reqd_val, sys_prop_val);

                break;
            }
        }
    }
    MANIFEST_TRACE_POP();
    return prop_item == NULL; // true if all predicates matched
}

// Populates `manifest` based on the elements in `bundle_item`
// With a single element, `manifest` is updated with that element
// With multiple elements, elements with a "sample" object are chosen based on a distribution of the value of the "sample"
// The sample is an integer value and counts as likely hood that that sample will be chosen from sum of all the sample values in the group.
// For example if the group contained bundle A with a sample of 1 and bundle B with a sample of 1 then the group is equally distributed across the sum 2 with each bundle having a 1/2 (50%) chance of being chosen.
// For example if the group contained bundle A with a sample of 1 and bundle B with a sample of 9 then A has a 1/10 chance and B has a 9/10 chance
// Returns `false` when an element can not be chosen
static bool choose_bundle_option(cJSON * const bundle_item, manifest_t * const manifest) {
    MANIFEST_TRACE_PUSH_FN();

    uint32_t total = 0;
    uint32_t border = 0;
    // sum the distribution range
    cJSON * bundle_elem;

    ON_ERROR(
        cJSON_GetArraySize(bundle_item) <= 0,
        do {
            MANIFEST_TRACE_POP();
            return false;
        } while (false),
        "The bundle array is empty");

    cJSON_ArrayForEach(bundle_elem, bundle_item) {
        ON_ERROR(
            !cJSON_IsObject(bundle_elem),
            continue,
            warning_msg,
            bundle_item->string,
            "JSON object");
        cJSON * const sample_item = cJSON_GetObjectItem(bundle_elem, c_sample);
        if (NULL == sample_item) {
            total++; // Bundles without a sample are given a weight of 1
        } else {
            ON_ERROR(
                !cJSON_IsNumber(sample_item), do {
                    MANIFEST_TRACE_POP();
                    return false;
                } while (false),
                not_found_err_msg,
                c_sample);
            total += sample_item->valueint;
        }
    }

    // pick a number in the distribution range
    const uint32_t pick = statics.select_bundle_fn(total);

    LOG_DEBUG(MANIFEST_TAG, "Choose bundle %d (from 0 to %d inclusive)", pick, (total > 0) ? total - 1 : 0);

    // It might seem like a good idea to load the other URL if the selected one failed, but that could lead to surprises
    // in the field such as the 'canary' being loaded on too many devices if the old version became unreachable.
    cJSON_ArrayForEach(bundle_elem, bundle_item) {
        ON_ERROR(
            !cJSON_IsObject(bundle_elem),
            continue,
            warning_msg,
            bundle_item->string,
            "JSON object");
        uint32_t sample = 1; // Default
        cJSON * const sample_item = cJSON_GetObjectItem(bundle_elem, c_sample);
        if (sample_item) {
            ON_ERROR(
                !cJSON_IsNumber(sample_item), do {
                    MANIFEST_TRACE_POP();
                    return false;
                } while (false),
                not_found_err_msg,
                c_sample);
            sample = sample_item->valueint;
        }

        border += sample;

        if (pick >= border) {
            // Not the right one bundle
            continue;
        }

        cJSON * const signature_item = cJSON_GetObjectItem(bundle_elem, c_signature);
        if (signature_item != NULL && cJSON_IsString(signature_item)) {
            strcpy_s(manifest->signature, ARRAY_SIZE(manifest->signature), signature_item->valuestring);
        }

        read_interpreter(bundle_elem, manifest);

        const char * const url = get_json_str_mem(bundle_elem, c_url);
        const char * const file = get_json_str_mem(bundle_elem, c_file);

        if (url != NULL && file != NULL) {
            LOG_ERROR(MANIFEST_TAG, "Both `url` and `file` options specified!");
            MANIFEST_TRACE_POP();
            return false;
        } else if (url != NULL) {
            manifest->resource_type = manifest_resource_url;
            strcpy_s(manifest->resource, ARRAY_SIZE(manifest->resource), url);
        } else if (file != NULL) {
            manifest->resource_type = manifest_resource_file;
            strcpy_s(manifest->resource, ARRAY_SIZE(manifest->resource), file);
        } else {
            LOG_ERROR(MANIFEST_TAG, "Either `%s` or `%s` option required", c_url, c_file);
            MANIFEST_TRACE_POP();
            return false;
        }
        MANIFEST_TRACE_POP();
        return true;
    }
    MANIFEST_TRACE_POP();
    return false;
}

// Attempts to load manifest using given rule, first tries to load from file, if specified, then from URL
bool attempt_load(cJSON * const option_obj, manifest_t * const manifest) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const bundle_item = cJSON_GetObjectItem(option_obj, c_bundle);
    ON_ERROR(
        !cJSON_IsArray(bundle_item), do {
            MANIFEST_TRACE_POP();
            return false;
        } while (false),
        not_found_err_msg,
        c_bundle);

    const bool status = choose_bundle_option(bundle_item, manifest);
    MANIFEST_TRACE_POP();
    return status;
}

// conditionally overwrites the value present in `out_value` if `key` is found in `parent`
static bool overwrite_if_present_uint32(const cJSON * const parent, const char * const key, uint32_t * const out_value) {
    MANIFEST_TRACE_PUSH_FN();

    const cJSON * const child_obj = cJSON_GetObjectItem(parent, key);
    if (!cJSON_IsNumber(child_obj)) {
        MANIFEST_TRACE_POP();
        return false;
    }
    if ((uint32_t)child_obj->valueint != *out_value) {
        LOG_DEBUG(MANIFEST_TAG, "'%s' changed from %u bytes to %u bytes.", key, *out_value, child_obj->valueint);
    }
    *out_value = child_obj->valueint;
    MANIFEST_TRACE_POP();
    return true;
}

// this struct _must_ match the order in source/adk/runtime/app/app.h: adk_low_memory_reservations_t
static const struct {
    const char * const runtime;
    const char * const rhi;
    const char * const render_device;
    const char * const bundle;
    const char * const canvas;
    const char * const canvas_font_scratchpad;
    const char * const cncbus;
    const char * const curl;
    const char * const curl_fragment_buffers;
    const char * const json_deflate;
    const char * const default_thread_pool;
    const char * const ssl;
    const char * const http2;
    const char * const httpx;
    const char * const httpx_fragment_buffers;
    const char * const reporting;
} manifest_low_mem_strings = {
    .runtime = "runtime",
    .rhi = "rhi",
    .render_device = "render_device",
    .bundle = "bundle",
    .canvas = "canvas",
    .canvas_font_scratchpad = "canvas_font_scratchpad",
    .cncbus = "cncbus",
    .curl = "curl",
    .curl_fragment_buffers = "curl_fragment_buffers",
    .json_deflate = "json_deflate",
    .default_thread_pool = "default_thread_pool",
    .ssl = "ssl",
    .http2 = "http2",
    .httpx = "httpx",
    .httpx_fragment_buffers = "httpx_fragment_buffers",
    .reporting = "reporting",
};

STATIC_ASSERT(sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t) == sizeof(manifest_low_mem_strings) / sizeof(const char *));

// this struct _must_ match the order in source/adk/runtime/app/app.h: adk_high_memory_reservations_t
static struct {
    const char * const canvas;
} manifest_high_mem_strings = {
    .canvas = "canvas",
};

STATIC_ASSERT(sizeof(adk_high_memory_reservations_t) / sizeof(uint32_t) == sizeof(manifest_high_mem_strings) / sizeof(const char *));

// Overwrites the given memory reservation with values from the JSON "option" node (i.e., a JSON object from the "options" array of the manifest).
static void conditional_overwrite_low_memory_reservations(const cJSON * const mem_res_obj, adk_low_memory_reservations_t * const low_mem) {
    MANIFEST_TRACE_PUSH_FN();

    const char * const * const low_reservation_strs = (const char **)&manifest_low_mem_strings;
    uint32_t * const low_reservations = (uint32_t *)low_mem;
    for (size_t i = 0; i < sizeof(manifest_low_mem_strings) / sizeof(const char *); ++i) {
        VERIFY_MSG(low_reservation_strs[i], "low_reservation string at [%i] was null.", i);
        overwrite_if_present_uint32(mem_res_obj, low_reservation_strs[i], &low_reservations[i]);
    }

    MANIFEST_TRACE_POP();
}

static void conditional_overwrite_high_memory_reservations(const cJSON * const mem_res_obj, adk_high_memory_reservations_t * const high_mem) {
    MANIFEST_TRACE_PUSH_FN();

    const char * const * const high_reservation_strs = (const char **)&manifest_high_mem_strings;
    uint32_t * const high_reservations = (uint32_t *)high_mem;
    for (size_t i = 0; i < sizeof(manifest_high_mem_strings) / sizeof(const char *); ++i) {
        VERIFY_MSG(high_reservation_strs[i], "high_reservation string at [%i] was null.", i);
        overwrite_if_present_uint32(mem_res_obj, high_reservation_strs[i], &high_reservations[i]);
    }

    MANIFEST_TRACE_POP();
}

static void manifest_read_logging_mode(const cJSON * const logging_mode_obj, logging_mode_e * const logging_mode) {
    if (!logging_mode_obj || !cJSON_IsString(logging_mode_obj)) {
        return;
    }
    if (strcmp(logging_mode_obj->valuestring, "disabled") == 0) {
        *logging_mode = logging_disabled;
    } else if (strcmp(logging_mode_obj->valuestring, "tty") == 0) {
        *logging_mode = logging_tty;
    } else if (strcmp(logging_mode_obj->valuestring, "metrics") == 0) {
        *logging_mode = logging_metrics;
    } else if (strcmp(logging_mode_obj->valuestring, "tty_and_metrics") == 0) {
        *logging_mode = logging_tty_and_metrics;
    } else {
        LOG_ERROR(MANIFEST_TAG, "Invalid logging_mode_e: [%s]", logging_mode_obj->valuestring);
    }
}

#ifndef _SHIP
static system_guard_page_mode_e bundle_get_guard_page_mode(const cJSON * const guard_page_obj) {
    MANIFEST_TRACE_PUSH_FN();

    if (!guard_page_obj || !cJSON_IsString(guard_page_obj)) {
        MANIFEST_TRACE_POP();
        return default_guard_page_mode;
    }

    if (strcmp(guard_page_obj->valuestring, "minimal") == 0) {
        MANIFEST_TRACE_POP();
        return system_guard_page_mode_minimal;
    }
    if (strcmp(guard_page_obj->valuestring, "enabled") == 0) {
        MANIFEST_TRACE_POP();
        return system_guard_page_mode_enabled;
    }
    if (strcmp(guard_page_obj->valuestring, "disabled") == 0) {
        MANIFEST_TRACE_POP();
        return system_guard_page_mode_disabled;
    }
    MANIFEST_TRACE_POP();
    return default_guard_page_mode;
}
#endif

static bool bundle_conditional_overwrite_http_max_pooled_connections(const cJSON * const out_max_pooled_connections_obj, uint32_t * const out_max_pooled_connections) {
    MANIFEST_TRACE_PUSH_FN();

    if (!out_max_pooled_connections_obj || !cJSON_IsNumber(out_max_pooled_connections_obj)) {
        MANIFEST_TRACE_POP();
        return false;
    }
    *out_max_pooled_connections = (uint32_t)out_max_pooled_connections_obj->valueint;
    MANIFEST_TRACE_POP();
    return true;
}

static void manifest_get_websocket_backend(const cJSON * const adk_websocket_obj, adk_websocket_backend_e * const out_backend) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const backend_obj = cJSON_GetObjectItem(adk_websocket_obj, "backend");
    if (backend_obj && cJSON_IsString(backend_obj)) {
        if (strcmp(backend_obj->valuestring, "websocket") == 0) {
            LOG_INFO(MANIFEST_TAG, "Websocket backend: [websocket] set!");
            *out_backend = adk_websocket_backend_websocket;
            MANIFEST_TRACE_POP();
            return;
        } else if (strcmp(backend_obj->valuestring, "null") == 0) {
            LOG_INFO(MANIFEST_TAG, "Websocket backend: [none] set!");
            *out_backend = adk_websocket_backend_null;
            MANIFEST_TRACE_POP();
            return;
        }
    }
    LOG_INFO(MANIFEST_TAG, "Websocket backend: [http2] set!");
    *out_backend = adk_websocket_backend_http2;
    MANIFEST_TRACE_POP();
}

static void manifest_conditional_overwrite_websocket_config(const cJSON * const adk_websocket_obj, websocket_config_t * const out_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const websocket_config_obj = cJSON_GetObjectItem(adk_websocket_obj, "websocket_config");
    if (!websocket_config_obj) {
        MANIFEST_TRACE_POP();
        return;
    }

    static const char * websocket_config_strs[] = {
        "ping_timeout",
        "no_activity_wait_period",
        "max_handshake_timeout",
        "max_receivable_message_size",
        "receive_buffer_size",
        "send_buffer_size",
        "header_buffer_size",
        "maximum_redirects"};
    STATIC_ASSERT(ARRAY_SIZE(websocket_config_strs) * sizeof(uint32_t) == sizeof(websocket_config_t));
    // currently websocket_config_t is just uint32_t's and wrappers around those with no additional fields.. so it's 'legal' to just pretend its an array.
    uint32_t * const config_array = (uint32_t *)out_config;
    for (size_t i = 0; i < ARRAY_SIZE(websocket_config_strs); ++i) {
        const cJSON * const websocket_config_value_obj = cJSON_GetObjectItem(websocket_config_obj, websocket_config_strs[i]);
        if (websocket_config_value_obj && cJSON_IsNumber(websocket_config_value_obj)) {
            LOG_INFO(MANIFEST_TAG, "Setting websocket config [%s] to: [%" PRIu32 "].", websocket_config_strs[i], websocket_config_value_obj->valueint);
            ASSERT(websocket_config_value_obj->valueint >= 0);
            config_array[i] = (uint32_t)websocket_config_value_obj->valueint;
        }
    }
    MANIFEST_TRACE_POP();
}

static void manifest_parse_adk_websocket(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const adk_websocket_obj = cJSON_GetObjectItem(sys_params_obj, "adk_websocket");
    if (!adk_websocket_obj) {
        MANIFEST_TRACE_POP();
        return;
    }
    manifest_get_websocket_backend(adk_websocket_obj, &runtime_config->websocket.backend);
    manifest_conditional_overwrite_websocket_config(adk_websocket_obj, &runtime_config->websocket.config);
    MANIFEST_TRACE_POP();
}

static void manifest_get_canvas_font_atlas_dims(const cJSON * const canvas_obj, int32_t * const width, int32_t * const height) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const font_atlas_obj = cJSON_GetObjectItem(canvas_obj, "font_atlas");
    if (!font_atlas_obj) {
        MANIFEST_TRACE_POP();
        return;
    }
    const cJSON * const width_obj = cJSON_GetObjectItem(font_atlas_obj, "width");
    if (width_obj && cJSON_IsNumber(width_obj)) {
        *width = width_obj->valueint;
    }
    const cJSON * const height_obj = cJSON_GetObjectItem(font_atlas_obj, "height");
    if (height_obj && cJSON_IsNumber(height_obj)) {
        *height = height_obj->valueint;
    }
    MANIFEST_TRACE_POP();
}

static void manifest_parse_canvas_gl(const cJSON * const canvas_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const gl_obj = cJSON_GetObjectItem(canvas_obj, "gl");
    if (!gl_obj || !cJSON_IsObject(gl_obj)) {
        MANIFEST_TRACE_POP();
        return;
    }
    {
        const cJSON * const internal_limits_obj = cJSON_GetObjectItem(gl_obj, "internal_limits");
        if (internal_limits_obj && cJSON_IsObject(internal_limits_obj)) {
            const cJSON * const max_verts_per_vertex_bank_obj = cJSON_GetObjectItem(internal_limits_obj, "max_verts_per_vertex_bank");
            if (max_verts_per_vertex_bank_obj && cJSON_IsNumber(max_verts_per_vertex_bank_obj)) {
                runtime_config->canvas.gl.internal_limits.max_verts_per_vertex_bank = (uint32_t)max_verts_per_vertex_bank_obj->valueint;
            }
            const cJSON * const num_vertex_banks_obj = cJSON_GetObjectItem(internal_limits_obj, "num_vertex_banks");
            if (num_vertex_banks_obj && cJSON_IsNumber(num_vertex_banks_obj)) {
                runtime_config->canvas.gl.internal_limits.num_vertex_banks = (uint32_t)num_vertex_banks_obj->valueint;
            }
            const cJSON * const num_meshes_obj = cJSON_GetObjectItem(internal_limits_obj, "num_meshes");
            if (num_meshes_obj && cJSON_IsNumber(num_meshes_obj) && (num_meshes_obj->valueint > 0)) {
                runtime_config->canvas.gl.internal_limits.num_meshes = (uint32_t)num_meshes_obj->valueint;
            }
        }
    }
}

static void manifest_parse_canvas(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const canvas_obj = cJSON_GetObjectItem(sys_params_obj, "canvas");
    if (!canvas_obj) {
        MANIFEST_TRACE_POP();
        return;
    }
    {
        const cJSON * const max_states_obj = cJSON_GetObjectItem(canvas_obj, "max_states");
        if (max_states_obj && cJSON_IsNumber(max_states_obj)) {
            runtime_config->canvas.internal_limits.max_states = (uint32_t)max_states_obj->valueint;
        }
    }
    {
        const cJSON * const max_tesselation_steps_obj = cJSON_GetObjectItem(canvas_obj, "max_tesselation_steps");
        if (max_tesselation_steps_obj && cJSON_IsNumber(max_tesselation_steps_obj)) {
            runtime_config->canvas.internal_limits.max_tessellation_steps = (uint32_t)max_tesselation_steps_obj->valueint;
        } else {
            const cJSON * const max_tessellation_steps_obj = cJSON_GetObjectItem(canvas_obj, "max_tessellation_steps");
            if (max_tessellation_steps_obj && cJSON_IsNumber(max_tessellation_steps_obj)) {
                runtime_config->canvas.internal_limits.max_tessellation_steps = (uint32_t)max_tessellation_steps_obj->valueint;
            }
        }
    }
    {
        const cJSON * const enable_punchthrough_blend_mode_fix_obj = cJSON_GetObjectItem(canvas_obj, "enable_punchthrough_blend_mode_fix");
        if (enable_punchthrough_blend_mode_fix_obj && cJSON_IsBool(enable_punchthrough_blend_mode_fix_obj)) {
            runtime_config->canvas.enable_punchthrough_blend_mode_fix = (bool)enable_punchthrough_blend_mode_fix_obj->valueint;
        }
    }
    {
        const cJSON * const text_mesh_cache_obj = cJSON_GetObjectItem(canvas_obj, "text_mesh_cache");
        if (text_mesh_cache_obj && cJSON_IsObject(text_mesh_cache_obj)) {
            const cJSON * const enabled_obj = cJSON_GetObjectItem(text_mesh_cache_obj, "enabled");
            if (enabled_obj && cJSON_IsBool(enabled_obj)) {
                runtime_config->canvas.text_mesh_cache.enabled = (bool)enabled_obj->valueint;
            }
            const cJSON * const size_obj = cJSON_GetObjectItem(text_mesh_cache_obj, "size");
            if (size_obj && cJSON_IsNumber(size_obj)) {
                runtime_config->canvas.text_mesh_cache.size = (uint32_t)size_obj->valueint;
            }
        }
    }
    {
        const cJSON * const gzip_limits_obj = cJSON_GetObjectItem(canvas_obj, "gzip_limits");
        if (gzip_limits_obj && cJSON_IsObject(gzip_limits_obj)) {
            const cJSON * const working_space_obj = cJSON_GetObjectItem(gzip_limits_obj, "working_space");
            if (working_space_obj && cJSON_IsNumber(working_space_obj)) {
                runtime_config->canvas.gzip_limits.working_space = (uint32_t)working_space_obj->valueint;
            }
        }
    }
    manifest_get_canvas_font_atlas_dims(canvas_obj, &runtime_config->canvas.font_atlas.width, &runtime_config->canvas.font_atlas.height);
    manifest_parse_canvas_gl(canvas_obj, runtime_config);
    MANIFEST_TRACE_POP();
}

static void manifest_parse_renderer(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();

    const cJSON * const renderer_obj = cJSON_GetObjectItem(sys_params_obj, "renderer");
    if (renderer_obj != NULL) {
        const cJSON * const device_obj = cJSON_GetObjectItem(renderer_obj, "device");
        if ((device_obj != NULL) && cJSON_IsObject(device_obj)) {
            const cJSON * const num_cmd_buffers_obj = cJSON_GetObjectItem(device_obj, "num_cmd_buffers");
            if (num_cmd_buffers_obj && cJSON_IsNumber(num_cmd_buffers_obj)) {
                runtime_config->renderer.device.num_cmd_buffers = num_cmd_buffers_obj->valueint;
            }

            const cJSON * const cmd_buf_size_obj = cJSON_GetObjectItem(device_obj, "cmd_buf_size");
            if (cmd_buf_size_obj && cJSON_IsNumber(cmd_buf_size_obj)) {
                runtime_config->renderer.device.cmd_buf_size = cmd_buf_size_obj->valueint;
            }
        }

        const cJSON * const rhi_command_diffing_obj = cJSON_GetObjectItem(renderer_obj, "rhi_command_diffing");
        if ((rhi_command_diffing_obj != NULL) && cJSON_IsObject(rhi_command_diffing_obj)) {
            const cJSON * const enabled_obj = cJSON_GetObjectItem(rhi_command_diffing_obj, "enabled");
            if (enabled_obj && cJSON_IsBool(enabled_obj)) {
                runtime_config->renderer.rhi_command_diffing.enabled = (bool)enabled_obj->valueint;
            }

            const cJSON * const verbose_obj = cJSON_GetObjectItem(rhi_command_diffing_obj, "verbose");
            if (verbose_obj && cJSON_IsBool(verbose_obj)) {
                runtime_config->renderer.rhi_command_diffing.verbose = (bool)verbose_obj->valueint;
            }

            const cJSON * const tracking_obj = cJSON_GetObjectItem(rhi_command_diffing_obj, "tracking");
            if ((tracking_obj != NULL) && cJSON_IsObject(tracking_obj)) {
                const cJSON * const tracking_enabled_obj = cJSON_GetObjectItem(tracking_obj, "enabled");
                if (tracking_enabled_obj && cJSON_IsBool(tracking_enabled_obj)) {
                    runtime_config->renderer.rhi_command_diffing.tracking.enabled = (bool)tracking_enabled_obj->valueint;
                }

                const cJSON * const buffer_size_obj = cJSON_GetObjectItem(tracking_obj, "buffer_size");
                if (buffer_size_obj && cJSON_IsNumber(buffer_size_obj)) {
                    runtime_config->renderer.rhi_command_diffing.tracking.buffer_size = (size_t)buffer_size_obj->valueint;
                }
            }
        }

        const cJSON * const render_resource_tracking_obj = cJSON_GetObjectItem(renderer_obj, "render_resource_tracking");
        if (render_resource_tracking_obj && cJSON_IsObject(render_resource_tracking_obj)) {
            manifest_read_logging_mode(cJSON_GetObjectItem(render_resource_tracking_obj, "periodic_logging"), &runtime_config->renderer.render_resource_tracking.periodic_logging);
        }
    }

    MANIFEST_TRACE_POP();
}

static void manifest_parse_reporting(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const reporting = cJSON_GetObjectItem(sys_params_obj, "reporting");
    if (reporting != NULL) {
        const cJSON * const capture_logs = cJSON_GetObjectItem(reporting, "capture_logs");
        if (capture_logs && cJSON_IsBool(capture_logs)) {
            runtime_config->reporting.capture_logs = (bool)capture_logs->valueint;
        }

        const cJSON * const minimum_event_level = cJSON_GetObjectItem(reporting, "minimum_event_level");
        if (minimum_event_level && cJSON_IsString(minimum_event_level)) {
            const char * const value = minimum_event_level->valuestring;
            if (strcmp(value, "debug") == 0) {
                runtime_config->reporting.minimum_event_level = event_level_debug;
            } else if (strcmp(value, "info") == 0) {
                runtime_config->reporting.minimum_event_level = event_level_info;
            } else if (strcmp(value, "warning") == 0) {
                runtime_config->reporting.minimum_event_level = event_level_warning;
            } else if (strcmp(value, "error") == 0) {
                runtime_config->reporting.minimum_event_level = event_level_error;
            } else if (strcmp(value, "fatal") == 0) {
                runtime_config->reporting.minimum_event_level = event_level_fatal;
            } else {
                LOG_ERROR(MANIFEST_TAG, "Invalid minimum event level: '%s'", value);
            }
        }

        const cJSON * const sentry_dsn = cJSON_GetObjectItem(reporting, "sentry_dsn");
        if (sentry_dsn && cJSON_IsString(sentry_dsn)) {
            strcpy_s(runtime_config->reporting.sentry_dsn, adk_reporting_max_string_length, sentry_dsn->valuestring);
        }

        const cJSON * const send_queue_size = cJSON_GetObjectItem(reporting, "send_queue_size");
        if (send_queue_size && cJSON_IsNumber(send_queue_size)) {
            runtime_config->reporting.send_queue_size = (uint32_t)send_queue_size->valueint;
        }
    }
    MANIFEST_TRACE_POP();
}

static runtime_configuration_renderer_t renderer_get_default_config() {
    return (runtime_configuration_renderer_t){
        .device = {
            .num_cmd_buffers = 32,
            .cmd_buf_size = 64 * 1024,
        },
        .rhi_command_diffing = {.enabled = false, .verbose = false, .tracking = {
                                                                        .enabled = false,
                                                                        .buffer_size = 4096,
                                                                    }},
        .render_resource_tracking = {
            .periodic_logging = logging_disabled,
        }};
}

static void manifest_parse_http(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const http = cJSON_GetObjectItem(sys_params_obj, "http");
    if (http != NULL) {
        const cJSON * const httpx_global_certs = cJSON_GetObjectItem(http, "httpx_global_certs");
        if (httpx_global_certs && cJSON_IsBool(httpx_global_certs)) {
            runtime_config->http.httpx_global_certs = (bool)httpx_global_certs->valueint;
        }
    }
    MANIFEST_TRACE_POP();
}

static void manifest_parse_http2(const cJSON * const sys_params_obj, runtime_configuration_t * const runtime_config) {
    MANIFEST_TRACE_PUSH_FN();
    const cJSON * const http = cJSON_GetObjectItem(sys_params_obj, "http2");
    if (http != NULL) {
        const cJSON * const enabled = cJSON_GetObjectItem(http, "enabled");
        if (enabled && cJSON_IsBool(enabled)) {
            runtime_config->http2.enabled = (bool)enabled->valueint;
        }

        const cJSON * const use_multiplexing = cJSON_GetObjectItem(http, "use_multiplexing");
        if (use_multiplexing && cJSON_IsBool(use_multiplexing)) {
            runtime_config->http2.use_multiplexing = (bool)use_multiplexing->valueint;
        }

        const cJSON * const multiplex_wait_for_existing_connection = cJSON_GetObjectItem(http, "multiplex_wait_for_existing_connection");
        if (multiplex_wait_for_existing_connection && cJSON_IsBool(multiplex_wait_for_existing_connection)) {
            runtime_config->http2.multiplex_wait_for_existing_connection = (bool)multiplex_wait_for_existing_connection->valueint;
        }
    }
    MANIFEST_TRACE_POP();
}

runtime_configuration_t get_default_runtime_configuration(void) {
    runtime_configuration_t config = {
        .memory_reservations = adk_get_default_memory_reservations(),
        .wasm_low_memory_size = 0,
        .wasm_high_memory_size = 0, // No default, will fail if not provided by bundle or manifest
        .wasm_heap_allocation_threshold = 100 * 1024,
        .log_input_events = false,
        .network_pump_fragment_size = 4096,
        .network_pump_sleep_period = 1,
        .watchdog = {
            .enabled = false,
            .suspend_threshold = 30,
            .warning_delay_ms = 100,
            .fatal_delay_ms = 3000,
        },
        .coredump_memory_size = 16 * 1024, // default set for merlin demos, unable to load manifest prior to demos
        .guard_page_mode = default_guard_page_mode,
        .http_max_pooled_connections = 4,
        .bundle_fetch = {.retry_max_attempts = 0, .retry_backoff_ms = {.ms = 0}},
        .websocket = {.backend = adk_websocket_backend_http2, .config = {.ping_timeout = {10000}, .no_activity_wait_period = {50000}, .max_handshake_timeout = {60 * 1000}, .receive_buffer_size = 1024, .send_buffer_size = 4 * 1024, .max_receivable_message_size = 1024 * 1024, .header_buffer_size = 2 * 1024, .maximum_redirects = 10}},
        .canvas = {.enable_punchthrough_blend_mode_fix = false, .internal_limits = {
                                                                    .max_states = cg_default_max_states,
                                                                    .max_tessellation_steps = cg_default_max_tesselation_steps,
                                                                },
                   .font_atlas = {
                       .width = 0,
                       .height = 0,
                   },
                   .text_mesh_cache = {
                       .size = cg_default_max_text_mesh_cache_size,
                       .enabled = false,
                   },
                   .gzip_limits = {
                       .working_space = cg_gzip_default_working_space,
                   },
                   .gl = {
                       .internal_limits = {
                           .max_verts_per_vertex_bank = cg_gl_default_max_verts_per_vertex_bank,
                           .num_vertex_banks = cg_gl_default_vertex_banks,
                           .num_meshes = cg_gl_default_num_meshes,
                       },
                   }},
        .renderer = renderer_get_default_config(),
        .reporting = {.capture_logs = true, .minimum_event_level = event_level_error, .sentry_dsn = {0}, .send_queue_size = 256},
        .http = {.httpx_global_certs = false},
        .http2 = {.enabled = false, .use_multiplexing = false, .multiplex_wait_for_existing_connection = false},
    };

    strcpy_s(config.reporting.sentry_dsn, adk_reporting_max_string_length, "https://d922c6eded824f99b3aeb083fefb999e@disney.my.sentry.io/31");

    return config;
}

static fetch_retry_context_t process_request_retry(const cJSON * const request_retry_obj) {
    MANIFEST_TRACE_PUSH_FN();

    fetch_retry_context_t request_retry = {0};
    const cJSON * const max_attempts = cJSON_GetObjectItem(request_retry_obj, c_retry_max_attempts);
    if (cJSON_IsNumber(max_attempts)) {
        if (max_attempts->valueint <= 0) {
            request_retry.retry_max_attempts = 0;
        } else {
            request_retry.retry_max_attempts = max_attempts->valueint;
        }
    }
    const cJSON * const backoff_ms = cJSON_GetObjectItem(request_retry_obj, c_retry_backoff_ms);
    if (cJSON_IsNumber(backoff_ms)) {
        if (backoff_ms->valueint <= 0) {
            request_retry.retry_backoff_ms.ms = 0;
        } else {
            request_retry.retry_backoff_ms.ms = backoff_ms->valueint;
        }
    }

    MANIFEST_TRACE_POP();
    return request_retry;
}

/* Processes the JSON bundle configuration node.  This is either the "config"
 * node of a option node (i.e., an element of the "options" node of a
 * manifest file) or the root of a bundle's configuration file. */
static void process_runtime_configuration(const cJSON * const runtime_config_obj, runtime_configuration_t * const config) {
    MANIFEST_TRACE_PUSH_FN();

    // Process the "sys_params" node
    const cJSON * const system_params_obj = cJSON_GetObjectItem(runtime_config_obj, c_sys_params);
    if (cJSON_IsObject(system_params_obj)) {
        // Extract WASM memory sizes
        // NOTE: since the the bin/.config file is currently optional, we are
        // not requiring any of its members and this parser will not fail if
        // "wasm_memory_size"is not present.
        // However, this property is required to load a WASM, so its absence
        // may cause a load failure.
        const cJSON * const wasm_memory_size_obj = cJSON_GetObjectItem(system_params_obj, c_wasm_memory_size);
        if (cJSON_IsObject(wasm_memory_size_obj)) {
            const cJSON * const wasm_low_mem_obj = cJSON_GetObjectItem(wasm_memory_size_obj, "low");
            if (cJSON_IsNumber(wasm_low_mem_obj)) {
                config->wasm_low_memory_size = wasm_low_mem_obj->valueint;
            }

            const cJSON * const wasm_high_mem_obj = cJSON_GetObjectItem(wasm_memory_size_obj, "high");
            if (cJSON_IsNumber(wasm_high_mem_obj)) {
                config->wasm_high_memory_size = wasm_high_mem_obj->valueint;
            }

            // Extract WASM heap allocation threshold
            const cJSON * const wasm_heap_allocation_threshold_obj = cJSON_GetObjectItem(wasm_memory_size_obj, "allocation_threshold");
            if (cJSON_IsNumber(wasm_heap_allocation_threshold_obj)) {
                config->wasm_heap_allocation_threshold = wasm_heap_allocation_threshold_obj->valueint;
            }
        } else if (cJSON_IsNumber(wasm_memory_size_obj)) {
            // Compatibility with the old wasm memory configuration
            config->wasm_low_memory_size = 0;
            config->wasm_high_memory_size = wasm_memory_size_obj->valueint;
            config->wasm_heap_allocation_threshold = 0;
        }

        const cJSON * const log_input_events_obj = cJSON_GetObjectItem(system_params_obj, c_log_input_events);
        if (cJSON_IsBool(log_input_events_obj)) {
            config->log_input_events = (bool)(log_input_events_obj->valueint);
        }

        // Extract network pump's fragment size
        const cJSON * const net_pump_fragment_size = cJSON_GetObjectItem(system_params_obj, c_network_pump_fragment_size);
        if (cJSON_IsNumber(net_pump_fragment_size)) {
            config->network_pump_fragment_size = net_pump_fragment_size->valueint;
        }

        const cJSON * const net_pump_sleep_period = cJSON_GetObjectItem(system_params_obj, c_network_pump_sleep_period_ms);
        if (cJSON_IsNumber(net_pump_sleep_period)) {
            config->network_pump_sleep_period = net_pump_sleep_period->valueint;
        }

        const cJSON * const watchdog_value = cJSON_GetObjectItem(system_params_obj, c_watchdog);
        if (cJSON_IsObject(watchdog_value)) {
            const cJSON * const enabled_value = cJSON_GetObjectItem(watchdog_value, "enabled");
            if (cJSON_IsBool(enabled_value)) {
                config->watchdog.enabled = (bool)enabled_value->valueint;
            }

            const cJSON * const suspend_threshold_value = cJSON_GetObjectItem(watchdog_value, "suspend_threshold");
            if (cJSON_IsNumber(suspend_threshold_value)) {
                config->watchdog.suspend_threshold = suspend_threshold_value->valueint;
            }

            const cJSON * const warning_delay_value = cJSON_GetObjectItem(watchdog_value, "warning_delay_ms");
            if (cJSON_IsNumber(warning_delay_value)) {
                config->watchdog.warning_delay_ms = warning_delay_value->valueint;
            }

            const cJSON * const watchdog_fatal_delay_value = cJSON_GetObjectItem(watchdog_value, "fatal_delay_ms");
            if (cJSON_IsNumber(watchdog_fatal_delay_value)) {
                config->watchdog.fatal_delay_ms = watchdog_fatal_delay_value->valueint;
            }
        }

        // Extract memory reservations
        const cJSON * const mem_res_obj = cJSON_GetObjectItem(system_params_obj, c_memory_reservations);
        if (cJSON_IsObject(mem_res_obj)) {
            const cJSON * const low_reservations = cJSON_GetObjectItem(mem_res_obj, c_low);
            if (cJSON_IsObject(low_reservations)) {
                conditional_overwrite_low_memory_reservations(low_reservations, &config->memory_reservations.low);
            } else {
                LOG_WARN(MANIFEST_TAG, "Could not find key [\"%s\":] in memory_reservations object (this may be intentional)", c_low);
            }
            const cJSON * const high_reservations = cJSON_GetObjectItem(mem_res_obj, c_high);
            if (cJSON_IsObject(high_reservations)) {
                conditional_overwrite_high_memory_reservations(high_reservations, &config->memory_reservations.high);
            } else {
                LOG_WARN(MANIFEST_TAG, "Could not find key [\"%s\":] in memory_reservations object (this may be intentional)", c_high);
            }
        }

        // Request retry value
        const cJSON * const request_retry_obj = cJSON_GetObjectItem(system_params_obj, c_bundle_fetch);
        if (cJSON_IsObject(request_retry_obj)) {
            config->bundle_fetch = process_request_retry(request_retry_obj);
        } else {
            LOG_DEBUG(MANIFEST_TAG, "No retry data available in runtime configuration.");
        }

        // Coredump memory reservation
        const cJSON * const coredump_mem_obj = cJSON_GetObjectItem(system_params_obj, c_coredump_memory_size);
        if (cJSON_IsNumber(coredump_mem_obj)) {
            config->coredump_memory_size = coredump_mem_obj->valueint;
        }

        // Number of threads in the thread pool
        const cJSON * const threadpool_mem_obj = cJSON_GetObjectItem(system_params_obj, c_thread_pool_thread_count);
        if (cJSON_IsNumber(threadpool_mem_obj)) {
            config->thread_pool_thread_count = (uint8_t)threadpool_mem_obj->valueint;
        }

#ifndef _SHIP
        config->guard_page_mode = bundle_get_guard_page_mode(cJSON_GetObjectItem(system_params_obj, c_guard_page_mode));
#endif
        bundle_conditional_overwrite_http_max_pooled_connections(cJSON_GetObjectItem(system_params_obj, "http_max_pooled_connections"), &config->http_max_pooled_connections);
        manifest_parse_adk_websocket(system_params_obj, config);
        manifest_parse_canvas(system_params_obj, config);
        manifest_parse_renderer(system_params_obj, config);
        manifest_parse_reporting(system_params_obj, config);
        manifest_parse_http(system_params_obj, config);
        manifest_parse_http2(system_params_obj, config);
    }

    MANIFEST_TRACE_POP();
}

bool parse_bundle_config(const char * const bundle_config_file_path, runtime_configuration_t * const out_config) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const bundle_config_root_obj = parse_bundle_config_json(bundle_config_file_path);
    if (bundle_config_root_obj) {
        if (cJSON_IsObject(bundle_config_root_obj)) {
            LOG_INFO(MANIFEST_TAG, "Applying configuration from '%s'.", bundle_config_file_path);
            process_runtime_configuration(bundle_config_root_obj, out_config);
        }
        cJSON_Delete(&statics.json_ctx, bundle_config_root_obj);
        MANIFEST_TRACE_POP();
        return true;
    }
    MANIFEST_TRACE_POP();
    return false;
}

// Evaluates wasm selection rules, returns first wasm successfully loaded
static void read_manifest_for_wasm(
    cJSON * const root,
    manifest_t * const manifest) {
    MANIFEST_TRACE_PUSH_FN();

    const sys_props_t props = get_sys_props();

    cJSON * const v1_item = cJSON_GetObjectItem(root, c_v1);
    ON_ERROR(
        !cJSON_IsObject(v1_item), do {
            MANIFEST_TRACE_POP();
            return;
        } while (false),
        not_found_err_msg,
        c_v1);

    cJSON * const options_arr = cJSON_GetObjectItem(v1_item, c_options);
    ON_ERROR(
        !cJSON_IsArray(options_arr), do {
            MANIFEST_TRACE_POP();
            return;
        } while (false),
        not_found_err_msg,
        c_options);

    if (cJSON_IsArray(options_arr)) {
        // Process array of wasm selection rules, ignoring invalid rules
        cJSON * option_obj;
        cJSON_ArrayForEach(option_obj, options_arr) {
            // user error, schema violation, try next rule
            ON_ERROR(
                !cJSON_IsObject(option_obj),
                continue,
                warning_msg,
                option_obj->string ? option_obj->string : "<unnamed scalar>",
                "JSON object");

            cJSON * const rules_obj = cJSON_GetObjectItem(option_obj, c_rules);
            if (evaluate_system_properties(rules_obj, &props)) {
                // Start with lowest precedence (default values)
                manifest->runtime_config = get_default_runtime_configuration();

                statics.manifest_config_json = cJSON_GetObjectItem(option_obj, c_runtime_config);

                if (!cJSON_IsObject(statics.manifest_config_json)) {
                    // No config, or invalidly formed
                    statics.manifest_config_json = NULL;
                } else {
                    // manifest config is high precedence
                    process_runtime_configuration(statics.manifest_config_json, &manifest->runtime_config);
                }

                if (attempt_load(option_obj, manifest)) {
                    break; // loaded!
                }
            }
        }
    }
    MANIFEST_TRACE_POP();
}

// Parses manifest into cJSON tree, evaluates wasm selection rules, returns first wasm successfully loaded
manifest_t manifest_parse(
    const_mem_region_t const manifest_blob) {
    MANIFEST_TRACE_PUSH_FN();

    manifest_t manifest = {0};

    strcpy_s(manifest.interpreter, ARRAY_SIZE(manifest.interpreter), "wasm3"); // our default

    if (statics.json_root != NULL) {
        manifest_free_parser();
    }

    statics.json_root = cJSON_Parse(&statics.json_ctx, manifest_blob);
    if (statics.json_root == NULL) {
        LOG_ERROR(MANIFEST_TAG, "invalid JSON syntax");
        MANIFEST_TRACE_POP();
        return manifest;
    }

    read_manifest_for_wasm(statics.json_root, &manifest);

    MANIFEST_TRACE_POP();
    return manifest;
}

void manifest_free_parser() {
    MANIFEST_TRACE_PUSH_FN();

    if (statics.json_root) {
        cJSON_Delete(&statics.json_ctx, statics.json_root);
        statics.json_root = NULL;
        statics.manifest_config_json = NULL;
    }

    MANIFEST_TRACE_POP();
}

manifest_t manifest_parse_fp(
    sb_file_t * const manifest_file,
    const size_t manifest_file_content_size) {
    MANIFEST_TRACE_PUSH_FN();
    const mem_region_t manifest_blob = MEM_REGION(.ptr = heap_alloc(&statics.heap, manifest_file_content_size, MALLOC_TAG), .size = manifest_file_content_size);

    sb_fread(manifest_blob.ptr, sizeof(uint8_t), manifest_file_content_size, manifest_file);

    manifest_t manifest = manifest_parse(manifest_blob.consted);
    heap_free(&statics.heap, manifest_blob.ptr, MALLOC_TAG);

    MANIFEST_TRACE_POP();
    return manifest;
}

bool bundle_config_parse_overwrite(
    const_mem_region_t const bundle_config_blob,
    runtime_configuration_t * const config) {
    MANIFEST_TRACE_PUSH_FN();
    cJSON * const config_root = cJSON_Parse(&statics.json_ctx, CONST_MEM_REGION(.ptr = bundle_config_blob.ptr, .size = bundle_config_blob.size));
    if (NULL == config_root) {
        MANIFEST_TRACE_POP();
        return false;
    }
    process_runtime_configuration(config_root, config);
    // Since there are no required members of the config root node, this function is considered a success as long as the root parses.
    MANIFEST_TRACE_POP();
    return true;
}

void manifest_config_parse_overwrite(
    runtime_configuration_t * const config) {
    MANIFEST_TRACE_PUSH_FN();
    if (statics.manifest_config_json) {
        process_runtime_configuration(statics.manifest_config_json, config);
    }
    MANIFEST_TRACE_POP();
}

mem_region_t manifest_alloc_bundle_file_blob(uint64_t size) {
    MANIFEST_TRACE_PUSH_FN();
    if (size == 0) {
        MANIFEST_TRACE_POP();
        return (mem_region_t){0}; // not found or empty
    }

    mem_region_t region = MEM_REGION(.ptr = heap_alloc(&statics.heap, size, MALLOC_TAG), .size = size);
    MANIFEST_TRACE_POP();
    return region;
}

bool manifest_is_empty(const manifest_t * const manifest) {
    MANIFEST_TRACE_PUSH_FN();

    manifest_t empty = {0};
    const bool status = (memcmp(manifest, &empty, sizeof(manifest_t)) == 0);
    MANIFEST_TRACE_POP();
    return status;
}

void manifest_set_bundle_selector(uint32_t (*select_func)(uint32_t total)) {
    if (select_func) {
        statics.select_bundle_fn = select_func;
    } else {
        statics.select_bundle_fn = &select_random_seed_with_device_id;
    }
}

// `manifest_config_json` points to the `runtime_config` portion of the manifest, platform_settings
// are located under `sys_params -> platform_settings`. From there, we look for a key with name `@platform`
// and within that object a key with name `@key`
static cJSON * manifest_get_platform_settings_obj(const char * const platform) {
    MANIFEST_TRACE_PUSH_FN();

    if (!statics.manifest_config_json) {
        MANIFEST_TRACE_POP();
        return NULL;
    }

    cJSON * const sys_params_obj = cJSON_GetObjectItem(statics.manifest_config_json, c_sys_params);

    if (!(sys_params_obj && cJSON_IsObject(sys_params_obj))) {
        MANIFEST_TRACE_POP();
        return NULL;
    }

    cJSON * const platform_settings_obj = cJSON_GetObjectItem(sys_params_obj, c_platform_settings);

    if (!(platform_settings_obj && cJSON_IsObject(platform_settings_obj))) {
        MANIFEST_TRACE_POP();
        return NULL;
    }

    cJSON * const requested_settings_obj = cJSON_GetObjectItem(platform_settings_obj, platform);

    if (!(requested_settings_obj && cJSON_IsObject(requested_settings_obj))) {
        MANIFEST_TRACE_POP();
        return NULL;
    }

    MANIFEST_TRACE_POP();
    return requested_settings_obj;
}

static cJSON * search_heirarchy(cJSON * base_obj, const char * const key) {
    MANIFEST_TRACE_PUSH_FN();

    ASSERT(base_obj);

    int num_children = cJSON_GetArraySize(base_obj);

    cJSON * curr_obj = base_obj;
    cJSON * found_obj;

    if (!cJSON_HasObjectItem(curr_obj, key)) {
        curr_obj = curr_obj->child;
        for (int i = 0; i < num_children; ++i) {
            if (cJSON_HasObjectItem(curr_obj, key)) {
                break;
            }
            if (cJSON_GetArraySize(curr_obj) > 0) {
                found_obj = search_heirarchy(curr_obj->child, key);
                if (found_obj != NULL) {
                    break;
                }
            }
            curr_obj = curr_obj->next;
        }
    }

    MANIFEST_TRACE_POP();
    return curr_obj;
}

static cJSON * manifest_get_object_from_heirarchy(cJSON * base_obj, const char * const key) {
    MANIFEST_TRACE_PUSH_FN();

    ASSERT(cJSON_IsObject(base_obj));

    int num_children = cJSON_GetArraySize(base_obj);

    cJSON * curr_obj = base_obj;
    cJSON * found_obj;

    if (!cJSON_HasObjectItem(curr_obj, key)) {
        curr_obj = curr_obj->child;
        for (int i = 0; i < num_children; ++i) {
            found_obj = search_heirarchy(curr_obj, key);
            if (found_obj != NULL) {
                cJSON * const node = cJSON_GetObjectItem(found_obj, key);
                MANIFEST_TRACE_POP();
                return node;
            }
            curr_obj = curr_obj->next;
        }
    }

    cJSON * const node = cJSON_GetObjectItem(curr_obj, key);
    MANIFEST_TRACE_POP();
    return node;
}

bool manifest_get_platform_setting_int(const char * const platform, const char * const key, int * out_value) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const settings_obj = manifest_get_platform_settings_obj(platform);

    if (settings_obj == NULL) {
        MANIFEST_TRACE_POP();
        return false;
    }

    cJSON * const value = manifest_get_object_from_heirarchy(settings_obj, key);

    if (!(out_value && cJSON_IsNumber(value))) {
        MANIFEST_TRACE_POP();
        return false;
    }

    *out_value = value->valueint;
    MANIFEST_TRACE_POP();
    return true;
}

bool manifest_get_platform_setting_bool(const char * const platform, const char * const key, bool * out_value) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const settings_obj = manifest_get_platform_settings_obj(platform);

    if (settings_obj == NULL) {
        MANIFEST_TRACE_POP();
        return false;
    }

    cJSON * const value = manifest_get_object_from_heirarchy(settings_obj, key);

    if (!(out_value && cJSON_IsBool(value))) {
        MANIFEST_TRACE_POP();
        return false;
    }

    *out_value = cJSON_IsTrue(value);
    MANIFEST_TRACE_POP();
    return true;
}

bool manifest_get_platform_setting_double(const char * const platform, const char * const key, double * out_value) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const settings_obj = manifest_get_platform_settings_obj(platform);

    if (settings_obj == NULL) {
        MANIFEST_TRACE_POP();
        return false;
    }

    cJSON * const value = manifest_get_object_from_heirarchy(settings_obj, key);

    if (!(out_value && cJSON_IsNumber(value))) {
        MANIFEST_TRACE_POP();
        return false;
    }

    *out_value = value->valuedouble;
    MANIFEST_TRACE_POP();
    return true;
}

bool manifest_get_platform_setting_string(const char * const platform, const char * const key, char ** out_value) {
    MANIFEST_TRACE_PUSH_FN();

    cJSON * const settings_obj = manifest_get_platform_settings_obj(platform);

    if (settings_obj == NULL) {
        MANIFEST_TRACE_POP();
        return false;
    }

    cJSON * const value = manifest_get_object_from_heirarchy(settings_obj, key);

    if (!(out_value && cJSON_IsString(value))) {
        MANIFEST_TRACE_POP();
        return false;
    }

    *out_value = cJSON_GetStringValue(value);
    MANIFEST_TRACE_POP();
    return true;
}
