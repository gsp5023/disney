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

    const pb_module_handle_t handle = (pb_module_handle_t)dlopen((const char *)filename, RTLD_NOW);

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
