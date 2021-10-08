/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/extender/extension.h"
#include "source/adk/extender/extension/functions.h"
#include "source/samples/extension/lib/extension.h"

static extension_info_t query_info() {
    const extension_info_t extension_info = {
        .name = "hello-ffi",
        .description = "Minimal extension meant to be used as an example",
        .version = {.major = 0, .minor = 1, .patch = 0},
    };

    return extension_info;
}

static extension_runtime_config_t get_runtime_config(void) {
    const extension_runtime_config_t rc = {
        .runtime_config_override = false};

    return rc;
}

static extension_status_e startup(const extension_startup_context_t * const ctx) {
    if (extensions_ffi_init(ctx->functions) == false) {
        return extension_status_failure;
    }

    return extension_status_success;
}

static bool tick(const void * arg) {
    return true;
}

static extension_status_e suspend(void) {
    return extension_status_success;
}

static extension_status_e resume(void) {
    return extension_status_success;
}

static extension_status_e shutdown(void) {
    return extension_status_success;
}

#ifdef _WASM3
extension_status_e wasm3_link_extension(void * const wasm_interpreter_instance);
#endif // _WASM3

#ifdef _WAMR
extension_status_e wamr_link_extension(void * const wasm_interpreter_instance);
#endif // _WAMR

DLL_EXPORT const extension_interface_t * extension_get_interface() {
    static const extension_interface_t extension_interface = {
        .ext_interface_size = sizeof(extension_interface_t),
        .query_info = query_info,
        .get_runtime_config = get_runtime_config,
        .startup = startup,
#ifdef _WASM3
        .wasm3_link = wasm3_link_extension,
#endif // _WASM3
#ifdef _WAMR
        .wamr_link = wamr_link_extension,
#endif // _WAMR
        .tick = tick,
        .suspend = suspend,
        .resume = resume,
        .shutdown = shutdown};

    return &extension_interface;
}
