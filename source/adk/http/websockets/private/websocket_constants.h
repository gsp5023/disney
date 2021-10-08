/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
    // websocket framing constants
    short_max_val = 0xffff,
    ws_byte_max_len = 125,
    ws_uint16_code = 126,
    ws_uint64_code = 127,
    ws_frame_header_max_len = 14,
};

#ifdef __cplusplus
}
#endif