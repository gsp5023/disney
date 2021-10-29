/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

// All bus messages are FOURCC codes to support message services that don't live
// inside NCP.
enum {
    cncbus_msg_type_utf8 = FOURCC('U', 'T', 'F', '8'),
    cncbus_msg_type_log_v1 = FOURCC('L', 'O', 'G', '1'),
    cncbus_msg_type_metric_v2 = FOURCC('M', 'E', 'T', '2'),
    cncbus_msg_type_event = FOURCC('E', 'V', 'N', 'T'),
};

#ifdef __cplusplus
}
#endif
