/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_system_metrics_posix.h
steamboat static system metrics functions
*/

#pragma once

#include "source/adk/runtime/app/app.h"

#ifdef __cplusplus
extern "C" {
#endif

    /*
     * PARTNER TODO:
     *  1) complete TODO lines at bottom of file.
     *  2) define one of:
     *     NUUDAYDK_YS4000 (default)
     *     NUUDAYDK_YS5000 
     */

#define SB_METRICS_PARTNER_GUID "0e0de8ec-bdc3-48cf-8941-bc073d32eacd"
#define NUUDAYDK_YS5000

#if defined(NUUDAYDK_YS5000) //NUUDAYDK_YS5000
#define SB_METRICS_DEVICE "ys-5000"
#define SB_METRICS_VENDOR "humax"
#else //NUUDAYDK_YS4000
#define SB_METRICS_DEVICE "ys-4000"
#define SB_METRICS_VENDOR "humax"
#endif // defined(NUUDAYDK_YS5000)

#define SB_METRICS_PARTNER "nuuday_dk"

#define SB_METRICS_FIRMWARE "partner provided" //TODO: Fill this in

#define SB_METRICS_SOFTWARE "partner provided" //TODO: Fill this in
#define SB_METRICS_REVISION "partner provided" //TODO: Fill this in
#define SB_METRICS_CPU "partner provided" //TODO: Fill this in
#define SB_METRICS_GPU "partner provided" //TODO: Fill this in

#define SB_METRICS_REGION "dk"
#define SB_METRICS_ADVERTISING_ID "" //TODO: Fill this in



enum {
    // num cores, main memory mbytes and num hardware threads are handled inline
    sb_metrics_video_memory_mbytes = 1024, //TODO: Fill this in
    sb_metrics_persistent_storage_available_bytes = 0, //TODO: Fill this in
    sb_metrics_persistent_storage_max_write_bps = 0, //TODO: Fill this in
    sb_metrics_texture_format = adk_gpu_ready_texture_format_etc1, //TODO: Fill this in
    sb_metrics_device_class = adk_device_class_stb
};

#ifdef __cplusplus
}
#endif
