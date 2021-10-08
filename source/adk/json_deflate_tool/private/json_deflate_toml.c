/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_view.c

JSON deflate model printer.
*/

#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"

void json_deflate_schema_print_toml(FILE * const toml_file, json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const type) {
    json_deflate_type_verify_offsets(area, type);

    const char * const type_name = json_deflate_bump_get_ptr(area, type->name);

    if (!json_deflate_type_is_builtin(type) || json_deflate_type_get_class(type) == schema_type_array || !strcmp("FOption", type_name) || !strcmp("FResult", type_name)) {
        if (type->rel_type) {
            json_deflate_schema_type_t * rel_type = json_deflate_bump_get_ptr(area, type->rel_type);
            json_deflate_schema_print_toml(toml_file, area, rel_type);
        }

        char type_class[16] = {0};
        if (json_deflate_type_get_class(type) == schema_type_struct) {
            strcpy_s(type_class, sizeof(type_class), "struct");
        } else if (json_deflate_type_get_class(type) == schema_type_variant) {
            strcpy_s(type_class, sizeof(type_class), "variant");
        } else {
            strcpy_s(type_class, sizeof(type_class), "array");
        }

        const char * bool_str[2] = {"\"False\"", "\"True\""};
        json_deflate_write_string(toml_file, "[");
        json_deflate_write_string(toml_file, type_name);
        json_deflate_write_string(toml_file, "]\nClass = \"");
        json_deflate_write_string(toml_file, type_class);
        json_deflate_write_string(toml_file, "\"\nSize = ");
        json_deflate_write_int(toml_file, type->size);
        json_deflate_write_string(toml_file, "\nStorage = ");
        json_deflate_write_int(toml_file, type->var);
        json_deflate_write_string(toml_file, "\nAlignment = ");
        json_deflate_write_int(toml_file, type->align);
        json_deflate_write_string(toml_file, "\nByRef = ");
        json_deflate_write_string(toml_file, bool_str[json_deflate_type_is_stored_by_reference(type)]);
        json_deflate_write_string(toml_file, "\n");

        if (type->field_count) {
            json_deflate_schema_field_t * fields = json_deflate_bump_get_ptr(area, type->fields);

            reorder_fields_by_offset(area, type);

            for (uint64_t i = 0; i < type->field_count; i++) {
                json_deflate_field_verify_offsets(area, &fields[i]);
                json_deflate_schema_field_t * field = &fields[i];
                const char * const field_name = json_deflate_bump_get_ptr(area, field->rust_name);
                json_deflate_schema_type_t * field_type = json_deflate_bump_get_ptr(area, field->type);
                const char * const field_type_name = json_deflate_bump_get_ptr(area, field_type->name);
                json_deflate_write_string(toml_file, "  [");
                json_deflate_write_string(toml_file, type_name);
                json_deflate_write_string(toml_file, ".Fields.");
                json_deflate_write_string(toml_file, field_name);
                json_deflate_write_string(toml_file, "]\n  Type = \"");
                json_deflate_write_string(toml_file, field_type_name);
                json_deflate_write_string(toml_file, "\"\n  Offset = ");
                json_deflate_write_int(toml_file, field->offset);
                json_deflate_write_string(toml_file, "\n  Size = ");
                json_deflate_write_int(toml_file, field_type->var);
                json_deflate_write_string(toml_file, "\n  CRC32 = \"");
                json_deflate_write_hex(toml_file, field->hash);
                json_deflate_write_string(toml_file, "\"\n  Activation = ");
                json_deflate_write_int(toml_file, field->choice_value);
                json_deflate_write_string(toml_file, "\n");
            }

            reorder_fields_by_hash(area, type);
        }

        json_deflate_write_string(toml_file, "\n");

        if (type->field_count) {
            json_deflate_schema_field_t * fields = json_deflate_bump_get_ptr(area, type->fields);
            for (uint64_t i = 0; i < type->field_count; i++) {
                json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, fields[i].type);
                json_deflate_schema_print_toml(toml_file, area, field_type);
            }
        }
    }
}
