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
#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"

#define SIGN(_x) (_x > 0 ? 1 : (_x < 0 ? -1 : 0))

void json_deflate_type_init(json_deflate_schema_type_t * const type) {
    ZEROMEM(type);
}

void json_deflate_field_init(json_deflate_schema_field_t * const field) {
    ZEROMEM(field);
}

int json_deflate_sort_key_by_offset(const void * const a, const void * const b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    int64_t result = (int64_t)t_a->offset - (int64_t)t_b->offset;
    return SIGN(result);
}

int json_deflate_sort_key_by_original_order(const void * const a, const void * const b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    return t_a->original_order - t_b->original_order;
}

int json_deflate_sort_key_by_offset_then_by_original_order(const void * const a, const void * b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    int64_t result = (int64_t)t_a->offset - (int64_t)t_b->offset;
    if (result) {
        return SIGN(result);
    } else {
        int64_t alt_result = (int64_t)t_a->original_order - (int64_t)t_b->original_order;
        return SIGN(alt_result);
    }
}

int json_deflate_sort_key_by_choice_value(const void * const a, const void * const b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    return t_a->choice_value - t_b->choice_value;
}

int json_deflate_sort_key_by_hash(const void * const a, const void * const b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    const int64_t result = (int64_t)t_a->hash - (int64_t)t_b->hash;
    return SIGN(result);
}

int json_deflate_sort_key_by_reserved(const void * const a, const void * const b) {
    const json_deflate_schema_field_t * const t_a = a;
    const json_deflate_schema_field_t * const t_b = b;
    const int64_t result = (int64_t)t_b->reserved - (int64_t)t_a->reserved;
    return SIGN(result);
}

void json_deflate_verify_reordering(const json_deflate_schema_field_t * const fields, uint32_t count, int (*comparison)(const void * const a, const void * const b)) {
    for (uint64_t i = 0; i < count - 1; i++) {
        const json_deflate_schema_field_t * const first = &fields[i];
        const json_deflate_schema_field_t * const second = &fields[i + 1];
        const int result = comparison(first, second);
        if (result > 0) {
            TRAP("Fields not ordered as expected.");
        }
    }
}

void json_deflate_detect_collisions(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    json_deflate_type_set_collisions(ty, 0);
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        uint32_t previous_hash = 0;
        for (uint32_t i = 0; i < ty->field_count; i++) {
            json_deflate_schema_field_t * const field = &fields[i];
            if (field->hash == previous_hash) {
                json_deflate_type_set_collisions(ty, 1);
                break;
            } else {
                previous_hash = field->hash;
            }
        }
    }
}

void reorder_fields_by_hash(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_hash);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_hash);
    }
}

void reorder_fields_by_original_order(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_original_order);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_original_order);
    }
}

void reorder_fields_by_offset(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_offset);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_offset);
    }
}

void reorder_fields_by_offset_then_by_original_order(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_offset_then_by_original_order);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_offset_then_by_original_order);
    }
}

void reorder_fields_by_choice_value(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_choice_value);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_choice_value);
    }
}

void reorder_fields_by_size(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const ty) {
    if (ty->fields) {
        json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, ty->fields);
        for (uint64_t i = 0; i < ty->field_count; i++) {
            json_deflate_schema_field_t * const field = &fields[i];
            const json_deflate_schema_type_t * const type = json_deflate_bump_get_ptr(area, field->type);
            field->reserved = type->var;
        }
        qsort(fields, ty->field_count, sizeof(json_deflate_schema_field_t), json_deflate_sort_key_by_reserved);
        json_deflate_verify_reordering(fields, ty->field_count, json_deflate_sort_key_by_reserved);
    }
}

json_deflate_schema_type_t * json_deflate_type_construct(json_deflate_bump_area_t * const area, json_deflate_schema_type_t * const type_constructor, const json_deflate_schema_type_t * const type_arg) {
    if (!json_deflate_type_is_type_constructor(type_constructor)) {
        return type_constructor;
    }

    json_deflate_schema_type_t * const new_type = json_deflate_bump_alloc(area, sizeof(json_deflate_schema_type_t), sizeof(json_deflate_schema_type_t));
    memcpy(new_type, type_constructor, sizeof(json_deflate_schema_type_t));

    json_deflate_type_set_type_constructor(new_type, 0);
    new_type->type_ctor = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type_constructor);
    new_type->rel_type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type_arg);

    json_deflate_schema_field_t * const type_constructor_fields = json_deflate_bump_get_ptr(area, type_constructor->fields);
    json_deflate_schema_field_t * const new_type_fields = json_deflate_bump_alloc_array(area, new_type->field_count, sizeof(json_deflate_schema_field_t), sizeof(json_deflate_schema_field_t));
    new_type->fields = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, new_type_fields);

    for (uint32_t i = 0; i < new_type->field_count; i++) {
        json_deflate_schema_field_t * const type_constructor_field = &type_constructor_fields[i];
        json_deflate_schema_type_t * const type_constructor_field_type = json_deflate_bump_get_ptr(area, type_constructor_field->type);

        json_deflate_schema_field_t * const new_type_field = &new_type_fields[i];
        memcpy(new_type_field, type_constructor_field, sizeof(json_deflate_schema_field_t));

        if (type_constructor_field_type) {
            json_deflate_schema_type_t * const new_type_field_type = json_deflate_type_construct(area, type_constructor_field_type, type_arg);
            new_type_field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, new_type_field_type);
        } else {
            new_type_field->type = (json_deflate_bump_ptr_t)json_deflate_bump_get_offset(area, type_arg);
        }
    }

    return new_type;
}

static uint32_t generate_hash_rec(const json_deflate_bump_area_t * const area, const json_deflate_schema_type_t * const type) {
    const uint32_t this_bits = ((type->var << 0) ^ (type->align << 2) ^ (json_deflate_type_get_class(type) << 3)) << 4;

    uint32_t sub_bits = 0;

    const json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(area, type->fields);
    for (uint64_t i = 0; i < type->field_count; i++) {
        const json_deflate_schema_field_t * const field = &fields[i];
        const json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(area, field->type);
        const uint32_t field_bits = generate_hash_rec(area, field_type);
        const uint32_t mod = sizeof(field_bits);
        const uint32_t shifted_field_bits = (field_bits << (i % mod) | (field_bits >> (mod * 8 - i) % mod));

        sub_bits = sub_bits ^ shifted_field_bits;
    }

    return this_bits ^ sub_bits;
}

uint32_t json_deflate_generate_hash(const json_deflate_bump_area_t * const area, const json_deflate_schema_context_t * const wasm_ctx, const json_deflate_schema_context_t * const native_ctx, const uint32_t seed) {
    const json_deflate_schema_type_t * const wasm_schema = json_deflate_bump_get_ptr(area, wasm_ctx->ty_root);
    const json_deflate_schema_type_t * const native_schema = json_deflate_bump_get_ptr(area, native_ctx->ty_root);
    const uint32_t wasm_bits = generate_hash_rec(area, wasm_schema);
    const uint32_t native_bits = generate_hash_rec(area, native_schema);
    const uint32_t hash = seed ^ wasm_bits ^ native_bits;
    return hash;
}
