/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 json_deflate_main.c

 JSON deflate console app entry point.
 */

#include "source/adk/json_deflate_tool/json_deflate_tool.h"

#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

enum {
    max_group_count = 1024,
    max_map_count = 1024,
};

static void * regular_malloc(const size_t size) {
    return malloc(size);
}

static void * regular_calloc(const size_t nmemb, const size_t size) {
    return calloc(nmemb, size);
}

static void * regular_realloc(void * const ptr, const size_t size) {
    return realloc(ptr, size);
}

static void regular_free(void * const ptr) {
    free(ptr);
}

static const char * lookup_optional(const char * const * const args, const int args_count, const char * const key, const char * const key_short) {
    for (int i = 0; i < args_count - 1; i++) {
        int m1 = key && !strcmp(key, args[i]);
        int m2 = key_short && !strcmp(key_short, args[i]);
        if (m1 || m2) {
            return args[i + 1];
        }
    }
    return NULL;
}

static const char * lookup_required(const char * const * const args, const int args_count, const char * const key, const char * const key_short) {
    const char * result = lookup_optional(args, args_count, key, key_short);
    if (key_short) {
        VERIFY_MSG(result, "[json_deflate_tool] Missing required command line argument: %s (or %s)", key, key_short);
    } else {
        VERIFY_MSG(result, "[json_deflate_tool] Missing required command line argument: %s", key);
    }
    return result;
}

static int lookup_multiple(const char * const * const args, const int args_count, const char * const key, const char * const key_short, const char ** output) {
    const char ** output_origin = output;

    for (int i = 0; i < args_count; i++) {
        int m1 = key && !strcmp(key, args[i]);
        int m2 = key_short && !strcmp(key_short, args[i]);
        if (m1 || m2) {
            *(output++) = strdup(args[i + 1]);
        }
    }

    return (int)(output - output_origin);
}

static bool is_present(const char * const * const args, const int args_count, const char * const key) {
    for (int i = 0; i < args_count; i++) {
        if (!strcmp(key, args[i])) {
            return true;
        }
    }

    return false;
}

json_deflate_schema_type_group_t * parse_groups(const char * const * const str, const int count) {
    json_deflate_schema_type_group_t * const groups = calloc(count, sizeof(json_deflate_schema_type_group_t));

    for (int i = 0; str[i]; i++) {
        char * dup = strdup(str[i]);

        char * next = NULL;
        char * first = strtok_s(dup, "=", &next);
        if (next) {
            groups[i].name = strdup(first);
        } else {
            next = strdup(first);
        }

        char * next_g = NULL;
        char * first_g = strtok_s(next, ",", &next_g);
        groups[i].paths[groups[i].length++] = strdup(first_g);
        while (next_g && *next_g) {
            first_g = strtok_s(NULL, ",", &next_g);
            groups[i].paths[groups[i].length++] = strdup(first_g);
        }

        free(dup);
    }

    return groups;
}

int cmdlet_json_deflate_tool(const int argc, const char * const * const argv) {
    json_deflate_options_t memory_hooks = {
        .calloc_hook = regular_calloc,
        .unchecked_calloc_hook = regular_calloc,
        .malloc_hook = regular_malloc,
        .realloc_hook = regular_realloc,
        .free_hook = regular_free,
    };
    json_deflate_init_hooks(&memory_hooks);

    VERIFY_MSG(argc >= 2, "[json_deflate_tool] A command must be provided. Valid commands are: { infer, print, prepare }");
    if (!strcmp(argv[1], "infer")) {
        const char * const sample_file_name = lookup_required(argv, argc, "--sample", NULL);
        const char * const schema_file_name = lookup_required(argv, argc, "--schema", "-s");
        const char * groups[max_group_count] = {0};
        const int group_count = lookup_multiple(argv, argc, "--group", "-g", &groups[0]);
        json_deflate_schema_type_group_t * const groups_parsed = parse_groups(groups, group_count);
        const char * maps[max_group_count] = {0};
        const int map_count = lookup_multiple(argv, argc, "--map", "-m", &maps[0]);
        const json_deflate_infer_options_t options = {
            .catch_required = is_present(argv, argc, "--catch-required")};
        json_deflate_infer_schema(sample_file_name, schema_file_name, groups_parsed, group_count, maps, map_count, &options);
        free(groups_parsed);
    } else if (!strcmp(argv[1], "print")) {
        const char * const schema_file_name = lookup_required(argv, argc, "--schema", "-s");
        const char * const output_file_name = lookup_required(argv, argc, "--output", "-o");
        json_deflate_parse_target_e target = is_present(argv, argc, "--wasm") ? json_deflate_parse_target_wasm : json_deflate_parse_target_native;
        json_deflate_print_schema(schema_file_name, output_file_name, target);
    } else if (!strcmp(argv[1], "prepare")) {
        const char * const schema_root_name = lookup_required(argv, argc, "--root", "-r");
        const char * const schema_text_file_name = lookup_required(argv, argc, "--schema", "-s");
        const char * const schema_binary_file_name = lookup_required(argv, argc, "--binary", "-b");
        const char * const schema_directory = lookup_required(argv, argc, "--include-binary-from", "-i");
        const char * const schema_rust_file_name = lookup_required(argv, argc, "--rust", "-rs");
        const char * const schema_slice = lookup_optional(argv, argc, "--slice", NULL);
        const char * const schema_header = lookup_optional(argv, argc, "--header", "-h");
        const bool skip_key_conversion = is_present(argv, argc, "--skip-key-conversion");
        const bool schema_unsafe_vectors = is_present(argv, argc, "--emit-ffi-unsafe-std-vectors");
        json_deflate_schema_options_t options = {
            .root_name = schema_root_name,
            .directory = schema_directory,
            .slice = schema_slice,
            .schema_header = schema_header,
            .skip_key_conversion = skip_key_conversion,
            .emit_ffi_unsafe_types = schema_unsafe_vectors,
        };
        json_deflate_prepare_schema(schema_text_file_name, schema_binary_file_name, schema_rust_file_name, &options);
    } else {
        TRAP("[json_deflate_tool] Unknown command. Valid commands are: { infer, print, prepare }");
    }

    return 0;
}
