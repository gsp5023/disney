/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/paddleboat/paddleboat.h"

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char * const dso_suffix = "so";

pb_module_handle_t pb_load_module(const char * const filename) {
    assert(filename);

    dlerror(); // clear any existing error report

    const pb_module_handle_t handle = (pb_module_handle_t)dlopen((const char *)filename, RTLD_GLOBAL | RTLD_NOW);

    if (handle == NULL) {
        const char * const dlerror_result = dlerror();
        assert(dlerror_result != NULL);

        printf("dlopen failed with %s: %s\n", filename, dlerror_result);
    }

    return handle;
}

bool pb_unload_module(const pb_module_handle_t handle) {
    assert(handle);

    dlerror(); // clear any existing error report
    const bool retval = (dlclose((void *)handle) == 0);

    if (!retval) {
        const char * const dlerror_result = dlerror();
        assert(dlerror_result != NULL);

        printf("dlclose failed: %s\n", dlerror_result);
    }

    return retval;
}

const void * pb_bind_symbol(const pb_module_handle_t handle, const char * const sym_name) {
    assert(handle);
    assert(sym_name);

    dlerror(); // clear any existing error report
    const void * const retval = dlsym((void *)handle, (const char *)sym_name);
    const char * const dlerror_result = dlerror(); // obtain any real error

    // NULL could be a successful return value from dlsym, so we always
    // have to check for error based on results from dlerror()
    if (dlerror_result) {
        printf("dlsym failed with %s: %s\n", sym_name, dlerror_result);
    }

    return retval;
}

static int sprintf_s(char * buff, size_t buff_size, const char * fmt, ...) {
    assert((buff != NULL) && (buff_size > 0));

    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buff, buff_size, fmt, args);
    va_end(args);

    assert(ret != -1);
    return ret;
}

const char * pb_get_driver_path(char * const buff, const size_t buff_size) {
    char exe_path[PATH_MAX + 1] = {0}; // readlink doesn't null-terminate stored path
    if (readlink("/proc/self/exe", exe_path, PATH_MAX) == -1) {
        fprintf(stderr, "Failed to get absolute path: %s\n", strerror(errno));
        return NULL;
    }

    const char * const folder_path = strrchr(exe_path, '/');
    sprintf_s(buff, buff_size, "%.*s/drivers", (int)(folder_path - exe_path), exe_path);
    return buff;
}
