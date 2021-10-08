/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_rust.c

JSON deflate generation of Rust types based on a schema.
*/

#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"

void json_deflate_rust_type_print_storage_location(char * const out, const size_t size, const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, const json_deflate_schema_type_t * const type, const json_deflate_schema_print_options_t * const options);

static void json_deflate_schema_type_list_add(json_deflate_schema_type_list_t * const list, json_deflate_schema_type_t * const type) {
    list->types[list->idx++] = type;
}

static int json_deflate_schema_type_list_contains(json_deflate_schema_type_list_t * const list, json_deflate_schema_type_t * const type) {
    for (int i = 0; i < list->idx; i++) {
        if (list->types[i] == type) {
            return 1;
        }
    }

    return 0;
}

static void json_deflate_string_convert_to_camel_case(char * const out, const size_t size) {
    char out_clean[256] = {0};

    int j = 0;
    for (int i = 0; out[i]; i++) {
        if (!i) {
            out_clean[j++] = (char)toupper(out[i]);
        } else if (out[i] == '-') {
            out_clean[j++] = (char)toupper(out[++i]);
        } else if (out[i] == '_') {
            out_clean[j++] = (char)toupper(out[++i]);
        } else if (isspace(out[i])) {
            out_clean[j++] = ' ';
            out_clean[j++] = (char)tolower(out[++i]);
        } else if (out[i] == ':') {
            out_clean[j++] = '_';
        } else if (out[i] == '+') {
            const char replacement[] = "_plus_";
            strcat_s(out_clean, sizeof(out_clean), replacement);
            j += sizeof(replacement) - 1;
        } else if (out[i] == '.') {
            const char replacement[] = "_dot_";
            strcat_s(out_clean, sizeof(out_clean), replacement);
            j += sizeof(replacement) - 1;
        } else {
            out_clean[j++] = out[i];
        }
    }

    out_clean[j] = 0;

    const size_t pre_final_length = strlen(out_clean);
    for (size_t i = pre_final_length - 1; i && out_clean[i] == '_'; i--) {
        out_clean[i] = 0;
    }

    strcpy_s(out, sizeof(out_clean), out_clean);
}

void json_deflate_string_convert_to_snake_case(char * const out, const size_t size) {
    char out_clean[256] = {0};

    int j = 0;
    for (int i = 0; out[i]; i++) {
        if (!i) {
            out_clean[j++] = (char)tolower(out[i]);
        } else if (out[i] == '_') {
            out_clean[j++] = '_';
            out_clean[j++] = (char)tolower(out[++i]);
        } else if (out[i] == '-') {
            out_clean[j++] = '_';
            out_clean[j++] = (char)tolower(out[++i]);
        } else if (out[i] == ':') {
            out_clean[j++] = '_';
        } else if (isspace(out[i])) {
            out_clean[j++] = ' ';
            out_clean[j++] = (char)tolower(out[++i]);
        } else if (i && isupper(out[i])) {
            if (isupper(out[i - 1])) {
                out_clean[j++] = (char)tolower(out[i]);
            } else {
                out_clean[j++] = '_';
                out_clean[j++] = (char)tolower(out[i]);
            }
        } else if (out[i] == '+') {
            const char replacement[] = "_plus_";
            strcat_s(out_clean, sizeof(out_clean), replacement);
            j += sizeof(replacement) - 1;
        } else if (out[i] == '.') {
            const char replacement[] = "_dot_";
            strcat_s(out_clean, sizeof(out_clean), replacement);
            j += sizeof(replacement) - 1;
        } else {
            out_clean[j++] = out[i];
        }
    }

    out_clean[j] = 0;

    const size_t pre_final_length = strlen(out_clean);
    for (size_t i = pre_final_length - 1; i && out_clean[i] == '_'; i--) {
        out_clean[i] = 0;
    }

    strcpy_s(out, sizeof(out_clean), out_clean);
}

static void json_deflate_rust_type_print_name(char * const out, const size_t size, const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, const json_deflate_schema_type_t * const type, const json_deflate_schema_print_options_t * const options) {
    const json_deflate_bump_ptr_t type_offset = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type);
    if (type_offset == ctx->ty_string) {
        strcpy_s(out, size, options->ffi_unsafe_types ? "String" : "FString");
    } else if (type_offset == ctx->ty_integer) {
        strcpy_s(out, size, "i32");
    } else if (type_offset == ctx->ty_number) {
        strcpy_s(out, size, "f64");
    } else if (type_offset == ctx->ty_boolean) {
        strcpy_s(out, size, "bool");
    } else {
        const char * original_type_name = json_deflate_bump_get_ptr(area, type->name);
        strcpy_s(out, size, original_type_name);
        json_deflate_string_convert_to_camel_case(out, size);
    }
}

static void json_deflate_rust_type_print_declared_name(char * const out, const size_t size, const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, const json_deflate_schema_type_t * const type, const json_deflate_schema_print_options_t * const options) {
    if (json_deflate_type_get_class(type) == schema_type_array) {
        if (type == json_deflate_bump_get_ptr(area, ctx->ty_string)) {
            strcpy_s(out, size, options->ffi_unsafe_types ? "String" : "FString");
        } else {
            json_deflate_schema_type_t * elem_type = json_deflate_bump_get_ptr(area, type->rel_type);

            char elem_type_decl_name[256] = {0};
            json_deflate_rust_type_print_storage_location(elem_type_decl_name, sizeof(elem_type_decl_name), area, ctx, elem_type, options);

            strcpy_s(out, size, options->ffi_unsafe_types ? "Vec<r#" : "FVec<r#");
            strcat_s(out, size, elem_type_decl_name);
            strcat_s(out, size, ">");
        }
    } else {
        json_deflate_rust_type_print_name(out, size, area, ctx, type, options);
        if (type->type_ctor == ctx->ty_result) {
            strcat_s(out, size, "<r#");
            json_deflate_schema_type_t * rel_type = json_deflate_bump_get_ptr(area, type->rel_type);
            char rel_type_decl_name[256] = {0};
            json_deflate_rust_type_print_storage_location(rel_type_decl_name, sizeof(rel_type_decl_name), area, ctx, rel_type, options);
            strcat_s(out, size, rel_type_decl_name);
            strcat_s(out, size, options->ffi_unsafe_types ? ", String>" : ", FString>");
        } else if (type->rel_type) {
            strcat_s(out, size, "<");
            json_deflate_schema_type_t * rel_type = json_deflate_bump_get_ptr(area, type->rel_type);
            char rel_type_decl_name[256] = {0};
            json_deflate_rust_type_print_storage_location(rel_type_decl_name, sizeof(rel_type_decl_name), area, ctx, rel_type, options);
            strcat_s(out, size, rel_type_decl_name);
            strcat_s(out, size, ">");
        }
    }
}

void json_deflate_rust_type_print_storage_location(char * const out, const size_t size, const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, const json_deflate_schema_type_t * const type, const json_deflate_schema_print_options_t * const options) {
    size_t len_so_far = strlen(out);
    json_deflate_rust_type_print_declared_name(out + len_so_far, size - len_so_far, area, ctx, type, options);
}

static void emit_size_assertion(FILE * const rust_schema_file, const char * const decl_name, const json_deflate_schema_type_t * const wasm_type, const json_deflate_schema_type_t * const native_type) {
    if (wasm_type) {
        json_deflate_write_string(rust_schema_file, "#[cfg(target_arch = \"wasm32\")]\n");
        json_deflate_write_string(rust_schema_file, "static_assertions::assert_eq_size!(");
        json_deflate_write_string(rust_schema_file, decl_name);
        json_deflate_write_string(rust_schema_file, ", [u8; ");
        json_deflate_write_uint(rust_schema_file, wasm_type->size);
        json_deflate_write_string(rust_schema_file, "]);\n");
    }

    if (native_type) {
        json_deflate_write_string(rust_schema_file, "#[cfg(not(target_arch = \"wasm32\"))]\n");
        json_deflate_write_string(rust_schema_file, "static_assertions::assert_eq_size!(");
        json_deflate_write_string(rust_schema_file, decl_name);
        json_deflate_write_string(rust_schema_file, ", [u8; ");
        json_deflate_write_uint(rust_schema_file, native_type->size);
        json_deflate_write_string(rust_schema_file, "]);\n");
    }

    if (wasm_type || native_type) {
        json_deflate_write_string(rust_schema_file, "\n");
    }
}

static void json_deflate_schema_print_rust_type(FILE * const rust_schema_file, json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const wasm_type, json_deflate_schema_type_t * const native_type, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx, json_deflate_schema_type_list_t * const list, const json_deflate_schema_print_options_t * const options) {
    if (json_deflate_schema_type_list_contains(list, wasm_type)) {
        return;
    }

    json_deflate_schema_type_list_add(list, wasm_type);

    json_deflate_type_verify_offsets(area, wasm_type);

    if (wasm_type->rel_type) {
        json_deflate_schema_type_t * const wasm_rel_type = json_deflate_bump_get_ptr(area, wasm_type->rel_type);
        json_deflate_schema_type_t * const native_rel_type = json_deflate_bump_get_ptr(area, native_type->rel_type);
        json_deflate_type_verify_offsets(area, wasm_rel_type);
        json_deflate_type_verify_offsets(area, native_rel_type);
        json_deflate_schema_print_rust_type(rust_schema_file, area, wasm_rel_type, native_rel_type, wasm_ctx, native_ctx, list, options);
    }

    if (!json_deflate_type_is_builtin(wasm_type)) {
        json_deflate_schema_field_t * const wasm_fields = json_deflate_bump_get_ptr(area, wasm_type->fields);
        json_deflate_schema_field_t * const native_fields = json_deflate_bump_get_ptr(area, native_type->fields);

        for (uint64_t i = 0; i < wasm_type->field_count; i++) {
            json_deflate_field_verify_offsets(area, &wasm_fields[i]);
            json_deflate_field_verify_offsets(area, &native_fields[i]);

            json_deflate_schema_type_t * const wasm_field_type = json_deflate_bump_get_ptr(area, wasm_fields[i].type);
            json_deflate_schema_type_t * const native_field_type = json_deflate_bump_get_ptr(area, native_fields[i].type);
            json_deflate_schema_print_rust_type(rust_schema_file, area, wasm_field_type, native_field_type, wasm_ctx, native_ctx, list, options);
        }

        char decl_name[256] = {0};
        json_deflate_rust_type_print_declared_name(decl_name, sizeof(decl_name), area, wasm_ctx, wasm_type, options);

        if (json_deflate_type_get_class(wasm_type) == schema_type_struct) {
            json_deflate_write_string(rust_schema_file, "#[repr(C)]\n");
            json_deflate_write_string(rust_schema_file, "#[derive(Debug, Clone, PartialEq)]\n");
            json_deflate_write_string(rust_schema_file, "#[allow(dead_code)]\n");

            if (options->skip_key_conversion) {
                json_deflate_write_string(rust_schema_file, "#[allow(non_snake_case)]\n");
            }

            json_deflate_write_string(rust_schema_file, "pub struct r#");
            json_deflate_write_string(rust_schema_file, decl_name);
            json_deflate_write_string(rust_schema_file, " {\n");

            reorder_fields_by_offset_then_by_original_order(area, wasm_type);
            reorder_fields_by_offset_then_by_original_order(area, native_type);

            for (uint64_t i = 0; i < wasm_type->field_count; i++) {
                json_deflate_field_verify_offsets(area, &wasm_fields[i]);

                json_deflate_schema_type_t * field_type = json_deflate_bump_get_ptr(area, wasm_fields[i].type);

                char field_type_name[256] = {0};
                json_deflate_rust_type_print_storage_location(field_type_name, sizeof(field_type_name), area, wasm_ctx, field_type, options);

                char * field_name = json_deflate_bump_get_ptr(area, wasm_fields[i].rust_name);
                char field_name_extracted[256] = {0};
                strcpy_s(field_name_extracted, sizeof(field_name_extracted), field_name);

                if (!options->skip_key_conversion) {
                    json_deflate_string_convert_to_snake_case(field_name_extracted, sizeof(field_name_extracted));
                }

                json_deflate_write_string(rust_schema_file, "  pub r#");
                json_deflate_write_string(rust_schema_file, field_name_extracted);
                json_deflate_write_string(rust_schema_file, ": r#");
                json_deflate_write_string(rust_schema_file, field_type_name);
                json_deflate_write_string(rust_schema_file, ",\n");
            }

            reorder_fields_by_hash(area, wasm_type);
            reorder_fields_by_hash(area, native_type);

            json_deflate_write_string(rust_schema_file, "}\n\n");
        } else if (json_deflate_type_get_class(wasm_type) == schema_type_variant) {
            if (wasm_type->size > 1) {
                json_deflate_write_string(rust_schema_file, "#[repr(C, i8)]\n");
            }

            json_deflate_write_string(rust_schema_file, "#[derive(Debug, Clone, PartialEq)]\n");
            json_deflate_write_string(rust_schema_file, "#[allow(dead_code)]\n");

            json_deflate_write_string(rust_schema_file, "pub enum r#");
            json_deflate_write_string(rust_schema_file, decl_name);
            json_deflate_write_string(rust_schema_file, " {\n");

            reorder_fields_by_choice_value(area, wasm_type);
            reorder_fields_by_choice_value(area, native_type);

            for (uint64_t i = 0; i < wasm_type->field_count; i++) {
                const char * field_name = json_deflate_bump_get_ptr(area, wasm_fields[i].rust_name);
                if (!strcmp(field_name, "tag")) {
                    continue;
                }

                json_deflate_schema_type_t * field_type = json_deflate_bump_get_ptr(area, wasm_fields[i].type);
                char field_type_name[256] = {0};
                json_deflate_rust_type_print_storage_location(field_type_name, sizeof(field_type_name), area, wasm_ctx, field_type, options);

                if (wasm_fields[i].type == wasm_ctx->ty_unit) {
                    json_deflate_write_string(rust_schema_file, "  r#");
                    json_deflate_write_string(rust_schema_file, field_name);
                    json_deflate_write_string(rust_schema_file, ",\n");
                } else {
                    json_deflate_write_string(rust_schema_file, "  r#");
                    json_deflate_write_string(rust_schema_file, field_name);
                    json_deflate_write_string(rust_schema_file, "(r#");
                    json_deflate_write_string(rust_schema_file, field_type_name);
                    json_deflate_write_string(rust_schema_file, "),\n");
                }
            }

            reorder_fields_by_hash(area, wasm_type);
            reorder_fields_by_hash(area, native_type);

            json_deflate_write_string(rust_schema_file, "}\n\n");
        }
    }

    char storage_loc[2048] = {0};
    json_deflate_rust_type_print_storage_location(storage_loc, sizeof(storage_loc), area, wasm_ctx, wasm_type, options);
    if (options->emit_layout_comments && (!json_deflate_type_is_builtin(wasm_type) || json_deflate_type_get_class(wasm_type) == schema_type_variant || json_deflate_type_get_class(wasm_type) == schema_type_map)) {
        emit_size_assertion(rust_schema_file, storage_loc, wasm_type, native_type);
    }
}

void json_deflate_schema_print_rust(FILE * const rust_schema_file, json_deflate_bump_area_t * const area, const json_deflate_metadata_t * const metadata, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx, const char * const directory, const json_deflate_schema_print_options_t * const options) {
    json_deflate_write_string(
        rust_schema_file,
        "/* ===========================================================================\n"
        " *\n"
        " * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.\n"
        " *\n"
        " * ==========================================================================*/\n"
        "\n"
        "/*\n"
        " * This is generated code. It will be regenerated by the json_deflate_tool cmdlet.\n"
        " */\n"
        "\n");

    json_deflate_write_string(rust_schema_file, options->schema_header ? options->schema_header : "use crate::ffi::*;");
    json_deflate_write_string(rust_schema_file, "\n\n");

    json_deflate_schema_type_list_t * const list = calloc(1, sizeof(json_deflate_schema_type_list_t));

    json_deflate_schema_type_t * const wasm_type = json_deflate_bump_get_ptr(area, wasm_ctx->ty_root);
    json_deflate_schema_type_t * const native_type = json_deflate_bump_get_ptr(area, native_ctx->ty_root);
    json_deflate_schema_print_rust_type(rust_schema_file, area, wasm_type, native_type, wasm_ctx, native_ctx, list, options);

    free(list);

    json_deflate_schema_type_t * const type_rel = json_deflate_bump_get_ptr(area, wasm_type->rel_type);

    if (json_deflate_type_get_class(type_rel) != schema_type_struct) {
        char type_name[1024] = {0};
        json_deflate_rust_type_print_declared_name(type_name, sizeof(type_name), area, wasm_ctx, type_rel, options);

        json_deflate_write_string(rust_schema_file, "#[repr(C)]\n");
        json_deflate_write_string(rust_schema_file, "#[derive(Debug, Clone, PartialEq)]\n");
        json_deflate_write_string(rust_schema_file, "#[allow(dead_code)]\n");

        if (options->skip_key_conversion) {
            json_deflate_write_string(rust_schema_file, "#[allow(non_snake_case)]\n");
        }

        json_deflate_write_string(rust_schema_file, "pub struct r#");
        json_deflate_write_string(rust_schema_file, options->root_name);
        json_deflate_write_string(rust_schema_file, "(");
        json_deflate_write_string(rust_schema_file, type_name);
        json_deflate_write_string(rust_schema_file, ");\n\n");
    }

    json_deflate_write_string(rust_schema_file, "implement_deflatable!(r#");
    json_deflate_write_string(rust_schema_file, options->root_name);
    json_deflate_write_string(rust_schema_file, ", \"");
    json_deflate_write_string(rust_schema_file, directory);
    json_deflate_write_string(rust_schema_file, "\", ");
    json_deflate_write_uint(rust_schema_file, metadata->rust_schema_hash);
    json_deflate_write_string(rust_schema_file, ");");
}
