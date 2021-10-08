/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/steamboat/sb_platform.h"

enum {
    json_deflate_cjson_area_size = 6 * 1024 * 1024,
};

typedef struct json_deflate_timings_t {
    microseconds_t zero_start;
    microseconds_t zero_end;
    microseconds_t binary_start;
    microseconds_t binary_end;
    microseconds_t protect_start;
    microseconds_t protect_end;
    microseconds_t json_parse_start;
    microseconds_t json_parse_end;
    microseconds_t deflate_start;
    microseconds_t deflate_end;
    microseconds_t free_start;
    microseconds_t free_end;
    microseconds_t full_start;
    microseconds_t full_end;
} json_deflate_timings_t;

static void * cjson_node_alloc(void * env, size_t size) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    json_deflate_bump_area_ctx_t * ctx = env;
    const size_t alignment = size > sizeof(void *) ? sizeof(void *) : size;
    void * const obj = json_deflate_bump_alloc(ctx->area, size, alignment);
    if (!obj) {
        ctx->out_of_memory = true;
    }

    JSON_DEFLATE_TRACE_POP();
    return obj;
}

static void cjson_node_free(void * env, void * obj) {
}

cJSON_Env json_deflate_create_alloc_ctx(json_deflate_bump_area_ctx_t * const ctx) {
    cJSON_Env json_ctx = {
        .ctx = ctx,
        .callbacks = {
            .malloc = cjson_node_alloc,
            .free = cjson_node_free,
        },
    };

    return json_ctx;
}

static const_mem_region_t conditional_copy_data_to_protected_region(const const_mem_region_t data, mem_region_t * const out_pages) {
    JSON_DEFLATE_TRACE_PUSH_FN();
#ifdef GUARD_PAGE_SUPPORT
    if (json_deflate_get_guard_mode() == system_guard_page_mode_enabled) {
        const size_t page_aligned_size = PAGE_ALIGN_INT(data.size);
        const size_t num_pages = page_aligned_size / get_sys_page_size();
        *out_pages = sb_map_pages((num_pages + 1) * get_sys_page_size(), system_page_protect_no_access);
        TRAP_OUT_OF_MEMORY(out_pages->ptr);
        const size_t page_size = get_sys_page_size();
        sb_protect_pages(MEM_REGION(.adr = out_pages->adr, .size = num_pages * page_size), system_page_protect_read_write);
        const mem_region_t region = MEM_REGION(.adr = out_pages->adr + num_pages * page_size - data.size, .size = data.size);
        memcpy(region.byte_ptr, data.ptr, region.size);
        JSON_DEFLATE_TRACE_POP();
        return region.consted;
    }
#endif
    *out_pages = (mem_region_t){0};
    JSON_DEFLATE_TRACE_POP();
    return data;
}

static void conditional_free_data_from_protected_region(const mem_region_t pages) {
    JSON_DEFLATE_TRACE_PUSH_FN();
#ifdef GUARD_PAGE_SUPPORT
    if (json_deflate_get_guard_mode() == system_guard_page_mode_enabled) {
        sb_unmap_pages(pages);
    }
#endif
    JSON_DEFLATE_TRACE_POP();
}

static json_deflate_parse_data_result_t create_error_data_result(const json_deflate_parse_status_e status, const bool retry) {
    json_deflate_parse_result_t result = {
        .offset = 0,
        .end = 0,
        .status = status,
    };

    json_deflate_parse_data_result_t data_result = {
        .result = result,
        .do_retry = retry,
    };

    return data_result;
}

json_deflate_parse_data_result_t json_deflate_parse_data(const const_mem_region_t schema_layout, const const_mem_region_t json_data, const mem_region_t output_buffer, const json_deflate_parse_target_e target, const uint32_t expected_size, const uint32_t schema_hash) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    if (json_data.size == 0) {
        JSON_DEFLATE_TRACE_POP();
        return create_error_data_result(json_deflate_parse_status_invalid_json, false);
    }

    json_deflate_timings_t timings = {0};

    timings.full_start = adk_read_microsecond_clock();

    timings.zero_start = adk_read_microsecond_clock();
    memset(output_buffer.ptr, 0, output_buffer.size);
    timings.zero_end = adk_read_microsecond_clock();

    timings.binary_start = adk_read_microsecond_clock();

    json_deflate_bump_area_t schema_area;
    ZEROMEM(&schema_area);

    json_deflate_metadata_t metadata = {0};
    json_deflate_schema_context_t wasm_ctx = {0};
    json_deflate_schema_context_t native_ctx = {0};

    const json_deflate_binary_read_status_e schema_read_status
        = json_deflate_binary_read_from_memory(schema_layout, &schema_area, &metadata, &wasm_ctx, &native_ctx);

    if (schema_read_status != json_deflate_binary_read_success) {
        json_deflate_parse_data_result_t ret = create_error_data_result(json_deflate_parse_status_invalid_binary_layout, false);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    }

    json_deflate_schema_context_t * const ctx = (target == json_deflate_parse_target_wasm) ? &wasm_ctx : &native_ctx;
    timings.binary_end = adk_read_microsecond_clock();

    timings.protect_start = adk_read_microsecond_clock();
    mem_region_t protected_pages = {0};
    const const_mem_region_t json_data_protected = conditional_copy_data_to_protected_region(json_data, &protected_pages);

    timings.protect_end = adk_read_microsecond_clock();

    timings.json_parse_start = adk_read_microsecond_clock();

    json_deflate_schema_type_t * const schema = json_deflate_bump_get_ptr(&schema_area, ctx->ty_root);

    VERIFY_MSG(schema_hash == metadata.rust_schema_hash,
               "[json_deflate] The Rust schema currently in use is incompatible with the binary layout file in some way. "
               "Please regenerate your Rust schema and your binary layout file using the same version of the tool.");

    VERIFY_MSG(expected_size == schema->var,
               "[json_deflate] The binary schema does not match the provided Rust types. "
               "This may be a result of tampering with the Rust types, an outdated schema file, or a bug in the json_deflate_tool.");

    json_deflate_bump_area_t graph_area;
    ZEROMEM(&graph_area);
    json_deflate_bump_owned_init(&graph_area, json_deflate_cjson_area_size, true);

    // If unable to alloc memory, retry later
    if (!json_deflate_bump_initialized(&graph_area)) {
        json_deflate_bump_owned_destroy(&graph_area);
        conditional_free_data_from_protected_region(protected_pages);
        json_deflate_parse_data_result_t ret = create_error_data_result(json_deflate_parse_status_success, true);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    }

    json_deflate_bump_area_ctx_t graph_area_ctx;
    ZEROMEM(&graph_area_ctx);
    graph_area_ctx.area = &graph_area;

    cJSON_Env json_ctx = json_deflate_create_alloc_ctx(&graph_area_ctx);

    cJSON * const data_root = cJSON_Parse(&json_ctx, json_data_protected);

    if (!data_root) {
        json_deflate_bump_owned_destroy(&graph_area);
        conditional_free_data_from_protected_region(protected_pages);

        if (graph_area_ctx.out_of_memory) {
            json_deflate_parse_data_result_t ret = create_error_data_result(json_deflate_parse_status_success, true);
            JSON_DEFLATE_TRACE_POP();
            return ret;
        } else {
            json_deflate_parse_data_result_t ret = create_error_data_result(json_deflate_parse_status_invalid_json, false);
            JSON_DEFLATE_TRACE_POP();
            return ret;
        }
    }

    const char * const slice_path = json_deflate_bump_get_ptr(&schema_area, ctx->slice_path);
    cJSON * const data_slice = slice_path ? json_deflate_data_navigate_to(data_root, slice_path) : data_root;
    VERIFY_MSG(data_slice, "[json_deflate] Failed to navigate to slice");

    timings.json_parse_end = adk_read_microsecond_clock();

    timings.deflate_start = adk_read_microsecond_clock();

    json_deflate_bump_area_t data_area;
    ZEROMEM(&data_area);
    json_deflate_bump_borrowed_init(&data_area, output_buffer);

    void * const root_ptr = json_deflate_process_document(&data_area, &schema_area, ctx, target, schema, data_slice);

    timings.deflate_end = adk_read_microsecond_clock();

    timings.free_start = adk_read_microsecond_clock();

    json_deflate_bump_owned_destroy(&graph_area);
    conditional_free_data_from_protected_region(protected_pages);

    timings.free_end = adk_read_microsecond_clock();

    if (!root_ptr) {
        json_deflate_parse_data_result_t ret = create_error_data_result(json_deflate_parse_status_out_of_target_memory, false);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    }

    timings.full_end = adk_read_microsecond_clock();

    LOG_DEBUG(TAG_JSON_DEFLATE, "[json_deflate] Timings -- Zeroing: %llu us, Binary: %llu us, Protect: %llu us, Parse: %llu us, Deflate: %llu us, Free: %llu us, Full: %llu us", timings.zero_end.us - timings.zero_start.us, timings.binary_end.us - timings.binary_start.us, timings.protect_end.us - timings.protect_start.us, timings.json_parse_end.us - timings.json_parse_start.us, timings.deflate_end.us - timings.deflate_start.us, timings.free_end.us - timings.free_start.us, timings.full_end.us - timings.full_start.us);

    json_deflate_parse_data_result_t success_result = {
        .result = (json_deflate_parse_result_t){
            .offset = (uint32_t)json_deflate_bump_get_offset(&data_area, root_ptr),
            .end = (uint32_t)data_area.payload.borrowed.next,
            .status = json_deflate_parse_status_success,
        },
        .do_retry = false,
    };

    JSON_DEFLATE_TRACE_POP();
    return success_result;
}
