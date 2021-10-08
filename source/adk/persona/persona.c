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
#include "source/adk/steamboat/sb_platform.h"

#define TAG_PERSONA FOURCC('P', 'R', 'S', 'A')

// Constants
#define DECL_CONST_STR(STR) static const char c_##STR[] = #STR;

DECL_CONST_STR(persona_heap_tag);
DECL_CONST_STR(v1);
DECL_CONST_STR(default_persona);
DECL_CONST_STR(personas);
DECL_CONST_STR(id);
DECL_CONST_STR(manifest_url);
DECL_CONST_STR(error_message);

#undef DECL_CONST_STR

enum {
    persona_heap_size = 32 * 1024,
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

                // Successfully retrieved a manifest url! Return early to avoid parsing the rest of the file.
                strcpy_s(mapping->manifest_url, ARRAY_SIZE(mapping->manifest_url), cJSON_GetStringValue(manifest_url_item));

                const cJSON * const message = cJSON_GetObjectItem(persona_item, c_error_message);
                if (cJSON_IsString(message)) {
                    strcpy_s(mapping->fallback_error_message, adk_max_message_length, cJSON_GetStringValue(message));
                } else {
                    mapping->fallback_error_message[0] = 0;
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
