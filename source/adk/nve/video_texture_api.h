/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
video texture api

when _NVE is defined the ADK will call these functions to get the latest available
video frame for rendering.

during SVP or otherwise non-texture-based video playback these functions should be
NOOP (and return NULL)
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nve_video_frame_t {
    struct rhi_texture_t * const * chroma;
    struct rhi_texture_t * const * luma;

    bool hdr10;
    int luma_tex_width;
    int luma_tex_height;
    int chroma_tex_width;
    int chroma_tex_height;
    int framesize_width;
    int framesize_height;
} nve_video_frame_t;

typedef struct nve_texture_api_rect_t {
    float x1, y1, x2, y2;
} nve_texture_api_rect_t;

// returns the video frame that should be displayed by the ADK for the current frame.
// this object must remain valid until nve_done_current_video_frame() is called.
// NOTE: This returns NULL during SVP or otherwise non-texture-based playback.
nve_video_frame_t nve_get_current_video_frame();

// signals the NVE that the ADK is done with this texture object. The NVE can recycle or
// release the object at its convenience.
void nve_done_video_frame(const nve_video_frame_t frame);

// returns the subtitle frame that should be displayed by the ADK for the current frame,
// as well as positioning information.
struct rhi_texture_t * const * nve_get_current_subtitle_frame(const nve_texture_api_rect_t drawn_video_rect, nve_texture_api_rect_t * const out_rect_to_draw_subtitles);

// signals the NVE that the ADK is done with this texture object
void nve_done_subtitle_frame(rhi_texture_t * const * const frame);

void nve_begin_vid_restart(void);

void nve_end_vid_restart(void);

#ifdef __cplusplus
}
#endif
