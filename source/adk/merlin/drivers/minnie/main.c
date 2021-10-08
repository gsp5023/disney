/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 main.c

 App runner for m5 (wasm) applications
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/bundle/bundle.h"
#include "source/adk/cache/cache.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/crypto/crypto.h"
#include "source/adk/extender/extender.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/interpreter/interp_all.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/log/log.h"
#include "source/adk/merlin/drivers/minnie/exit_codes.h"
#include "source/adk/merlin/drivers/minnie/merlin.h"
#include "source/adk/merlin/drivers/minnie/resources.h"
#include "source/adk/persona/persona.h"
#include "source/adk/splash/splash.h"
#include "source/adk/steamboat/sb_platform.h"

#include <ctype.h>

#ifdef _TTFI_TRACE
#include "source/adk/telemetry/telemetry.h"
#define TTFI_TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ##__VA_ARGS__)
#define TTFI_TRACE_TIME_SPAN_END(_id) TRACE_TIME_SPAN_END(_id)
#else
#define TTFI_TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define TTFI_TRACE_TIME_SPAN_END(_id)
#endif

#ifdef _MERLIN_TRACE
#include "source/adk/telemetry/telemetry.h"
#define MERLIN_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define MERLIN_TRACE_PUSH(_name) TRACE_PUSH(_name)
#define MERLIN_TRACE_POP() TRACE_POP()
#else
#define MERLIN_TRACE_PUSH_FN()
#define MERLIN_TRACE_PUSH(_name)
#define MERLIN_TRACE_POP()
#endif

#if _MERLIN_DEMOS
extern int canvas_demo_main(const int argc, const char * const * const argv);
extern int font_demo_main(const int argc, const char * const * const argv);
extern int hello_cube_main(const int argc, const char * const * const argv);
#ifdef _CANVAS_EXPERIMENTAL
extern int canvas_experimental_demo_main(const int argc, const char * const * const argv);
#endif
static const char default_demo_asset_root[] = "assets/samples/";
#endif

// Constants
//
static const char manifest_cache_key[] = "app-manifest";
static const char bundle_cache_key[] = "app-bundle";

enum {
    max_argv = 256,
    fragment_size = 4 * 1014,
    cache_region_size = 1 * 1024 * 1024,
};

// Used when no retry data is available
static const fetch_retry_context_t default_fetch_retry = {.retry_max_attempts = 4, .retry_backoff_ms.ms = 1000};

#define TAG_MERLIN FOURCC('M', 'R', 'L', 'N')

// Command-line definitions
//

// Command-line options (short option char), options prefixed 'opt_act_' are actions.  Only one action per run is allowed.
typedef enum opt_e {
    opt_unknown = '\0',
    opt_act_help = 'h',
    opt_act_version = 'V',
#ifndef _SHIP
    opt_act_load_bundle_file = 'b',
    opt_act_load_wasm_file = 'w',
    opt_skip_signature = 's',
    opt_use_config = 'c',
    opt_test = 't',
#endif
#if !defined(_SHIP) || defined(_SHIP_DEV)
    opt_act_load_manifest_url = 'm',
    opt_act_load_manifest_file = 'M',
    opt_act_load_persona_file = 'P',
    opt_set_persona_id = 'p',
#endif
#if _MERLIN_DEMOS
    opt_act_run_demo = 'd',
    opt_set_demo_asset_root = 'r',
#endif
    opt_no_app_load = 'x',
} opt_e;

typedef wasm_memory_region_t (*loader_func_t)(const char * const path);

// Command-line args
typedef struct app_args_t {
    // action to be performed (only one action per run)
    opt_e action;
    // argument to the action
    const char * action_arg;
    // wasm loader func, for load actions
    loader_func_t loader;
    // path to persona file
    const char * persona_file_path;
    // persona id to load
    const char * persona_id;
    // format for manifest URL
    const char * manifest_url;
    // Skip the signature verification for bundles
    bool skip_signature;
    // Path to config file to be used with local wasm file
    const char * config_file_path;
#if _MERLIN_DEMOS
    // asset root directory for demo
    const char * demo_asset_root;
#endif
#ifndef _SHIP
    bool test;
#endif
    bool no_app_load;
} app_args_t;

// File-scope data
static struct {
    app_args_t args;

    mem_region_t cache_pages;
    cache_t * cache;

    uint32_t wasm_memory_size;
    bool has_processed_manifest;
    bool displayed_error_splash;
    bundle_t * bundle;
    char fallback_error_message[adk_max_message_length];
} statics = {
    .has_processed_manifest = false,
    .displayed_error_splash = false,
    .args = {
        .action = opt_unknown,
        .persona_file_path = default_persona_file_prod,
        .persona_id = "",
        .skip_signature = false,
#if _MERLIN_DEMOS
        .demo_asset_root = default_demo_asset_root,
#endif
    },
};

#if _MERLIN_DEMOS
const char * merlin_asset(const char * const relpath, char * const asset_path_buff, const size_t asset_path_buff_len) {
    sprintf_s(asset_path_buff, asset_path_buff_len, "%s%s", statics.args.demo_asset_root, relpath);
    return asset_path_buff;
}
#endif

// Displays the default merlin error splash screen
void display_default_error_splash() {
    MERLIN_TRACE_PUSH_FN();

    if (statics.displayed_error_splash) {
        MERLIN_TRACE_POP();
        return;
    }
    statics.displayed_error_splash = true;

    app_shutdown_main_display();

    // Setup subsystems for the splash screen
    if (!app_init_subsystems(get_default_runtime_configuration())) {
        MERLIN_TRACE_POP();
        return;
    }

    splash_main((splash_screen_contents_t){
        .app_name = "ADK Error",
        .image_path = default_error_image_path,
        .fallback_error_message = statics.fallback_error_message});
    MERLIN_TRACE_POP();
}

// Displays the merlin error splash screen from an opened bundle
void display_bundle_error_splash(bundle_t * const bundle) {
    MERLIN_TRACE_PUSH_FN();

    if (statics.displayed_error_splash) {
        MERLIN_TRACE_POP();
        return;
    }

    if (bundle) {
        // Loop over bundle_error_image_paths and try to use each one in order
        for (int i = 0; i < ARRAY_SIZE(bundle_error_image_paths); i++) {
            const sb_stat_result_t bs = bundle_stat(bundle, bundle_error_image_paths[i]);
            if (bs.error == sb_stat_success) {
                LOG_INFO(TAG_MERLIN, "Displaying fallback error image in bundle: %s", bundle_error_image_paths[i]);
                statics.displayed_error_splash = true;

                app_shutdown_main_display();

                // Setup subsystems for the splash screen
                if (!app_init_subsystems(get_default_runtime_configuration())) {
                    MERLIN_TRACE_POP();
                    return;
                }

                splash_main((splash_screen_contents_t){
                    .app_name = "ADK Error",
                    .image_path = bundle_error_image_paths[i],
                    .fallback_error_message = statics.fallback_error_message});
                MERLIN_TRACE_POP();
                return;
            }
        }

        // Unable to find any error images in the bundle
        LOG_INFO(TAG_MERLIN, "Bundle does not contain error image");
        MERLIN_TRACE_POP();
        return;
    }

    LOG_ERROR(TAG_MERLIN, "Could not open bundle file for error image");
    MERLIN_TRACE_POP();
}

// Cargo passes to its app runner the OS local path.  On windows this means we're given `\` for path
// separators, vs we expect only `/` so convert the wasm's path.
static void to_fwd_slash(char * const dest, size_t dest_size, const char * const src) {
    if (dest_size > 0) {
        --dest_size; // reserved for null term
        size_t ix;
        for (ix = 0; ix < dest_size && src[ix]; ++ix) {
            dest[ix] = (src[ix] == '\\') ? '/' : src[ix];
        }
        dest[ix] = '\0';
    }
}

static void clear_bundle_cache() {
    MERLIN_TRACE_PUSH_FN();

    LOG_DEBUG(TAG_MERLIN, "Clearing bundle cache");
    if (statics.cache) {
        cache_delete_key(statics.cache, bundle_cache_key);
    }

    MERLIN_TRACE_POP();
}

static void clear_manifest_cache() {
    MERLIN_TRACE_PUSH_FN();

    LOG_DEBUG(TAG_MERLIN, "Clearing manifest cache");
    if (statics.cache) {
        cache_delete_key(statics.cache, manifest_cache_key);
    }
    MERLIN_TRACE_POP();
}

static size_t read_bundle_file(void * const buffer, const size_t size, void * const file) {
    return bundle_fread(buffer, 1, size, (bundle_file_t *)file);
}

static void conditional_overwrite_bundle_config(bundle_t * const bundle, runtime_configuration_t * const runtime_config) {
    MERLIN_TRACE_PUSH_FN();

    sb_stat_result_t cs = bundle_stat(bundle, bundle_config_path);
    if (cs.error == sb_stat_success) {
        bundle_file_t * const config_file = bundle_fopen(bundle, bundle_config_path);
        if (config_file) {
            mem_region_t bundle_config_blob = manifest_alloc_bundle_file_blob(cs.stat.size);
            bundle_fread(bundle_config_blob.ptr, sizeof(uint8_t), cs.stat.size, config_file);
            bundle_config_parse_overwrite(CONST_MEM_REGION(.ptr = bundle_config_blob.ptr, .size = bundle_config_blob.size), runtime_config);
            manifest_free_file_blob(bundle_config_blob);
            bundle_fclose(config_file);
        }
    }

    MERLIN_TRACE_POP();
}

#ifndef _ADK_BUNDLE_KEY
// Shared private key used for signing/verifying bundle
// - [default] generated using `openssl rand -base64 64`
#define _ADK_BUNDLE_KEY "L0qVPwhh0jjXUcQOGTVw3Tsa2M40UCRTpCWa14i6BWexGpEdMVLE9WMQqe2erxUeb8RLZ3YEdKEVXIwGN+O82Q=="
#endif

static const uint8_t bundle_signature_key[] = _ADK_BUNDLE_KEY;

static bool verify_signature(
    const char * const signature,
    sb_file_t * const bundle_file,
    const size_t bundle_file_offset,
    const const_mem_region_t bundle_signature_key_region,
    uint8_t computed_signature[crypto_hmac256_base64_max_size],
    size_t * const computed_signature_length) {
    MERLIN_TRACE_PUSH_FN();

    uint8_t decoded_bundle_key[1024];
    const size_t decoded_length = crypto_decode_base64(
        bundle_signature_key_region,
        MEM_REGION(.byte_ptr = decoded_bundle_key, .size = ARRAY_SIZE(decoded_bundle_key)));
    VERIFY(decoded_length <= ARRAY_SIZE(decoded_bundle_key));

    crypto_hmac_ctx_t hmac = {0};
    crypto_hmac_ctx_init(&hmac, CONST_MEM_REGION(.byte_ptr = decoded_bundle_key, .size = decoded_length));

    uint8_t buffer[1024];
    for (;;) {
        const size_t num_bytes_read = sb_fread(buffer, 1, ARRAY_SIZE(buffer), bundle_file);

        crypto_hmac_ctx_update(&hmac, CONST_MEM_REGION(.byte_ptr = buffer, .size = num_bytes_read));

        if (num_bytes_read < ARRAY_SIZE(buffer)) {
            break;
        }
    }

    if (!sb_fseek(bundle_file, (long)bundle_file_offset, sb_seek_set)) {
        LOG_ERROR(TAG_MERLIN, "Failed to reset bundle seek location");
        MERLIN_TRACE_POP();
        return false;
    }

    uint8_t output[crypto_sha256_size];
    crypto_hmac_ctx_finish(&hmac, output);

    *computed_signature_length = crypto_encode_base64(
        CONST_MEM_REGION(.byte_ptr = output, .size = ARRAY_SIZE(output)),
        MEM_REGION(.byte_ptr = computed_signature, .size = crypto_hmac256_base64_max_size));

    MERLIN_TRACE_POP();
    return memcmp(signature, computed_signature, *computed_signature_length) == 0;
}

static cache_fetch_status_e retry_cache_fetch_resource_from_url(
    cache_t * const cache,
    const char * const key,
    const char * const url,
    const cache_update_mode_e update_mode,
    fetch_retry_context_t retry) {
    MERLIN_TRACE_PUSH_FN();

    cache_fetch_status_e fetch_status = cache_fetch_resource_from_url(cache, key, url, update_mode);
    for (int attempt = 0; fetch_status != cache_fetch_success && attempt < (int)retry.retry_max_attempts; ++attempt) {
        LOG_WARN(TAG_MERLIN, "Fetch of URL \"%s\" failed - will retry with attempt %d in %d ms.", url, attempt + 1, retry.retry_backoff_ms.ms);
        sb_thread_sleep(retry.retry_backoff_ms);
        fetch_status = cache_fetch_resource_from_url(cache, key, url, update_mode);
    }
    if (fetch_status != cache_fetch_success) {
        LOG_ERROR(TAG_MERLIN, "Could not fetch URL \"%s\" after %d retries", url, retry.retry_max_attempts);
    }

    MERLIN_TRACE_POP();
    return fetch_status;
}

static sb_file_t * load_bundle_file_from_resource(
    const manifest_resource_type_e resource_type,
    const char * const resource,
    size_t * bundle_file_offset,
    fetch_retry_context_t retry) {
    MERLIN_TRACE_PUSH_FN();

    ASSERT(bundle_file_offset != NULL);

    sb_file_t * bundle_file = NULL;

    if (resource_type == manifest_resource_file) {
        bundle_file = sb_fopen(sb_app_root_directory, resource, "rb");
        *bundle_file_offset = 0;
    } else {
        if (cache_fetch_success == retry_cache_fetch_resource_from_url(statics.cache, bundle_cache_key, resource, cache_update_mode_atomic, retry)) {
            size_t bundle_file_content_size = 0;
            if (cache_get_content(statics.cache, bundle_cache_key, &bundle_file, &bundle_file_content_size)) {
                *bundle_file_offset = (size_t)sb_ftell(bundle_file);
            }
        }
    }

    MERLIN_TRACE_POP();
    return bundle_file;
}

static wasm_memory_region_t load_wasm_from_bundle_file(sb_file_t * const bundle_file, const size_t bundle_file_offset) {
    MERLIN_TRACE_PUSH_FN();
    wasm_memory_region_t wasm_memory = {0};

    bundle_t * const bundle = bundle_open_fp(bundle_file, bundle_file_offset);

    if (bundle) {
        adk_mount_bundle(bundle);
        statics.bundle = bundle;

        sb_stat_result_t bs = bundle_stat(bundle, bundle_app_wasm_path);
        if (bs.error == sb_stat_success) {
            runtime_configuration_t runtime_config = get_default_runtime_configuration();

            conditional_overwrite_bundle_config(bundle, &runtime_config);
            VERIFY_MSG(runtime_config.wasm_memory_size > 0, "The WASM memory size was not defined in the bundle");

            // Apply manifest settings as highest precedence
            manifest_config_parse_overwrite(&runtime_config);

            // All config settings are merged at this point
            manifest_get_runtime_configuration()->memory_reservations = runtime_config.memory_reservations;
            manifest_get_runtime_configuration()->guard_page_mode = runtime_config.guard_page_mode;
            manifest_get_runtime_configuration()->coredump_memory_size = runtime_config.coredump_memory_size;
            statics.wasm_memory_size = runtime_config.wasm_memory_size;
            statics.has_processed_manifest = true;

            bundle_file_t * const wasm_file = bundle_fopen(bundle, bundle_app_wasm_path);
            if (wasm_file) {
                wasm_memory = get_active_wasm_interpreter()->load_fp(wasm_file, read_bundle_file, bs.stat.size, statics.wasm_memory_size);
                bundle_fclose(wasm_file);

                if (wasm_memory.wasm_mem_region.region.ptr == NULL) {
                    LOG_ERROR(TAG_MERLIN, "Failed to initialize WASM: %s", bundle_app_wasm_path);
                    sb_on_app_load_failure();
                    display_bundle_error_splash(bundle);
                    adk_unmount_bundle(bundle);
                    bundle_close(bundle);
                    clear_bundle_cache();

                    MERLIN_TRACE_POP();
                    return (wasm_memory_region_t){0};
                }
                LOG_INFO(TAG_MERLIN, "Loaded WASM from bundle: %s", bundle_app_wasm_path);
            } else {
                LOG_ERROR(TAG_MERLIN, "Failed to open item in bundle: %s", bundle_app_wasm_path);
                sb_on_app_load_failure();
                display_bundle_error_splash(bundle);
                adk_unmount_bundle(bundle);
                bundle_close(bundle);
                clear_bundle_cache();

                MERLIN_TRACE_POP();
                return (wasm_memory_region_t){0};
            }

        } else {
            LOG_ERROR(TAG_MERLIN, "Bundle does not contain item: %s", bundle_app_wasm_path);
            sb_on_app_load_failure();
            display_bundle_error_splash(bundle);
            adk_unmount_bundle(bundle);
            bundle_close(bundle);
            clear_bundle_cache();

            MERLIN_TRACE_POP();
            return (wasm_memory_region_t){0};
        }
    } else {
        LOG_ERROR(TAG_MERLIN, "Could not open bundle file");
        clear_bundle_cache();
    }

    MERLIN_TRACE_POP();
    return wasm_memory;
}

static wasm_interpreter_t * get_wasm_interpreter_by_name(const char * const name) {
    wasm_interpreter_t * available_interpreters[] = {
#ifdef _WASM3
        get_wasm3_interpreter(),
#endif // _WASM3
#ifdef _WAMR
        get_wamr_interpreter(),
#endif // _WAMR
    };

    for (size_t i = 0; i < ARRAY_SIZE(available_interpreters); i++) {
        if (!strcmp(name, available_interpreters[i]->name)) {
            return available_interpreters[i];
        }
    }

    return NULL;
}

wasm_memory_region_t load_wasm_from_manifest(const manifest_t * const manifest) {
    MERLIN_TRACE_PUSH_FN();
    *manifest_get_runtime_configuration() = manifest->runtime_config;
    statics.wasm_memory_size = manifest->runtime_config.wasm_memory_size;
    statics.has_processed_manifest = true;

    wasm_interpreter_t * const requested_interpreter = get_wasm_interpreter_by_name(manifest->interpreter);
    if (!requested_interpreter) {
        LOG_ERROR(TAG_MERLIN, "The interpreter referenced in the manifest does not exist in this build of Merlin: %s", manifest->interpreter);
        MERLIN_TRACE_POP();
        return (wasm_memory_region_t){0};
    } else {
        set_active_wasm_interpreter(requested_interpreter);
    }

    LOG_INFO(TAG_MERLIN, "Attempting to load WASM bundle: %s", manifest->resource);
    sb_file_t * bundle_file = NULL;
    size_t bundle_file_offset = 0;
    for (uint32_t retry_index = 0; retry_index <= manifest->runtime_config.bundle_fetch.retry_max_attempts; ++retry_index) {
        if (retry_index > 0) {
            LOG_WARN(TAG_MERLIN, "Invalid signature: will retry bundle download with attempt %d in %d ms.", retry_index, manifest->runtime_config.bundle_fetch.retry_backoff_ms.ms);
            sb_thread_sleep(manifest->runtime_config.bundle_fetch.retry_backoff_ms);
        }
        bundle_file_offset = 0;
        bundle_file = load_bundle_file_from_resource(manifest->resource_type, manifest->resource, &bundle_file_offset, manifest->runtime_config.bundle_fetch);

        if (bundle_file == NULL) {
            LOG_ERROR(TAG_MERLIN, "failed to load WASM bundle");
            clear_bundle_cache();
            MERLIN_TRACE_POP();
            return (wasm_memory_region_t){0};
        }

        // Check bundle signature in manifest against embedded key (current and old) - `break` on success

        size_t computed_signature_length = 0;
        uint8_t computed_signature[crypto_hmac256_base64_max_size];

        const const_mem_region_t bundle_signature_key_region
            = CONST_MEM_REGION(.byte_ptr = bundle_signature_key, .size = ARRAY_SIZE(bundle_signature_key) - 1);

        if (verify_signature(
                manifest->signature,
                bundle_file,
                bundle_file_offset,
                bundle_signature_key_region,
                computed_signature,
                &computed_signature_length)) {
            break;
        } else {
            LOG_ERROR(TAG_MERLIN, "Invalid signature! '%s' != '%.*s'", manifest->signature, (int)computed_signature_length, computed_signature);
            if (statics.args.skip_signature) {
                break;
            }
        }

        sb_fclose(bundle_file);
        clear_bundle_cache();
        bundle_file = NULL;
    }

    if (!bundle_file) {
        clear_bundle_cache();
        LOG_ERROR(TAG_MERLIN, "Could not verify signature after %d retry attempts.", manifest->runtime_config.bundle_fetch.retry_max_attempts);
        MERLIN_TRACE_POP();
        return (wasm_memory_region_t){0};
    }

    const wasm_memory_region_t wasm_mem = load_wasm_from_bundle_file(bundle_file, bundle_file_offset);
    MERLIN_TRACE_POP();
    return wasm_mem;
}

// Inits curl, loads manifest from given url, loads wasm via the manifest, shuts down curl
static wasm_memory_region_t load_wasm_from_manifest_url(const char * const manifest_url) {
    MERLIN_TRACE_PUSH_FN();
    wasm_memory_region_t wasm_memory = {0};

    const cache_fetch_status_e fetch_status = retry_cache_fetch_resource_from_url(
        statics.cache, manifest_cache_key, manifest_url, cache_update_mode_atomic, default_fetch_retry);

    bool should_clear_cache = false; // manifest file is removed from cache when error occurs

    if (fetch_status == cache_fetch_success) {
        sb_file_t * manifest_file = NULL;
        size_t manifest_file_content_size = 0;
        LOG_ALWAYS(TAG_MERLIN, "Fetched manifest from URL \"%s\"", manifest_url);

        if (cache_get_content(statics.cache, manifest_cache_key, &manifest_file, &manifest_file_content_size)) {
            const manifest_t manifest = manifest_parse_fp(manifest_file, manifest_file_content_size);
            if (manifest_is_empty(&manifest)) {
                LOG_ERROR(TAG_MERLIN, "Could not parse manifest at URL \"%s\"", manifest_url);
                should_clear_cache = true;
            } else {
                wasm_memory = load_wasm_from_manifest(&manifest);
                if (!wasm_memory.wasm_mem_region.region.ptr) {
                    LOG_ERROR(TAG_MERLIN, "Could not load a WASM file with the manifest at URL \"%s\"", manifest_url);
                    should_clear_cache = true;
                }
            }

            sb_fclose(manifest_file);
        }
    } else {
        LOG_ERROR(TAG_MERLIN, "Could not fetch URL \"%s\" after %d retries", manifest_url, default_fetch_retry.retry_max_attempts);
        should_clear_cache = true;
    }

    if (should_clear_cache) {
        clear_manifest_cache();
    }

    MERLIN_TRACE_POP();
    return wasm_memory;
}

// Loads manifest from local file then loads wasm via the manifest
static wasm_memory_region_t load_wasm_from_manifest_file(const char * const path) {
    MERLIN_TRACE_PUSH_FN();

    char manifest_path[sb_max_path_length];
    to_fwd_slash(manifest_path, ARRAY_SIZE(manifest_path), path);

    wasm_memory_region_t wasm_memory = {0};

    mem_region_t manifest_blob = manifest_alloc_file_blob(manifest_path);
    if (manifest_blob.ptr) {
        if (load_artifact_data(sb_app_root_directory, manifest_blob, manifest_path, 0)) {
            LOG_INFO(TAG_MERLIN, "Opened manifest \"%s\"", path);
            const manifest_t manifest = manifest_parse(manifest_blob.consted);
            if (manifest_is_empty(&manifest)) {
                LOG_ERROR(TAG_MERLIN, "Could not parse manifest \"%s\"", path);
            } else {
                wasm_memory = load_wasm_from_manifest(&manifest);
                if (!wasm_memory.wasm_mem_region.region.ptr) {
                    LOG_ERROR(TAG_MERLIN, "Could not load a WASM file with the manifest \"%s\"", path);
                }
            }
        }
        manifest_free_file_blob(manifest_blob);
    }

    MERLIN_TRACE_POP();
    return wasm_memory;
}

static bool string_starts_with(const char * const s, const char * const prefix) {
    return strncmp(prefix, s, strlen(prefix)) == 0;
}

wasm_memory_region_t load_wasm_from_bundle(const char * const path) {
    MERLIN_TRACE_PUSH_FN();

    size_t bundle_file_offset = 0;
    sb_file_t * bundle_file;

    if (string_starts_with(path, "http://") || string_starts_with(path, "https://")) {
        bundle_file = load_bundle_file_from_resource(manifest_resource_url, path, &bundle_file_offset, default_fetch_retry);
    } else {
        char bundle_path[sb_max_path_length];
        to_fwd_slash(bundle_path, ARRAY_SIZE(bundle_path), path);

        bundle_file = load_bundle_file_from_resource(manifest_resource_file, bundle_path, &bundle_file_offset, default_fetch_retry);
    }

    if (bundle_file == NULL) {
        LOG_ERROR(TAG_MERLIN, "Failed to read bundle file: %s", path);
        sb_on_app_load_failure();
        display_default_error_splash();
        MERLIN_TRACE_POP();
        return (wasm_memory_region_t){0};
    }

    const wasm_memory_region_t wasm_mem = load_wasm_from_bundle_file(bundle_file, bundle_file_offset);
    MERLIN_TRACE_POP();
    return wasm_mem;
}

// Loads wasm from local file with local .config
static wasm_memory_region_t load_wasm_from_file_path(const char * const path) {
    MERLIN_TRACE_PUSH_FN();

    char file_path[sb_max_path_length];
    to_fwd_slash(file_path, ARRAY_SIZE(file_path), path);

    runtime_configuration_t config = get_default_runtime_configuration();

    char config_file_path[sb_max_path_length];
    to_fwd_slash(
        config_file_path,
        ARRAY_SIZE(config_file_path),
        (statics.args.config_file_path != NULL) ? statics.args.config_file_path : default_bundle_config_file_path);

    if (!parse_bundle_config(config_file_path, &config)) {
        LOG_ERROR(
            TAG_MERLIN,
            "Failed to load configuration file (%s) - expected either the --config $FILE option or a file at %s",
            config_file_path,
            default_bundle_config_file_path);

        MERLIN_TRACE_POP();
        return (wasm_memory_region_t){0};
    }

    VERIFY_MSG(config.wasm_memory_size > 0, "The WASM memory size was not defined in the bundle");
    manifest_get_runtime_configuration()->memory_reservations = config.memory_reservations;
    manifest_get_runtime_configuration()->guard_page_mode = config.guard_page_mode;
    manifest_get_runtime_configuration()->coredump_memory_size = config.coredump_memory_size;
    statics.wasm_memory_size = config.wasm_memory_size;

    const wasm_memory_region_t wasm_memory = get_active_wasm_interpreter()->load(sb_app_root_directory, file_path, statics.wasm_memory_size);
    MERLIN_TRACE_POP();
    return wasm_memory;
}

// Loads Wasm from a manifest URL retrieved from persona file
static wasm_memory_region_t load_wasm_from_persona_file(const char * const path) {
    MERLIN_TRACE_PUSH_FN();

    if (statics.args.manifest_url == NULL || statics.args.manifest_url[0] == 0) {
        MERLIN_TRACE_POP();
        return (wasm_memory_region_t){0};
    }

    static char formatted_manifest_url[1024];
    manifest_format_default_url(formatted_manifest_url, sizeof(formatted_manifest_url), statics.args.manifest_url);

    const wasm_memory_region_t wasm_memory = load_wasm_from_manifest_url(formatted_manifest_url);
    MERLIN_TRACE_POP();
    return wasm_memory;
}

static const char no_arg[] = ""; // signifies that option doesn't take an argument

// Define all command-line options.
static const struct {
    opt_e opt; // option char
    const char * long_opt; // long option name
    const char * arg_help; // option arg help text, or 'no_arg'
    const char * help; // help text
    loader_func_t loader; // wasm loader func, if applicable
} options[] = {
    {opt_act_help, "help", no_arg, "Print usage notes", NULL},
    {opt_act_version, "version", no_arg, "Print version info", NULL},
#ifndef _SHIP
    {opt_act_load_bundle_file, "bundle", "<BUNDLE-FILE>", "Load the specified bundle from the local file system", load_wasm_from_bundle},
    {opt_act_load_wasm_file, "wasm", "<WASM-FILE>", "Load the specified Wasm application from the local file system", load_wasm_from_file_path},
    {opt_skip_signature, "skip-signature", no_arg, "Skip the signature verification for bundles", NULL},
    {opt_use_config, "config", "<CONFIG-FILE>", "Load wasm application selected by config file from the local file system", NULL},
    {opt_test, "test", no_arg, "Test command-line parsing", NULL},
#endif
#if !defined(_SHIP) || defined(_SHIP_DEV)
    {opt_act_load_manifest_url, "manifest-url", "<MANIFEST-URL>", "Load wasm application selected by manifest found at specified URL", load_wasm_from_manifest_url},
    {opt_act_load_manifest_file, "manifest", "<MANIFEST-FILE>", "Load wasm application selected by manifest from the local file system", load_wasm_from_manifest_file},
    {opt_act_load_persona_file, "persona-file", "<PERSONA-FILE>", "Load wasm application selected by persona file from the local file system", load_wasm_from_persona_file},
    {opt_set_persona_id, "persona-id", "<PERSONA-ID>", "Override the persona ID to be selected from the persona file", NULL},
#endif
#if _MERLIN_DEMOS
    {opt_set_demo_asset_root, "asset-root", "<PATH>", "Set demo app's asset root directory", NULL},
    {opt_act_run_demo, "demo", "<DEMO>", "Run native demo", NULL},
#endif
#ifndef _SHIP
    {'I', "log-canvas-image-load", no_arg, "Enable canvas image logging on non _SHIP builds", NULL},
#endif
    {opt_no_app_load, "no-app-load", no_arg, "Refrain from running the default app", NULL},
};

#if _MERLIN_DEMOS
typedef int (*demo_main_t)(const int argc, const char * const * const argv);

static const struct {
    const char * name;
    demo_main_t main;
} demos[] = {
    {"canvas-demo", canvas_demo_main},
    {"font-demo", font_demo_main},
    {"hello-cube", hello_cube_main},
#ifdef _CANVAS_EXPERIMENTAL
    {"canvas-exp", canvas_experimental_demo_main},
#endif
};
#endif

// Prints usage notes.
static void print_help() {
    debug_write_line(
        "\n"
        "m5 app runner\n"
        "\n"
        "USAGE:\n"
        "    merlin [OPTIONS]\n"
        "\n"
        "OPTIONS:\n");

    for (int optx = 0; optx < ARRAY_SIZE(options); optx++) {
        const int n = max_int(26 - (int)strlen(options[optx].long_opt), 0);
        debug_write_line("    --%s %-*s%s", options[optx].long_opt, n, options[optx].arg_help, options[optx].help);
        if (options[optx].opt != opt_unknown) {
            debug_write_line("    -%c\n", options[optx].opt);
        }
    }

#if _MERLIN_DEMOS
    debug_write_line(
        "PATH:\n"
        "    Relative path to demo assets, default: %s\n",
        default_demo_asset_root);

    debug_write_line("DEMO:");

    for (int dx = 0; dx < ARRAY_SIZE(demos); dx++) {
        debug_write_line("    %s", demos[dx].name);
    }

    debug_write_line("");
#endif
}

// Prints ADK version.  This is redundant in debug builds due to version logging.
static void print_version() {
    debug_write_line(
        "DSS ADK version %s\n"
        "(c) 2020-2021 The Walt Disney Company. All rights reserved.",
        ADK_VERSION_STRING);
}

// Returns true if argument looks like an option, i.e. matches /^-[^0-9]/
static inline bool is_option(const char * const arg) {
    return (arg[0] == '-') && !isdigit(arg[1]);
}

// Parses command-line args into 'statics.args'.  Iterates all args for predictable behavior when given bogus args.
// Examples:
//
//      merlin -w my.wasm -w my-other.wasm      # Which one should be loaded?
//      merlin -w my.wasm -M my-manifest.json   # Should wasm or manifest be used?
//      merlin -w my.wasm -u -m -M              # Missing args on last 3 options
//
static merlin_exit_code_e parse_args(const int argc, const char * const * const argv) {
    MERLIN_TRACE_PUSH_FN();

    if (argc > max_argv) {
        debug_write_line("%d command-line args exceeds limit of %d", argc, max_argv);
        MERLIN_TRACE_POP();
        return merlin_exit_code_cli_too_many_args;
    }

    for (int argx = 0; argx < argc; argx++) {
        if (is_option(argv[argx])) {
            const bool is_long_opt = (argv[argx][1] == '-');

            // Lookup long or short option
            int optx;
            for (optx = 0; optx < ARRAY_SIZE(options); optx++) {
                if ((is_long_opt && (strcasecmp(options[optx].long_opt, argv[argx] + 2) == 0))
                    || (!is_long_opt && ((char)options[optx].opt == argv[argx][1]) && !argv[argx][2])) {
                    break; // matched
                }
            }
            if (optx >= ARRAY_SIZE(options)) {
                continue; // ignore unknown option, possible partner extension
            }

            // If option requires an argument, check that it's present
            int opt_arg = argx;
            if (options[optx].arg_help != no_arg) {
                ++opt_arg;
                if ((opt_arg >= argc) || is_option(argv[opt_arg])) {
                    debug_write_line("Missing argument for \"%s\"", argv[argx]);
                    MERLIN_TRACE_POP();
                    return merlin_exit_code_cli_missing_parameter;
                }
            }

            // First handle non-action options
            if (options[optx].opt == opt_no_app_load) {
                statics.args.no_app_load = true;
                continue;
            }

#ifndef _SHIP
            if (options[optx].opt == opt_set_persona_id) {
                statics.args.persona_id = argv[opt_arg];
            } else if (options[optx].opt == opt_skip_signature) {
                statics.args.skip_signature = true;
            } else if (options[optx].opt == opt_use_config) {
                statics.args.config_file_path = argv[opt_arg];
            } else if (options[optx].opt == opt_test) {
                statics.args.test = true;
            } else
#endif
#if _MERLIN_DEMOS
                if (options[optx].opt == opt_set_demo_asset_root) {
                statics.args.demo_asset_root = argv[opt_arg];
            } else
#endif
            {
                // Handle action options

                // Process action with no args immediately
                if (opt_arg == argx) {
                    statics.args.action = options[optx].opt;
                    return merlin_exit_code_success;
                }

                // Allow only one action per invocation
                if (statics.args.action != opt_unknown) {
                    debug_write_line("Option \"%s\" is redundant, action already specified", argv[argx]);
                    return merlin_exit_code_cli_redundant_action;
                }

                // Save action and related info
                statics.args.action = options[optx].opt;
                statics.args.action_arg = (opt_arg == argx) ? NULL : argv[opt_arg];
                statics.args.loader = options[optx].loader; // may be NULL
            }

            argx = opt_arg;
        }
    }

    MERLIN_TRACE_POP();
    return merlin_exit_code_success;
}

#ifndef _SHIP
static void print_args() {
    debug_write_line(
        "statics.args = {\n"
        "  .action =          '%c',\n"
        "  .action_arg =      \"%s\",\n"
        "  .loader =          0x%p,\n"
        "  .persona_file_path = \"%s\",\n"
        "  .persona_id = \"%s\",\n"
        "  .manifest_url = \"%s\",\n"
        "  .no_app_load = \"%s\",\n"
#if _MERLIN_DEMOS
        "  .demo_asset_root = \"%s\",\n"
#endif
        "};\n",
        statics.args.action ? statics.args.action : ' ',
        statics.args.action_arg ? statics.args.action_arg : "",
        statics.args.loader,
        statics.args.persona_file_path,
        statics.args.persona_id,
        statics.args.manifest_url,
        statics.args.no_app_load ? "true" : "false"
#if _MERLIN_DEMOS
        ,
        statics.args.demo_asset_root
#endif
    );
}
#endif

void verify_wasm_call_and_halt_on_failure(const wasm_call_result_t call_result) {
    if (call_result.status != wasm_call_success) {
        LOG_ERROR(TAG_MERLIN, "wasm call to %s has failed with code %i", call_result.func_name, call_result.status);

#ifdef _SHIP
        enum { message_buffer_size = 1024 };
        char message_buffer[message_buffer_size] = {0};
        snprintf(message_buffer, message_buffer_size, "%s\n%i", statics.fallback_error_message, call_result.status);
#else
        enum { message_buffer_size = 4096 };
        char message_buffer[message_buffer_size] = {0};
        snprintf(message_buffer, message_buffer_size, "%s(%d): HALT: %s\n%s", __FILE__, __LINE__, statics.fallback_error_message, call_result.details);
#endif

        adk_halt(message_buffer);
    }
}

// Periodically runs wasm's 'app_tick' function
static int tick(const uint32_t abstime, const float dt, void * arg) {
    MERLIN_TRACE_PUSH_FN();

    uint32_t ret = 0;
    const wasm_call_result_t tick_call_result = get_active_wasm_interpreter()->call_ri_ifp("app_tick", &ret, abstime, dt, arg);
    verify_wasm_call_and_halt_on_failure(tick_call_result);

    MERLIN_TRACE_POP();
    return ret && !tick_call_result.status;
}

// Main body of app
static merlin_exit_code_e app_body(const int argc, const char * const * const argv) {
    adk_system_metrics_t sm;
    adk_get_system_metrics(&sm);

    { // Retrieve system metrics for persona id and update default persona file based on tenancy

        statics.args.persona_id = sm.persona_id;

        if (strcmp(sm.tenancy, "prod") != 0) {
            statics.args.persona_file_path = default_persona_file_non_prod;
        }
    }

    const int parse_ret = parse_args(argc, argv);

    // Handle command line argument test or failure
#ifndef _SHIP
    if (statics.args.test) {
        print_args();
        return merlin_exit_code_success;
    }
#endif

    if (parse_ret != merlin_exit_code_success) {
        return parse_ret;
    }

    // Handle non-wasm-load actions
#if _MERLIN_DEMOS
    if (statics.args.action == opt_act_run_demo) {
        for (int dx = 0; dx < ARRAY_SIZE(demos); ++dx) {
            if (strcasecmp(statics.args.action_arg, demos[dx].name) == 0) {
                the_app.display_settings._720p_hack = true;
                if (!app_init_subsystems(*manifest_get_runtime_configuration())) {
                    return merlin_exit_code_app_init_subsystems_failure;
                }
                demos[dx].main(argc, argv);
                return merlin_exit_code_success;
            }
        }
        debug_write_line("Unknown --demo \"%s\"", statics.args.action_arg);
        return merlin_exit_code_unknown;
    } else if (statics.args.demo_asset_root != default_demo_asset_root) {
        debug_write_line("Ignoring --asset-root \"%s\", not applicable", statics.args.demo_asset_root);
    }
#endif

    if (statics.args.action == opt_act_help) {
        print_help();
        return merlin_exit_code_success;
    } else if (statics.args.action == opt_act_version) {
        print_version();
        return merlin_exit_code_success;
    }

    // Start extensions
    const char * const extensions_path = getargarg("--extensions", argc, argv);
    extender_status_e extender_status = bind_extensions(extensions_path);
    LOG_DEBUG(TAG_MERLIN, "bind_extensions %s", (extender_status == extender_status_success) ? "succeeded" : "failed");

    bundle_init();
    if (extender_status == extender_status_success) {
        extender_status = start_extensions(&the_app.bus);
    }

    if (extender_status != extender_status_success) {
        return merlin_exit_code_extensions_failure;
    }

    // If no load action specified, load the wasm according to the default persona
    if (statics.args.loader == NULL && statics.args.no_app_load == false) {
        statics.args.action_arg = statics.args.persona_file_path;
        statics.args.loader = load_wasm_from_persona_file;
    }

    statics.cache_pages = sb_map_pages(PAGE_ALIGN_INT(cache_region_size), system_page_protect_read_write);

    // Handle wasm-load actions
    merlin_exit_code_e retval = merlin_exit_code_success;
    while (statics.args.loader != NULL) {
        // Finalize persona values, fetch manifest url, and create cache
        if (statics.args.loader == load_wasm_from_persona_file) {
            persona_mapping_t mapping;
            strcpy_s(mapping.file, ARRAY_SIZE(mapping.file), statics.args.action_arg);
            strcpy_s(mapping.id, ARRAY_SIZE(mapping.id), statics.args.persona_id);
            mapping.manifest_url[0] = 0;
            mapping.fallback_error_message[0] = 0;

            get_persona_mapping(&mapping);
            statics.args.persona_id = mapping.id;
            statics.args.manifest_url = mapping.manifest_url;

            if (mapping.fallback_error_message[0] != '\0') {
                strcpy_s(statics.fallback_error_message, adk_max_message_length, mapping.fallback_error_message);
            }

            // Build cache directory path from persona id
            char cache_path[sb_max_path_length];
            sprintf_s(cache_path, ARRAY_SIZE(cache_path), "persona/%s/", mapping.id);
            statics.cache = cache_create(cache_path, statics.cache_pages);
        } else {
            statics.cache = cache_create("native/", statics.cache_pages);
            strcpy_s(statics.fallback_error_message, adk_max_message_length, default_fallback_error_message);
        }

        manifest_init(&sm);

        // Run the appropriate wasm loader
        wasm_memory_region_t wasm_memory = statics.args.loader(statics.args.action_arg);
        if (wasm_memory.wasm_mem_region.region.ptr == NULL) {
            LOG_ERROR(TAG_MERLIN, "Unable to load \"%s\"", statics.args.action_arg);
            retval = merlin_exit_code_wasm_load_failure;
            break;
        }

        runtime_configuration_t rt = *manifest_get_runtime_configuration();

        if (!app_init_subsystems(rt)) {
            retval = merlin_exit_code_app_init_subsystems_failure;
            break;
        }

        TTFI_TRACE_TIME_SPAN_END(&time_to_first_interaction.main_timestamp);
        TTFI_TRACE_TIME_SPAN_BEGIN(&time_to_first_interaction.app_init_timestamp, TIME_TO_FIRST_INTERACTION_STR " app init");
        time_to_first_interaction.app_init_timestamp = adk_read_millisecond_clock();

        uint32_t init_result_restart = 0;
        const wasm_call_result_t wasm_init_result_restart = get_active_wasm_interpreter()->call_ri_argv("__app_init", &init_result_restart, (uint32_t)argc, (void *)argv);
        verify_wasm_call_and_halt_on_failure(wasm_init_result_restart);

        app_event_loop(tick, NULL);

        uint32_t shutdown_result_restart = 0;
        const wasm_call_result_t wasm_shutdown_result_restart = get_active_wasm_interpreter()->call_ri("app_shutdown", &shutdown_result_restart);
        verify_wasm_call_and_halt_on_failure(wasm_shutdown_result_restart);

        get_active_wasm_interpreter()->unload(wasm_memory);

        if (statics.bundle) {
            adk_unmount_bundle(statics.bundle);
            bundle_close(statics.bundle);
        }

        cache_destroy(statics.cache);

        break;
    }

    manifest_shutdown();

    sb_unmap_pages(statics.cache_pages);

    bundle_shutdown();

    stop_extensions();
    extender_status = unbind_extensions();
    LOG_DEBUG(TAG_MERLIN, "unbind_extensions %s", (extender_status == extender_status_success) ? "succeeded" : "failed");

    return retval;
}

static void initialize_wasm_interpreter() {
#ifdef _WASM3
    set_active_wasm_interpreter(get_wasm3_interpreter());
#elif defined(_WAMR)
    set_active_wasm_interpreter(get_wamr_interpreter());
#else
#error "Could not find a default Wasm interpreter, at least one must be enabled to compile Merlin."
#endif

#ifdef _WASM3
    extender_status_e wasm3_link_adk(void * const wasm_interpreter_instance);
    extender_status_e wasm3_link_nve(void * const wasm_interpreter_instance);
    wasm3_register_linker(wasm3_link_adk);
    wasm3_register_linker(wasm3_link_nve);
    wasm3_register_linker(link_extensions_in_wasm3);
#endif // _WASM3

#ifdef _WAMR
    extender_status_e wamr_link_adk(void * const wasm_interpreter_instance);
    extender_status_e wamr_link_nve(void * const wasm_interpreter_instance);
    wamr_register_linker(wamr_link_adk);
    wamr_register_linker(wamr_link_nve);
    wamr_register_linker(link_extensions_in_wamr);
#endif // _WAMR
}

// Entry point
int app_main(const int argc, const char * const * const argv) {
    adk_cjson_context_initialize();

    *manifest_get_runtime_configuration() = get_default_runtime_configuration();

    initialize_wasm_interpreter();

    const merlin_exit_code_e exit_code = app_body(argc, argv);

    if ((exit_code != merlin_exit_code_success) && (exit_code != merlin_exit_code_app_shutdown_failure)) {
        LOG_DEBUG(TAG_MERLIN, "ADK did not exit successfully: %d", exit_code);
        sb_on_app_load_failure();
        display_default_error_splash();
    }

    return exit_code;
}
