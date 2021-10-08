/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
Analytics API

Queries information about current video player statistics when _NVE is defined.

*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nve_video_analytics_t {
    int playhead_time;
    int buffer_length;
    float rendered_framerate;
    int min_buffer_length;
    const char * const player_type;
    char * player_version;
} nve_video_analytics_t;

// Returns video analytics for a given player handle
nve_video_analytics_t nve_get_video_analytics(void * handle);

#ifdef __cplusplus
}
#endif