/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
persona.c

Support for running multiple apps with the same m5 core
*/

#include "persona.h"

#include "extern/cjson/cJSON.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/file/file.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_locale.h"
#include "source/adk/steamboat/sb_platform.h"

#define TAG_PERSONA FOURCC('P', 'R', 'S', 'A')

// Constants
#define DECL_CONST_STR(STR) static const char c_##STR[] = #STR;

DECL_CONST_STR(persona_heap_tag);
DECL_CONST_STR(v1);
DECL_CONST_STR(partner);
DECL_CONST_STR(default_persona);
DECL_CONST_STR(personas);
DECL_CONST_STR(id);
DECL_CONST_STR(manifest_url);
DECL_CONST_STR(error_message);
DECL_CONST_STR(error);
DECL_CONST_STR(cache_request_timeout_in_seconds);
DECL_CONST_STR(retry_config);
DECL_CONST_STR(default_locales);
DECL_CONST_STR(localized);

#undef DECL_CONST_STR

enum {
    persona_heap_size = 1 * 1024 * 1024,
};

// File-scope variables
static struct {
    heap_t heap;
    mem_region_t pages;
    cJSON * json_root;
    cJSON_Env json_ctx;
} statics;

static void * cjson_malloc(void * const ctx, size_t const size) {
    heap_t * const heap = (heap_t *)ctx;
    return heap_alloc(heap, size, MALLOC_TAG);
}

static void cjson_free(void * const ctx, void * const ptr) {
    if (ptr != NULL) {
        heap_t * const heap = (heap_t *)ctx;
        heap_free(heap, ptr, MALLOC_TAG);
    }
}

// File blob alloc/free
mem_region_t persona_alloc_file_blob(const char * const persona_file) {
    const size_t persona_file_size = get_artifact_size(sb_app_root_directory, persona_file);
    if (persona_file_size == 0) {
        return (mem_region_t){0};
    }

    return MEM_REGION(.ptr = heap_alloc(&statics.heap, persona_file_size, MALLOC_TAG), .size = persona_file_size);
}

void persona_free_file_blob(mem_region_t const persona_blob) {
    if (persona_blob.ptr != NULL) {
        heap_free(&statics.heap, persona_blob.ptr, MALLOC_TAG);
    }
}

// State cleanup
void persona_reset_state() {
    if (statics.json_root) {
        cJSON_Delete(&statics.json_ctx, statics.json_root);
        statics.json_root = NULL;
    }
}

// Init / Shutdown
void persona_init() {
    // Create persona heap
    statics.pages = sb_map_pages(PAGE_ALIGN_INT(persona_heap_size), system_page_protect_read_write);
    TRAP_OUT_OF_MEMORY(statics.pages.ptr);
    heap_init_with_region(&statics.heap, statics.pages, 8, 0, c_persona_heap_tag);

    statics.json_ctx = (cJSON_Env){
        .ctx = &statics.heap,
        .callbacks = {
            .malloc = cjson_malloc,
            .free = cjson_free,
        }};
}

void persona_shutdown() {
    persona_reset_state();
    heap_destroy(&statics.heap, c_persona_heap_tag);
    sb_unmap_pages(statics.pages);
}

static void build_localized_fallback_message(persona_mapping_t * mapping, cJSON * const persona_item, const cJSON * const error_message) {
    const cJSON * const localized_messages = cJSON_GetObjectItem(error_message, c_localized);
    if (!cJSON_IsObject(localized_messages)) {
        LOG_ERROR(TAG_PERSONA, "Invalid json \"%s\"", c_localized);
        return;
    }

    enum { locale_str_max_length = sb_locale_str_size * 2 };
    const sb_locale_t user_locale = sb_get_locale();
    char user_locale_str[locale_str_max_length] = {0};
    snprintf(user_locale_str, locale_str_max_length, "%s-%s", (char *)user_locale.language, (char *)user_locale.region);
    bool user_locale_found = false;

    size_t offset = 0;

    const cJSON * const default_locales = cJSON_GetObjectItem(error_message, c_default_locales);
    if (cJSON_IsArray(default_locales)) {
        cJSON * default_locale_item;
        cJSON_ArrayForEach(default_locale_item, default_locales) {
            if (!cJSON_IsString(default_locale_item)) {
                LOG_ERROR(TAG_PERSONA, "Invalid locale element in \"%s\"", c_default_locales);
                continue;
            }

            const char * default_locale_str = cJSON_GetStringValue(default_locale_item);

            const cJSON * const message = cJSON_GetObjectItem(localized_messages, default_locale_str);
            if (!cJSON_IsString(message)) {
                LOG_ERROR(TAG_PERSONA, "Localized message for locale '%s' not found", message);
                continue;
            }

            const char * message_value = cJSON_GetStringValue(message);
            offset += snprintf(mapping->fallback_error_message + offset, adk_max_message_length - offset, "%s\n", message_value);

            user_locale_found = user_locale_found || (strcasecmp(user_locale_str, default_locale_str) == 0);
        }
    }

    if (!user_locale_found) {
        const cJSON * const message = cJSON_GetObjectItem(localized_messages, user_locale_str);
        if (!cJSON_IsString(message)) {
            LOG_WARN(TAG_PERSONA, "Localized message for locale '%s' not found", user_locale_str);
            return;
        }

        const char * message_value = cJSON_GetStringValue(message);
        snprintf(mapping->fallback_error_message + offset, adk_max_message_length - offset, "%s", message_value);
    }
}

bool persona_parse_for_mapping(persona_mapping_t * mapping, const_mem_region_t const persona_blob) {
    if (statics.json_root != NULL) {
        persona_reset_state();
    }

    statics.json_root = cJSON_Parse(&statics.json_ctx, persona_blob);
    if (statics.json_root == NULL) {
        LOG_ERROR(TAG_PERSONA, "Invalid JSON syntax");
        return false;
    }

    // Get "v1"
    cJSON * const v1_item = cJSON_GetObjectItem(statics.json_root, c_v1);
    if (!cJSON_IsObject(v1_item)) {
        LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_v1);
        return false;
    }

    // Get "default_persona" and update mapping->id only if required
    if (mapping->id[0] == 0) {
        cJSON * const default_persona_item = cJSON_GetObjectItem(v1_item, c_default_persona);
        if (!cJSON_IsString(default_persona_item)) {
            LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_default_persona);
            return false;
        }

        strcpy_s(mapping->id, ARRAY_SIZE(mapping->id), cJSON_GetStringValue(default_persona_item));
        LOG_INFO(TAG_PERSONA, "Retrieved default_persona id: \"%s\"", mapping->id);

        if (mapping->id[0] == 0) {
            LOG_ERROR(TAG_PERSONA, "default_persona string is required, but empty", c_default_persona);
            return false;
        }
    }

    cJSON * const partner_item = cJSON_GetObjectItem(v1_item, c_partner);
    if (!cJSON_IsObject(partner_item)) {
        LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_partner);
        return false;
    } else {
        cJSON * const partner_name_item = cJSON_GetObjectItem(partner_item, "name");
        if (!cJSON_IsString(partner_name_item)) {
            LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"partner.name\"");
            return false;
        }
        strcpy_s(mapping->partner_name, ARRAY_SIZE(mapping->partner_name), cJSON_GetStringValue(partner_name_item));

        cJSON * const partner_guid_item = cJSON_GetObjectItem(partner_item, "guid");
        if (!cJSON_IsString(partner_guid_item)) {
            LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"partner.guid\"");
            return false;
        }
        strcpy_s(mapping->partner_guid, ARRAY_SIZE(mapping->partner_guid), cJSON_GetStringValue(partner_guid_item));
    }

    // Get "personas"
    cJSON * const personas_array = cJSON_GetObjectItem(v1_item, c_personas);
    if (!cJSON_IsArray(personas_array)) {
        LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_personas);
        return false;
    }

    // Iterate over personas_array searching for a matching persona
    cJSON * persona_item;
    cJSON_ArrayForEach(persona_item, personas_array) {
        // Get "id"
        cJSON * const id_item = cJSON_GetObjectItem(persona_item, c_id);
        if (!cJSON_IsString(id_item)) {
            LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_id);
        } else {
            // Check for persona ID match
            if (strcmp(cJSON_GetStringValue(id_item), mapping->id) == 0) {
                // Get "manifest_url"
                cJSON * const manifest_url_item = cJSON_GetObjectItem(persona_item, c_manifest_url);
                if (!cJSON_IsString(manifest_url_item)) {
                    LOG_ERROR(TAG_PERSONA, "Invalid JSON: \"%s\"", c_manifest_url);
                    return false;
                }

                strcpy_s(mapping->manifest_url, ARRAY_SIZE(mapping->manifest_url), cJSON_GetStringValue(manifest_url_item));

                // Check the new "error" field first, if there's no such field, fallback to the "old" error_message otherwise throw...
                const cJSON * const error_object = cJSON_GetObjectItem(persona_item, c_error);
                if (cJSON_IsObject(error_object)) {
                    build_localized_fallback_message(mapping, persona_item, error_object);
                } else {
                    // If "error" field is not specified in persona, fallback to the older "error_message" field
                    const cJSON * const error_message_object = cJSON_GetObjectItem(persona_item, c_error_message);
                    if (cJSON_IsString(error_message_object)) {
                        strcpy_s(mapping->fallback_error_message, adk_max_message_length, cJSON_GetStringValue(error_message_object));
                    }
                }

                cJSON * const cache_request_timeout_obj = cJSON_GetObjectItem(persona_item, c_cache_request_timeout_in_seconds);
                if (cJSON_IsNumber(cache_request_timeout_obj)) {
                    mapping->cache_request_timeout = cache_request_timeout_obj->valueint;
                }

                cJSON * const retry_config_obj = cJSON_GetObjectItem(persona_item, c_retry_config);
                if (cJSON_IsObject(retry_config_obj)) {
                    cJSON * const max_retries_obj = cJSON_GetObjectItem(retry_config_obj, "max_retries");
                    if (cJSON_IsNumber(max_retries_obj)) {
                        mapping->retry_config.max_retries = max_retries_obj->valueint;
                    }

                    cJSON * const retry_backoff_ms_obj = cJSON_GetObjectItem(retry_config_obj, "retry_backoff_in_millis");
                    if (cJSON_IsNumber(retry_backoff_ms_obj)) {
                        mapping->retry_config.retry_backoff_ms = (milliseconds_t){.ms = retry_backoff_ms_obj->valueint};
                    }
                }

                return true;
            }
        }
    }

    LOG_ERROR(TAG_PERSONA, "Could not find match for persona ID: \"%s\"", mapping->id);

    return false;
}

// Opens the file inside the person mapping and attempts to retrieve a manifest url matching the given persona id
// If the given persona ID is NULL or an empty string, it will be overwritten in the mapping by the default persona id provided in the json
// If there is a match for the persona id, the manifest url field will be overwritten by the value retrieved from the json
bool get_persona_mapping(persona_mapping_t * mapping) {
    LOG_INFO(TAG_PERSONA, "Fetching manifest URL from persona file \"%s\" with persona ID \"%s\"", mapping->file, mapping->id);

    // Exit early if the mapping or persona file have not been provided
    if (!mapping || (mapping->file[0] == 0)) {
        LOG_ERROR(TAG_PERSONA, "Invalid persona mapping struct provided");
        return false;
    }

    bool success = false;

    persona_init();
    mem_region_t persona_blob = persona_alloc_file_blob(mapping->file);

    if (!persona_blob.ptr) {
        LOG_ERROR(TAG_PERSONA, "Persona file not found or empty: \"%s\"", mapping->file);
    } else {
        if (load_artifact_data(sb_app_root_directory, persona_blob, mapping->file, 0)) {
            success = persona_parse_for_mapping(mapping, persona_blob.consted);
            if (success) {
                LOG_INFO(TAG_PERSONA, "Retrieved manifest URL: \"%s\"", mapping->manifest_url);
            }
        }
    }

    persona_free_file_blob(persona_blob);
    persona_shutdown();

    return success;
}
