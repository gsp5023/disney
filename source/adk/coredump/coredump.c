/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
adk_coredump.h

core dump data collection
*/

#include "coredump.h"

#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef _RESTRICTED
#include "source/adk/steamboat/restricted/sb_coredump.h"
#endif

enum {
    manifest_heap_size = 512 * 1024,
};

static struct {
    heap_t heap;
    mem_region_t pages;
    sb_mutex_t * mutex;
    adk_coredump_data_t * data;
    bool init;
} statics;

static const char coredump_tag[] = "coredump_tag";

static void alloc_value(const char ** value, const char ** dest) {
    char * _value = (char *)heap_alloc(&statics.heap, strlen(*value) + 1, MALLOC_TAG);
    strcpy(_value, *value);
    *dest = _value;
}

static adk_coredump_data_t * allocate_data(const char * name, const char * value) {
    adk_coredump_data_t * current = (adk_coredump_data_t *)heap_alloc(&statics.heap, sizeof(adk_coredump_data_t), MALLOC_TAG);
    ZEROMEM(current);
    alloc_value(&name, &current->name);
    alloc_value(&value, &current->value);

    return current;
}

static void add_or_update_data(const char * name, const char * value) {
    adk_coredump_data_t * prev = NULL;
    adk_coredump_data_t * current = statics.data;

    if (current == NULL) {
        // Create head if it does nto exist
        statics.data = allocate_data(name, value);
        return;
    }

    do {
        if (strcmp(current->name, name) == 0) {
            // Update data value when name is the same
            heap_free(&statics.heap, (void *)current->value, MALLOC_TAG);
            alloc_value(&value, &current->value);
            return;
        }

        prev = current;
        current = current->next;
    } while (current != NULL);

    // If we are here the data is new, create and append to dataset
    prev->next = allocate_data(name, value);
}

/*
 *  adk_coredump_handler
 *  Must be called in callback registered in sb_coredump_init
 */
void adk_coredump_handler() {
    // TODO: add crash handle logic here.
}

void adk_coredump_init(int coredump_stack_size) {
    if (statics.init == false) {
        statics.init = true;
        statics.mutex = sb_create_mutex(MALLOC_TAG);
        TRAP_OUT_OF_MEMORY(statics.mutex);
        statics.pages = sb_map_pages(PAGE_ALIGN_INT(coredump_stack_size), system_page_protect_read_write);
        TRAP_OUT_OF_MEMORY(statics.pages.ptr);
        heap_init_with_region(&statics.heap, statics.pages, 8, 0, coredump_tag);

#ifdef _RESTRICTED
        sb_coredump_init(coredump_stack_size);
#endif
    }
}

void adk_coredump_shutdown() {
    adk_coredump_data_t * prev = NULL;
    adk_coredump_data_t * current = statics.data;
    while (current != NULL) {
        prev = current;
        current = current->next;
        heap_free(&statics.heap, (void *)prev->name, MALLOC_TAG);
        heap_free(&statics.heap, (void *)prev->value, MALLOC_TAG);
        heap_free(&statics.heap, (void *)prev, MALLOC_TAG);
    }

    if (statics.mutex) {
        sb_destroy_mutex(statics.mutex, MALLOC_TAG);
    }
    sb_unmap_pages(statics.pages);
    heap_destroy(&statics.heap, MALLOC_TAG);

#ifdef _RESTRICTED
    sb_coredump_shutdown();
#endif

    statics.init = false;
}

void adk_coredump_add_data(const char * name, const char * value) {
#ifdef SB_COREDUMP_PRIVATE_DATA_ENABLED
    sb_lock_mutex(statics.mutex);
    add_or_update_data(name, value);
    sb_unlock_mutex(statics.mutex);
#else
    // Not adding data, platform does not have access to private data.
#endif
}

void adk_coredump_add_data_public(const char * name, const char * value) {
    sb_lock_mutex(statics.mutex);
    add_or_update_data(name, value);
    sb_unlock_mutex(statics.mutex);
}

void adk_coredump_get_data(adk_coredump_data_t ** data) {
    *data = statics.data;
}
