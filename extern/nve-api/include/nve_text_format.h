/****************************************************************************
 * Copyright (c) 2020 Disney
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __NVE_TEXT_FORMATS_H__
#define __NVE_TEXT_FORMATS_H__

#include "nve_defines.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT typedef enum nve_font_e {
    nve_font_default = 0,
    nve_font_monospace_with_serifs = 1,
    nve_font_proportional_with_serifs = 2,
    nve_font_monospace_without_serifs = 3,
    nve_font_proportional_without_serifs = 4,
    nve_font_casual = 5,
    nve_font_cursive = 6,
    nve_font_small_capitals = 7,
    FORCE_ENUM_INT32(nve_font_e)
} nve_font_e;

FFI_EXPORT typedef enum nve_color_e {
    nve_color_default = 0,
    nve_color_black = 1,
    nve_color_gray = 2,
    nve_color_white = 3,
    nve_color_bright_white = 4,
    nve_color_dark_red = 5,
    nve_color_red = 6,
    nve_color_bright_red = 7,
    nve_color_dark_green = 8,
    nve_color_green = 9,
    nve_color_bright_green = 10,
    nve_color_dark_blue = 11,
    nve_color_blue = 12,
    nve_color_bright_blue = 13,
    nve_color_dark_yellow = 14,
    nve_color_yellow = 15,
    nve_color_bright_yellow = 16,
    nve_color_dark_magenta = 17,
    nve_color_magenta = 18,
    nve_color_bright_magenta = 19,
    nve_color_dark_cyan = 20,
    nve_color_cyan = 21,
    nve_color_bright_cyan = 22,
    FORCE_ENUM_INT32(nve_color_e)
} nve_color_e;

FFI_EXPORT typedef enum nve_size_e {
    nve_size_default = 0,
    nve_size_small = 1,
    nve_size_medium = 2,
    nve_size_large = 3,
    FORCE_ENUM_INT32(nve_size_e)
} nve_size_e;

FFI_EXPORT typedef enum nve_font_edge_e {
    nve_font_edge_default = 0,
    nve_font_edge_none = 1,
    nve_font_edge_raised = 2,
    nve_font_edge_depressed = 3,
    nve_font_edge_uniform = 4,
    nve_font_edge_drop_shadow_left = 5,
    nve_font_edge_drop_shadow_right = 6,
    FORCE_ENUM_INT32(nve_font_edge_e)
} nve_font_edge_e;

FFI_EXPORT typedef struct {
    nve_font_e font;
    nve_size_e size;
    nve_font_edge_e font_edge;
    nve_color_e font_color;
    nve_color_e background_color;
    nve_color_e fill_color;
    nve_color_e edge_color;
    int32_t font_opacity;
    int32_t background_opacity;
    int32_t fill_opacity;
    int32_t treat_space_as_alpha_num;
    char bottom_inset[16];
    char safe_area[16];
} nve_text_format_t;

// Initialize default text format
FFI_EXPORT_NVE void nve_text_format_init(FFI_PTR_WASM nve_text_format_t * params);

FFI_EXPORT_NVE void nve_media_player_get_text_style(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_text_format_t * cc_style);
FFI_EXPORT_NVE void nve_media_player_set_text_style(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM const nve_text_format_t * cc_style);

#ifdef __cplusplus
}
#endif

#endif
