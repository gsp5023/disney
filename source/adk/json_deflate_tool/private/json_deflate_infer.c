/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_infer.c

JSON schema inference.
*/

#include "source/adk/json_deflate_tool/private/json_deflate_tool.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

enum {
    json_deflate_tool_max_string_length = 2048,
    json_deflate_tool_max_mappings = 4096,
};

typedef struct json_deflate_infer_path_mapping_t {
    char key[json_deflate_tool_max_string_length];
    json_deflate_infer_path_node_t * value;
} json_deflate_infer_path_mapping_t;

typedef struct json_deflate_infer_path_map_t {
    json_deflate_infer_path_mapping_t mappings[json_deflate_tool_max_mappings];
    int count;
} json_deflate_infer_path_map_t;

static void add_path_mapping(json_deflate_infer_path_map_t * const map, const char * const key, json_deflate_infer_path_node_t * const value) {
    VERIFY(map->count < ARRAY_SIZE(map->mappings));

    strcpy_s(map->mappings[map->count].key, sizeof(map->mappings[map->count].key), key);
    map->mappings[map->count].value = value;
    map->count++;
}

static json_deflate_infer_path_node_t * path_node_add(json_deflate_infer_path_node_t * const parent, char * const value, cJSON * const json) {
    json_deflate_infer_path_node_t * const node = calloc(1, sizeof(json_deflate_infer_path_node_t));

    node->value = strdup(value);
    node->parent = parent;
    node->json = json;

    return node;
}

static void path_node_delete(json_deflate_infer_path_node_t * const node) {
    if (node) {
        free(node->value);
        free(node);
    }
}

static void get_relative_path_str(const json_deflate_infer_path_node_t * const reference, const json_deflate_infer_path_node_t * const node, char * path, size_t size) {
    strcpy_s(path, size, "");

    const json_deflate_infer_path_node_t * current = node;
    while (current != reference) {
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

static void get_absolute_path_str(void * const env, const json_deflate_infer_path_node_t * const node, char * path, size_t size) {
    get_relative_path_str(env, node, path, size);
}

static int path_matches(const char * const p1, const char * const p2) {
    char d1[json_deflate_tool_max_string_length] = {0};
    strcpy_s(d1, sizeof(d1), p1);

    char d2[json_deflate_tool_max_string_length] = {0};
    strcpy_s(d2, sizeof(d2), p2);

    char * tail_1 = NULL;
    char * tail_2 = NULL;

    char * head_1 = strtok_s(d1, "/", &tail_1);
    char * head_2 = strtok_s(d2, "/", &tail_2);

    while (head_1 && *head_1 && head_2 && *head_2) {
        if (!strcmp(head_1, "*") || !strcmp(head_2, "*") || !strcmp(head_1, head_2)) {
            head_1 = strtok_s(NULL, "/", &tail_1);
            head_2 = strtok_s(NULL, "/", &tail_2);
        } else {
            return 0;
        }
    }

    if ((head_1 && *head_1) || (head_2 && *head_2)) {
        return 0;
    }

    return 1;
}

static int path_matches_group(const json_deflate_schema_type_group_t * const group, const char * const path) {
    for (int i = 0; group->paths[i]; i++) {
        if (path_matches(group->paths[i], path)) {
            return 1;
        }
    }

    return 0;
}

static json_deflate_schema_type_group_t * path_matches_any(json_deflate_schema_type_group_t * const groups, const int group_count, const char * const path) {
    for (int i = 0; i < group_count; i++) {
        if (path_matches_group(&groups[i], path)) {
            return &groups[i];
        }
    }

    return NULL;
}

static const json_deflate_infer_path_node_t * get_lowest_common_ancestor(const json_deflate_infer_path_node_t * const n1, const json_deflate_infer_path_node_t * const n2) {
    if (!n1 || !n2) {
        return NULL;
    }

    if (n1 == n2) {
        return n1;
    }

    const json_deflate_infer_path_node_t * const left = get_lowest_common_ancestor(n1->parent, n2);
    if (left) {
        return left;
    }

    const json_deflate_infer_path_node_t * const right = get_lowest_common_ancestor(n1, n2->parent);
    if (right) {
        return right;
    }

    return NULL;
}

static cJSON * get_leader(void * const env, json_deflate_infer_path_node_t * const node, json_deflate_schema_type_group_t * const groups, const int group_count, cJSON * const schema, json_deflate_schema_type_group_t ** group_output) {
    char current_path[json_deflate_tool_max_string_length] = {0};
    get_absolute_path_str(env, node, current_path, sizeof(current_path));

    // Does this belong in a group?
    json_deflate_schema_type_group_t * const current_group = path_matches_any(groups, group_count, current_path);
    if (!current_group) {
        return NULL;
    }

    // It does!
    *group_output = current_group;

    // If there is no leader for this group, take the lead.
    if (!current_group->leader) {
        current_group->leader = node;
        return schema;
    }

    // If there is a leader, return it.
    return current_group->leader->json;
}

static cJSON * update_type(void * const env, cJSON * const json, const char * const ty) {
    cJSON * const existing = cJSON_GetObjectItem(json, "type");
    if (existing) {
        VERIFY(!strcmp(existing->valuestring, ty));
        return existing;
    }

    return cJSON_AddStringToObject(env, json, "type", ty);
}

static cJSON * update_object(void * const env, cJSON * const json, const char * const p) {
    cJSON * const existing = cJSON_GetObjectItem(json, p);
    if (existing) {
        return existing;
    }

    return cJSON_AddObjectToObject(env, json, p);
}

static cJSON * update_array(void * const env, cJSON * const json, const char * const p) {
    cJSON * const existing = cJSON_GetObjectItem(json, p);
    if (existing) {
        return existing;
    }

    return cJSON_AddObjectToObject(env, json, p);
}

static void update_item_in_string_array(void * const env, cJSON * const json, const char * const p) {
    cJSON * item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (!strcmp(item->valuestring, p)) {
            return;
        }
    }

    cJSON_AddItemToArray(json, cJSON_CreateString(env, p));
}

static void remove_item_from_string_array(void * const env, cJSON * const json, const char * const p) {
    int i = 0;
    cJSON * item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (!strcmp(item->valuestring, p)) {
            cJSON_DeleteItemFromArray(env, json, i);
            return;
        }

        i++;
    }
}

static cJSON * emit_bottom(void * const env) {
    cJSON * const empty = cJSON_CreateObject(env);
    cJSON_AddArrayToObject(env, empty, "type");
    return empty;
}

static cJSON * combine_objects(void * const env, const cJSON * const lnode, const cJSON * const rnode) {
    VERIFY_MSG(lnode || rnode, "[json_deflate_tool] L-node or R-node is required");

    if (!lnode) {
        return cJSON_Duplicate(env, rnode, true);
    }

    if (!rnode) {
        return cJSON_Duplicate(env, lnode, true);
    }

    cJSON * const combined = cJSON_CreateObject(env);

    bool lobj = false;
    bool robj = false;
    bool arr = false;
    bool map = false;
    bool number = false;

    cJSON * const ltype = cJSON_GetObjectItem(lnode, "type");
    cJSON * const rtype = cJSON_GetObjectItem(rnode, "type");
    cJSON * const ctype = cJSON_AddArrayToObject(env, combined, "type");

    if (cJSON_IsArray(ltype)) {
        cJSON * ty = NULL;
        cJSON_ArrayForEach(ty, ltype) {
            if (!strcmp("object", ty->valuestring)) {
                lobj = true;
            }
            if (!strcmp("array", ty->valuestring)) {
                arr = true;
            }
            if (!strcmp("number", ty->valuestring)) {
                number = true;
            }
            if (!strcmp("map", ty->valuestring)) {
                map = true;
            }

            update_item_in_string_array(env, ctype, ty->valuestring);
        }
    } else if (ltype) {
        if (!strcmp("object", ltype->valuestring)) {
            lobj = true;
        }
        if (!strcmp("array", ltype->valuestring)) {
            arr = true;
        }
        if (!strcmp("number", ltype->valuestring)) {
            number = true;
        }
        if (!strcmp("map", ltype->valuestring)) {
            map = true;
        }

        update_item_in_string_array(env, ctype, ltype->valuestring);
    }

    if (cJSON_IsArray(rtype)) {
        cJSON * ty = NULL;
        cJSON_ArrayForEach(ty, rtype) {
            if (!strcmp("object", ty->valuestring)) {
                robj = true;
            }
            if (!strcmp("array", ty->valuestring)) {
                arr = true;
            }
            if (!strcmp("number", ty->valuestring)) {
                number = true;
            }
            if (!strcmp("map", ty->valuestring)) {
                map = true;
            }

            update_item_in_string_array(env, ctype, ty->valuestring);
        }
    } else if (rtype) {
        if (!strcmp("object", rtype->valuestring)) {
            robj = true;
        }
        if (!strcmp("array", rtype->valuestring)) {
            arr = true;
        }
        if (!strcmp("number", rtype->valuestring)) {
            number = true;
        }
        if (!strcmp("map", rtype->valuestring)) {
            map = true;
        }

        update_item_in_string_array(env, ctype, rtype->valuestring);
    }

    if (number) {
        remove_item_from_string_array(env, ctype, "integer");
    }

    if (cJSON_GetArraySize(ctype) == 1) {
        cJSON * const child = cJSON_Duplicate(env, ctype->child, true);
        cJSON_ReplaceItemInObject(env, combined, "type", child);
    }

    if (lobj || robj) {
        cJSON * const lprops = cJSON_GetObjectItem(lnode, "properties");
        cJSON * const rprops = cJSON_GetObjectItem(rnode, "properties");
        cJSON * const cprops = cJSON_CreateObject(env);
        cJSON_AddItemToObject(env, combined, "properties", cprops);

        {
            cJSON * lprop = NULL;
            cJSON_ArrayForEach(lprop, lprops) {
                cJSON * const rprop = cJSON_GetObjectItem(rprops, lprop->string);
                cJSON * const cprop = combine_objects(env, lprop, rprop);
                cJSON_AddItemToObject(env, cprops, lprop->string, cprop);
            }
        }

        {
            cJSON * rprop = NULL;
            cJSON_ArrayForEach(rprop, rprops) {
                cJSON * const lprop = cJSON_GetObjectItem(lprops, rprop->string);
                if (!lprop) {
                    cJSON_AddItemToObject(env, cprops, rprop->string, cJSON_Duplicate(env, rprop, true));
                }
            }
        }

        cJSON * const lreqs = cJSON_GetObjectItem(lnode, "required");
        cJSON * const rreqs = cJSON_GetObjectItem(rnode, "required");
        cJSON * const creqs = cJSON_CreateArray(env);
        cJSON_AddItemToObject(env, combined, "required", creqs);

        if (lobj && robj) {
            cJSON * lreq = NULL;
            cJSON_ArrayForEach(lreq, lreqs) {
                cJSON * rreq = NULL;
                cJSON_ArrayForEach(rreq, rreqs) {
                    if (!strcmp(lreq->valuestring, rreq->valuestring)) {
                        update_item_in_string_array(env, creqs, lreq->valuestring);
                    }
                }
            }
        } else if (lobj) {
            cJSON * lreq = NULL;
            cJSON_ArrayForEach(lreq, lreqs) {
                cJSON_AddItemToArray(creqs, cJSON_CreateString(env, lreq->valuestring));
            }
        } else if (robj) {
            cJSON * rreq = NULL;
            cJSON_ArrayForEach(rreq, rreqs) {
                cJSON_AddItemToArray(creqs, cJSON_CreateString(env, rreq->valuestring));
            }
        }
    }

    if (arr || map) {
        cJSON * const litems = cJSON_GetObjectItem(lnode, "items");
        cJSON * const ritems = cJSON_GetObjectItem(rnode, "items");
        cJSON * const citems = combine_objects(env, litems, ritems);
        cJSON_AddItemToObject(env, combined, "items", citems);
    }

    cJSON * const lname = cJSON_GetObjectItem(lnode, "name");
    cJSON * const rname = cJSON_GetObjectItem(rnode, "name");
    if (lname) {
        VERIFY_MSG(!rname, "[json_deflate_tool] Conflicting names: %s versus %s", lname->valuestring, rname->valuestring);
        cJSON_AddStringToObject(env, combined, "name", lname->valuestring);
    }

    return combined;
}

static bool any_type_inferred(const cJSON * const schema) {
    if (!schema) {
        return false;
    }

    const cJSON * const schema_type = cJSON_GetObjectItem(schema, "type");
    if (!schema_type) {
        return false;
    }

    if (!cJSON_IsArray(schema_type)) {
        return true;
    }

    if (cJSON_GetArraySize(schema_type) == 0) {
        return false;
    }

    return true;
}

cJSON * infer_rec(void * const env, const cJSON * const data) {
    cJSON * schema = cJSON_CreateObject(env);

    if (cJSON_IsBool(data)) {
        cJSON_AddStringToObject(env, schema, "type", "boolean");
    } else if (cJSON_IsNumber(data)) {
        if (data->decimalpoint) {
            cJSON_AddStringToObject(env, schema, "type", "number");
        } else {
            cJSON_AddStringToObject(env, schema, "type", "integer");
        }
    } else if (cJSON_IsString(data)) {
        cJSON_AddStringToObject(env, schema, "type", "string");
    } else if (cJSON_IsObject(data)) {
        cJSON_AddStringToObject(env, schema, "type", "object");
        cJSON * const properties = cJSON_AddObjectToObject(env, schema, "properties");
        cJSON * const required = cJSON_AddArrayToObject(env, schema, "required");
        const cJSON * item = NULL;
        cJSON_ArrayForEach(item, data) {
            cJSON * const child_schema = infer_rec(env, item);
            cJSON_AddItemToObject(env, properties, item->string, child_schema);
            if (any_type_inferred(child_schema)) {
                cJSON_AddItemToArray(required, cJSON_CreateString(env, item->string));
            }
        }
    } else if (cJSON_IsArray(data)) {
        cJSON_AddStringToObject(env, schema, "type", "array");

        const cJSON * item = NULL;
        cJSON_ArrayForEach(item, data) {
            cJSON * const child_schema = infer_rec(env, item);
            cJSON * const schema_items = cJSON_GetObjectItem(schema, "items");
            cJSON * const new_items = combine_objects(env, schema_items, child_schema);
            if (schema_items) {
                cJSON_ReplaceItemInObject(env, schema, "items", new_items);
            } else {
                cJSON_AddItemToObject(env, schema, "items", new_items);
            }
        }

        if (!cJSON_GetObjectItem(schema, "items")) {
            cJSON_AddItemToObject(env, schema, "items", emit_bottom(env));
        }
    }

    const cJSON * verify = NULL;
    cJSON_ArrayForEach(verify, schema) {
        int vtype = !strcmp("type", verify->string);
        int vprop = !strcmp("properties", verify->string);
        int vreq = !strcmp("required", verify->string);
        int vcatch = !strcmp("catch", verify->string);
        int vname = !strcmp("name", verify->string);
        int vitems = !strcmp("items", verify->string);
        VERIFY_MSG(vtype || vprop || vreq || vcatch || vname || vitems, "[json_deflate_tool] Created invalid schema");
    }

    return schema;
}

static void construct_all_paths(void * const env, cJSON * const data, json_deflate_infer_path_node_t * const current, json_deflate_infer_path_map_t * const map) {
    char path[json_deflate_tool_max_path_length] = {0};
    get_absolute_path_str(env, current, path, ARRAY_SIZE(path));

    add_path_mapping(map, path, current);

    cJSON * const properties = cJSON_GetObjectItem(data, "properties");
    cJSON * item = NULL;
    cJSON_ArrayForEach(item, properties) {
        json_deflate_infer_path_node_t * const child = path_node_add(current, item->string, item);
        construct_all_paths(env, item, child, map);
    }
}

static void destroy_all_paths(json_deflate_infer_path_map_t * const map) {
    for (int i = 0; i < map->count; i++) {
        path_node_delete(map->mappings[i].value);
    }
}

static json_deflate_infer_path_mapping_t * find_path(json_deflate_infer_path_map_t * const map, const char * const path) {
    for (int i = 0; i < map->count; i++) {
        if (!strcmp(map->mappings[i].key, path)) {
            return &map->mappings[i];
        }
    }

    return NULL;
}

static void convert_object_to_map(void * const env, cJSON * const schema, const char * const path) {
    cJSON * const type = cJSON_GetObjectItem(schema, "type");
    VERIFY_MSG(!strcmp(type->valuestring, "object"), "[json_deflate_tool] The node at %s has been inferred as a %s, not an \"object\"", path, type->valuestring);

    cJSON_ReplaceItemInObject(env, schema, "type", cJSON_CreateString(env, "map"));

    cJSON_DeleteItemFromObject(env, schema, "required");

    cJSON * const properties = cJSON_GetObjectItem(schema, "properties");
    cJSON * items = cJSON_CreateObject(env);
    cJSON * prop = NULL;
    cJSON_ArrayForEach(prop, properties) {
        items = combine_objects(env, items, prop);
    }

    if (!cJSON_GetObjectItem(items, "type")) {
        cJSON_AddItemToObject(env, items, "type", cJSON_CreateArray(env));
    }

    cJSON_AddItemToObject(env, schema, "items", items);

    cJSON_DeleteItemFromObject(env, schema, "properties");
}

static void convert_objects_to_maps_rec(void * const env, cJSON * const schema, const char * const * const maps, const int map_count, const char * const current_path) {
    cJSON * const items = cJSON_GetObjectItem(schema, "items");
    cJSON * const base = items ? items : schema;
    cJSON * const properties = cJSON_GetObjectItem(base, "properties");

    cJSON * prop = NULL;
    cJSON_ArrayForEach(prop, properties) {
        char next_path[json_deflate_tool_max_string_length] = {0};
        strcpy_s(next_path, sizeof(next_path), current_path);
        strcat_s(next_path, sizeof(next_path), "/");
        strcat_s(next_path, sizeof(next_path), prop->string);
        convert_objects_to_maps_rec(env, prop, maps, map_count, next_path);
    }

    for (int i = 0; i < map_count; i++) {
        const char * const map = maps[i];
        if (path_matches(map, current_path)) {
            convert_object_to_map(env, schema, current_path);
            break;
        }
    }
}

static void convert_objects_to_maps(void * const env, cJSON * const schema, const char * const * const maps, const int map_count) {
    if (map_count) {
        convert_objects_to_maps_rec(env, schema, maps, map_count, "");
    }
}

static void update_mappings(json_deflate_infer_path_map_t * const map, json_deflate_infer_path_mapping_t * const mapping, cJSON * const new_json) {
    mapping->value->json = new_json;

    cJSON * const properties = cJSON_GetObjectItem(new_json, "properties");
    for (int i = 0; i < map->count; i++) {
        json_deflate_infer_path_mapping_t * const current = &map->mappings[i];
        if (current->value->parent == mapping->value) {
            cJSON * const child = cJSON_GetObjectItem(properties, current->value->value);
            update_mappings(map, current, child);
        }
    }
}

static void unify(void * const env, cJSON * const schema, json_deflate_schema_type_group_t * const groups, const int group_count) {
    for (int i = 0; i < group_count; i++) {
        json_deflate_infer_path_map_t * const map = calloc(1, sizeof(json_deflate_infer_path_map_t));
        json_deflate_infer_path_node_t * const node = path_node_add(env, "", schema);
        construct_all_paths(env, schema, node, map);

        for (int j = 0; j < map->count; j++) {
            json_deflate_schema_type_group_t * const group = &groups[i];
            json_deflate_infer_path_mapping_t * const mapping = &map->mappings[j];
            if (path_matches_group(group, mapping->key)) {
                if (!group->leader) {
                    group->leader = mapping->value;

                    cJSON * const leader = group->leader->json;
                    cJSON_AddStringToObject(env, leader, "name", group->name);
                } else {
                    char leader_path[json_deflate_tool_max_path_length] = {0};
                    get_absolute_path_str(env, group->leader, leader_path, sizeof(leader_path));
                    cJSON * const follower_stub = cJSON_CreateObject(env);
                    cJSON_AddStringToObject(env, follower_stub, "type", leader_path);

                    cJSON * const leader = group->leader->json;
                    cJSON * const follower = mapping->value->json;
                    cJSON * const combination = combine_objects(env, leader, follower);

                    group->leader->json = combination;
                    json_deflate_infer_path_mapping_t * const leader_mapping = find_path(map, leader_path);

                    update_mappings(map, leader_mapping, combination);
                    update_mappings(map, mapping, combination);

                    char leader_string[json_deflate_tool_max_string_length] = {0};
                    strcpy_s(leader_string, sizeof(leader_string), leader->string);

                    char follower_string[json_deflate_tool_max_string_length] = {0};
                    strcpy_s(follower_string, sizeof(follower_string), follower->string);

                    cJSON * const leader_parent = group->leader->parent->json;
                    cJSON * const leader_parent_properties = cJSON_GetObjectItem(leader_parent, "properties");
                    cJSON_ReplaceItemInObject(env, leader_parent_properties, leader_string, combination);

                    cJSON * const follower_parent = mapping->value->parent->json;
                    cJSON * const follower_parent_properties = cJSON_GetObjectItem(follower_parent, "properties");
                    cJSON_ReplaceItemInObject(env, follower_parent_properties, follower_string, follower_stub);
                }
            }
        }

        destroy_all_paths(map);
        free(map);
    }
}

static void catch_required_rec(void * const env, cJSON * const schema) {
    const cJSON * const required = cJSON_GetObjectItem(schema, "required");

    if (required) {
        cJSON * const pop_name = cJSON_Duplicate(env, cJSON_GetObjectItem(schema, "name"), true);
        cJSON_DeleteItemFromObject(env, schema, "name");

        if (!cJSON_GetObjectItem(schema, "catch")) {
            cJSON_AddArrayToObject(env, schema, "catch");
        }

        cJSON * const catches = cJSON_GetObjectItem(schema, "catch");
        cJSON * req = NULL;
        cJSON_ArrayForEach(req, required) {
            update_item_in_string_array(env, catches, req->valuestring);
        }

        cJSON_AddItemToObject(env, schema, "name", pop_name);
    }

    const cJSON * const properties = cJSON_GetObjectItem(schema, "properties");
    cJSON * prop = NULL;
    cJSON_ArrayForEach(prop, properties) {
        catch_required_rec(env, prop);
    }

    const cJSON * const items = cJSON_GetObjectItem(schema, "items");
    if (items) {
        catch_required_rec(env, schema);
    }
}

cJSON * json_deflate_infer_json_schema(void * const env, const cJSON * const data, json_deflate_schema_type_group_t * const groups, const int group_count, const char * const * const maps, const int map_count, const json_deflate_infer_options_t * const options) {
    cJSON * const schema = infer_rec(env, data);

    convert_objects_to_maps(env, schema, maps, map_count);

    unify(env, schema, groups, group_count);

    if (options->catch_required) {
        catch_required_rec(env, schema);
    }

    return schema;
}
