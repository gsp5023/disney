/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
extension.h

Public interface for ADK extensions
*/

#include "source/adk/extender/generated/ffi.h"
#include "source/adk/manifest/manifest.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct extension_version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} extension_version_t;

typedef struct extension_info_t {
    const char * name;
    const char * description;
    extension_version_t version;
} extension_info_t;

typedef struct extension_runtime_config_t {
    bool runtime_config_override;
    runtime_configuration_t runtime_config;
} extension_runtime_config_t;

typedef enum extension_status_e {
    extension_status_failure,
    extension_status_success
} extension_status_e;

struct cncbus_t;
struct adk_native_functions_t;

typedef struct extension_startup_context_t {
    struct cncbus_t * bus;
    const struct adk_native_functions_t * functions;
} extension_startup_context_t;

typedef struct extension_interface_t {
    uint32_t ext_interface_size;
    extension_info_t (*query_info)();
    extension_runtime_config_t (*get_runtime_config)();
    extension_status_e (*startup)(const extension_startup_context_t *);
    extension_status_e (*wasm3_link)(void * const wasm_interpreter_instance);
    extension_status_e (*wamr_link)(void * const wasm_interpreter_instance);
    bool (*tick)(const void *);
    extension_status_e (*suspend)(void);
    extension_status_e (*resume)(void);
    extension_status_e (*shutdown)(void);
} extension_interface_t;

#ifdef __cplusplus
}
#endif
