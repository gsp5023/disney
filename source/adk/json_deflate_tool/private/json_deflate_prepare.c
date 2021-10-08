/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_prepare.c

JSON deflate generation of binary schema based on a text schema.
*/

#include "extern/cjson/cJSON.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/json_deflate_tool/json_deflate_tool.h"
#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"

#include <ctype.h>

enum {
    json_deflate_tool_max_string_length = 2048
};

cJSON * json_deflate_schema_navigate_to(cJSON * const json, const char * const path) {
    if (!path) {
        return NULL;
    }

    char temp[json_deflate_max_string_length] = {0};
    for (int i = 0; path[i] && path[i + 1] && path[i + 1] != '/'; i++) {
        temp[i] = path[i + 1];
    }

    if (!temp[0]) {
        return json;
    }

    cJSON * const properties = cJSON_GetObjectItem(json, "properties");
    cJSON * const child = cJSON_GetObjectItem(properties, temp);
    if (!child) {
        return NULL;
    }

    const char * const child_path = path + strlen(temp) + 1;

    return json_deflate_schema_navigate_to(child, child_path);
}

static void json_deflate_schema_type_set_size_and_alignment(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    const json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);

    type->size = 0;
    json_deflate_bump_size_t align = 0;
    for (uint32_t i = 0; i < type->field_count; i++) {
        const json_deflate_schema_field_t * const field = &fields[i];
        const json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);

        if (field->offset + field_type->var > type->size) {
            type->size = field->offset + field_type->var;
        }

        if (field_type->align > align) {
            align = field_type->align;
        }
    }

    // Add any padding at the end.
    type->align = json_deflate_type_is_stored_by_reference(type) ? type->var : align;
    if (align) {
        type->size = ALIGN_INT(type->size, align);
    }

    // Set the size of this type's variables.
    if (json_deflate_type_is_stored_by_reference(type)) {
        json_deflate_schema_type_t * const ptr_type = json_deflate_bump_get_ptr(area, ctx->ty_ptr);
        type->var = ptr_type->size;
    } else {
        type->var = type->size;
    }
}

static void json_deflate_schema_type_arrange_struct(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    // Rearrange fields by the order they are going to appear in the Rust file (currently this is the same as their original order).
    json_deflate_type_verify_offsets(area, type);
    reorder_fields_by_original_order(area, type);
    json_deflate_type_verify_offsets(area, type);

    // Calculate the field offsets with padding between them.
    json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);
    json_deflate_bump_size_t off = 0;
    for (uint32_t i = 0; i < type->field_count; i++) {
        json_deflate_schema_field_t * const field = &fields[i];
        json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);

        if (field_type->align) {
            field->offset = (json_deflate_bump_size_t)FWD_ALIGN_INT(off, field_type->align);
        } else {
            field->offset = off;
        }
        off = field->offset + field_type->var;
        type->size = off;
    }

    json_deflate_schema_type_set_size_and_alignment(area, ctx, type);

    // Rearrange fields by name hash.
    json_deflate_type_verify_offsets(area, type);
    reorder_fields_by_hash(area, type);
    json_deflate_detect_collisions(area, type);
    json_deflate_type_verify_offsets(area, type);
}

static void json_deflate_schema_type_arrange_variant(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    if (type->field_count == 1) {
        type->field_count = 0;
    }

    json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);

    type->align = 0;
    for (uint32_t i = 0; i < type->field_count; i++) {
        const json_deflate_schema_field_t * const field = &fields[i];
        const json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
        if (field_type->align > type->align) {
            type->align = field_type->align;
        }
    }

    for (uint32_t i = 0; i < type->field_count; i++) {
        json_deflate_schema_field_t * const field = &fields[i];

        const char * field_name = json_deflate_bump_get_ptr(area, field->json_name);
        if (!strcmp("tag", field_name)) {
            field->offset = 0;
        } else {
            field->offset = (json_deflate_bump_size_t)FWD_ALIGN_INT(1, type->align);
        }
    }

    json_deflate_schema_type_set_size_and_alignment(area, ctx, type);
}

static void json_deflate_schema_type_arrange_array(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    json_deflate_schema_field_t * const ptr = json_deflate_type_lookup_field(area, type, "ptr");
    json_deflate_schema_field_t * const cap = json_deflate_type_lookup_field(area, type, "cap");
    json_deflate_schema_field_t * const len = json_deflate_type_lookup_field(area, type, "len");
    json_deflate_schema_type_t * const ptr_type = json_deflate_bump_get_ptr(area, ptr->type);
    json_deflate_schema_type_t * const cap_type = json_deflate_bump_get_ptr(area, cap->type);
    json_deflate_schema_type_t * const len_type = json_deflate_bump_get_ptr(area, len->type);

    ptr->offset = 0;
    cap->offset = (json_deflate_bump_size_t)FWD_ALIGN_INT(ptr_type->var, cap_type->align);
    len->offset = (json_deflate_bump_size_t)FWD_ALIGN_INT(cap->offset + cap_type->var, len_type->align);

    json_deflate_schema_type_set_size_and_alignment(area, ctx, type);
}

static void json_deflate_schema_type_arrange_map(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    json_deflate_schema_field_t * const keys = json_deflate_type_lookup_field(area, type, "keys");
    json_deflate_schema_field_t * const values = json_deflate_type_lookup_field(area, type, "values");
    json_deflate_schema_field_t * const strings = json_deflate_type_lookup_field(area, type, "strings");

    json_deflate_schema_type_t * const keys_type = json_deflate_bump_get_ptr(area, keys->type);
    json_deflate_schema_type_t * const values_type = json_deflate_bump_get_ptr(area, values->type);
    json_deflate_schema_type_t * const strings_type = json_deflate_bump_get_ptr(area, strings->type);

    json_deflate_schema_type_arrange(area, ctx, keys_type);
    json_deflate_schema_type_arrange(area, ctx, values_type);
    json_deflate_schema_type_arrange(area, ctx, strings_type);

    json_deflate_schema_type_arrange_struct(area, ctx, type);
}

void json_deflate_schema_type_arrange(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    reorder_fields_by_hash(area, type);
    json_deflate_detect_collisions(area, type);
    if (json_deflate_type_get_class(type) == schema_type_struct) {
        json_deflate_schema_type_arrange_struct(area, ctx, type);
    } else if (json_deflate_type_get_class(type) == schema_type_variant) {
        json_deflate_schema_type_arrange_variant(area, ctx, type);
    } else if (json_deflate_type_get_class(type) == schema_type_array) {
        json_deflate_schema_type_arrange_array(area, ctx, type);
    } else if (json_deflate_type_get_class(type) == schema_type_map) {
        json_deflate_schema_type_arrange_map(area, ctx, type);
    } else {
        TRAP("[json_deflate_tool] Unhandled type class");
    }
}

json_deflate_schema_type_t * json_deflate_schema_get_type_at_first_byte(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    if (json_deflate_type_get_class(type) == schema_type_array) {
        return type;
    }

    json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);

    for (uint32_t i = 0; i < type->field_count; i++) {
        json_deflate_schema_field_t * const field = &fields[i];
        if (!field->offset) {
            json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
            if (field_type->var) {
                return json_deflate_schema_get_type_at_first_byte(area, ctx, field_type);
            }
        }
    }

    return type;
}

json_deflate_schema_type_t * json_deflate_schema_turn_into_result_type(json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const ctx, json_deflate_schema_type_t * const type) {
    json_deflate_schema_type_t * const result_type_ctor = json_deflate_bump_get_ptr(area, ctx->ty_result);
    json_deflate_schema_type_t * const result_type = json_deflate_type_construct(area, result_type_ctor, type);
    json_deflate_schema_field_t * const result_ok_choice = json_deflate_type_lookup_field(area, result_type, "Ok");
    result_ok_choice->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type);

    json_deflate_schema_type_arrange(area, ctx, result_type);

    return result_type;
}

static json_deflate_schema_field_t * lookup_field_linear(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const type, const char * const name) {
    json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);
    for (uint32_t i = 0; i < type->field_count; i++) {
        json_deflate_schema_field_t * const field = &fields[i];
        char * const field_name = json_deflate_bump_get_ptr(area, field->json_name);
        if (!strcmp(name, field_name)) {
            return field;
        }
    }

    return NULL;
}

static json_deflate_schema_path_node_t add_to_path(const json_deflate_schema_path_node_t * const node, const char * const part) {
    json_deflate_schema_path_node_t new_node = {0};

    new_node.parent = node;
    new_node.value = part;

    return new_node;
}

static void get_path_str(const json_deflate_schema_path_node_t * const node, char * const path, size_t size) {
    strcpy_s(path, size, "");

    const json_deflate_schema_path_node_t * current = node;
    while (current) {
        char temp[json_deflate_tool_max_string_length] = {0};
        strcpy_s(temp, sizeof(temp), path);

        path[0] = 0;
        strcpy_s(path, size, current->value);
        strcat_s(path, size, "/");

        strcat_s(path, size, temp);

        current = current->parent;
    }

    const size_t len = strlen(path);
    path[len - 1] = 0;
}

static json_deflate_schema_type_t * lookup_type_by_path(json_deflate_schema_type_map_t * const map, const char * const path) {
    for (int i = 0; i < map->idx; i++) {
        if (!strcmp(map->mappings[i].path, path)) {
            return map->mappings[i].type;
        }
    }

    TRAP("[json_deflate_tool] Path %s does not exist or has not been built yet. Make sure the string is spelled correctly and that it comes after the referenced path (order matters currently).", path);

    return NULL;
}

static void add_path_str(json_deflate_schema_type_map_t * const map, const char * const str, json_deflate_schema_type_t * const type) {
    strcpy_s(map->mappings[map->idx].path, ARRAY_SIZE(map->mappings[map->idx].path), str);
    map->mappings[map->idx].type = type;
    map->idx++;
}

static void add_path(json_deflate_schema_type_map_t * const map, const json_deflate_schema_path_node_t * const node, json_deflate_schema_type_t * const type) {
    char path[json_deflate_tool_max_string_length] = {0};
    get_path_str(node, path, sizeof(path));
    add_path_str(map, path, type);
}

json_deflate_schema_type_t * json_deflate_schema_prepare_specific_rec(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, const cJSON * const choice, json_deflate_schema_type_map_t * const map, const json_deflate_schema_path_node_t * const node, const char * name_hint, const json_deflate_schema_options_t * const options) {
    const json_deflate_schema_type_t * const ptr_type = json_deflate_bump_get_ptr(area, ctx->ty_ptr);

    int root = !name_hint;

    if (root) {
        name_hint = options->root_name;
    }

    if (!strcmp("object", choice->valuestring)) {
        json_deflate_schema_type_t * const type = json_deflate_bump_alloc(area, sizeof(json_deflate_schema_type_t), sizeof(json_deflate_schema_type_t));
        json_deflate_type_init(type);

        add_path(map, node, type);

        if (root) {
            ctx->ty_root = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type);
        }

        json_deflate_type_set_class(type, schema_type_struct);
        type->var = ptr_type->size;

        const cJSON * const schema_name_hint = cJSON_GetObjectItem(json, "name");
        if (schema_name_hint) {
            const char * const name = json_deflate_bump_store_str(area, schema_name_hint->valuestring);
            type->name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, name);
        } else {
            const char * const name = json_deflate_bump_store_str(area, name_hint);
            type->name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, name);
        }
        const char * const type_name = json_deflate_bump_get_ptr(area, type->name);

        const cJSON * const props = cJSON_GetObjectItem(json, "properties");
        type->field_count = cJSON_GetArraySize(props);

        ASSERT_MSG(type->field_count < 16384, "[json_deflate_tool] Field count limit exceeded.");

        json_deflate_schema_field_t * fields = NULL;
        if (type->field_count) {
            fields = json_deflate_bump_alloc_array(area, type->field_count, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));
            type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, fields);
        }

        const cJSON * prop = NULL;
        uint32_t index = 0;
        cJSON_ArrayForEach(prop, props) {
            json_deflate_schema_field_t * const field = &fields[index];
            json_deflate_field_init(field);

            field->original_order = index;

            const char * const field_name = json_deflate_bump_store_str(area, prop->string);
            field->json_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_name);
            field->hash = json_deflate_schema_calculate_field_name_hash(field_name);

            const cJSON * const field_name_override = cJSON_GetObjectItem(prop, "field-name");
            if (field_name_override) {
                const char * const field_name_override_str = json_deflate_bump_store_str(area, field_name_override->valuestring);
                field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_name_override_str);
            } else {
                field->rust_name = field->json_name;
            }

            char field_name_hint[1024] = {0};
            strcpy_s(field_name_hint, sizeof(field_name_hint), type_name);
            strcat_s(field_name_hint, sizeof(field_name_hint), prop->string);
            field_name_hint[strlen(type_name)] = (char)toupper(field_name_hint[strlen(type_name)]);

            const json_deflate_schema_path_node_t child_node = add_to_path(node, prop->string);
            json_deflate_schema_type_t * const field_type = json_deflate_schema_prepare_rec(area, ctx, prop, map, &child_node, field_name_hint, options);
            field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_type);

            json_deflate_field_verify_offsets(area, field);

            index++;
        }

        json_deflate_type_verify_offsets(area, type);

        reorder_fields_by_hash(area, type);
        json_deflate_detect_collisions(area, type);

        json_deflate_type_verify_offsets(area, type);

        const cJSON * const reqs = cJSON_GetObjectItem(json, "required");
        const cJSON * req = NULL;
        cJSON_ArrayForEach(req, reqs) {
            const char * const req_field_name = cJSON_GetStringValue(req);
            json_deflate_schema_field_t * const req_field = json_deflate_type_lookup_field(area, type, req_field_name);
            VERIFY_MSG(req_field, "[json_deflate_tool] Property %s listed in \"required\" is not among the defined properties", req_field_name);
            json_deflate_field_set_required(req_field, 1);
        }

        const cJSON * const catches = cJSON_GetObjectItem(json, "catch");
        const cJSON * catch_ = NULL;
        cJSON_ArrayForEach(catch_, catches) {
            const char * const catch_field_name = cJSON_GetStringValue(catch_);
            json_deflate_schema_field_t * const catch_field = json_deflate_type_lookup_field(area, type, catch_field_name);
            VERIFY_MSG(catch_field, "[json_deflate_tool] Property %s listed in \"catch\" is not among the defined properties", catch_field_name);
            json_deflate_field_set_can_fail(catch_field, 1);
        }

        for (uint32_t i = 0; i < type->field_count; i++) {
            json_deflate_schema_field_t * const field = &fields[i];

            if (json_deflate_field_can_fail(field)) {
                json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
                json_deflate_schema_type_t * const result_type = json_deflate_schema_turn_into_result_type(area, ctx, field_type);
                json_deflate_schema_field_t * const result_ok_choice = json_deflate_type_lookup_field(area, result_type, "Ok");
                json_deflate_schema_field_t * const result_err_choice = json_deflate_type_lookup_field(area, result_type, "Err");
                json_deflate_field_set_activation_value(result_ok_choice, 1, 0);
                json_deflate_field_set_activation_value(result_err_choice, 1, 1);

                field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, result_type);

                json_deflate_schema_type_arrange(area, ctx, result_type);
            }

            if (!json_deflate_field_is_required(field)) {
                json_deflate_schema_type_t * const optional_type_ctor = json_deflate_bump_get_ptr(area, ctx->ty_option);
                json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
                json_deflate_schema_type_t * const optional_type = json_deflate_type_construct(area, optional_type_ctor, field_type);
                json_deflate_schema_field_t * const optional_none_choice = json_deflate_type_lookup_field(area, optional_type, "None");
                json_deflate_schema_field_t * const optional_some_choice = json_deflate_type_lookup_field(area, optional_type, "Some");
                json_deflate_field_set_activation_value(optional_none_choice, 1, 0);
                json_deflate_field_set_activation_value(optional_some_choice, 1, 1);

                optional_some_choice->type = field->type;

                json_deflate_schema_type_arrange(area, ctx, optional_type);

                field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, optional_type);
            }
        }

        json_deflate_schema_type_arrange(area, ctx, type);
        return type;
    } else if (!strcmp("array", choice->valuestring)) {
        const cJSON * const items = cJSON_GetObjectItem(json, "items");
        VERIFY_MSG(items, "No \"items\" field");

        json_deflate_schema_type_t * const type = json_deflate_bump_alloc(area, sizeof(json_deflate_schema_type_t), sizeof(json_deflate_schema_type_t));
        memcpy(type, json_deflate_bump_get_ptr(area, ctx->ty_array), sizeof(json_deflate_schema_type_t));

        type->type_ctor = ctx->ty_array;

        add_path(map, node, type);

        if (root) {
            ctx->ty_root = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type);
        }

        const json_deflate_schema_path_node_t items_node = add_to_path(node, "<items>");
        json_deflate_schema_type_t * const elem_type = json_deflate_schema_prepare_rec(area, ctx, items, map, &items_node, name_hint, options);

        char * const name = json_deflate_bump_alloc_array(area, 256, sizeof(char), sizeof(char));
        const char prefix[] = "ArrayOf";
        strcpy_s(name, 256 * sizeof(char), prefix);
        const char * const elem_type_name = json_deflate_bump_get_ptr(area, elem_type->name);
        strcat_s(name, 256 * sizeof(char), elem_type_name);
        name[sizeof(prefix) - 1] = (char)toupper(name[sizeof(prefix) - 1]);
        type->name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, name);

        type->rel_type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, elem_type);

        json_deflate_schema_type_arrange(area, ctx, type);
        return type;
    } else if (!strcmp("map", choice->valuestring)) {
        const cJSON * const items = cJSON_GetObjectItem(json, "items");
        VERIFY_MSG(items, "No \"items\" field");

        const json_deflate_schema_path_node_t items_node = add_to_path(node, "<items>");
        json_deflate_schema_type_t * const elem_type = json_deflate_schema_prepare_rec(area, ctx, items, map, &items_node, name_hint, options);

        json_deflate_schema_type_t * const map_type_ctor = json_deflate_bump_get_ptr(area, ctx->ty_map);
        json_deflate_schema_type_t * const map_type = json_deflate_type_construct(area, map_type_ctor, elem_type);

        if (root) {
            ctx->ty_root = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_type);
        }

        json_deflate_schema_type_arrange(area, ctx, map_type);
        return map_type;
    } else if (!strcmp("boolean", choice->valuestring)) {
        return json_deflate_bump_get_ptr(area, ctx->ty_boolean);
    } else if (!strcmp("integer", choice->valuestring)) {
        return json_deflate_bump_get_ptr(area, ctx->ty_integer);
    } else if (!strcmp("number", choice->valuestring)) {
        return json_deflate_bump_get_ptr(area, ctx->ty_number);
    } else if (!strcmp("string", choice->valuestring)) {
        return json_deflate_bump_get_ptr(area, ctx->ty_string);
    } else {
        TRAP("Unknown JSON type: %s", choice->valuestring);
        return NULL;
    }
}

json_deflate_schema_type_t * json_deflate_schema_prepare_rec(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, json_deflate_schema_type_map_t * const map, const json_deflate_schema_path_node_t * const node, const char * name_hint, const json_deflate_schema_options_t * const options) {
    const json_deflate_schema_type_t * const ptr_type = json_deflate_bump_get_ptr(area, ctx->ty_ptr);

    const cJSON * const json_ty = cJSON_GetObjectItem(json, "type");
    if (json_ty) {
        if (json_ty->valuestring && json_ty->valuestring[0] == '/') {
            return lookup_type_by_path(map, json_ty->valuestring);
        } else if (!cJSON_IsArray(json_ty)) {
            return json_deflate_schema_prepare_specific_rec(area, ctx, json, json_ty, map, node, name_hint, options);
        } else {
            if (!cJSON_GetArraySize(json_ty)) {
                char offending_path[json_deflate_tool_max_string_length] = {0};
                get_path_str(node, offending_path, sizeof(offending_path));
                TRAP(
                    "[json_deflate_tool] %s indicates an empty set of candidate types, which is not allowed because it cannot be instantiated. \
This type may have been inferred from fields that had no type information in the sample. Please specify some type information in the schema to continue.",
                    offending_path);
            }

            json_deflate_schema_type_t * const type = json_deflate_bump_alloc(area, sizeof(json_deflate_schema_type_t), sizeof(json_deflate_schema_type_t));
            json_deflate_type_init(type);

            add_path(map, node, type);

            json_deflate_type_set_class(type, schema_type_variant);

            type->var = ptr_type->size;
            type->align = type->var;

            char variant_type_name[json_deflate_tool_max_string_length] = {0};
            strcpy_s(variant_type_name, sizeof(variant_type_name), name_hint);
            strcat_s(variant_type_name, sizeof(variant_type_name), "Choice");
            const char * const name = json_deflate_bump_store_str(area, variant_type_name);
            type->name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, name);

            type->field_count = 1 + cJSON_GetArraySize(json_ty);
            json_deflate_schema_field_t * const fields = json_deflate_bump_alloc_array(area, type->field_count, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));

            json_deflate_schema_field_t * const tag_field = &fields[0];

            char * const stored_tag_field_name = json_deflate_bump_store_str(area, "tag");
            tag_field->json_name = tag_field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, stored_tag_field_name);
            tag_field->hash = json_deflate_schema_calculate_field_name_hash(stored_tag_field_name);

            tag_field->type = ctx->ty_boolean;

            tag_field->offset = 0;

            type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, fields);

            const cJSON * it = NULL;
            json_deflate_bump_size_t largest_choice = 0;
            uint32_t index = 1;
            cJSON_ArrayForEach(it, json_ty) {
                json_deflate_schema_field_t * const field = &fields[index];
                json_deflate_field_init(field);

                char indexer[32] = {0};
                strcpy_s(indexer, sizeof(indexer), it->valuestring);
                const json_deflate_schema_path_node_t child_node = add_to_path(node, indexer);
                json_deflate_schema_type_t * const field_type = json_deflate_schema_prepare_specific_rec(area, ctx, json, it, map, &child_node, name_hint, options);

                field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_type);

                field->choice_value = index - 1;

                const char * const field_type_name = json_deflate_bump_get_ptr(area, field_type->name);
                const char * const field_type_name_object = "object";

                char field_name[256] = {0};
                strcat_s(field_name, sizeof(field_name), "Is");
                const size_t len_so_far = strlen(field_name);
                if (json_deflate_type_is_builtin(field_type)) {
                    strcat_s(field_name, sizeof(field_name), field_type_name);
                } else {
                    strcat_s(field_name, sizeof(field_name), field_type_name_object);
                }
                field_name[len_so_far] = (char)toupper(field_name[len_so_far]);

                const size_t field_name_buffer_length = strlen(field_name) + 1;
                char * const field_name_alloc = json_deflate_bump_alloc(area, field_name_buffer_length, sizeof(char));
                strcpy_s(field_name_alloc, field_name_buffer_length, field_name);
                field->json_name = field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_name_alloc);

                field->hash = json_deflate_schema_calculate_field_name_hash(field_name);

                json_deflate_field_verify_offsets(area, field);

                index++;
            }

            for (uint32_t i = 0; i < type->field_count; i++) {
                json_deflate_schema_field_t * const field = &fields[i];
                if (field == tag_field) {
                    continue;
                }

                const json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
                field->offset = field_type->align;

                if (field->offset + field_type->var > largest_choice) {
                    largest_choice = field->offset + field_type->var;
                }
            }

            type->size = largest_choice;

            reorder_fields_by_hash(area, type);
            json_deflate_detect_collisions(area, type);

            json_deflate_type_verify_offsets(area, type);

            json_deflate_schema_type_arrange(area, ctx, type);

            return type;
        }
    } else {
        TRAP("Missing or invalid type annotation: %s", json->valuestring);
        return NULL;
    }
}

json_deflate_schema_type_t * json_deflate_schema_prepare(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const cJSON * const json, const char * name_hint, const json_deflate_schema_options_t * const options) {
    json_deflate_schema_type_map_t map = {0};
    map.mappings = json_deflate_calloc(json_deflate_tool_max_type_count, sizeof(json_deflate_schema_type_mapping_t));
    json_deflate_schema_path_node_t node = {0};
    node.value = "";
    json_deflate_schema_type_t * const user_defined_root_type = json_deflate_schema_prepare_rec(area, ctx, json, &map, &node, name_hint, options);

    json_deflate_schema_type_t * const root_result_type = json_deflate_schema_turn_into_result_type(area, ctx, user_defined_root_type);
    json_deflate_schema_type_arrange(area, ctx, root_result_type);
    json_deflate_type_set_stored_by_reference(root_result_type, 0);
    ctx->ty_root = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, root_result_type);

    json_deflate_free(map.mappings);
    return root_result_type;
}

void json_deflate_offset_verify(const json_deflate_bump_area_t * const base, const json_deflate_bump_ptr_t offset) {
    json_deflate_bump_get_ptr(base, offset);
}

void json_deflate_field_verify_offsets(const json_deflate_bump_area_t * const base, const json_deflate_schema_field_t * const f) {
    json_deflate_offset_verify(base, f->json_name);
    json_deflate_offset_verify(base, f->type);
}

void json_deflate_type_verify_offsets(const json_deflate_bump_area_t * const base, const json_deflate_schema_type_t * const t) {
    json_deflate_offset_verify(base, t->name);
    if (t->field_count) {
        json_deflate_offset_verify(base, t->fields);
    }
}

json_deflate_schema_type_t * json_deflate_type_register_builtin(json_deflate_bump_area_t * const area, const char * const name, const json_deflate_bump_size_t size, const json_deflate_bump_size_t ptr_size, const char ref) {
    json_deflate_schema_type_t * const type = json_deflate_bump_alloc(area, sizeof(json_deflate_schema_type_t), sizeof(json_deflate_schema_type_t));
    json_deflate_type_init(type);

    const size_t len0 = strlen(name) + 1;
    char * const stored_name = json_deflate_bump_alloc(area, len0, sizeof(char));
    strcpy_s(stored_name, len0, name);
    type->name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, stored_name);

    type->size = size;
    type->var = ref ? ptr_size : size;
    json_deflate_type_set_builtin(type, 1);

    type->align = type->var;

    return type;
}

void json_deflate_context_init(json_deflate_bump_area_t * const area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, const json_deflate_schema_options_t * const options) {
    const json_deflate_bump_size_t ptr_size = target == json_deflate_parse_target_wasm ? sizeof(uint32_t) : sizeof(void *);

    json_deflate_schema_type_t * const ty_integer = json_deflate_type_register_builtin(area, "integer", sizeof(int32_t), ptr_size, 0);
    ctx->ty_integer = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ty_integer);

    json_deflate_schema_type_t * const ty_number = json_deflate_type_register_builtin(area, "double", sizeof(double), ptr_size, 0);
    ctx->ty_number = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ty_number);

    json_deflate_schema_type_t * const ty_boolean = json_deflate_type_register_builtin(area, "boolean", sizeof(uint8_t), ptr_size, 0);
    ctx->ty_boolean = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ty_boolean);
    json_deflate_type_set_non_exhaustive(ty_boolean, 1);

    ctx->ty_string = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, json_deflate_type_register_builtin(area, "string", ptr_size, ptr_size, 0));
    ctx->ty_array = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, json_deflate_type_register_builtin(area, "array", ptr_size, ptr_size, 0));
    ctx->ty_ptr = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, json_deflate_type_register_builtin(area, "ptr", ptr_size, ptr_size, 0));
    ctx->ty_size = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, json_deflate_type_register_builtin(area, "usize", ptr_size, ptr_size, 0));
    ctx->ty_char = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, json_deflate_type_register_builtin(area, "byte", sizeof(char), ptr_size, 0));

    json_deflate_schema_type_t * const ty_struct = json_deflate_type_register_builtin(area, "StructPrototype", 0, ptr_size, 0);
    ctx->ty_struct = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ty_struct);
    json_deflate_type_set_class(ty_struct, schema_type_struct);
    json_deflate_type_set_builtin(ty_struct, 1);

    json_deflate_schema_type_t * const ty_enum = json_deflate_type_register_builtin(area, "EnumPrototype", 0, ptr_size, 0);
    ctx->ty_enum = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ty_enum);
    json_deflate_type_set_class(ty_enum, schema_type_variant);
    json_deflate_type_set_builtin(ty_enum, 1);

    json_deflate_schema_type_t * const unit_type = json_deflate_type_register_builtin(area, "unit", 0, ptr_size, 0);
    ctx->ty_unit = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, unit_type);

    json_deflate_schema_type_t * const ptr_type = json_deflate_bump_get_ptr(area, ctx->ty_ptr);
    json_deflate_schema_type_t * const len_type = json_deflate_bump_get_ptr(area, ctx->ty_size);

    json_deflate_schema_field_t * const fields_array = json_deflate_bump_alloc_array(area, 3, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));

    json_deflate_schema_field_t * const field_ptr = &fields_array[0];
    json_deflate_schema_field_t * const field_cap = &fields_array[1];
    json_deflate_schema_field_t * const field_len = &fields_array[2];

    json_deflate_field_init(field_ptr);
    json_deflate_field_init(field_cap);
    json_deflate_field_init(field_len);

    field_ptr->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, ptr_type);
    field_cap->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, len_type);
    field_len->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, len_type);

    const char * const field_ptr_name = json_deflate_bump_store_str(area, "ptr");
    const char * const field_cap_name = json_deflate_bump_store_str(area, "cap");
    const char * const field_len_name = json_deflate_bump_store_str(area, "len");

    field_ptr->json_name = field_ptr->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_ptr_name);
    field_cap->json_name = field_cap->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_cap_name);
    field_len->json_name = field_len->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_len_name);

    field_ptr->hash = json_deflate_schema_calculate_field_name_hash(field_ptr_name);
    field_cap->hash = json_deflate_schema_calculate_field_name_hash(field_cap_name);
    field_len->hash = json_deflate_schema_calculate_field_name_hash(field_len_name);

    json_deflate_schema_type_t * const array_type = json_deflate_bump_get_ptr(area, ctx->ty_array);
    json_deflate_type_set_class(array_type, schema_type_array);
    json_deflate_type_set_non_exhaustive(array_type, 1);
    json_deflate_type_set_type_constructor(array_type, 1);
    array_type->field_count = 3;
    array_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_ptr);
    json_deflate_schema_type_arrange(area, ctx, array_type);

    json_deflate_schema_type_t * const string_type = json_deflate_bump_get_ptr(area, ctx->ty_string);
    json_deflate_type_set_non_exhaustive(string_type, 1);
    json_deflate_type_set_class(string_type, schema_type_array);
    string_type->field_count = 3;
    string_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_ptr);
    string_type->rel_type = ctx->ty_char;
    json_deflate_schema_type_arrange(area, ctx, string_type);

    json_deflate_schema_type_t * const result_type = json_deflate_type_register_builtin(area, "FResult", ptr_size, ptr_size, 0);
    ctx->ty_result = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, result_type);

    json_deflate_type_set_class(result_type, schema_type_variant);
    json_deflate_type_set_type_constructor(result_type, 1);

    result_type->field_count = 3;

    json_deflate_schema_field_t * const result_fields = json_deflate_bump_alloc_array(area, 3, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));
    result_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, result_fields);

    json_deflate_schema_field_t * const result_tag = &result_fields[0];
    const char * field_result_tag_name = json_deflate_bump_store_str(area, "tag");
    result_tag->json_name = result_tag->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_result_tag_name);
    result_tag->hash = json_deflate_schema_calculate_field_name_hash(field_result_tag_name);
    result_tag->type = ctx->ty_char;
    result_tag->offset = 0;
    result_tag->original_order = 0;

    json_deflate_schema_field_t * const result_ok = &result_fields[1];
    const char * const field_result_ok_name = json_deflate_bump_store_str(area, "Ok");
    result_ok->json_name = result_ok->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_result_ok_name);
    result_ok->hash = json_deflate_schema_calculate_field_name_hash(field_result_ok_name);
    result_ok->original_order = 1;
    json_deflate_field_set_activation_value(result_ok, 1, 0);

    json_deflate_schema_field_t * const result_err = &result_fields[2];
    const char * const field_result_err_name = json_deflate_bump_store_str(area, "Err");
    result_err->json_name = result_err->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_result_err_name);
    result_err->hash = json_deflate_schema_calculate_field_name_hash(field_result_err_name);
    result_err->type = ctx->ty_string;
    result_err->original_order = 2;
    json_deflate_field_set_activation_value(result_err, 1, 1);

    reorder_fields_by_hash(area, result_type);
    json_deflate_detect_collisions(area, result_type);

    json_deflate_schema_type_t * const option_type = json_deflate_type_register_builtin(area, "FOption", ptr_size, ptr_size, 0);
    ctx->ty_option = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, option_type);

    json_deflate_type_set_class(option_type, schema_type_variant);
    json_deflate_type_set_type_constructor(option_type, 1);

    option_type->field_count = 3;

    json_deflate_schema_field_t * const option_fields = json_deflate_bump_alloc_array(area, 3, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));
    option_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, option_fields);

    json_deflate_schema_field_t * const option_tag = &option_fields[0];
    const char * const field_option_tag_name = json_deflate_bump_store_str(area, "tag");
    option_tag->json_name = option_tag->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_option_tag_name);
    option_tag->hash = json_deflate_schema_calculate_field_name_hash(field_option_tag_name);
    option_tag->type = ctx->ty_char;
    option_tag->offset = 0;
    option_tag->original_order = 0;

    json_deflate_schema_field_t * const option_none = &option_fields[1];
    const char * const field_option_none_name = json_deflate_bump_store_str(area, "None");
    option_none->json_name = option_none->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_option_none_name);
    option_none->hash = json_deflate_schema_calculate_field_name_hash(field_option_none_name);
    option_none->type = ctx->ty_unit;
    option_none->offset = 0;
    option_none->original_order = 1;

    json_deflate_schema_field_t * option_some = &option_fields[2];
    const char * const field_option_some_name = json_deflate_bump_store_str(area, "Some");
    option_some->json_name = option_some->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, field_option_some_name);
    option_some->hash = json_deflate_schema_calculate_field_name_hash(field_option_some_name);
    option_some->original_order = 2;

    reorder_fields_by_hash(area, option_type);
    json_deflate_detect_collisions(area, option_type);

    json_deflate_schema_type_t * const map_type = json_deflate_type_register_builtin(area, "FPropertyBag", array_type->size * 2, ptr_size, 0);
    ctx->ty_map = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_type);

    json_deflate_type_set_class(map_type, schema_type_map);
    json_deflate_type_set_type_constructor(map_type, 1);

    map_type->field_count = 3;

    json_deflate_schema_field_t * const map_fields = json_deflate_bump_alloc_array(area, map_type->field_count, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));
    map_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_fields);

    // hashes :: Vec<u32>
    json_deflate_schema_field_t * const map_keys_field = &map_fields[0];
    const char * const map_keys_field_name = json_deflate_bump_store_str(area, "keys");
    map_keys_field->json_name = map_keys_field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_keys_field_name);
    map_keys_field->hash = json_deflate_schema_calculate_field_name_hash(map_keys_field_name);
    json_deflate_schema_type_t * const map_keys_array = json_deflate_type_construct(area, array_type, ty_integer);
    map_keys_field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_keys_array);
    map_keys_field->offset = 0;
    map_keys_field->original_order = 0;

    // values :: Vec<T>
    json_deflate_schema_field_t * const map_values_field = &map_fields[1];
    const char * const map_values_field_name = json_deflate_bump_store_str(area, "values");
    map_values_field->json_name = map_values_field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_values_field_name);
    map_values_field->hash = json_deflate_schema_calculate_field_name_hash(map_values_field_name);
    map_values_field->type = ctx->ty_array;
    map_values_field->offset = 0;
    map_values_field->original_order = 1;

    // keys :: Vec<String>
    json_deflate_schema_field_t * const map_strings_field = &map_fields[2];
    const char * const map_strings_field_name = json_deflate_bump_store_str(area, "strings");
    map_strings_field->json_name = map_strings_field->rust_name = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_strings_field_name);
    map_strings_field->hash = json_deflate_schema_calculate_field_name_hash(map_strings_field_name);
    json_deflate_schema_type_t * const map_strings_array = json_deflate_type_construct(area, array_type, string_type);
    map_strings_field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, map_strings_array);
    map_strings_field->offset = 0;
    map_strings_field->original_order = 2;

    reorder_fields_by_hash(area, map_type);
    json_deflate_detect_collisions(area, map_type);
}
