/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is a sample extension with minimal dependence on m5 code.
 * It only references the extension interface and a few SB prototypes in steamboat and runtime.
*/

#include "source/adk/extender/extension.h"
#include "source/adk/extender/extension/functions.h"
#include "source/adk/steamboat/sb_thread.h"

static const struct {
    const char * const extension_name;
    const char * const extension_description;
    const extension_version_t extension_version;
} consts = {
    .extension_name = "hello",
    .extension_description = "Minimal extension meant to be used as an example",
    .extension_version = {.major = 0, .minor = 1, .patch = 0},
};

typedef enum ext_style_e {
    ext_style_driven,
    ext_style_threaded
} ext_style_e;

static struct {
    sb_thread_id_t tid;
    ext_style_e extension_style;
} statics = {
    .tid = 0,
    .extension_style = ext_style_driven,
};

static void extension_trace(const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    sb_vadebug_write_line(msg, args);
    va_end(args);
}

#define EXTENSION_TRACE(msg) \
    extension_trace("%s %s: %s", _SAMPLE_EXTENSION_NAME, __FUNCTION__, msg)

static extension_info_t query_info_called_from_host() {
    const extension_info_t extension_info = {
        .name = consts.extension_name,
        .description = consts.extension_description,
        .version = consts.extension_version,
    };

    return extension_info;
}

static extension_runtime_config_t get_runtime_config_called_from_host(void) {
    const extension_runtime_config_t rc = {
        .runtime_config_override = false};

    return rc;
}

static int extension_thread_fn(void * const arg) {
    EXTENSION_TRACE("exercised");

    sb_unformatted_debug_write_line("driven by the extension's thread");

    sb_thread_sleep((milliseconds_t){1000});

    return 0;
}

static extension_status_e startup_called_from_host(const extension_startup_context_t * const ctx) {
    if (extensions_ffi_init(ctx->functions) == false) {
        return extension_status_failure;
    }

    EXTENSION_TRACE("exercised");

    if (strstr(_SAMPLE_EXTENSION_NAME, "_threaded")) {
        statics.extension_style = ext_style_threaded;
    }

    if (statics.extension_style == ext_style_threaded) {
        statics.tid = sb_create_thread(
            consts.extension_name,
            sb_thread_default_options,
            extension_thread_fn,
            NULL,
            consts.extension_name);

        if (statics.tid == 0) {
            return extension_status_failure;
        }
    }

    return extension_status_success;
}

static bool tick_called_from_host(const void * arg) {
    static bool has_been_called = false;

    if (!has_been_called) {
        if (statics.extension_style == ext_style_driven) {
            EXTENSION_TRACE("exercised");
            if (arg == NULL) {
                extension_trace("driven by the loader's thread (arg: NULL)");
            } else {
                extension_trace("driven by the loader's thread (*arg: %d)", *((int *)arg));
            }
        } else {
            EXTENSION_TRACE("(noop)");
        }

        has_been_called = true;
    }

    return true;
}

static extension_status_e suspend_called_from_host(void) {
    EXTENSION_TRACE("exercised");

    return extension_status_success;
}

static extension_status_e resume_called_from_host(void) {
    EXTENSION_TRACE("exercised");

    return extension_status_success;
}

static extension_status_e shutdown_called_from_host(void) {
    EXTENSION_TRACE("exercised");

    if (statics.tid) {
        sb_join_thread(statics.tid);
    }

    return extension_status_success;
}

static const extension_interface_t extension_interface = {
    .ext_interface_size = sizeof(extension_interface_t),
    .query_info = query_info_called_from_host,
    .get_runtime_config = get_runtime_config_called_from_host,
    .wasm3_link = NULL,
    .wamr_link = NULL,
    .startup = startup_called_from_host,
    .tick = tick_called_from_host,
    .suspend = suspend_called_from_host,
    .resume = resume_called_from_host,
    .shutdown = shutdown_called_from_host};

DLL_EXPORT const extension_interface_t * extension_get_interface() {
    return &extension_interface;
}
