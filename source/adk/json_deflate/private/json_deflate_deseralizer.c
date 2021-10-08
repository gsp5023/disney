/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_parse.c

JSON deflate data parsing based on a given binary schema.
*/

#include "source/adk/interpreter/interp_api.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/log/log.h"

enum {
    json_deflate_max_error_message_result_length = 1024,
};

#define NEW_CONTEXT_INFO(_parent, _str)      \
    (json_deflate_context_info_t) {          \
        .parent = _parent, .path_part = _str \
    }

static void write_path(json_deflate_context_info_t * const info, char * const out, const size_t out_size) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    if (info && info->parent) {
        write_path(info->parent, out, out_size);
    }

    strcat_s(out, out_size, "/");

    if (info) {
        strcat_s(out, out_size, info->path_part);
    }
    JSON_DEFLATE_TRACE_POP();
}

const char * const JSON_BOOLEAN = "boolean";
const char * const JSON_INTEGER = "integer";
const char * const JSON_NUMBER = "number";
const char * const JSON_STRING = "string";
const char * const JSON_OBJECT = "object";
const char * const JSON_MAP = "map (object)";
const char * const JSON_ARRAY = "array";
const char * const JSON_NULL = "null";
const char * const JSON_UNKNOWN = "unknown node";

static const char * get_node_type_string(const cJSON * const json) {
    if (cJSON_IsBool(json)) {
        return JSON_BOOLEAN;
    } else if (cJSON_IsNumber(json)) {
        if (json->decimalpoint) {
            return JSON_NUMBER;
        } else {
            return JSON_INTEGER;
        }
    } else if (cJSON_IsString(json)) {
        return JSON_STRING;
    } else if (cJSON_IsObject(json)) {
        return JSON_OBJECT;
    } else if (cJSON_IsArray(json)) {
        return JSON_ARRAY;
    } else if (cJSON_IsNull(json)) {
        return JSON_NULL;
    } else {
        return JSON_UNKNOWN;
    }
}

static char * write_expectation_error_message(json_deflate_bump_area_t * const area, json_deflate_context_info_t * const info, const char * const expectation, const cJSON * const json) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    char message[json_deflate_max_error_message_result_length] = {0};
    char path[json_deflate_max_error_message_result_length] = {0};
    write_path(info, path, sizeof(path));
    sprintf_s(message, sizeof(message), "Expected %s but found %s at %s", expectation, get_node_type_string(json), path);
    char * ret = json_deflate_bump_store_str(area, message);
    JSON_DEFLATE_TRACE_PUSH_FN();
    return ret;
}

static char * write_missing_field_error_message(json_deflate_bump_area_t * const area, json_deflate_context_info_t * const info, const char * const field_name, const cJSON * const json) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    char message[json_deflate_max_error_message_result_length] = {0};
    char path[json_deflate_max_error_message_result_length] = {0};
    write_path(info, path, sizeof(path));
    sprintf_s(message, sizeof(message), "Missing required field \"%s\" at %s", field_name, path);
    char * ret = json_deflate_bump_store_str(area, message);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

typedef struct parse_environment_t {
    json_deflate_bump_area_t * area;
} parse_environment_t;

static uint32_t wasm_ptr_to_offset_adapter(void * const addr) {
    return get_active_wasm_interpreter()->translate_ptr_native_to_wasm(addr).ofs;
}

uint32_t crc_str_32(const char * str);

static uint32_t hash_for_binary_map(const char * const str) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    uint32_t ret = crc_str_32(str);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

static int sort_by_value(const void * const s1, const void * const s2) {
    const uint32_t v1 = *(uint32_t *)s1;
    const uint32_t v2 = *(uint32_t *)s2;
    if (v1 < v2) {
        return -1;
    } else if (v1 > v2) {
        return 1;
    } else {
        return 0;
    }
}

static void json_deflate_set_variant_tag(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, json_deflate_schema_field_t * const choice, uint8_t * const dest) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    const json_deflate_schema_field_t * const tag_field = json_deflate_type_lookup_field(schema_area, type, "tag");
    uint8_t * const tag_address = dest + tag_field->offset;
    memcpy(tag_address, &choice->choice_value, sizeof(choice->choice_value));
    JSON_DEFLATE_TRACE_POP();
}

static void * json_deflate_process_variable(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, cJSON * const json, void * const override, uint8_t * const dest, json_deflate_context_info_t * const info, char ** error_msg);

static void * json_deflate_process_value(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, cJSON * const json, void * const override, uint8_t * const dest, json_deflate_context_info_t * const info, char ** error_msg) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    struct vectype_t {
        void * ptr;
        int32_t len;
    };

    union {
        void ** ptr;
        int32_t * i32;
        uint32_t * u32;
        double * f64;
        char * i8;
        char ** string;
        size_t * usize;
        struct vectype_t * vec;
    } override_val = {
        .ptr = override};

    union ptr_t {
        uint8_t * u8;
        uint16_t * u16;
        uint32_t * u32;
        uint64_t * u64;
    };

    const void * src = NULL;
    char valuechar;
    uint8_t valuebool;
    int32_t valueint;
    uint32_t valueuint;
    double valuedouble;
    void * valueptr;
    size_t valuesz;
    if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_integer)) {
        if (cJSON_IsNumber(json)) {
            valueint = (int32_t)json->valueint;
        } else if (!json && override) {
            valueint = *override_val.i32;
        } else {
            *error_msg = write_expectation_error_message(data_area, info, JSON_INTEGER, json);
            JSON_DEFLATE_TRACE_POP();
            return dest;
        }
        src = &valueint;
    } else if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_number)) {
        if (cJSON_IsNumber(json)) {
            valuedouble = json->valuedouble;
        } else if (!json && override) {
            valuedouble = *override_val.f64;
        } else {
            *error_msg = write_expectation_error_message(data_area, info, JSON_NUMBER, json);
            JSON_DEFLATE_TRACE_POP();
            return dest;
        }
        src = &valuedouble;
    } else if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_boolean)) {
        if (cJSON_IsBool(json)) {
            valuebool = json->valueint != 0;
        } else if (!json && override) {
            valuebool = *override_val.i8 != 0;
        } else {
            *error_msg = write_expectation_error_message(data_area, info, JSON_BOOLEAN, json);
            JSON_DEFLATE_TRACE_POP();
            return dest;
        }
        src = &valuebool;
    } else if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_ptr)) {
        if (target == json_deflate_parse_target_native) {
            valueptr = *override_val.ptr;
            src = &valueptr;
        } else if (target == json_deflate_parse_target_wasm) {
            ASSERT_MSG(type->size == sizeof(uint32_t), "[json_deflate] Wasm target may only declare 32-bit pointers");
            valueuint = wasm_ptr_to_offset_adapter(*override_val.ptr);
            src = &valueuint;
        } else {
            TRAP("[json_deflate] Unsupported deflate target");
        }
    } else if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_size)) {
        if (target == json_deflate_parse_target_native) {
            valuesz = *override_val.usize;
            src = &valuesz;
        } else if (target == json_deflate_parse_target_wasm) {
            ASSERT_MSG(type->size == sizeof(uint32_t), "[json_deflate] Wasm target may only declare 32-bit sizes");
            ASSERT_MSG(*override_val.usize <= UINT32_MAX, "[json_deflate] Cannot fit %llu in a 32-bit integer.", *override_val.usize);
            valueuint = (uint32_t)*override_val.usize;
            src = &valueuint;
        } else {
            TRAP("[json_deflate] Unsupported deflate target");
        }
    } else if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_char)) {
        if (cJSON_IsString(json)) {
            valuechar = *json->valuestring;
        } else {
            valuechar = *override_val.i8;
        }
        src = &valuechar;
    } else if (type->type_ctor == ctx->ty_result) {
        json_deflate_schema_field_t * const ok_field = json_deflate_type_lookup_field(schema_area, type, "Ok");
        json_deflate_set_variant_tag(data_area, schema_area, ctx, target, type, ok_field, dest);
        json_deflate_schema_type_t * const ok_type = json_deflate_bump_get_ptr(schema_area, ok_field->type);
        uint8_t * const ok_address = dest + ok_field->offset;
        char * inner_error_msg = NULL;
        if (!json_deflate_process_variable(data_area, schema_area, ctx, target, ok_type, json, NULL, ok_address, info, &inner_error_msg)) {
            JSON_DEFLATE_TRACE_POP();
            return NULL;
        }

        if (inner_error_msg) {
            json_deflate_schema_field_t * const err_field = json_deflate_type_lookup_field(schema_area, type, "Err");
            json_deflate_set_variant_tag(data_area, schema_area, ctx, target, type, err_field, dest);
            json_deflate_schema_type_t * const err_type = json_deflate_bump_get_ptr(schema_area, err_field->type);
            uint8_t * const err_address = dest + err_field->offset;
            if (!json_deflate_process_variable(data_area, schema_area, ctx, target, err_type, NULL, &inner_error_msg, err_address, info, NULL)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }
        }
    } else if (type->type_ctor == ctx->ty_option) {
        if (json && !cJSON_IsNull(json)) {
            json_deflate_schema_field_t * some_field = json_deflate_type_lookup_field(schema_area, type, "Some");
            json_deflate_set_variant_tag(data_area, schema_area, ctx, target, type, some_field, dest);
            json_deflate_schema_type_t * const some_type = json_deflate_bump_get_ptr(schema_area, some_field->type);
            uint8_t * some_address = dest + some_field->offset;
            if (!json_deflate_process_variable(data_area, schema_area, ctx, target, some_type, json, NULL, some_address, info, error_msg)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }
        } else {
            json_deflate_schema_field_t * none_field = json_deflate_type_lookup_field(schema_area, type, "None");
            json_deflate_set_variant_tag(data_area, schema_area, ctx, target, type, none_field, dest);
            json_deflate_schema_type_t * const none_type = json_deflate_bump_get_ptr(schema_area, none_field->type);
            uint8_t * none_address = dest + none_field->offset;
            if (!json_deflate_process_variable(data_area, schema_area, ctx, target, none_type, NULL, NULL, none_address, info, error_msg)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }
        }
    } else {
        if (json_deflate_type_get_class(type) == schema_type_struct) {
            if (json && !cJSON_IsObject(json)) {
                *error_msg = write_expectation_error_message(data_area, info, JSON_OBJECT, json);
                JSON_DEFLATE_TRACE_POP();
                return dest;
            }

            ASSERT_MSG(type->field_count < 16384, "[json_deflate] Field count limit exceeded.");
            uint8_t checkmarks[16384] = {0};

            json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(schema_area, type->fields);
            for (uint32_t i = 0; i < type->field_count; i++) {
                if (!json_deflate_field_is_required(&fields[i])) {
                    json_deflate_schema_type_t * const optional_field_type = json_deflate_bump_get_ptr(schema_area, fields[i].type);
                    if (optional_field_type->type_ctor == ctx->ty_option) {
                        uint8_t * const place = dest + fields[i].offset;
                        if (!json_deflate_process_value(data_area, schema_area, ctx, target, optional_field_type, NULL, NULL, place, info, error_msg)) {
                            JSON_DEFLATE_TRACE_POP();
                            return NULL;
                        }

                        checkmarks[i] = 1;
                    }
                }
            }
            cJSON * elem = NULL;
            cJSON_ArrayForEach(elem, json) {
                json_deflate_schema_field_t * const field = json_deflate_type_lookup_field(schema_area, type, elem->string);
                if (field) {
                    json_deflate_schema_type_t * const field_type = json_deflate_bump_get_ptr(schema_area, field->type);
                    const char * const field_name = json_deflate_bump_get_ptr(schema_area, field->json_name);

                    uint8_t * const place = dest + field->offset;

                    json_deflate_context_info_t info_new = NEW_CONTEXT_INFO(info, field_name);
                    if (!json_deflate_process_variable(data_area, schema_area, ctx, target, field_type, elem, NULL, place, &info_new, error_msg)) {
                        JSON_DEFLATE_TRACE_POP();
                        return NULL;
                    }

                    if (*error_msg) {
                        break;
                    }

                    checkmarks[field - fields] = 1;
                }
            }

            if (!*error_msg) {
                for (uint32_t i = 0; i < type->field_count; i++) {
                    if (!checkmarks[i]) {
                        json_deflate_schema_field_t * const field = &fields[i];
                        const char * field_name = json_deflate_bump_get_ptr(schema_area, field->json_name);
                        *error_msg = write_missing_field_error_message(data_area, info, field_name, json);
                        break;
                    }
                }
            }

        } else if (json_deflate_type_get_class(type) == schema_type_array) {
            bool is_override = !json && override;
            if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_string) && !cJSON_IsString(json) && !override) {
                *error_msg = write_expectation_error_message(data_area, info, JSON_STRING, json);
                JSON_DEFLATE_TRACE_POP();
                return dest;
            } else if (type->type_ctor == ctx->ty_array && !cJSON_IsArray(json) && !override) {
                *error_msg = write_expectation_error_message(data_area, info, JSON_ARRAY, json);
                JSON_DEFLATE_TRACE_POP();
                return dest;
            } else {
                bool is_vec_override = type->type_ctor == ctx->ty_array && is_override;

                size_t length;
                size_t extra;
                if (!is_vec_override) {
                    if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_string)) {
                        length = strlen(json ? cJSON_GetStringValue(json) : *override_val.string);
                        extra = 1;
                    } else {
                        length = cJSON_GetArraySize(json);
                        extra = 0;
                    }
                } else {
                    length = override_val.vec->len;
                    extra = 0;
                }

                json_deflate_schema_type_t * const elem_type = json_deflate_bump_get_ptr(schema_area, type->rel_type);
                uint8_t * array_start = NULL;

                if (!is_vec_override) {
                    array_start = json_deflate_bump_alloc(data_area, (length + extra) * elem_type->var, elem_type->align);
                } else {
                    array_start = override_val.vec->ptr;
                }

                if (!array_start) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }

                json_deflate_schema_field_t * const field_ptr = json_deflate_type_lookup_field(schema_area, type, "ptr");
                json_deflate_schema_field_t * const field_cap = json_deflate_type_lookup_field(schema_area, type, "cap");
                json_deflate_schema_field_t * const field_len = json_deflate_type_lookup_field(schema_area, type, "len");

                json_deflate_schema_type_t * const field_ptr_type = json_deflate_bump_get_ptr(schema_area, field_ptr->type);
                json_deflate_schema_type_t * const field_cap_type = json_deflate_bump_get_ptr(schema_area, field_cap->type);
                json_deflate_schema_type_t * const field_len_type = json_deflate_bump_get_ptr(schema_area, field_len->type);

                uint8_t * const field_ptr_place = dest + field_ptr->offset;
                uint8_t * const field_cap_place = dest + field_cap->offset;
                uint8_t * const field_len_place = dest + field_len->offset;

                if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_ptr_type, NULL, &array_start, field_ptr_place, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }

                if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_cap_type, NULL, &length, field_cap_place, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }

                if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_len_type, NULL, &length, field_len_place, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }

                if (type == json_deflate_bump_get_ptr(schema_area, ctx->ty_string)) {
                    char * str_source = NULL;
                    if (cJSON_IsString(json)) {
                        str_source = cJSON_GetStringValue(json);
                    } else if (override) {
                        str_source = *override_val.string;
                    } else {
                        *error_msg = write_expectation_error_message(data_area, info, JSON_STRING, json);
                        JSON_DEFLATE_TRACE_POP();
                        return dest;
                    }
                    const size_t str_length = (length + extra) * sizeof(char);
                    strcpy_s((char *)array_start, str_length, str_source);
                } else {
                    cJSON * elem = NULL;
                    int i = 0;
                    cJSON_ArrayForEach(elem, json) {
                        uint8_t * const place = array_start + elem_type->var * i;
                        char index_num[10] = {0};
                        sprintf_s(&index_num[0], sizeof(index_num), "<%d>", i);
                        i++;
                        json_deflate_context_info_t info_new = NEW_CONTEXT_INFO(info, &index_num[0]);
                        if (!json_deflate_process_variable(data_area, schema_area, ctx, target, elem_type, elem, NULL, place, &info_new, error_msg)) {
                            JSON_DEFLATE_TRACE_POP();
                            return NULL;
                        }
                    }
                }
            }
        } else if (json_deflate_type_get_class(type) == schema_type_map) {
            if (json && !cJSON_IsObject(json)) {
                *error_msg = write_expectation_error_message(data_area, info, JSON_MAP, json);
                JSON_DEFLATE_TRACE_POP();
                return dest;
            }

            json_deflate_schema_field_t * const field_keys = json_deflate_type_lookup_field(schema_area, type, "keys");
            json_deflate_schema_field_t * const field_values = json_deflate_type_lookup_field(schema_area, type, "values");
            json_deflate_schema_field_t * const field_strings = json_deflate_type_lookup_field(schema_area, type, "strings");

            json_deflate_schema_type_t * const field_keys_type = json_deflate_bump_get_ptr(schema_area, field_keys->type);
            json_deflate_schema_type_t * const field_values_type = json_deflate_bump_get_ptr(schema_area, field_values->type);
            json_deflate_schema_type_t * const field_strings_type = json_deflate_bump_get_ptr(schema_area, field_strings->type);

            json_deflate_schema_type_t * const key_type = json_deflate_bump_get_ptr(schema_area, ctx->ty_integer);
            json_deflate_schema_type_t * const elem_type = json_deflate_bump_get_ptr(schema_area, type->rel_type);
            json_deflate_schema_type_t * const string_type = json_deflate_bump_get_ptr(schema_area, ctx->ty_string);

            const int32_t elem_count = (int32_t)cJSON_GetArraySize(json);
            uint8_t * const array_start_keys = json_deflate_bump_alloc(data_area, elem_count * key_type->var, key_type->align);
            uint8_t * const array_start_values = json_deflate_bump_alloc(data_area, elem_count * elem_type->var, elem_type->align);
            uint8_t * const array_start_strings = json_deflate_bump_alloc(data_area, elem_count * string_type->var, string_type->align);

            if (!array_start_keys || !array_start_values || !array_start_strings) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }

            uint8_t * const field_keys_ptr = dest + field_keys->offset;
            uint8_t * const field_values_ptr = dest + field_values->offset;
            uint8_t * const field_strings_ptr = dest + field_strings->offset;

            struct vectype_t vec_keys = {0};
            vec_keys.ptr = array_start_keys;
            vec_keys.len = elem_count;
            if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_keys_type, NULL, &vec_keys, field_keys_ptr, info, error_msg)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }

            int32_t i = 0;
            cJSON * elem = NULL;
            cJSON_ArrayForEach(elem, json) {
                uint8_t * const place = array_start_keys + key_type->var * (i++);
                uint32_t hash = hash_for_binary_map(elem->string);
                if (!json_deflate_process_value(data_area, schema_area, ctx, target, key_type, NULL, &hash, place, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }
            }

            qsort(array_start_keys, elem_count, key_type->var, sort_by_value);

            struct vectype_t vec_values = {0};
            vec_values.ptr = array_start_values;
            vec_values.len = elem_count;
            if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_values_type, NULL, &vec_values, field_values_ptr, info, error_msg)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }

            struct vectype_t vec_strings = {0};
            vec_strings.ptr = array_start_strings;
            vec_strings.len = elem_count;
            if (!json_deflate_process_value(data_area, schema_area, ctx, target, field_strings_type, NULL, &vec_strings, field_strings_ptr, info, error_msg)) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }

            elem = NULL;
            cJSON_ArrayForEach(elem, json) {
                const uint32_t hash = hash_for_binary_map(elem->string);
                uint32_t * found_ptr = bsearch(&hash, array_start_keys, elem_count, key_type->var, sort_by_value);
                ASSERT_MSG(found_ptr, "[json_deflate] Internal consistency of map compromised");

                const uintptr_t found_offset = (uintptr_t)found_ptr - (uintptr_t)array_start_keys;
                const uint32_t initial_found_index = (uint32_t)(found_offset / key_type->var);

                uint32_t found_index = initial_found_index;
                union ptr_t string_place = {
                    .u8 = array_start_strings + string_type->var * found_index};
                union ptr_t key_place = {
                    .u8 = array_start_keys + key_type->var * found_index};

                // The choice of a uint64_t * here means different things in Wasm versus native,
                // but since we are peeking into the layout of a string, the check should return
                // true or false in exactly the same cases.
                while ((int32_t)found_index < elem_count && *key_place.u32 == hash && *string_place.u64) {
                    string_place.u8 = array_start_strings + string_type->var * (++found_index);
                    key_place.u8 = array_start_keys + key_type->var * found_index;
                }

                if ((int32_t)found_index >= elem_count || *key_place.u32 != hash) {
                    found_index = initial_found_index;
                    string_place.u8 = array_start_strings + string_type->var * found_index;
                    key_place.u8 = array_start_keys + key_type->var * found_index;
                    while ((int32_t)found_index >= 0 && *key_place.u32 == hash && *string_place.u64) {
                        string_place.u8 = array_start_strings + string_type->var * (--found_index);
                        key_place.u8 = array_start_keys + key_type->var * found_index;
                    }

                    ASSERT_MSG(((int32_t)found_index >= 0) && (*key_place.u32 == hash), "[json_deflate] Not enough space in the map");
                }

                if (!json_deflate_process_value(data_area, schema_area, ctx, target, string_type, NULL, &elem->string, string_place.u8, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }

                uint8_t * const elem_place = array_start_values + elem_type->var * found_index;
                json_deflate_context_info_t info_new = NEW_CONTEXT_INFO(info, elem->string);
                if (!json_deflate_process_variable(data_area, schema_area, ctx, target, elem_type, elem, NULL, elem_place, &info_new, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }
            }
        } else if (json_deflate_type_get_class(type) == schema_type_variant) {
            ASSERT_MSG(type->field_count >= 2, "[json_deflate] Enums without variants are not allowed");

            struct candidates_t {
                json_deflate_schema_field_t * boolean_variant;
                json_deflate_schema_field_t * integer_variant;
                json_deflate_schema_field_t * number_variant;
                json_deflate_schema_field_t * string_variant;
                json_deflate_schema_field_t * array_variant;
                json_deflate_schema_field_t * map_variant;
                json_deflate_schema_field_t * object_variant;
            } candidates = {0};

            json_deflate_schema_field_t * const fields = json_deflate_bump_get_ptr(schema_area, type->fields);
            for (uint32_t i = 0; i < type->field_count; i++) {
                json_deflate_schema_field_t * const choice = &fields[i];

                const char * const field_name = json_deflate_bump_get_ptr(schema_area, choice->json_name);
                if (!strcmp(field_name, "tag")) {
                    continue;
                }

                json_deflate_schema_type_t * const choice_type = json_deflate_bump_get_ptr(schema_area, choice->type);
                if (choice_type == json_deflate_bump_get_ptr(schema_area, ctx->ty_boolean)) {
                    candidates.boolean_variant = choice;
                } else if (choice_type == json_deflate_bump_get_ptr(schema_area, ctx->ty_integer)) {
                    candidates.integer_variant = choice;
                } else if (choice_type == json_deflate_bump_get_ptr(schema_area, ctx->ty_number)) {
                    candidates.number_variant = choice;
                } else if (choice_type == json_deflate_bump_get_ptr(schema_area, ctx->ty_string)) {
                    candidates.string_variant = choice;
                } else if (json_deflate_type_get_class(choice_type) == schema_type_array) {
                    candidates.array_variant = choice;
                } else if (json_deflate_type_get_class(choice_type) == schema_type_map) {
                    candidates.map_variant = choice;
                } else if (json_deflate_type_get_class(choice_type) == schema_type_struct && !json_deflate_type_is_builtin(choice_type)) {
                    candidates.object_variant = choice;
                }
            }

            json_deflate_schema_field_t * choice = NULL;
            if (cJSON_IsBool(json)) {
                choice = candidates.boolean_variant;
            } else if (cJSON_IsString(json)) {
                choice = candidates.string_variant;
            } else if (cJSON_IsArray(json)) {
                choice = candidates.array_variant;
            } else if (cJSON_IsObject(json)) {
                choice = candidates.map_variant ? candidates.map_variant : candidates.object_variant;
            } else if (cJSON_IsNumber(json)) {
                if (candidates.integer_variant && candidates.number_variant) {
                    choice = json->decimalpoint ? candidates.number_variant : candidates.integer_variant;
                } else {
                    choice = candidates.number_variant ? candidates.number_variant : candidates.integer_variant;
                }
            }

            if (choice) {
                json_deflate_schema_type_t * const choice_type = json_deflate_bump_get_ptr(schema_area, choice->type);
                uint8_t * const choice_address = dest + choice->offset;
                json_deflate_set_variant_tag(data_area, schema_area, ctx, target, type, choice, dest);
                if (!json_deflate_process_variable(data_area, schema_area, ctx, target, choice_type, json, NULL, choice_address, info, error_msg)) {
                    JSON_DEFLATE_TRACE_POP();
                    return NULL;
                }
            } else {
                const char * const type_name = json_deflate_bump_get_ptr(schema_area, type->name);

                char msg[1024] = {0};
                strcpy_s(msg, sizeof(msg), "Expected one of the choices of: ");
                strcat_s(msg, sizeof(msg), type_name);
                *error_msg = json_deflate_bump_store_str(data_area, msg);
            }
        }
    }

    if (src) {
        memcpy(dest, src, type->var);
    }

    JSON_DEFLATE_TRACE_POP();
    return dest;
}

static void * json_deflate_process_variable(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, cJSON * const json, void * const override, uint8_t * const dest, json_deflate_context_info_t * const info, char ** error_msg) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    if (json_deflate_type_is_stored_by_reference(type)) {
        uint8_t * const instance_address = json_deflate_bump_alloc(data_area, type->size, type->align);
        if (!instance_address) {
            JSON_DEFLATE_TRACE_POP();
            return NULL;
        }

        if (target == json_deflate_parse_target_native) {
            memcpy(dest, &instance_address, sizeof(instance_address));
        } else if (target == json_deflate_parse_target_wasm) {
            uint32_t placed_address = wasm_ptr_to_offset_adapter(instance_address);
            memcpy(dest, &placed_address, sizeof(placed_address));
        } else {
            TRAP("[json_deflate] Unsupported deflate target");
        }

        void * ret = json_deflate_process_value(data_area, schema_area, ctx, target, type, json, override, instance_address, info, error_msg);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    } else {
        void * ret = json_deflate_process_value(data_area, schema_area, ctx, target, type, json, override, dest, info, error_msg);
        JSON_DEFLATE_TRACE_POP();
        return ret;
    }
    JSON_DEFLATE_TRACE_POP();
}

void * json_deflate_process_document(json_deflate_bump_area_t * const data_area, json_deflate_bump_area_t * const schema_area, json_deflate_schema_context_t * const ctx, const json_deflate_parse_target_e target, json_deflate_schema_type_t * const type, cJSON * const json) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    uint8_t * const base = json_deflate_bump_alloc(data_area, type->var, type->align);
    if (!base) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    char * error_msg = NULL;
    if (!json_deflate_process_value(data_area, schema_area, ctx, target, type, json, NULL, base, NULL, &error_msg)) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    JSON_DEFLATE_TRACE_POP();
    return base;
}

#undef NEW_CONTEXT_INFO
