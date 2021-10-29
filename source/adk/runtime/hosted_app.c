/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#include "hosted_app.h"
#include "source/adk/steamboat/sb_platform.h"

static struct hosted_app {
    adk_app_metrics_t info;
    sb_mutex_t * mutex;
    bool hosting_app;
} statics;

void adk_app_metrics_init() {
    statics.mutex = sb_create_mutex(MALLOC_TAG);
    statics.hosting_app = false;
    ZEROMEM(&statics.info);
}

void adk_app_metrics_shutdown() {
    statics.hosting_app = false;
    sb_destroy_mutex(statics.mutex, MALLOC_TAG);
}

void adk_app_metrics_report(
    const char * const app_id,
    const char * const app_name,
    const char * const app_version) {
    sb_lock_mutex(statics.mutex);
    strcpy(statics.info.app_id, app_id);
    strcpy(statics.info.app_name, app_name);
    strcpy(statics.info.app_version, app_version);
    sb_report_app_metrics(app_id, app_name, app_version);
    statics.hosting_app = true;
    sb_unlock_mutex(statics.mutex);
}

adk_app_metrics_result_e adk_app_metrics_get(adk_app_metrics_t * app_info) {
    sb_lock_mutex(statics.mutex);
    if(statics.hosting_app){
        strcpy(app_info->app_id, statics.info.app_id);
        strcpy(app_info->app_name, statics.info.app_name);
        strcpy(app_info->app_version, statics.info.app_version);
        sb_unlock_mutex(statics.mutex);
        return adk_app_metrics_success;
    }
    sb_unlock_mutex(statics.mutex);
    return adk_app_metrics_no_app;
}   

void adk_app_metrics_clear() {
    sb_lock_mutex(statics.mutex);
    statics.hosting_app = false;
    ZEROMEM(&statics.info);
    sb_unlock_mutex(statics.mutex);
}
