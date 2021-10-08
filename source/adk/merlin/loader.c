/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/paddleboat/paddleboat.h"

#include <assert.h>

// defined/initialized in paddleboat platform-specific implementation
extern const char * const dso_suffix;

#include <stdio.h>
#include <string.h>
#if defined(__GNUC__)
#include <strings.h>
#elif defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

typedef int (*adk_main_t)(const int, const char * const * const);

static int findarg(const char * arg, const int argc, const char * const * const argv) {
    for (int i = 0; i < argc; ++i) {
        if (!strcasecmp(arg, argv[i])) {
            return i;
        }
    }

    return -1;
}

static const char * getargarg(const char * arg, const int argc, const char * const * const argv) {
    const int i = findarg(arg, argc, argv);
    if ((i != -1) && ((i + 1) < argc)) {
        return argv[i + 1];
    }

    return NULL;
}

static pb_module_handle_t driver_roulette(const char * subdir_arg) {
    const char * dso_basename[] = {"daisy", "minnie"};

    enum {
        driver_subdir_path_max_length = 240,
        dso_relative_path_max_length = 256,
    };

    char driver_subdir[driver_subdir_path_max_length] = {0};
    if (subdir_arg) {
        assert(strlen(subdir_arg) < driver_subdir_path_max_length);
        strcpy(driver_subdir, subdir_arg);
    } else {
        pb_get_driver_path(driver_subdir, driver_subdir_path_max_length);
    }

    char dso_relative_path[dso_relative_path_max_length] = {0};
    pb_module_handle_t handle = NULL;

    const size_t n = sizeof(dso_basename) / sizeof(dso_basename[0]);
    for (size_t k = 0; k < n; ++k) {
        snprintf(dso_relative_path, sizeof(dso_relative_path) - 1, "%s/%s.%s", driver_subdir, dso_basename[k], dso_suffix);

        handle = pb_load_module(dso_relative_path);

        if (handle) {
            break;
        }
    }

    if (!handle) {
        printf("\nNo drivers found in '%s'.\n", driver_subdir);
#if !defined(_SHIP) || defined(_SHIP_DEV)
        printf("You may specify a different location via '--drivers <arg>'.\n");
#endif
    }

    return handle;
}

int thunk_main(const int argc, const char * const * const argv) {
    int retval = -1;

#if !defined(_SHIP) || defined(_SHIP_DEV)
    pb_module_handle_t handle = driver_roulette(getargarg("--drivers", argc, argv));
#else
    pb_module_handle_t handle = driver_roulette(NULL);
#endif

    if (handle) {
        const adk_main_t adk_main = (adk_main_t)pb_bind_symbol(handle, "adk_main");

        if (adk_main) {
            retval = adk_main(argc, argv);
        }

        pb_unload_module(handle);
    }

    return retval;
}
