/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
json_deflate_common.h

JSON deflate common tool/library code.
*/

#include "extern/cjson/cJSON.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/json_deflate_protocol.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_thread.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    json_deflate_max_string_length = 1024,
};

typedef struct json_deflate_bump_area_owned_t {
    uint8_t ** owned_buffers;
    size_t number_of_owned_buffers;
    size_t capacity_per_buffer;
    size_t next;
    bool growable;
} json_deflate_bump_area_owned_t;

typedef struct json_deflate_bump_area_borrowed_t {
    uint8_t * borrowed_buffer;
    size_t capacity;
    size_t next;
    bool readonly;
} json_deflate_bump_area_borrowed_t;

typedef union json_deflate_bump_area_union_t {
    json_deflate_bump_area_owned_t owned;
    json_deflate_bump_area_borrowed_t borrowed;
} json_deflate_bump_area_union_t;

typedef struct json_deflate_bump_area_t {
    bool owned;
    json_deflate_bump_area_union_t payload;
} json_deflate_bump_area_t;

typedef struct json_deflate_bump_area_ctx_t {
    json_deflate_bump_area_t * area;
    bool out_of_memory;
} json_deflate_bump_area_ctx_t;

typedef struct json_deflate_binary_header_t {
    json_deflate_binary_magic_t magic;
    json_deflate_binary_protocol_version_t version;
    uint8_t reserved;
} json_deflate_binary_header_t;

typedef enum json_deflate_binary_read_status_e {
    json_deflate_binary_read_success = 0,
    json_deflate_binary_read_invalid_format = 1,
    json_deflate_binary_read_unsupported_version = 2,
    json_deflate_binary_read_unexpected_eof = 3
} json_deflate_binary_read_status_e;

typedef struct json_deflate_context_info_t {
    struct json_deflate_context_info_t * parent;
    const char * const path_part;
} json_deflate_context_info_t;

thread_pool_t * json_deflate_get_pool(void);
sb_mutex_t * json_deflate_get_parallel_mutex(void);
sb_condition_variable_t * json_deflate_get_cv(void);
system_guard_page_mode_e json_deflate_get_guard_mode(void);

cJSON * json_deflate_data_navigate_to(cJSON * const json, const char * const path);

json_deflate_binary_read_status_e json_deflate_binary_read(sb_file_t * const f, json_deflate_bump_area_t * const area, json_deflate_metadata_t * const metadata, json_deflate_schema_context_t * const wasm_ctx, json_deflate_schema_context_t * const native_ctx);
json_deflate_binary_read_status_e json_deflate_binary_read_from_memory(const const_mem_region_t, json_deflate_bump_area_t * const area, json_deflate_metadata_t * const metadata, json_deflate_schema_context_t * const wasm_ctx, json_deflate_schema_context_t * const native_ctx);

void json_deflate_bump_owned_init(json_deflate_bump_area_t * const area, const size_t capacity, const bool growable);
void json_deflate_bump_owned_destroy(json_deflate_bump_area_t * const area);
void json_deflate_bump_borrowed_init(json_deflate_bump_area_t * const area, const mem_region_t region);
void json_deflate_bump_borrowed_init_readonly(json_deflate_bump_area_t * const area, const const_mem_region_t region);
bool json_deflate_bump_initialized(json_deflate_bump_area_t * const area);

void * json_deflate_bump_alloc(json_deflate_bump_area_t * const area, const size_t size, const size_t alignment);
void * json_deflate_bump_alloc_array(json_deflate_bump_area_t * const area, const size_t count, const size_t elem_size, const size_t alignment);
char * json_deflate_bump_store_str(json_deflate_bump_area_t * const base, const char * const str);
void * json_deflate_bump_copy_object(json_deflate_bump_area_t * const base, const void * const src, const size_t size, const size_t alignment);

void * json_deflate_get_single_buffer(const json_deflate_bump_area_t * const area);
void * json_deflate_bump_get_ptr(const json_deflate_bump_area_t * const area, const json_deflate_bump_ptr_t offset);
size_t json_deflate_bump_get_offset(const json_deflate_bump_area_t * const area, const void * const ptr);

uint32_t json_deflate_schema_calculate_field_name_hash(const char * const text);

int32_t json_deflate_flag_mask_calculate_shift(const uint32_t mask);

uint8_t json_deflate_flags_get_value(const uint32_t * const flags, const uint32_t mask);
void json_deflate_flags_set_value(uint32_t * const flags, const uint8_t val, const uint32_t mask);

uint8_t json_deflate_type_is_stored_by_reference(const json_deflate_schema_type_t * const type);
void json_deflate_type_set_stored_by_reference(json_deflate_schema_type_t * const type, const uint8_t val);

uint8_t json_deflate_type_is_builtin(const json_deflate_schema_type_t * const type);
void json_deflate_type_set_builtin(json_deflate_schema_type_t * const type, const uint8_t val);

uint8_t json_deflate_type_is_type_constructor(const json_deflate_schema_type_t * const type);
void json_deflate_type_set_type_constructor(json_deflate_schema_type_t * const type, const uint8_t val);

uint8_t json_deflate_type_is_non_exhaustive(const json_deflate_schema_type_t * const type);
void json_deflate_type_set_non_exhaustive(json_deflate_schema_type_t * const type, const uint8_t val);

void json_deflate_type_set_collisions(json_deflate_schema_type_t * const type, const uint8_t val);
uint8_t json_deflate_type_get_collisions(const json_deflate_schema_type_t * const type);

json_deflate_schema_type_class_t json_deflate_type_get_class(const json_deflate_schema_type_t * const type);
void json_deflate_type_set_class(json_deflate_schema_type_t * const type, const json_deflate_schema_type_class_t val);

uint8_t json_deflate_field_is_required(const json_deflate_schema_field_t * const field);
void json_deflate_field_set_required(json_deflate_schema_field_t * const field, const uint8_t val);

uint8_t json_deflate_field_has_activation_value(const json_deflate_schema_field_t * const field);
void json_deflate_field_set_activation_value(json_deflate_schema_field_t * const field, const int val, const uint8_t activation_value);

uint8_t json_deflate_field_can_fail(const json_deflate_schema_field_t * const field);
void json_deflate_field_set_can_fail(json_deflate_schema_field_t * const field, const int val);

int json_deflate_find_by_hash(const void * const key, const void * const field);

json_deflate_schema_field_t * json_deflate_type_lookup_field(const json_deflate_bump_area_t * const area, const json_deflate_schema_type_t * const ty, const char * const name);

void * json_deflate_process_document(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, cJSON * const json);

cJSON_Env json_deflate_create_alloc_ctx(json_deflate_bump_area_ctx_t * const area);

#ifdef __cplusplus
}
#endif
