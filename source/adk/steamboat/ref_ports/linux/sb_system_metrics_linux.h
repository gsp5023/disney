/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_system_metrics_linux.h
steamboat static system metrics functions
*/

#pragma once

#include "source/adk/runtime/app/app.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PARTNER TODO: 
 * Implement static system metrics for appropriate platform.
 * Device ID, tenancy, and persona not defined here.
 */

/*
 * Default values for Linux
 */
#define SB_METRICS_PARTNER_GUID "0e0de8ec-bdc3-48cf-8941-bc073d32eacd"

#if defined(NUUDAYDK_YS5000) //NUUDAYDK_YS5000
#define SB_METRICS_DEVICE "ys-5000"
#define SB_METRICS_VENDOR "humax"
#else //NUUDAYDK_YS4000
#define SB_METRICS_DEVICE "ys-4000"
#define SB_METRICS_VENDOR "humax"
#endif // defined(NUUDAYDK_YS5000)

#define SB_METRICS_PARTNER "nuuday_dk"

#define SB_METRICS_FIRMWARE "0.1.0"
#define SB_METRICS_SOFTWARE "Debian"
#define SB_METRICS_REVISION "10"
#define SB_METRICS_GPU "nvidia"
#define SB_METRICS_REGION "dk"
#define SB_METRICS_ADVERTISING_ID "0000-0000"

#if defined(_X86)
#define SB_METRICS_CPU "x86"
#elif defined(_X86_64)
#define SB_METRICS_CPU "x86_64"
#elif defined(_ARM64)
#define SB_METRICS_CPU "arm64"
#elif defined(_ARM)
#define SB_METRICS_CPU "arm32"
#endif

enum {
    // main memory bytes, num hardware threads and num cores handled inline
    sb_metrics_video_memory_mbytes = 1024,
    sb_metrics_persistent_storage_available_bytes = 0,
    sb_metrics_persistent_storage_max_write_bps = 0,
    sb_metrics_texture_format = adk_gpu_ready_texture_format_none,
    sb_metrics_device_class = adk_device_class_desktop_pc
};

#ifdef __cplusplus
}
#endif
