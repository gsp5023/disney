/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
    ADK reporting sentry.
*/

#pragma once

#include "adk_reporting_send_queue.h"
#include "extern/cjson/cJSON.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/reporting/adk_reporting_sentry_options.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENTRY_EVENT_ID_LENGTH 32
#define MAX_SENTRY_MESSAGE_LENGTH 8192

static const char sentry_client[] = "adk_reporting_sentry";
static const char sentry_version[] = "7";
static const char sentry_platform[] = "native";
static const char sentry_environment[] = "dev";

// --- Testing purposes
typedef void (*adk_reporting_sent_status_t)(const bool success, const char * const error_message, void * data);
typedef void (*adk_report_sentry_override_send_cb_t)(adk_httpx_client_t * const client, const char * endpoint, const char ** headers, const char * body, void * data);
// ---

typedef struct adk_reporting_instance_t {
    heap_t * heap;
    sb_mutex_t * heap_mutex;
    adk_httpx_client_t * httpx_client;
    adk_reporting_send_queue_t * send_queue;
    cJSON_Env json_ctx;

    const char * reporter_name;
    char * sentry_event_endpoint;
    char * sentry_hostname;
    char * x_sentry_auth_header;
    adk_reporting_key_val_t * instance_tags;

    cJSON * event_contexts;
    adk_reporting_event_level_e min_report_level;

    adk_system_metrics_t system_metrics;

    // --- Testing purposes
    adk_report_sentry_override_send_cb_t override_send_cb;
    adk_reporting_sent_status_t sent_status;
    void * sent_status_extra_data;
    void * override_send_extra_data;
    // ---
} adk_reporting_sentry_instance_t;

void adk_reporting_sentry_tag_push(adk_reporting_instance_t * const instance, adk_reporting_key_val_t ** tag, const char * const key, const char * const value);
void adk_reporting_sentry_tags_free(adk_reporting_instance_t * const instance, adk_reporting_key_val_t ** tag);

void adk_reporting_sentry_get_event_id(char * const out_event_id, const size_t event_id_buffer_size);
char * adk_reporting_sentry_get_base_endpoint(adk_reporting_instance_t * const instance, const adk_reporting_url_info_t * const url_info);
char * adk_reporting_sentry_get_auth_header(adk_reporting_instance_t * const instance, const adk_reporting_url_info_t * const url_info);
char * adk_reporting_sentry_get_transaction(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func);
char * create_sentry_x_auth_header(heap_t * const heap, const adk_reporting_url_info_t * const url_info);

///
/// JSON BUILDERS
///

cJSON * adk_reporting_build_event_json(adk_reporting_instance_t * instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags);
bool adk_reporting_append_message_json(cJSON_Env * const ctx, const char * const message, cJSON ** event);
bool adk_reporting_append_exception_json(cJSON_Env * const ctx, const char * const error_type, const char * const error_message, void ** stacktrace, const size_t stack_size, cJSON ** event);

void * adk_reporting_malloc(adk_reporting_sentry_instance_t * instance, size_t size, const char * const tag);

void * adk_reporting_realloc(adk_reporting_sentry_instance_t * instance, void * ptr, size_t size, const char * const tag);

void adk_reporting_free(adk_reporting_sentry_instance_t * instance, void * ptr, const char * const tag);

// --- Below for testing purposes.
// Set callback on instance with result of sending event to sentry
void adk_reporting_sentry_sent_status_set(adk_reporting_instance_t * const instance, adk_reporting_sent_status_t sent_status, void * data);
// Set callback to override sending an event to sentry (plain JSON event submission)
void adk_reporting_sentry_override_send_set(adk_reporting_instance_t * const instance, adk_report_sentry_override_send_cb_t override_send, void * data);
// ---

#ifdef __cplusplus
}
#endif