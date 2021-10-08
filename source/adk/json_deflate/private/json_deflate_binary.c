/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_binary.c

Binary JSON schema format serializer and deserializer.
*/

#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/steamboat/sb_file.h"

json_deflate_binary_read_status_e json_deflate_binary_read(sb_file_t * const f, json_deflate_bump_area_t * const area, json_deflate_metadata_t * const metadata, json_deflate_schema_context_t * const wasm_ctx, json_deflate_schema_context_t * const native_ctx) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    sb_fseek(f, 0, sb_seek_set);

    json_deflate_binary_header_t header = {0};
    sb_fread(&header, sizeof(header), 1, f);
    if (header.magic != (json_deflate_binary_magic_t)json_deflate_binary_magic_number) {
        JSON_DEFLATE_TRACE_POP();
        return json_deflate_binary_read_invalid_format;
    }
    if (header.version != json_deflate_binary_version) {
        JSON_DEFLATE_TRACE_POP();
        return json_deflate_binary_read_unsupported_version;
    }

    sb_fread(metadata, sizeof(json_deflate_metadata_t), 1, f);
    sb_fread(wasm_ctx, sizeof(json_deflate_schema_context_t), 1, f);
    sb_fread(native_ctx, sizeof(json_deflate_schema_context_t), 1, f);

    const long start_of_blob = sb_ftell(f);
    sb_fseek(f, 0, sb_seek_end);
    const long end_of_blob = sb_ftell(f);
    sb_fseek(f, start_of_blob, sb_seek_set);
    const size_t size_of_blob = (size_t)(end_of_blob - start_of_blob);

    json_deflate_bump_owned_init(area, size_of_blob, false);
    uint8_t * const buffer = json_deflate_get_single_buffer(area);

    const size_t length = sb_fread(buffer, sizeof(uint8_t), size_of_blob, f);
    VERIFY(length == size_of_blob);

    JSON_DEFLATE_TRACE_POP();
    return json_deflate_binary_read_success;
}

json_deflate_binary_read_status_e json_deflate_binary_read_from_memory(const const_mem_region_t schema, json_deflate_bump_area_t * const area, json_deflate_metadata_t * const metadata, json_deflate_schema_context_t * const wasm_ctx, json_deflate_schema_context_t * const native_ctx) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT_ALIGNED(schema.adr, json_deflate_minimum_alignment_requirement);

    const json_deflate_binary_header_t header = *(json_deflate_binary_header_t *)(&schema.byte_ptr[0]);

    if (header.magic != (json_deflate_binary_magic_t)json_deflate_binary_magic_number) {
        JSON_DEFLATE_TRACE_POP();
        return json_deflate_binary_read_invalid_format;
    }
    if (header.version != json_deflate_binary_version) {
        JSON_DEFLATE_TRACE_POP();
        return json_deflate_binary_read_unsupported_version;
    }

    *metadata = *(json_deflate_metadata_t *)(&schema.byte_ptr[sizeof(json_deflate_binary_header_t)]);

    *wasm_ctx = *(json_deflate_schema_context_t *)(&schema.byte_ptr[sizeof(json_deflate_binary_header_t) + sizeof(json_deflate_metadata_t)]);
    *native_ctx = *(json_deflate_schema_context_t *)(&schema.byte_ptr[sizeof(json_deflate_binary_header_t) + sizeof(json_deflate_metadata_t) + sizeof(json_deflate_schema_context_t)]);

    const size_t start_of_blob = sizeof(json_deflate_binary_header_t) + sizeof(json_deflate_metadata_t) + 2 * sizeof(json_deflate_schema_context_t);
    const size_t length_of_blob = schema.size - start_of_blob;
    const const_mem_region_t schema_blob = CONST_MEM_REGION(&schema.byte_ptr[start_of_blob], length_of_blob);

    json_deflate_bump_borrowed_init_readonly(area, schema_blob);

    JSON_DEFLATE_TRACE_POP();
    return json_deflate_binary_read_success;
}
