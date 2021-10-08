/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
extender.h

Loading dynamic modules and calling their functions
*/

#include "source/adk/extender/extender_status.h"
#include "source/adk/extender/extension.h"
#include "source/adk/interpreter/interp_all.h"
#include "source/adk/paddleboat/paddleboat.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const extension_interface_t * (*extension_get_interface_fn_t)();

// interface for the extender

struct cncbus_t;

extender_status_e bind_extensions(const char * extdir_path_override);
extender_status_e start_extensions(struct cncbus_t * bus);

#ifdef _WASM3
extender_status_e link_extensions_in_wasm3(void * const wasm_interpreter_instance);
#endif // _WASM3

#ifdef _WAMR
extender_status_e link_extensions_in_wamr(void * const wasm_interpreter_instance);
#endif // _WAMR

extender_status_e tick_extensions(const void * arg);
extender_status_e suspend_extensions(void);
extender_status_e resume_extensions(void);
extender_status_e stop_extensions(void);
extender_status_e unbind_extensions(void);

// extension handle iterator, mostly intended for use by tools drivers
typedef struct extensions_iter_t {
    int current;
    pb_module_handle_t (*next)(struct extensions_iter_t *);
} extensions_iter_t;

extensions_iter_t extensions_iter_init(void);

#ifdef __cplusplus
}
#endif
