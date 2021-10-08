/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_schema.c

JSON deflate schema model and operations.
*/

#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/runtime/crc.h"

#define SIGN(_x) (_x > 0 ? 1 : (_x < 0 ? -1 : 0))

uint32_t json_deflate_schema_calculate_field_name_hash(const char * const text) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    uint32_t ret = crc_str_32(text);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

int32_t json_deflate_flag_mask_calculate_shift(const uint32_t mask) {
    uint32_t m_mask = mask;
    if (!m_mask) {
        return -1;
    }

    return __builtin_ctz(m_mask);
}

uint8_t json_deflate_flags_get_value(const uint32_t * const flags, const uint32_t mask) {
    int32_t shift = json_deflate_flag_mask_calculate_shift(mask);
    uint32_t masked = (*flags & mask) >> shift;
    return (uint8_t)masked;
}

void json_deflate_flags_set_value(uint32_t * const flags, const uint8_t val, const uint32_t mask) {
    *flags &= ~mask;
    if (val) {
        int32_t shift = json_deflate_flag_mask_calculate_shift(mask);
        uint32_t val_mask = val << shift;
        *flags |= val_mask;
        ASSERT_MSG(json_deflate_flags_get_value(flags, mask) == val, "[json_deflate] Bitflag inconsistency");
    }
}

void json_deflate_type_set_stored_by_reference(json_deflate_schema_type_t * const type, const uint8_t val) {
    json_deflate_flags_set_value(&type->flags, (uint8_t)val, 0x80);
}

uint8_t json_deflate_type_is_stored_by_reference(const json_deflate_schema_type_t * const type) {
    return json_deflate_flags_get_value(&type->flags, 0x80);
}

void json_deflate_type_set_builtin(json_deflate_schema_type_t * const type, const uint8_t val) {
    json_deflate_flags_set_value(&type->flags, (uint8_t)val, 0x40);
}

uint8_t json_deflate_type_is_builtin(const json_deflate_schema_type_t * const type) {
    return json_deflate_flags_get_value(&type->flags, 0x40);
}

void json_deflate_type_set_type_constructor(json_deflate_schema_type_t * const type, const uint8_t val) {
    json_deflate_flags_set_value(&type->flags, (uint8_t)val, 0x20);
}

uint8_t json_deflate_type_is_type_constructor(const json_deflate_schema_type_t * const type) {
    return json_deflate_flags_get_value(&type->flags, 0x20);
}

void json_deflate_type_set_non_exhaustive(json_deflate_schema_type_t * const type, const uint8_t val) {
    json_deflate_flags_set_value(&type->flags, val, 0x10);
}

uint8_t json_deflate_type_is_non_exhaustive(const json_deflate_schema_type_t * const type) {
    return json_deflate_flags_get_value(&type->flags, 0x10);
}

void json_deflate_type_set_collisions(json_deflate_schema_type_t * const type, const uint8_t val) {
    json_deflate_flags_set_value(&type->flags, val, 0x08);
}

uint8_t json_deflate_type_get_collisions(const json_deflate_schema_type_t * const type) {
    return json_deflate_flags_get_value(&type->flags, 0x08);
}

void json_deflate_type_set_class(json_deflate_schema_type_t * const type, const json_deflate_schema_type_class_t val) {
    json_deflate_flags_set_value(&type->flags, val, 0x03);
}

json_deflate_schema_type_class_t json_deflate_type_get_class(const json_deflate_schema_type_t * const type) {
    return (json_deflate_schema_type_class_t)json_deflate_flags_get_value(&type->flags, 0x03);
}

uint8_t json_deflate_field_is_required(const json_deflate_schema_field_t * const field) {
    return json_deflate_flags_get_value(&field->flags, 0x80);
}

void json_deflate_field_set_required(json_deflate_schema_field_t * const field, const uint8_t val) {
    json_deflate_flags_set_value(&field->flags, val, 0x80);
}

uint8_t json_deflate_field_has_activation_value(const json_deflate_schema_field_t * const field) {
    return json_deflate_flags_get_value(&field->flags, 0x40);
}

void json_deflate_field_set_activation_value(json_deflate_schema_field_t * const field, const int val, const uint8_t choice_value) {
    json_deflate_flags_set_value(&field->flags, (uint8_t)val, 0x40);
    if (val) {
        field->choice_value = choice_value;
    }
}

uint8_t json_deflate_field_can_fail(const json_deflate_schema_field_t * const field) {
    return json_deflate_flags_get_value(&field->flags, 0x20);
}

void json_deflate_field_set_can_fail(json_deflate_schema_field_t * const field, const int val) {
    json_deflate_flags_set_value(&field->flags, (uint8_t)val, 0x20);
}

int json_deflate_find_by_hash(const void * const key, const void * const field) {
    const uint32_t * const t_key = key;
    const json_deflate_schema_field_t * const t_field = field;
    const int64_t result = (int64_t)*t_key - (int64_t)t_field->hash;
    return SIGN(result);
}

static json_deflate_schema_field_t * lookup_field_binary_resolve_collisions(const json_deflate_bump_area_t * const area, json_deflate_schema_field_t * const fields, const int64_t field_count, const char * const name, const uint32_t hash) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    json_deflate_schema_field_t * field = bsearch(&hash, fields, field_count, sizeof(json_deflate_schema_field_t), json_deflate_find_by_hash);
    if (!field) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    ptrdiff_t index = field - fields;
    bool no_collisions_left = index == 0 || fields[index - 1].hash != hash;
    bool no_collisions_right = index == field_count - 1 || fields[index + 1].hash != hash;
    if (no_collisions_left && no_collisions_right) {
        const char * const field_name = json_deflate_bump_get_ptr(area, field->json_name);
        if (!strcmp(name, field_name)) {
            JSON_DEFLATE_TRACE_POP();
            return field;
        } else {
            JSON_DEFLATE_TRACE_POP();
            return NULL;
        }
    }

    int32_t left_index = (int32_t)index;
    while (left_index >= 0 && fields[left_index].hash == hash) {
        const char * const field_name = json_deflate_bump_get_ptr(area, fields[left_index].json_name);
        if (!strcmp(field_name, name)) {
            JSON_DEFLATE_TRACE_POP();
            return &fields[left_index];
        }

        left_index--;
    }

    int32_t right_index = (int32_t)index;
    while (right_index < field_count && fields[right_index].hash == hash) {
        const char * const field_name = json_deflate_bump_get_ptr(area, fields[right_index].json_name);
        if (!strcmp(field_name, name)) {
            JSON_DEFLATE_TRACE_POP();
            return &fields[right_index];
        }

        right_index++;
    }

    JSON_DEFLATE_TRACE_POP();
    return NULL;
}

json_deflate_schema_field_t * json_deflate_type_lookup_field(const json_deflate_bump_area_t * const area, const json_deflate_schema_type_t * const ty, const char * const name) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    const uint32_t hash = json_deflate_schema_calculate_field_name_hash(name);
    json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
    if (json_deflate_type_get_collisions(ty) || ty->field_count < 2) {
        json_deflate_schema_field_t * ret = lookup_field_binary_resolve_collisions(area, fields, ty->field_count, name, hash);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    } else {
        json_deflate_schema_field_t * const found = bsearch(&hash, fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_find_by_hash);
        if (found) {
            ASSERT_MSG(!strcmp(json_deflate_bump_get_ptr(area, found->json_name), name), "[json_deflate] Field lookup inconsistency (searched for \"%s\" but lookup returned \"%s\")", name, json_deflate_bump_get_ptr(area, found->json_name));
        }
        JSON_DEFLATE_TRACE_POP();
        return found;
    }
}

cJSON * json_deflate_data_navigate_to(cJSON * const json, const char * const path) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    if (!path) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    char temp[json_deflate_max_string_length] = {0};
    for (int i = 0; path[i] && path[i + 1] && path[i + 1] != '/'; i++) {
        temp[i] = path[i + 1];
    }

    if (!temp[0]) {
        JSON_DEFLATE_TRACE_POP();
        return json;
    }

    cJSON * const child = cJSON_GetObjectItem(json, temp);
    if (!child) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    const char * const child_path = path + strlen(temp) + 1;
    cJSON * node = json_deflate_data_navigate_to(child, child_path);
    JSON_DEFLATE_TRACE_POP();
    return node;
}
