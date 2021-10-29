/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 extender.c

 Loading dynamic modules and making their functions available for use
 */

#include "extender.h"

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/extender/generated/ffi.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/private/file.h"

#define TAG_EXTENDER FOURCC('X', 'N', 'D', 'R')

const char * extension_default_subdir = "extensions";
extern const char * const dso_suffix;

typedef struct module_rec_t {
    pb_module_handle_t handle;
    const extension_interface_t * ftable;
    char filename[sb_max_path_length];
    extension_info_t info;
} module_rec_t;

#define MODULES_MAX 16
static module_rec_t module_handle_table[MODULES_MAX] = {0};
static uint8_t module_free_slot = 0;

static void store_module_info(
    pb_module_handle_t handle,
    const extension_interface_t * ftable,
    const char * filename,
    const extension_info_t info) {
    ASSERT(handle != NULL);
    ASSERT(filename != NULL);
    ASSERT(module_free_slot < MODULES_MAX);

    module_rec_t * const module = &module_handle_table[module_free_slot++];

    ASSERT(module->handle == NULL);
    ASSERT(module->filename[0] == '\0');

    module->handle = handle;
    module->ftable = ftable;
    strcpy_s(module->filename, sb_max_path_length, filename);
    module->info = info;
}

static void runtime_config_bubble_up_max_values(runtime_configuration_t * dst, const runtime_configuration_t * src) {
    ASSERT(dst && src);

    dst->memory_reservations.high.canvas = max_uint32_t(dst->memory_reservations.high.canvas, src->memory_reservations.high.canvas);

    static const int num_systems = sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t);
    uint32_t * const sizes_dst = (uint32_t *)&dst->memory_reservations.low;
    const uint32_t * const sizes_src = (const uint32_t *)&src->memory_reservations.low;

    for (int i = 0; i < num_systems; ++i) {
        sizes_dst[i] = max_uint32_t(sizes_dst[i], sizes_src[i]);
        ASSERT_MSG(sizes_dst[i], "An extension is trying to set a memory reservation value of zero at position %i", i);
    }

    ASSERT(dst->guard_page_mode == src->guard_page_mode); // Extensions shouldn't conflict here

    dst->wasm_low_memory_size = max_uint32_t(dst->wasm_low_memory_size, src->wasm_low_memory_size);
    dst->wasm_high_memory_size = max_uint32_t(dst->wasm_high_memory_size, src->wasm_high_memory_size);
    dst->wasm_heap_allocation_threshold = max_uint32_t(dst->wasm_heap_allocation_threshold, src->wasm_heap_allocation_threshold);
    dst->http_max_pooled_connections = max_uint32_t(dst->http_max_pooled_connections, src->http_max_pooled_connections);
    dst->bundle_fetch.retry_max_attempts = max_uint32_t(dst->bundle_fetch.retry_max_attempts, src->bundle_fetch.retry_max_attempts);
    dst->bundle_fetch.retry_backoff_ms.ms = max_uint32_t(dst->bundle_fetch.retry_backoff_ms.ms, src->bundle_fetch.retry_backoff_ms.ms);
    dst->coredump_memory_size = max_uint32_t(dst->coredump_memory_size, src->coredump_memory_size);
}

extender_status_e bind_extensions(const char * extdir_path_override) {
    extender_status_e retval = extender_status_success;

    const char * const extension_subdir = extdir_path_override != NULL ? extdir_path_override : extension_default_subdir;

    sb_directory_t * const dir = sb_open_directory(sb_app_root_directory, extension_subdir);

    if (dir == NULL) {
        if (extension_subdir == extdir_path_override) {
            LOG_ERROR(TAG_EXTENDER, "sb_open_directory failed to open %s", extension_subdir);
            // The extension directory provided on the command-line could not
            // be opened, so we'll treat that as a hard failure (unlike the
            // case where the default directory does not exist). Note: it would
            // be ideal to refine sb_open_directory to return a reason for the
            // failure, so we could distinguish between "does not exist" and
            // permissions issues, etc.
            return extender_status_failure;
        }

        LOG_WARN(TAG_EXTENDER, "sb_open_directory failed to open %s", extension_subdir);
        return retval;
    }

    extension_runtime_config_t rc_gather = {.runtime_config_override = false};
    rc_gather.runtime_config = get_default_runtime_configuration();

    while (true) {
        if (module_free_slot == MODULES_MAX) {
            LOG_DEBUG(TAG_EXTENDER, "max (%d) modules reached", MODULES_MAX);
            break;
        }

        const sb_read_directory_result_t read_result = sb_read_directory(dir);

        if (read_result.entry_type == sb_directory_entry_null) {
            break;
        }

        const char * const ext_filename = read_result.entry ? sb_get_directory_entry_name(read_result.entry) : NULL;
        if ((read_result.entry_type == sb_directory_entry_file) && (ext_filename)) {
            size_t filename_length = strlen(ext_filename);
            size_t suffix_length = strlen(dso_suffix);

            // there should at least be a one-character basename, a dot, and the platform-specific suffix
            if (filename_length > suffix_length + 1) {
                const char * file_ext = ext_filename + filename_length - suffix_length;

                if (file_ext && !strncmp(file_ext, dso_suffix, suffix_length) && (*--file_ext == '.')) {
                    LOG_DEBUG(TAG_EXTENDER, "found module: %s", ext_filename);
                } else {
                    // not an m5 extension
                    LOG_DEBUG(TAG_EXTENDER, "skipping %s", ext_filename);
                    continue;
                }
            } else {
                // filename is too small to be an m5 extension
                LOG_DEBUG(TAG_EXTENDER, "skipping %s", ext_filename);
                continue;
            }
        } else {
            // not a regular file
            LOG_DEBUG(TAG_EXTENDER, "skipping %s", ext_filename);
            continue;
        }

        char extdir_fullpath[sb_max_path_length] = {0};
        const char * const cwd = adk_get_file_directory_path(sb_app_root_directory);

        sprintf_s(extdir_fullpath, sb_max_path_length, "%s/%s/%s", cwd, extension_subdir, ext_filename);

        const pb_module_handle_t handle = pb_load_module(extdir_fullpath);

        if (handle != NULL) {
            LOG_DEBUG(TAG_EXTENDER, "pb_load_module %s success", ext_filename);

            const extension_get_interface_fn_t extension_get_interface = (extension_get_interface_fn_t)pb_bind_symbol(handle, "extension_get_interface");

            if (extension_get_interface == NULL) {
                LOG_DEBUG(TAG_EXTENDER, "'extension_get_interface' symbol not found in module %s (assuming this is not an extension)", ext_filename);

                if (!pb_unload_module(handle)) {
                    LOG_ERROR(TAG_EXTENDER, "pb_unload_module %s failed", ext_filename);
                    retval = extender_status_failure;
                    break;
                } else {
                    continue;
                }
            }

            const extension_interface_t * const ftable = extension_get_interface();
            if (ftable != NULL) {
                ASSERT(ftable);
                ASSERT(ftable->ext_interface_size == sizeof(extension_interface_t));
                ASSERT(ftable->query_info);
                ASSERT(ftable->get_runtime_config);
                ASSERT(ftable->startup);
                ASSERT(ftable->tick);
                ASSERT(ftable->suspend);
                ASSERT(ftable->resume);
                ASSERT(ftable->shutdown);

                const extension_info_t info = ftable->query_info();

                store_module_info(handle, ftable, ext_filename, info);

                LOG_DEBUG(
                    TAG_EXTENDER,
                    "query_info success (extension name: %s, desc: %s, ver: %d.%d.%d)",
                    info.name,
                    info.description,
                    info.version.major,
                    info.version.minor,
                    info.version.patch);

                const extension_runtime_config_t rc = ftable->get_runtime_config();
                if (rc.runtime_config_override) {
                    // if this is the first extension to request override...
                    if (rc_gather.runtime_config_override == false) {
                        // in this case, make the first extension values
                        // the default for future comparison...
                        rc_gather.runtime_config.guard_page_mode = rc.runtime_config.guard_page_mode;
                        rc_gather.runtime_config_override = true;
                    }

                    runtime_config_bubble_up_max_values(&rc_gather.runtime_config, &rc.runtime_config);
                }
            } else {
                LOG_ERROR(TAG_EXTENDER, "Failed to get extension interface for module (%s)", ext_filename);
                retval = extender_status_failure;
                break;
            }
        } else {
            LOG_ERROR(TAG_EXTENDER, "pb_load_module %s failed", extdir_fullpath);
            retval = extender_status_failure;
            break;
        }
    }

    if (rc_gather.runtime_config_override) {
        if (!app_init_subsystems(rc_gather.runtime_config)) {
            LOG_ERROR(TAG_EXTENDER, "app_init_subsystems failed");
            retval = extender_status_failure;
        }
    }

    sb_close_directory(dir);

    return retval;
}

extender_status_e tick_extensions(const void * arg) {
    extender_status_e retval = extender_status_success;

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        if (module_handle_table[k].ftable->tick(arg) == extension_status_failure) {
            LOG_ERROR(TAG_EXTENDER, "%s: tick failed!", module_handle_table[k].filename);
            retval = extender_status_failure;
            // mark the failure and continue...
        }
    }

    return retval;
}

extender_status_e start_extensions(struct cncbus_t * bus) {
    extern adk_native_functions_t adk_native_functions;

    extender_status_e retval = extender_status_success;

    LOG_DEBUG(TAG_EXTENDER, "Loaded extension count: %d", module_free_slot);

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        const extension_startup_context_t ctx = {
            .bus = bus,
            .functions = &adk_native_functions};

        if (module_handle_table[k].ftable->startup(&ctx) == extension_status_success) {
            LOG_DEBUG(TAG_EXTENDER, "startup succeeded");
        } else {
            LOG_ERROR(TAG_EXTENDER, "startup failed");
            retval = extender_status_failure;
            // mark the failure and continue...
        }
    }

    return retval;
}

#ifdef _WASM3
extender_status_e link_extensions_in_wasm3(void * const wasm_interpreter_instance) {
    for (int k = 0; k < module_free_slot; ++k) {
        const module_rec_t * const handle = &module_handle_table[k];

        if (handle->ftable->wasm3_link != NULL) {
            handle->ftable->wasm3_link(wasm_interpreter_instance);
        }
    }

    return extender_status_success;
}
#endif // _WASM3

#ifdef _WAMR
extender_status_e link_extensions_in_wamr(void * const wasm_interpreter_instance) {
    for (int k = 0; k < module_free_slot; ++k) {
        const module_rec_t * const handle = &module_handle_table[k];

        if (handle->ftable->wamr_link != NULL) {
            handle->ftable->wamr_link(wasm_interpreter_instance);
        }
    }

    return extender_status_success;
}
#endif // _WAMR

extender_status_e suspend_extensions(void) {
    extender_status_e retval = extender_status_success;

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        if (module_handle_table[k].ftable->suspend() == extension_status_success) {
            LOG_DEBUG(TAG_EXTENDER, "suspend succeeded");
        } else {
            LOG_ERROR(TAG_EXTENDER, "suspend failed");
            retval = extender_status_failure;
            // mark the failure and continue...
        }
    }

    return retval;
}

extender_status_e resume_extensions(void) {
    extender_status_e retval = extender_status_success;

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        if (module_handle_table[k].ftable->resume() == extension_status_success) {
            LOG_DEBUG(TAG_EXTENDER, "resume succeeded");
        } else {
            LOG_ERROR(TAG_EXTENDER, "resume failed");
            retval = extender_status_failure;
            // mark the failure and continue...
        }
    }

    return retval;
}

extender_status_e stop_extensions(void) {
    extender_status_e retval = extender_status_success;

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        if (module_handle_table[k].ftable->shutdown() == extension_status_success) {
            LOG_DEBUG(TAG_EXTENDER, "shutdown succeeded");
        } else {
            LOG_ERROR(TAG_EXTENDER, "shutdown failed");
            retval = extender_status_failure;
            // mark the failure and continue...
        }
    }

    return retval;
}

extender_status_e unbind_extensions(void) {
    extender_status_e retval = extender_status_success;

    for (int k = 0; k < module_free_slot; ++k) {
        ASSERT(module_handle_table[k].handle != NULL);
        ASSERT(module_handle_table[k].ftable != NULL);
        ASSERT(module_handle_table[k].filename[0] != '\0');

        if (pb_unload_module(module_handle_table[k].handle)) {
            LOG_DEBUG(TAG_EXTENDER, "Module %s unloaded", module_handle_table[k].filename);
        } else {
            LOG_ERROR(TAG_EXTENDER, "pb_unload_module %s failed", module_handle_table[k].filename);
            retval = extender_status_failure;
            // mark the failure and continue...
        }

        module_handle_table[k].handle = NULL;
        module_handle_table[k].ftable = NULL;
        module_handle_table[k].filename[0] = '\0';
    }

    module_free_slot = 0;

    return retval;
}

static pb_module_handle_t extensions_next(struct extensions_iter_t * const iter) {
    ASSERT(iter);
    ASSERT(iter->current <= module_free_slot);
    ASSERT(module_free_slot <= MODULES_MAX);

    pb_module_handle_t retval = NULL;

    if (iter->current < module_free_slot) {
        ASSERT(module_handle_table[iter->current].handle != NULL);
        retval = module_handle_table[iter->current].handle;
        ++iter->current;
    }

    return retval;
}

extensions_iter_t extensions_iter_init(void) {
    return (extensions_iter_t){.current = 0, .next = extensions_next};
}
