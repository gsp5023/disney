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

#include "extern/cjson/cJSON.h"
#include "source/adk/json_deflate/json_deflate.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct json_deflate_schema_options_t {
    const char * root_name;
    const char * directory;
    const char * slice;
    const char * schema_header;
    bool skip_key_conversion;
    bool emit_ffi_unsafe_types;
} json_deflate_schema_options_t;

typedef struct json_deflate_infer_options_t {
    int catch_required;
} json_deflate_infer_options_t;

typedef struct json_deflate_infer_path_node_t {
    char * value;
    struct json_deflate_infer_path_node_t * parent;
    cJSON * json;
} json_deflate_infer_path_node_t;

typedef struct json_deflate_schema_type_group_t {
    char * name;
    int32_t length;
    char * paths[64];
    json_deflate_infer_path_node_t * leader;
} json_deflate_schema_type_group_t;

void json_deflate_infer_schema(const char * const sample_file_name, const char * const schema_file_name, json_deflate_schema_type_group_t * const groups, const int groups_count, const char * const * const maps, const int maps_count, const json_deflate_infer_options_t * const options);
void json_deflate_prepare_schema(const char * const schema_text_file, const char * const schema_binary_file, const char * const schema_rust_file, const json_deflate_schema_options_t * const options);
void json_deflate_print_schema(const char * const schema_file, const char * const out_file, const json_deflate_parse_target_e target);

#ifdef __cplusplus
}
#endif