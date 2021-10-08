/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
adk_reporting_sentry.c

adk_reporting implementation to upload to sentry
*/

#include "source/adk/reporting/private/adk_reporting_sentry.h"

#include "extern/cjson/cJSON.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/crypto/crypto.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/http/private/adk_http_utils.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/reporting/adk_reporting_sentry_options.h"
#include "source/adk/reporting/adk_reporting_utils.h"
#include "source/adk/reporting/private/adk_reporting_send_queue.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/hosted_app.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"

enum {
    adk_reporting_max_valid_retry_delay_seconds = 60 * 5 // will not queue retry if greater than this
};

enum http_status_codes {
    HTTP_STATUS_SUCCESS = 200,
    HTTP_STATUS_RETRY = 429
};

static cJSON * adk_reporting_sentry_contexts_generate(cJSON_Env * const ctx, heap_t * const heap, const adk_system_metrics_t * const metrics);

// cJSON hooks
static void * cjson_malloc(void * const ctx, size_t const size) {
    adk_reporting_instance_t * const instance = (adk_reporting_instance_t *)ctx;
    return adk_reporting_malloc(instance, size, MALLOC_TAG);
}

static void cjson_free(void * const ctx, void * const ptr) {
    adk_reporting_instance_t * const instance = (adk_reporting_instance_t *)ctx;
    adk_reporting_free(instance, ptr, MALLOC_TAG);
}

static void generate_sentry_endpoint_info(adk_reporting_instance_t * const instance, const char * const sentry_dsn) {
    adk_reporting_url_info_t * sentry_url_info = adk_reporting_parse_href(instance, sentry_dsn, MALLOC_TAG);
    instance->x_sentry_auth_header = adk_reporting_sentry_get_auth_header(instance, sentry_url_info);

    static const char store_endpoint[] = "store/"; // NOTE: Must contain slash at end;
    char * const sentry_base_endpoint = adk_reporting_sentry_get_base_endpoint(instance, sentry_url_info);
    const size_t sentry_event_endpoint_buff_size = strlen(sentry_base_endpoint) + strlen(store_endpoint) + 1;
    instance->sentry_event_endpoint = adk_reporting_malloc(instance, sentry_event_endpoint_buff_size, MALLOC_TAG);
    sprintf_s(instance->sentry_event_endpoint, sentry_event_endpoint_buff_size, "%s%s", sentry_base_endpoint, store_endpoint);
    adk_reporting_free(instance, sentry_base_endpoint, MALLOC_TAG);

    const size_t sentry_hostname_buff_size = strlen(sentry_url_info->host_info.host) + 1;
    instance->sentry_hostname = adk_reporting_malloc(instance, sentry_hostname_buff_size, MALLOC_TAG);
    strcpy_s(instance->sentry_hostname, sentry_hostname_buff_size, sentry_url_info->host_info.host);

    adk_reporting_free_url_info(instance, sentry_url_info, MALLOC_TAG);
}

void * adk_reporting_malloc(adk_reporting_sentry_instance_t * instance, size_t size, const char * const tag) {
    ASSERT(instance != NULL);
    ASSERT(instance->heap != NULL);
    ASSERT(instance->heap_mutex != NULL);
    sb_lock_mutex(instance->heap_mutex);
    void * mem = heap_alloc(instance->heap, size, tag);
    sb_unlock_mutex(instance->heap_mutex);
    return mem;
}

void * adk_reporting_realloc(adk_reporting_sentry_instance_t * instance, void * ptr, size_t size, const char * const tag) {
    ASSERT(instance != NULL);
    ASSERT(instance->heap != NULL);
    ASSERT(instance->heap_mutex != NULL);
    sb_lock_mutex(instance->heap_mutex);
    void * mem = heap_realloc(instance->heap, ptr, size, tag);
    sb_unlock_mutex(instance->heap_mutex);
    return mem;
}

void adk_reporting_free(adk_reporting_sentry_instance_t * instance, void * ptr, const char * const tag) {
    ASSERT(instance != NULL);
    ASSERT(instance->heap != NULL);
    ASSERT(instance->heap_mutex != NULL);
    if (ptr) {
        sb_lock_mutex(instance->heap_mutex);
        heap_free(instance->heap, ptr, MALLOC_TAG);
        sb_unlock_mutex(instance->heap_mutex);
    }
}

adk_reporting_sentry_instance_t * adk_reporting_instance_create(const adk_reporting_init_options_t * const options) {
    ASSERT(options != NULL);
    ASSERT(options->sentry_dsn != NULL);
    ASSERT(options->reporter_name != NULL);
    ASSERT(options->httpx_client != NULL);
    ASSERT(options->heap != NULL);
    ASSERT(options->min_report_level > event_level_unknown && options->min_report_level <= event_level_fatal);

    adk_reporting_sentry_instance_t * instance = heap_alloc(options->heap, sizeof(adk_reporting_sentry_instance_t), MALLOC_TAG);
    ZEROMEM(instance);

    instance->heap_mutex = sb_create_mutex(MALLOC_TAG);
    instance->heap = options->heap;
    instance->httpx_client = options->httpx_client;

    instance->send_queue = adk_reporting_create_send_queue(instance->heap, instance->heap_mutex, options->send_queue_size);

    generate_sentry_endpoint_info(instance, options->sentry_dsn);

    instance->json_ctx = (cJSON_Env){
        .ctx = instance,
        .callbacks = {
            .malloc = cjson_malloc,
            .free = cjson_free,
        }};

    const size_t reporter_name_buffer_length = strlen(options->reporter_name) + 1;
    instance->reporter_name = heap_alloc(instance->heap, reporter_name_buffer_length, MALLOC_TAG);
    strcpy_s((char *)instance->reporter_name, reporter_name_buffer_length, options->reporter_name);

    adk_get_system_metrics(&instance->system_metrics);

    instance->event_contexts = adk_reporting_sentry_contexts_generate(&instance->json_ctx, instance->heap, &instance->system_metrics);
    instance->min_report_level = options->min_report_level;

    adk_reporting_instance_push_tag(instance, "partner", instance->system_metrics.partner);
    adk_reporting_instance_push_tag(instance, "config", instance->system_metrics.config);

    return instance;
}

static void free_send_queue(adk_reporting_sentry_instance_t * const instance) {
    ASSERT(instance->heap != NULL);
    ASSERT(instance->heap_mutex != NULL);
    if (instance->send_queue) {
        adk_reporting_send_node_t * head = NULL;
        sb_lock_mutex(instance->send_queue->mutex);
        head = instance->send_queue->head;
        instance->send_queue->head = NULL;
        instance->send_queue->tail = NULL;
        while (head) {
            adk_reporting_send_node_t * stale = head;
            head = head->next;
            cJSON_Delete(&instance->json_ctx, stale->event);
            adk_reporting_free(instance, stale, MALLOC_TAG);
        }
        sb_unlock_mutex(instance->send_queue->mutex);

        sb_destroy_mutex(instance->send_queue->mutex, MALLOC_TAG);

        adk_reporting_free(instance, instance->send_queue, MALLOC_TAG);

        instance->send_queue = NULL;
    }
}

void adk_reporting_instance_free(adk_reporting_sentry_instance_t * const instance) {
    ASSERT(instance != NULL);

    free_send_queue(instance);
    instance->sent_status = NULL;
    instance->sent_status_extra_data = NULL;
    instance->override_send_cb = NULL;
    instance->override_send_extra_data = NULL;

    heap_t * const heap = instance->heap;
    adk_reporting_free(instance, instance->sentry_hostname, MALLOC_TAG);
    instance->sentry_hostname = NULL;
    adk_reporting_free(instance, instance->sentry_event_endpoint, MALLOC_TAG);
    instance->sentry_event_endpoint = NULL;
    adk_reporting_free(instance, instance->x_sentry_auth_header, MALLOC_TAG);
    instance->x_sentry_auth_header = NULL;
    adk_reporting_free(instance, (char *)instance->reporter_name, MALLOC_TAG);
    instance->reporter_name = NULL;
    adk_reporting_instance_clear_tags(instance);
    cJSON_Delete(&instance->json_ctx, instance->event_contexts);
    sb_destroy_mutex(instance->heap_mutex, MALLOC_TAG);
    heap_free(heap, instance, MALLOC_TAG);
}

char * adk_reporting_sentry_get_auth_header(adk_reporting_instance_t * const instance, const adk_reporting_url_info_t * const url_info) {
    ASSERT(instance);
    ASSERT(url_info && url_info->auth_info.auth);

    static const char x_sentry_auth_first[] = "Sentry sentry_key=";
    static const char x_sentry_auth_second[] = ",sentry_version=";
    static const char x_sentry_auth_third[] = ",sentry_client=";

    const size_t x_sentry_auth_buffer_length = strlen(x_sentry_auth_first) + strlen(url_info->auth_info.auth) + strlen(x_sentry_auth_second) + strlen(sentry_version) + strlen(x_sentry_auth_third) + strlen(sentry_client) + 1;

    char * const x_sentry_auth_header = adk_reporting_malloc(instance, x_sentry_auth_buffer_length, MALLOC_TAG);

    sprintf_s(x_sentry_auth_header, x_sentry_auth_buffer_length, "%s%s%s%s%s%s", x_sentry_auth_first, url_info->auth_info.auth, x_sentry_auth_second, sentry_version, x_sentry_auth_third, sentry_client);

    return x_sentry_auth_header;
}

char * adk_reporting_sentry_get_base_endpoint(adk_reporting_instance_t * const instance, const adk_reporting_url_info_t * const url_info) {
    ASSERT(instance);
    ASSERT(url_info && url_info->origin && url_info->path_info.path);
    // Create sentry base URL
    /*
        * DSN format
        {PROTOCOL}://{PUBLIC_KEY}@{HOST}/{PROJECT_ID}
        {BASE_URI} = {PROTOCOL}://{HOST}

        * Final Endpoint
        {BASE_URI}/api/{PROJECT_ID}/{ENDPOINT}/

        href.origin + /api/ + href.path + / + [endpoint/]
    */
    static const char * api_path = "/api";
    static const size_t api_path_length = 4;
    static const char * slash = "/";
    static const size_t slash_length = 1;

    const size_t sentry_base_endpoint_buffer_length = strlen(url_info->origin) + api_path_length + strlen(url_info->path_info.path) + slash_length + 1;
    char * sentry_base_endpoint = adk_reporting_malloc(instance, sentry_base_endpoint_buffer_length, MALLOC_TAG);
    sprintf_s(sentry_base_endpoint, sentry_base_endpoint_buffer_length, "%s%s%s%s", url_info->origin, api_path, url_info->path_info.path, slash);

    return sentry_base_endpoint;
}

// format: [file]::[func].line
char * adk_reporting_sentry_get_transaction(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func) {
    static const char double_colon[] = "::";
    static const size_t double_colon_length = 2;
    static const char dot[] = ".";
    static const size_t dot_length = 1;

    const char * const line_num = VAPRINTF("%d", line);
    const size_t file_length = strlen(file);
    const size_t line_num_length = strlen(line_num);
    const size_t func_length = strlen(func);
    const size_t transaction_buffer_length = file_length + double_colon_length + func_length + dot_length + line_num_length + 1;

    char * const transaction = adk_reporting_malloc(instance, transaction_buffer_length, MALLOC_TAG);
    memcpy(transaction, file, file_length);
    memcpy(transaction + file_length, double_colon, double_colon_length);
    memcpy(transaction + file_length + double_colon_length, func, func_length);
    memcpy(transaction + file_length + double_colon_length + func_length, dot, dot_length);
    memcpy(transaction + file_length + double_colon_length + func_length + dot_length, line_num, line_num_length);
    transaction[transaction_buffer_length - 1] = 0;

    return transaction;
}

void adk_reporting_sentry_get_event_id(char * const out_event_id, const size_t event_id_buffer_size) {
    ASSERT(out_event_id != NULL);
    ASSERT(event_id_buffer_size > SENTRY_EVENT_ID_LENGTH);

    const sb_uuid_t uuid = sb_generate_uuid();
    uint8_t encoded[SENTRY_EVENT_ID_LENGTH + 1];
    const mem_region_t encoded_region = MEM_REGION(.byte_ptr = encoded, .size = ARRAY_SIZE(encoded));
    const const_mem_region_t input_region = CONST_MEM_REGION(.byte_ptr = uuid.id, .size = ARRAY_SIZE(uuid.id));
    const uint8_t * p = crypto_encode_hex(input_region, encoded_region);
    memcpy(out_event_id, p, ARRAY_SIZE(encoded));
}

#define JSON_FAILED(env, json, ...)       \
    do {                                  \
        if (json != NULL) {               \
            cJSON_Delete(env, json);      \
        }                                 \
        ASSERT_MSG(false, ##__VA_ARGS__); \
    } while (0)

cJSON * adk_reporting_build_event_json(adk_reporting_instance_t * instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags) {
    cJSON_Env * const ctx = &instance->json_ctx;
    cJSON * const event = cJSON_CreateObject(ctx);

    // EVENT
    char event_id[SENTRY_EVENT_ID_LENGTH + 1];
    adk_reporting_sentry_get_event_id(event_id, SENTRY_EVENT_ID_LENGTH + 1);
    if (cJSON_AddStringToObject(ctx, event, "event_id", event_id) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add event_id string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddStringToObject(ctx, event, "release", "ncp-core@" ADK_VERSION_STRING) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add release string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddStringToObject(ctx, event, "environment", instance->system_metrics.tenancy) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add environment string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddStringToObject(ctx, event, "platform", sentry_platform) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add platform string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddStringToObject(ctx, event, "logger", instance->reporter_name) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add logger string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddStringToObject(ctx, event, "level", string_from_event_level(level)) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add level string to sentry_event_json");
        return NULL;
    }
    if (cJSON_AddNumberToObject(ctx, event, "timestamp", sb_get_time_since_epoch().seconds) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add timestamp string to sentry_event_json");
        return NULL;
    }
    char * const transaction = adk_reporting_sentry_get_transaction(instance, file, line, func);
    if (cJSON_AddStringToObject(ctx, event, "transaction", transaction) == NULL) {
        JSON_FAILED(ctx, event, "Failed to add transaction string to sentry_event_json");
        return NULL;
    }
    adk_reporting_free(instance, transaction, MALLOC_TAG);

    // TAGS
    if (tags != NULL || instance->instance_tags != NULL) {
        cJSON * const tags_json = cJSON_AddObjectToObject(ctx, event, "tags");
        if (tags_json == NULL) {
            JSON_FAILED(ctx, event, "Failed to create tags object in sentry_event_json");
            return NULL;
        }

        adk_reporting_key_val_t * instance_tags = instance->instance_tags;
        while (instance_tags != NULL) {
            if (cJSON_AddStringToObject(ctx, tags_json, instance_tags->key, instance_tags->value) == NULL) {
                JSON_FAILED(ctx, event, "Failed to add %s:%s to sentry_event_json:tags", instance_tags->key, instance_tags->value);
                return NULL;
            }
            instance_tags = instance_tags->next;
        }

        adk_reporting_key_val_t * func_tags = tags;
        while (func_tags != NULL) {
            if (cJSON_AddStringToObject(ctx, tags_json, func_tags->key, func_tags->value) == NULL) {
                JSON_FAILED(ctx, event, "Failed to add %s:%s to sentry_event_json:tags", func_tags->key, func_tags->value);
                return NULL;
            }
            func_tags = func_tags->next;
        }
    }

    if (!cJSON_HasObjectItem(instance->event_contexts, "app")) {
        adk_app_metrics_t app_info;
        if (adk_app_metrics_get(&app_info) == adk_app_metrics_success) {
            cJSON * const app_context = cJSON_AddObjectToObject(ctx, instance->event_contexts, "app");
            cJSON_AddStringToObject(ctx, app_context, "app_identifier", app_info.app_id);
            cJSON_AddStringToObject(ctx, app_context, "app_name", app_info.app_name);
            cJSON_AddStringToObject(ctx, app_context, "app_version", app_info.app_version);
        }
    }

    // "contexts" object will not be deleted during cJSON_Delete. Reference of instance->event_contexts
    cJSON_AddItemReferenceToObject(ctx, event, "contexts", instance->event_contexts);

    return event;
}

bool adk_reporting_append_message_json(cJSON_Env * const ctx, const char * const message, cJSON ** event) {
    cJSON * const message_json = cJSON_AddObjectToObject(ctx, *event, "message");
    if (message_json == NULL) {
        return false;
    }
    if (cJSON_AddStringToObject(ctx, message_json, "formatted", message) == NULL) {
        return false;
    }
    return true;
}

bool adk_reporting_append_exception_json(cJSON_Env * const ctx, const char * const error_type, const char * const error_message, void ** stacktrace, const size_t stack_size, cJSON ** event) {
    cJSON * const exception = cJSON_AddObjectToObject(ctx, *event, "exception");
    if (exception == NULL) {
        return false;
    }
    cJSON * const values_arr = cJSON_AddArrayToObject(ctx, exception, "values");
    if (values_arr == NULL) {
        return false;
    }
    cJSON * const value_ob = cJSON_CreateObject(ctx);
    if (value_ob == NULL) {
        return false;
    }
    cJSON_AddItemToArray(values_arr, value_ob);

    if (cJSON_AddStringToObject(ctx, value_ob, "type", error_type) == NULL) {
        return false;
    }
    if (cJSON_AddStringToObject(ctx, value_ob, "value", error_message) == NULL) {
        return false;
    }

    if (stacktrace != NULL) {
        cJSON * const stacktrace_ob = cJSON_AddObjectToObject(ctx, value_ob, "stacktrace");
        if (stacktrace_ob == NULL) {
            return false;
        }
        cJSON * const frame_arr = cJSON_AddArrayToObject(ctx, stacktrace_ob, "frames");
        if (frame_arr == NULL) {
            return false;
        }

        for (size_t i = 0; i < stack_size; i++) {
            cJSON * const frame_ob = cJSON_CreateObject(ctx);
            if (frame_ob == NULL) {
                return false;
            }
            cJSON_AddItemToArray(frame_arr, frame_ob);

            if (cJSON_AddStringToObject(ctx, frame_ob, "instruction_addr", VAPRINTF("0x%" PRIx64 "", (uint64_t)(size_t)stacktrace[i])) == NULL) {
                return false;
            }
        }
    }

    return true;
}

#ifdef _RESTRICTED
#include "source/adk/steamboat/restricted/sb_unwind.h"

typedef struct debug_image_info_extra_t {
    bool success;
    cJSON * event;
    cJSON_Env * const env;
} debug_image_info_extra_t;

void adk_reporting_append_debug_meta_json(void * const ctx, sb_executable_image_info_t * info) {
    debug_image_info_extra_t * extra = (debug_image_info_extra_t *)ctx;
    cJSON_Env * const env = extra->env;

    cJSON * debug = cJSON_AddObjectToObject(env, extra->event, "debug_meta");
    cJSON * const image_arr = cJSON_AddArrayToObject(env, debug, "images");
    if (image_arr == NULL) {
        return;
    }

    cJSON * const image_ob = cJSON_CreateObject(env);
    if (image_ob == NULL) {
        return;
    }
    cJSON_AddItemToArray(image_arr, image_ob);
    if (info->debug_file && cJSON_AddStringToObject(env, image_ob, "debug_file", info->debug_file) == NULL) {
        return;
    }
    if (cJSON_AddStringToObject(env, image_ob, "code_file", info->name) == NULL) {
        return;
    }
    if (cJSON_AddStringToObject(env, image_ob, "debug_id", info->debug_id) == NULL) {
        return;
    }
    if (cJSON_AddStringToObject(env, image_ob, "type", info->type) == NULL) {
        return;
    }
    if (cJSON_AddStringToObject(env, image_ob, "image_addr", VAPRINTF("0x%" PRIx64 "", (uint64_t)info->address)) == NULL) {
        return;
    }
    if (cJSON_AddNumberToObject(env, image_ob, "image_size", (double)info->size) == NULL) {
        return;
    }

    extra->success = true;
}
#endif

static bool is_retry_eligible(const adk_httpx_response_t * const response, int32_t * seconds_delay) {
    static const char retry_after_header_key[] = "Retry-After";
    if (adk_httpx_response_get_response_code(response) != HTTP_STATUS_RETRY) {
        return false;
    }

    const_mem_region_t retry_after_value = http_parse_header_for_key(retry_after_header_key, adk_httpx_response_get_headers(response));
    if (retry_after_value.ptr == NULL) {
        debug_write_line("Reporting blocked because the server's rate limit was exceeded, but no retry time was given.  Will NOT retry.");
        return false;
    }

    // Parse retry_after_value value as integer
    char retry_after_length_value_buffer[128] = {0};
    memcpy(retry_after_length_value_buffer, retry_after_value.ptr, retry_after_value.size);
    *seconds_delay = (int32_t)strtol(retry_after_length_value_buffer, NULL, 10);

    if (*seconds_delay > adk_reporting_max_valid_retry_delay_seconds) {
        debug_write_line(
            "Reporting blocked because the server's rate limit was exceeded, "
            "but the retry delay (%d seconds) exceeds the maximum delay "
            "(%d seconds).  Will NOT retry.",
            *seconds_delay,
            adk_reporting_max_valid_retry_delay_seconds);
        return false;
    }

    debug_write_line("Reporting blocked because the server's rate limit was exceeded.  Will queue retry for %d seconds", *seconds_delay);
    return true;
}

typedef struct adk_reporting_response_data_t {
    adk_reporting_instance_t * instance;
    cJSON * event;
} adk_reporting_response_data_t;

static void event_sent_on_complete_cb(adk_httpx_response_t * const response, void * userdata) {
    ASSERT(response != NULL);
    ASSERT(userdata != NULL);

    adk_reporting_response_data_t * data = (adk_reporting_response_data_t *)userdata;
    ASSERT(data->event);
    ASSERT(data->instance);

    adk_reporting_instance_t * instance = data->instance;

    if (adk_httpx_response_get_status(response) != adk_future_status_ready) {
        if (instance->sent_status != NULL) {
            instance->sent_status(false, VAPRINTF("ERROR: httpx returned a status other than adk_future_status_ready (%d) in the ...on_complete() callback.", adk_future_status_ready), instance->sent_status_extra_data);
        }
        debug_write_line("ERROR: httpx returned a status other than adk_future_status_ready (%d) in the ...on_complete() callback.", adk_future_status_ready);
    } else if (adk_httpx_response_get_response_code(response) != HTTP_STATUS_SUCCESS) {
        int32_t seconds_delay;
        if (is_retry_eligible(response, &seconds_delay)) {
            adk_reporting_pause_sending_queue(instance, seconds_delay);

            adk_reporting_enqueue_to_send(instance, data->event);
            data->event = NULL;
            if (instance->sent_status != NULL) {
                instance->sent_status(false, VAPRINTF("The server is currently not accepting events.  Re-queueing event to be sent in %u seconds.", seconds_delay), instance->sent_status_extra_data);
            }
            debug_write_line("The server is currently not accepting events.  Re-queueing event to be sent in %u seconds.", seconds_delay);
        } else {
            if (instance->sent_status != NULL) {
                instance->sent_status(false, VAPRINTF("The adk_reporting upload to sentry failed with HTTP code %" PRId64 ".  The event will NOT be resent.", adk_httpx_response_get_response_code(response)), instance->sent_status_extra_data);
            }
            debug_write_line("The adk_reporting upload to sentry failed with HTTP code %" PRId64 ".  The event will NOT be resent.", adk_httpx_response_get_response_code(response));
        }
    } else {
        if (instance->sent_status != NULL) {
            instance->sent_status(true, NULL, instance->sent_status_extra_data);
        }
    }

    adk_httpx_response_free(response);
    if (data->event) {
        cJSON_Delete(&instance->json_ctx, data->event);
    }
    adk_reporting_free(instance, userdata, MALLOC_TAG);
}

static void adk_reporting_post_event(adk_reporting_instance_t * const instance, cJSON * const event) {
    cJSON_Env * const ctx = &instance->json_ctx;
    adk_reporting_sent_status_t sent_status = instance->sent_status;
    // Serialize json
    char * json_event_body = cJSON_PrintUnformatted(ctx, event);
    if (json_event_body == NULL) {
        cJSON_Delete(ctx, event);
        if (sent_status != NULL) {
            sent_status(false, "Failed to create json string from sentry event json object", instance->sent_status_extra_data);
        }
        return;
    }

    const char * headers[] = {
        "Content-Type:application/json",
        VAPRINTF("%s:%d", "Content-Length", strlen(json_event_body)),
        VAPRINTF("%s:%s", "x-sentry-auth", instance->x_sentry_auth_header),
        VAPRINTF("%s:%s", "Host", instance->sentry_hostname)};
    const int header_size = ARRAY_SIZE(headers);

    // Exit early if override_send_cb is set to send event to cb instead
    if (instance->override_send_cb != NULL) {
        instance->override_send_cb(instance->httpx_client, instance->sentry_event_endpoint, headers, json_event_body, instance->override_send_extra_data);
        cJSON_Delete(ctx, event);
        cJSON_free(ctx, json_event_body);
        if (sent_status != NULL) {
            sent_status(true, "override_send_cb was used", instance->sent_status_extra_data);
        }
        return;
    }

    // Post event to sentry
    adk_httpx_request_t * post = adk_httpx_client_request(instance->httpx_client, adk_httpx_method_post, instance->sentry_event_endpoint);
    if (post == NULL) {
        cJSON_Delete(ctx, event);
        cJSON_free(ctx, json_event_body);
        if (sent_status != NULL) {
            sent_status(false, "Unable to create httpx client request", instance->sent_status_extra_data);
        }
        return;
    }
    for (int i = 0; i < header_size; i++) {
        adk_httpx_request_set_header(post, headers[i]);
    }

    adk_httpx_request_set_body(post, (const uint8_t *)json_event_body, strlen(json_event_body));

    adk_httpx_request_set_on_complete(post, event_sent_on_complete_cb);

    adk_reporting_response_data_t * const data = adk_reporting_malloc(instance, sizeof(adk_reporting_response_data_t), MALLOC_TAG);
    data->event = event;
    data->instance = instance;
    adk_httpx_request_set_userdata(post, (void *)data); // what to pass in?

    adk_httpx_response_t * response = adk_httpx_send(post);
    cJSON_free(ctx, json_event_body);
    if (response == NULL) {
        cJSON_Delete(ctx, event);
        if (sent_status != NULL) {
            sent_status(false, "Unable to send httpx client request", instance->sent_status_extra_data);
        }
        return;
    }
}

bool adk_reporting_tick(adk_reporting_instance_t * const instance) {
    ASSERT(instance != NULL);
    ASSERT(instance->httpx_client != NULL);

    // will return null if queue is paused
    adk_reporting_send_node_t * head = adk_reporting_flush_send_queue(instance, regard_pause);

    while (head != NULL) {
        adk_reporting_send_node_t * current = head;
        head = head->next;

        // send event
        adk_reporting_post_event(instance, current->event);

        // free event
        current->event = NULL;
        adk_reporting_free_send_node(instance, current);
    }

    if (adk_httpx_client_tick(instance->httpx_client)) {
        return true;
    }

    return !adk_reporting_is_send_queue_empty(instance);
}

void adk_reporting_report_msg(
    adk_reporting_instance_t * const instance,
    const char * const file,
    const int line,
    const char * const func,
    const adk_reporting_event_level_e level,
    adk_reporting_key_val_t * const tags,
    const char * const msg,
    va_list args) {
    adk_reporting_sent_status_t sent_status = instance->sent_status;
    if (level < instance->min_report_level) {
        if (sent_status != NULL) {
            sent_status(false, "Level is less than minimum reporting level. Skipping upload.", instance->sent_status_extra_data);
        }
        return;
    }

    // Build Event
    cJSON_Env * const ctx = &instance->json_ctx;
    cJSON * event = adk_reporting_build_event_json(instance, file, line, func, level, tags);

    // Append Message
    char message[MAX_SENTRY_MESSAGE_LENGTH];
    // Copying va_list for safe usage of vsnprintf (https://stackoverflow.com/a/37789384)
    va_list args_copy;
    va_copy(args_copy, args);
    vsnprintf(message, MAX_SENTRY_MESSAGE_LENGTH, msg, args_copy);
    va_end(args_copy);
    if (!adk_reporting_append_message_json(ctx, message, &event)) {
        JSON_FAILED(ctx, event, "Failed to append message to event");
        if (sent_status != NULL) {
            sent_status(false, "Failed to create event", instance->sent_status_extra_data);
        }
        return;
    }

    // Enqueue events to be sent when possible
    adk_reporting_enqueue_to_send(instance, event);
}

void adk_reporting_report_exception(
    adk_reporting_instance_t * const instance,
    const char * const file,
    const int line,
    const char * const func,
    const adk_reporting_event_level_e level,
    adk_reporting_key_val_t * const tags,
    void ** stacktrace,
    const size_t stack_size,
    const char * const error_type,
    const char * const error_message,
    va_list args) {
    adk_reporting_sent_status_t sent_status = instance->sent_status;
    if (level < instance->min_report_level) {
        if (sent_status != NULL) {
            sent_status(false, "Level is less than minimum reporting level. Skipping upload.", instance->sent_status_extra_data);
        }
        return;
    }

    // Build Event
    cJSON_Env * const ctx = &instance->json_ctx;
    cJSON * event = adk_reporting_build_event_json(instance, file, line, func, level, tags);

    // Append Exception
    char error[MAX_SENTRY_MESSAGE_LENGTH];
    vsnprintf(error, MAX_SENTRY_MESSAGE_LENGTH, error_message, args);
    if (!adk_reporting_append_exception_json(ctx, error_type, error, stacktrace, stack_size, &event)) {
        JSON_FAILED(ctx, event, "Failed to append exception to event");
        if (sent_status != NULL) {
            sent_status(false, "Failed to append exception to event", instance->sent_status_extra_data);
        }
        return;
    }

#ifdef _RESTRICTED
    debug_image_info_extra_t extra = {.event = event, .success = false, .env = ctx};
    const bool have_executable_image_info = sb_get_executable_image_info(&extra, adk_reporting_append_debug_meta_json);
    if (have_executable_image_info && !extra.success) {
        JSON_FAILED(ctx, event, "Failed to append debug_meta to event");
        if (sent_status != NULL) {
            sent_status(false, "Failed to append debug_meta to event", instance->sent_status_extra_data);
        }
        return;
    }
#endif

    // Enqueue event to be send later
    adk_reporting_enqueue_to_send(instance, event);
}

void adk_reporting_sentry_tag_push(adk_reporting_instance_t * const instance, adk_reporting_key_val_t ** tag, const char * const key, const char * const value) {
    adk_reporting_key_val_t ** head = tag;
    while (*head != NULL) {
        // duplicate tag, update value
        if (!strcmp((*head)->key, key)) {
            const size_t val_buffer_length = strlen(value) + 1;
            (*head)->value = (char *)adk_reporting_realloc(instance, (char *)(*head)->value, val_buffer_length, MALLOC_TAG);
            strcpy_s((char *)(*head)->value, val_buffer_length, value);
            return;
        }
        head = &(*head)->next;
    }

    *head = adk_reporting_malloc(instance, sizeof(adk_reporting_key_val_t), MALLOC_TAG);

    const size_t key_buffer_length = strlen(key) + 1;
    (*head)->key = (char *)adk_reporting_malloc(instance, key_buffer_length, MALLOC_TAG);
    strcpy_s((char *)(*head)->key, key_buffer_length, key);

    const size_t val_buffer_length = strlen(value) + 1;
    (*head)->value = (char *)adk_reporting_malloc(instance, val_buffer_length, MALLOC_TAG);
    strcpy_s((char *)(*head)->value, val_buffer_length, value);

    (*head)->next = NULL;
}

void adk_reporting_sentry_tags_free(adk_reporting_instance_t * const instance, adk_reporting_key_val_t ** tag) {
    if (tag == NULL || *tag == NULL) {
        return;
    }

    while (*tag != NULL) {
        adk_reporting_free(instance, (char *)(*tag)->key, MALLOC_TAG);
        adk_reporting_free(instance, (char *)(*tag)->value, MALLOC_TAG);
        adk_reporting_key_val_t * next = (*tag)->next;
        adk_reporting_free(instance, (adk_reporting_key_val_t *)*tag, MALLOC_TAG);
        *tag = next;
    }
}

void adk_reporting_instance_push_tag(adk_reporting_instance_t * const instance, const char * const key, const char * const value) {
    ASSERT(instance != NULL);
    adk_reporting_sentry_tag_push(instance, &instance->instance_tags, key, value);
}

void adk_reporting_instance_clear_tags(adk_reporting_instance_t * const instance) {
    ASSERT(instance != NULL);
    adk_reporting_sentry_tags_free(instance, &instance->instance_tags);
}

static cJSON * adk_reporting_sentry_contexts_generate(cJSON_Env * const ctx, heap_t * const heap, const adk_system_metrics_t * const metrics) {
    cJSON * context = cJSON_CreateObject(ctx);

    cJSON * device_contexts = cJSON_AddObjectToObject(ctx, context, "device");
    cJSON * os_context = cJSON_AddObjectToObject(ctx, context, "os");
    cJSON * gpu_context = cJSON_AddObjectToObject(ctx, context, "gpu");

    // DEVICE
    cJSON_AddStringToObject(ctx, device_contexts, "name", metrics->device);
    char * device_class;
    switch (metrics->device_class) {
        case adk_device_class_desktop_pc:
            device_class = "Desktop";
            break;
        case adk_device_class_game_console:
            device_class = "Console";
            break;
        case adk_device_class_stb:
            device_class = "STB";
            break;
        case adk_device_class_tv:
            device_class = "TV";
            break;
        case adk_device_class_mobile:
            device_class = "MOBILE";
            break;
        case adk_device_class_dvr:
        case adk_device_class_minature_sbc:
        default:
            device_class = "Unknown";
    }
    cJSON_AddStringToObject(ctx, device_contexts, "model", device_class);
    cJSON_AddStringToObject(ctx, device_contexts, "model_id", (const char *)metrics->device_id.bytes);
    cJSON_AddStringToObject(ctx, device_contexts, "arch", metrics->cpu);
    cJSON_AddNumberToObject(ctx, device_contexts, "memory_size", metrics->main_memory_mbytes);
    cJSON_AddNumberToObject(ctx, device_contexts, "num_cores", metrics->num_cores);
    cJSON_AddNumberToObject(ctx, device_contexts, "num_threads", metrics->num_hardware_threads);

    // OS
    cJSON_AddStringToObject(ctx, os_context, "name", metrics->software);
    cJSON_AddStringToObject(ctx, os_context, "version", metrics->revision);

    // GPU
    cJSON_AddStringToObject(ctx, gpu_context, "name", metrics->gpu);
    cJSON_AddNumberToObject(ctx, gpu_context, "memory_size", metrics->video_memory_mbytes);

    return context;
}

void adk_reporting_sentry_sent_status_set(adk_reporting_instance_t * const instance, adk_reporting_sent_status_t sent_status, void * data) {
    instance->sent_status = sent_status;
    instance->sent_status_extra_data = data;
}

void adk_reporting_sentry_override_send_set(adk_reporting_instance_t * const instance, adk_report_sentry_override_send_cb_t override_send, void * data) {
    instance->override_send_cb = override_send;
    instance->override_send_extra_data = data;
}
