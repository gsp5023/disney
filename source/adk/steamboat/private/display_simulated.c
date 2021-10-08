/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
display_simulated.c

display mode simulation
*/

#include _PCH

#include "display_simulated.h"

#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_file.h"

#define DS_TAG FOURCC('D', 'S', 'P', 'L')

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996) // no we will not rewrite strtok to use strtok_s.
#endif

void * sb_runtime_heap_alloc(const size_t size, const char * const tag);
void sb_runtime_heap_free(void * const ptr, const char * const tag);

void load_simulated_display_modes(simulated_display_modes_t * const out_display_modes, sb_display_mode_t max_screen_size) {
    static const char * const file_name = "simulated_display_modes.txt";
    sb_file_t * const fp = sb_fopen(sb_app_root_directory, file_name, "rt");
    if (!fp) {
        LOG_DEBUG(DS_TAG, "Could not find simulated_display_modes.txt, defaulting single full screen mode.");
        out_display_modes->display_modes[0] = max_screen_size;
        return;
    }
    VERIFY(fp);

    sb_fseek(fp, 0, sb_seek_end);
    long file_len = sb_ftell(fp);
    sb_fseek(fp, 0, sb_seek_set);
    char * const buff = sb_runtime_heap_alloc(file_len + 1, MALLOC_TAG);
    buff[file_len] = '\0';
    sb_fread(buff, 1, file_len, fp);
    sb_fclose(fp);
    char * substr = strtok(buff, "\n");

    out_display_modes->display_mode_count = 0;
    while (substr != NULL) {
        size_t substr_size = strlen(substr);
        const char * width_str = NULL;
        const char * height_str = NULL;
        const char * hz_str = NULL;

        bool found_non_numeric = true;
        for (size_t i = 0; i < substr_size; ++i) {
            if ((substr[i] >= '0') && (substr[i] <= '9')) {
                if (!found_non_numeric) {
                    continue;
                }
                if (!width_str) {
                    width_str = &substr[i];
                } else if (!height_str) {
                    height_str = &substr[i];
                } else if (!hz_str) {
                    hz_str = &substr[i];
                }
                found_non_numeric = false;
            } else {
                substr[i] = '\0';
                found_non_numeric = true;
            }
        }

        if (width_str) {
            VERIFY_MSG(width_str && height_str && hz_str, "Format error in simulated_display_modes.txt\nFormat is: WIDTHxHEIGHTxHZ\nwidth detected as: [%s]\nheight detected as: [%s]\nhz detected as: [%s]", width_str, height_str, hz_str);

            out_display_modes->display_modes[out_display_modes->display_mode_count++] = (sb_display_mode_t){
                .width = atoi(width_str),
                .height = atoi(height_str),
                .hz = atoi(hz_str)};

            sb_display_mode_t * curr_mode = &out_display_modes->display_modes[out_display_modes->display_mode_count - 1];
            if ((curr_mode->width > max_screen_size.width) || (curr_mode->height > max_screen_size.height)) {
                --out_display_modes->display_mode_count; // if the dimensions of the custom screen is larger than our physical monitor we won't be able to display it.
            }

            VERIFY_MSG(out_display_modes->display_mode_count < max_simulated_display_modes, "Too many display modes to load from config file.\nlimit is: %i", max_simulated_display_modes - 1);
        }

        substr = strtok(NULL, "\n");
    }
    VERIFY_MSG(out_display_modes->display_mode_count > 0, "Failed to load any display modes (is the file %s empty?)", file_name);
    sb_runtime_heap_free(buff, MALLOC_TAG);

    // if we didn't load our native display resolution, tack it on the end.

    bool have_primary_video_mode = false;
    for (int i = 0; i < out_display_modes->display_mode_count; ++i) {
        const sb_display_mode_t * const selected_mode = &out_display_modes->display_modes[i];
        if ((selected_mode->height == max_screen_size.height) && (selected_mode->width == max_screen_size.width)) {
            have_primary_video_mode = true;
            break;
        }
    }

    if (!have_primary_video_mode) {
        out_display_modes->display_modes[out_display_modes->display_mode_count++] = max_screen_size;
    }
}
#ifdef _WIN32
#pragma warning(pop)
#endif
