/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
bundle.c

bundle file system for accessing compressed application bundles
 */

#include "bundle.h"

#include "source/adk/bundle/private/bundle_zip_alloc.h"
#include "source/adk/bundle/private/bundle_zip_source.h"
#include "source/adk/file/file.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"

#define TAG_BUNDLE FOURCC('B', 'N', 'D', 'L')

enum {
    bundle_heap_size = 1024 * 1024, // heap size related to max_open_bundle_files
};

// File-scope variables
static struct {
    int num_inits;
    heap_t heap;
    mem_region_t pages;
} statics;

static const char c_bundle_heap_tag[] = "BFS";

void bundle_init() {
    VERIFY(!statics.heap.internal.init); // must not be initialized

    // Create bundle heap, primarily for libzip.  Note that bundle subsys memory isn't currently pre-allocated by
    // app_init_subsystems() because we can't (currently) init the subsystems before the wasm is loaded.
    statics.pages = sb_map_pages(PAGE_ALIGN_INT(bundle_heap_size), system_page_protect_read_write);
    TRAP_OUT_OF_MEMORY(statics.pages.ptr);
    heap_init_with_region(&statics.heap, statics.pages, 8, 0, c_bundle_heap_tag);

    VERIFY(bundle_zip_set_heap(&statics.heap) == NULL);
}

void bundle_shutdown() {
    VERIFY(statics.heap.internal.init); // must be initialized
    VERIFY(bundle_zip_set_heap(NULL) == &statics.heap);
    heap_destroy(&statics.heap, c_bundle_heap_tag);
    sb_unmap_pages(statics.pages);
}

bundle_t * bundle_open(const sb_file_directory_e directory, const char * const path) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t * zs = bundle_zip_source_new(directory, path, &error);
    if (zs) {
        zip_t * zip = zip_open_from_source(zs, ZIP_RDONLY | ZIP_CHECKCONS, &error);
        if (zip) {
            return (bundle_t *)zip; // success
        }
        zip_source_close(zs);
    }
    return NULL;
}

/// Opens a bundle on an existing file for reading
bundle_t * bundle_open_fp(sb_file_t * const file, const size_t initial_offset) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t * const zs = bundle_zip_source_new_from_fp(file, initial_offset, &error);
    if (zs) {
        zip_t * const zip = zip_open_from_source(zs, ZIP_RDONLY | ZIP_CHECKCONS, &error);
        if (zip) {
            return (bundle_t *)zip; // success
        }
        zip_source_close(zs);
    }
    return NULL;
}

bool bundle_close(bundle_t * const bundle) {
    if (adk_is_mounted_bundle(bundle)) {
        LOG_WARN(TAG_BUNDLE, "Attempting to close mounted bundle!");
        return false; // don't close mounted bundle
    }
    zip_close((zip_t *)bundle); // accepts NULL
    return true;
}

bundle_file_t * bundle_fopen(bundle_t * const bundle, const char * const subpath) {
    return (bundle_file_t *)zip_fopen((zip_t *)bundle, subpath, 0); // accepts NULL
}

bool bundle_fclose(bundle_file_t * const file) {
    return file && (zip_fclose((zip_file_t *)file) == 0);
}

sb_stat_result_t bundle_stat(bundle_t * const bundle, const char * const subpath) {
    zip_stat_t zs;
    int res = zip_stat((zip_t *)bundle, subpath, ZIP_FL_NOCASE, &zs);
    sb_stat_result_t bs = {0};
    if (res < 0) {
        const int ze = zip_error_code_zip(zip_get_error((zip_t *)bundle));
        bs.error = (ze == ZIP_ER_NOENT) ? sb_stat_error_no_entry : sb_stat_error_unknown;
        bs.stat.mode = sb_file_mode_none;
    } else {
        if ((zs.valid & ZIP_STAT_SIZE) == ZIP_STAT_SIZE) {
            bs.stat.size = zs.size;
        }
        if ((zs.valid & ZIP_STAT_MTIME) == ZIP_STAT_MTIME) {
            bs.stat.modification_time_s = zs.mtime;
        }
    }
    return bs;
}

size_t bundle_fread(void * buffer, const size_t elem_size, const size_t elem_count, bundle_file_t * const file) {
    zip_int64_t rc = zip_fread((zip_file_t *)file, buffer, elem_size * elem_count);
    return rc < 0 ? 0 : rc / elem_size;
}

bool bundle_feof(bundle_file_t * const file) {
    zip_error_t * const ze = zip_file_get_error((zip_file_t *)file);
    return zip_error_code_zip(ze) == ZIP_ER_OK; // eof if no error
}
