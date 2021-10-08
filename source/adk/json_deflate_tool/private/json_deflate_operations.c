/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_operations.c

JSON deflate high-level operations (infer, prepare, view, parse).
*/

#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate_tool/json_deflate_tool.h"
#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_file.h"

enum {
    json_deflate_tool_cjson_area_size = 512 * 1024,
    json_deflate_tool_max_json_file_size = 2 * 1024 * 1024,
    json_deflate_tool_max_bump_area_size = 2 * 1024 * 1024,
};

static void * cjson_node_alloc(void * env, size_t size) {
    const size_t alignment = size > sizeof(void *) ? sizeof(void *) : size;
    return json_deflate_bump_alloc(env, size, alignment);
}

static void cjson_node_free(void * env, void * obj) {
}

static void * regular_malloc(void * env, size_t size) {
    return malloc(size);
}

static void regular_free(void * env, void * obj) {
    free(obj);
}

static cJSON_Env * malloc_ctx() {
    static cJSON_Env ctx = {
        .ctx = NULL,
        .callbacks = {
            .malloc = regular_malloc,
            .free = regular_free,
        },
    };

    return &ctx;
}

void json_deflate_binary_write(FILE * const file_ptr, const json_deflate_bump_area_t * const area, const json_deflate_metadata_t * const metadata, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx) {
    fseek(file_ptr, 0, SEEK_SET);

    json_deflate_binary_header_t header = {0};
    header.magic = (json_deflate_binary_magic_t)json_deflate_binary_magic_number;
    header.version = json_deflate_binary_version;
    header.reserved = 0;
    fwrite(&header, sizeof(json_deflate_binary_header_t), 1, file_ptr);

    fwrite(metadata, sizeof(json_deflate_metadata_t), 1, file_ptr);
    fwrite(wasm_ctx, sizeof(json_deflate_schema_context_t), 1, file_ptr);
    fwrite(native_ctx, sizeof(json_deflate_schema_context_t), 1, file_ptr);
    ASSERT(area->payload.owned.next);
    fwrite(json_deflate_get_single_buffer(area), sizeof(uint8_t), area->payload.owned.next, file_ptr);
}

void json_deflate_prepare_schema(
    const char * const schema_text_file,
    const char * const schema_binary_file,
    const char * const schema_rust_file,
    const json_deflate_schema_options_t * const options) {
    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Allocating memory for JSON data");
    char * const data = json_deflate_calloc(json_deflate_tool_max_json_file_size, sizeof(char));

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Reading JSON schema file %s", schema_text_file);
    sb_file_t * const f = sb_fopen(sb_app_root_directory, schema_text_file, "rt");
    VERIFY_MSG(f, "[json_deflate_tool] Could not open JSON schema file");
    sb_fread(data, sizeof(char), json_deflate_tool_max_json_file_size, f);
    sb_fclose(f);

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Parsing JSON schema");

    const const_mem_region_t data_region = CONST_MEM_REGION(.ptr = data, .size = strlen(data));

    cJSON * const root = cJSON_Parse(malloc_ctx(), data_region);
    VERIFY_MSG(root, "[json_deflate_tool] Invalid JSON");

    const char * const slice_path = options->slice ? options->slice : "/";
    cJSON * const slice = json_deflate_schema_navigate_to(root, slice_path);
    VERIFY_MSG(slice, "[json_deflate_tool] Invalid slice path");

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Building schema");

    json_deflate_bump_area_t area;
    ZEROMEM(&area);
    json_deflate_bump_owned_init(&area, json_deflate_tool_max_bump_area_size, false);
    ASSERT(json_deflate_bump_initialized(&area));

    char * const slice_path_stored = json_deflate_bump_store_str(&area, slice_path);
    json_deflate_bump_ptr_t slice_path_stored_offset = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(&area, slice_path_stored);

    json_deflate_schema_context_t wasm_ctx = {0};
    json_deflate_context_init(&area, &wasm_ctx, json_deflate_parse_target_wasm, options);
    json_deflate_schema_prepare(&area, &wasm_ctx, slice, NULL, options);
    wasm_ctx.slice_path = slice_path_stored_offset;

    json_deflate_schema_context_t native_ctx = {0};
    json_deflate_context_init(&area, &native_ctx, json_deflate_parse_target_native, options);
    json_deflate_schema_prepare(&area, &native_ctx, slice, NULL, options);
    native_ctx.slice_path = slice_path_stored_offset;

    json_deflate_metadata_t metadata = {0};
    const uint32_t rust_schema_hash_seed = 0x03030303;
    metadata.rust_schema_hash = json_deflate_generate_hash(&area, &wasm_ctx, &native_ctx, rust_schema_hash_seed);

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Writing schema to file %s", schema_binary_file);
    FILE * const bs = fopen(schema_binary_file, "wb+");
    VERIFY_MSG(bs, "[json_deflate_tool] Failed to open file %s", schema_binary_file);
    json_deflate_binary_write(bs, &area, &metadata, &wasm_ctx, &native_ctx);
    fclose(bs);

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Writing Rust to file %s", schema_rust_file);
    FILE * const rust_schema_fp = fopen(schema_rust_file, "w+");
    VERIFY_MSG(rust_schema_fp, "[json_deflate_tool] Failed to open file %s", schema_rust_file);
    json_deflate_schema_print_options_t print_options = {0};
    print_options.emit_layout_comments = true;
    print_options.skip_key_conversion = options->skip_key_conversion;
    print_options.root_name = options->root_name;
    print_options.ffi_unsafe_types = options->emit_ffi_unsafe_types;
    print_options.schema_header = options->schema_header;
    json_deflate_schema_print_rust(rust_schema_fp, &area, &metadata, &wasm_ctx, &native_ctx, options->directory, &print_options);
    fclose(rust_schema_fp);

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Deallocating memory");
    json_deflate_free(data);
    cJSON_Delete(malloc_ctx(), root);

    json_deflate_bump_owned_destroy(&area);
}

void json_deflate_print_schema(const char * const schema_file, const char * const out_file, const json_deflate_parse_target_e target) {
    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate_tool] Reading schema: %s", schema_file);

    json_deflate_bump_area_t schema_area;
    ZEROMEM(&schema_area);

    sb_file_t * const bs = sb_fopen(sb_app_root_directory, schema_file, "rb");
    json_deflate_metadata_t metadata = {0};
    json_deflate_schema_context_t wasm_ctx = {0};
    json_deflate_schema_context_t native_ctx = {0};
    json_deflate_binary_read(bs, &schema_area, &metadata, &wasm_ctx, &native_ctx);
    sb_fclose(bs);

    json_deflate_schema_context_t * const ctx = target == json_deflate_parse_target_wasm ? &wasm_ctx : &native_ctx;

    json_deflate_schema_type_t * const schema = json_deflate_bump_get_ptr(&schema_area, ctx->ty_root);

    FILE * const ts = fopen(out_file, "w+");
    json_deflate_schema_print_toml(ts, &schema_area, schema);
    fclose(ts);

    json_deflate_bump_owned_destroy(&schema_area);
}

void json_deflate_infer_schema(const char * const sample_file_name, const char * const schema_file_name, json_deflate_schema_type_group_t * const groups, const int group_count, const char * const * const maps, const int map_count, const json_deflate_infer_options_t * const options) {
    char * const sample_data = json_deflate_calloc(json_deflate_tool_max_json_file_size, sizeof(char));
    sb_file_t * const sample_file = sb_fopen(sb_app_root_directory, sample_file_name, "rb");
    VERIFY_MSG(sample_file, "[json_deflate_tool] Sample file could not be opened");
    sb_fread(sample_data, sizeof(char), json_deflate_tool_max_json_file_size, sample_file);
    sb_fclose(sample_file);

    const const_mem_region_t sample_data_region = CONST_MEM_REGION(.ptr = sample_data, .size = strlen(sample_data));

    cJSON * const root = cJSON_Parse(malloc_ctx(), sample_data_region);

    json_deflate_bump_area_t cjson_area;
    ZEROMEM(&cjson_area);
    json_deflate_bump_owned_init(&cjson_area, json_deflate_tool_cjson_area_size, true);
    ASSERT(json_deflate_bump_initialized(&cjson_area));

    cJSON_Env area_ctx = {
        .ctx = &cjson_area,
        .callbacks = {
            .malloc = cjson_node_alloc,
            .free = cjson_node_free,
        },
    };

    cJSON * const schema = json_deflate_infer_json_schema(&area_ctx, root, groups, group_count, maps, map_count, options);

    char * const schema_text = cJSON_Print(malloc_ctx(), schema);

    FILE * const schema_file = fopen(schema_file_name, "w+");
    VERIFY_MSG(schema_file, "[json_deflate_tool] Sample file could not be created");
    fwrite(schema_text, sizeof(char), strlen(schema_text), schema_file);
    fclose(schema_file);

    free(schema_text);
    json_deflate_free(sample_data);
    cJSON_Delete(malloc_ctx(), root);

    json_deflate_bump_owned_destroy(&cjson_area);
}
