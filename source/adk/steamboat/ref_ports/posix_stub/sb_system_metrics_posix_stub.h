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
 * Implement static system metrics for appropriate platform.
 * Device ID, tenancy, and persona not defined here.
 */

#define SB_METRICS_VENDOR "Vendor_Stub"
#define SB_METRICS_DEVICE "PlatformStub"
#define SB_METRICS_FIRMWARE "0.1.0"
#define SB_METRICS_SOFTWARE "SW_STUB"
#define SB_METRICS_REVISION "1.0.0"
#define SB_METRICS_GPU "GPU_STUB"
#define SB_METRICS_REGION "1"
#define SB_METRICS_ADVERTISING_ID "0000-0000"

#if defined(_X86)
#define SB_METRICS_CPU "x86"
#elif defined(_X86_64)
#define SB_METRICS_CPU "x86_64"
#elif defined(_MIPS64)
#define SB_METRICS_CPU "mips64"
#elif defined(_MIPS)
#define SB_METRICS_CPU "mips32"
#elif defined(_ARM64)
#define SB_METRICS_CPU "arm64"
#elif defined(_ARM)
#define SB_METRICS_CPU "arm32"
#else
#error "unrecognized architecture"
#endif

enum {
    // num cores, main memory mbytes and num hardware threads are handled inline
    sb_metrics_video_memory_mbytes = 1024,
    sb_metrics_persistent_storage_available_bytes = 0,
    sb_metrics_persistent_storage_max_write_bps = 0,
    sb_metrics_texture_format = adk_gpu_ready_texture_format_etc1,
    sb_metrics_device_class = adk_device_class_stb
};

#ifdef __cplusplus
}
#endif
