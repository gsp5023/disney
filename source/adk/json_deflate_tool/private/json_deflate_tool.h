/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
json_deflate_tool.h

JSON deflate tool code.
*/

#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/json_deflate_tool/json_deflate_tool.h"
#include "source/adk/steamboat/sb_file.h"

#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    json_deflate_tool_max_path_length = 1024,
    json_deflate_tool_max_type_count = 2048
};

typedef struct json_deflate_schema_type_mapping_t {
    char path[json_deflate_tool_max_path_length];
    json_deflate_schema_type_t * type;
} json_deflate_schema_type_mapping_t;

typedef struct json_deflate_schema_type_map_t {
    int32_t idx;
    json_deflate_schema_type_mapping_t * mappings;
} json_deflate_schema_type_map_t;

typedef struct json_deflate_schema_type_list_t {
    int32_t idx;
    json_deflate_schema_type_t * types[json_deflate_tool_max_type_count];
} json_deflate_schema_type_list_t;

typedef struct json_deflate_schema_path_node_t {
    const char * value;
    const struct json_deflate_schema_path_node_t * parent;
} json_deflate_schema_path_node_t;

typedef struct json_deflate_schema_print_options_t {
    bool emit_layout_comments;
    bool skip_key_conversion;
    const char * root_name;
    bool ffi_unsafe_types;
    const char * schema_header;
} json_deflate_schema_print_options_t;

cJSON * json_deflate_schema_navigate_to(cJSON * const json, const char * const path);

void json_deflate_binary_write(FILE * const f, const json_deflate_bump_area_t * const area, const json_deflate_metadata_t * const metadata, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx);

cJSON * json_deflate_infer_json_schema(void * const area, const cJSON * const data, json_deflate_schema_type_group_t * const groups, const int group_count, const char * const * const maps, const int map_count, const json_deflate_infer_options_t * const options);

json_deflate_schema_type_t * json_deflate_schema_prepare(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, const char * name_hint, const json_deflate_schema_options_t * const options);
json_deflate_schema_type_t * json_deflate_schema_prepare_rec(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, json_deflate_schema_type_map_t * const map, const json_deflate_schema_path_node_t * const node, const char * name_hint, const json_deflate_schema_options_t * const options);
json_deflate_schema_type_t * json_deflate_schema_prepare_specific_rec(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, const cJSON * const choice, json_deflate_schema_type_map_t * const map, const json_deflate_schema_path_node_t * const node, const char * name_hint, const json_deflate_schema_options_t * const options);

void json_deflate_write_string(FILE * const f, const char * const str);
void json_deflate_write_int(FILE * const f, const int num);
void json_deflate_write_uint(FILE * const f, const unsigned int num);
void json_deflate_write_hex(FILE * const f, const unsigned int num);

#ifndef _WIN32
#if defined(_VADER) || defined(_LEIA)
char * vader_strok_s(char * str, const char * delim, char ** saveptr);
#define strtok_s(_str, _delim, _saveptr) vader_strok_s(_str, _delim, _saveptr)
#else
char * strtok_s(char * str, const char * delim, char ** saveptr);
#endif
#endif

void json_deflate_schema_print_rust(FILE * const rust_schema_file, json_deflate_bump_area_t * const area, const json_deflate_metadata_t * const metadata, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx, const char * const directory, const json_deflate_schema_print_options_t * const options);
void json_deflate_schema_print_toml(FILE * const toml_file, json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const type);

int json_deflate_sort_key_by_offset(const void * const a, const void * const b);
int json_deflate_sort_key_by_original_order(const void * const a, const void * const b);
int json_deflate_sort_key_by_offset_then_by_original_order(const void * const a, const void * const b);
int json_deflate_sort_key_by_choice_value(const void * const a, const void * const b);
int json_deflate_sort_key_by_hash(const void * const a, const void * const b);
int json_deflate_sort_key_by_reserved(const void * const a, const void * const b);
void json_deflate_detect_collisions(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_hash(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_original_order(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_offset(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_offset_then_by_original_order(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_choice_value(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void reorder_fields_by_size(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty);
void json_deflate_schema_type_arrange(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type);

json_deflate_schema_type_t * json_deflate_type_register_builtin(json_deflate_bump_area_t * const area, const char * const name, const json_deflate_bump_size_t size, const json_deflate_bump_size_t ptr_size, const char ref);

void json_deflate_type_init(json_deflate_schema_type_t * const type);
void json_deflate_field_init(json_deflate_schema_field_t * const field);

void json_deflate_offset_verify(const json_deflate_bump_area_t * const base, const json_deflate_bump_ptr_t offset);
void json_deflate_field_verify_offsets(const json_deflate_bump_area_t * const base, const json_deflate_schema_field_t * const f);
void json_deflate_type_verify_offsets(const json_deflate_bump_area_t * const base, const json_deflate_schema_type_t * const t);

void json_deflate_context_init(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, const json_deflate_schema_options_t * const options);

json_deflate_schema_type_t * json_deflate_type_construct(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const type_constructor, const json_deflate_schema_type_t * const type_arg);

uint32_t json_deflate_generate_hash(const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx, const uint32_t seed);

#ifdef __cplusplus
}
#endif
