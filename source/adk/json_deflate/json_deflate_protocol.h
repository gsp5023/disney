/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
json_deflate_protocol_03.h

JSON deflate types for binary protocol version 0x03.
*/

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t json_deflate_binary_magic_t;
typedef uint8_t json_deflate_binary_protocol_version_t;

typedef uint32_t json_deflate_bump_ptr_t;
typedef uint32_t json_deflate_bump_size_t;
typedef json_deflate_bump_ptr_t json_deflate_bump_snap_t;

enum {
    json_deflate_binary_magic_number = 0x076D,
    json_deflate_binary_version = 0x4,
    json_deflate_minimum_alignment_requirement = sizeof(json_deflate_bump_ptr_t)
};

typedef enum json_deflate_schema_type_class_t {
    schema_type_struct = 0x00,
    schema_type_variant = 0x01,
    schema_type_array = 0x02,
    schema_type_map = 0x03,
} json_deflate_schema_type_class_t;

typedef struct json_deflate_schema_type_t {
    json_deflate_bump_ptr_t name;
    json_deflate_bump_ptr_t fields;
    json_deflate_bump_ptr_t type_ctor;
    json_deflate_bump_ptr_t rel_type;
    json_deflate_bump_size_t size;
    json_deflate_bump_size_t var;
    json_deflate_bump_size_t align;
    uint32_t field_count;
    uint32_t flags;
} json_deflate_schema_type_t;

typedef struct json_deflate_schema_field_t {
    json_deflate_bump_ptr_t json_name;
    json_deflate_bump_ptr_t rust_name;
    json_deflate_bump_ptr_t type;
    uint32_t hash;
    uint32_t reserved;
    json_deflate_bump_size_t offset;
    uint32_t flags;
    uint32_t choice_value;
    uint32_t original_order;
} json_deflate_schema_field_t;

typedef struct json_deflate_schema_context_t {
    json_deflate_bump_ptr_t ty_root;
    json_deflate_bump_ptr_t ty_ptr;
    json_deflate_bump_ptr_t ty_size;
    json_deflate_bump_ptr_t ty_unit;

    json_deflate_bump_ptr_t ty_boolean;
    json_deflate_bump_ptr_t ty_integer;
    json_deflate_bump_ptr_t ty_number;
    json_deflate_bump_ptr_t ty_char;

    json_deflate_bump_ptr_t ty_string;
    json_deflate_bump_ptr_t ty_array;
    json_deflate_bump_ptr_t ty_option;
    json_deflate_bump_ptr_t ty_struct;

    json_deflate_bump_ptr_t ty_enum;
    json_deflate_bump_ptr_t ty_result;
    json_deflate_bump_ptr_t slice_path;
    json_deflate_bump_ptr_t ty_map;
} json_deflate_schema_context_t;

typedef struct json_deflate_metadata_t {
    uint32_t rust_schema_hash;
} json_deflate_metadata_t;

#ifdef __cplusplus
}
#endif
