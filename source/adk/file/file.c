/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * file.c
 *
 * Interface to file operations
 */

#include "file.h"

#include "source/adk/steamboat/sb_thread.h"

#include <stdint.h>

static const char bundle_fp_map_node_tag_str[] = "BUNDLE_TAG_M5";

typedef struct bundle_file_map_node_t {
    char tag[ARRAY_SIZE(bundle_fp_map_node_tag_str)];
    struct bundle_file_map_node_t * next;
    struct bundle_file_map_node_t * prev;

    bundle_file_t * bundle_fp;
} bundle_file_map_node_t;

static struct {
    bundle_t * mounted_bundle; // mounted bundle, if any
    struct {
        bundle_file_map_node_t handles_buffer[max_open_bundle_files];
        bundle_file_map_node_t * active_head;
        bundle_file_map_node_t * active_tail;
        bundle_file_map_node_t * free_head;
        bundle_file_map_node_t * free_tail;
    } bundle_map;

    struct {
        sb_atomic_int32_t amount_recently_written;
        int32_t max_bytes_per_second;
        float bytes_to_drain;
    } rate_limiting;
} statics;

// Initializes bfs mappings free-list
static void init_bundle_map() {
    ZEROMEM(&statics.bundle_map);
    for (size_t ind = 0; ind < ARRAY_SIZE(statics.bundle_map.handles_buffer); ++ind) {
        LL_ADD(&statics.bundle_map.handles_buffer[ind], prev, next, statics.bundle_map.free_head, statics.bundle_map.free_tail);
        memcpy(statics.bundle_map.handles_buffer[ind].tag, bundle_fp_map_node_tag_str, ARRAY_SIZE(bundle_fp_map_node_tag_str));
    }
}

#if defined(_LEIA) || defined(_VADER)
static const char download0[] = "download0://";
#endif

static inline bool is_app_root_directory(const sb_file_directory_e directory, const char * const path) {
#if defined(_LEIA) || defined(_VADER)
    return (directory == sb_app_root_directory) && (strncmp(path, download0, sizeof(download0) - 1) != 0);
#else
    (void)path;
    return directory == sb_app_root_directory;
#endif
}

// Returns true if bfs is mounted, may only be mounted on 'sb_app_root_directory'.
static inline bool bundle_fs_is_mounted(const sb_file_directory_e directory, const char * const path) {
    return (statics.mounted_bundle != NULL) && is_app_root_directory(directory, path);
}

// Return true if bfs mapping free-list is not empty
static inline bool have_free_bundle_fp() {
    return !!statics.bundle_map.free_head;
}

static bundle_file_map_node_t * map_bundle_file(bundle_file_t * const bundle_fp) {
    VERIFY(have_free_bundle_fp());

    bundle_file_map_node_t * const node = statics.bundle_map.free_head;
    LL_REMOVE(node, prev, next, statics.bundle_map.free_head, statics.bundle_map.free_tail);
    LL_ADD(node, prev, next, statics.bundle_map.active_head, statics.bundle_map.active_tail);
    node->bundle_fp = bundle_fp;

    return node;
}

static void unmap_bundle_file(bundle_file_map_node_t * const node) {
    node->bundle_fp = NULL;
    LL_REMOVE(node, prev, next, statics.bundle_map.active_head, statics.bundle_map.active_tail);
    LL_ADD(node, prev, next, statics.bundle_map.free_head, statics.bundle_map.free_tail);
}

static inline bool is_bundle_mapped_file(const sb_file_t * const file) {
    bundle_file_map_node_t * const node = (bundle_file_map_node_t *)file;
    return statics.mounted_bundle && (memcmp(node->tag, bundle_fp_map_node_tag_str, ARRAY_SIZE(bundle_fp_map_node_tag_str)) == 0);
}

// If the sb_file_t * has been redirected to a bundle file, returns the bundle file ptr else returns NULL
static inline bundle_file_t * get_bundle_fp(const sb_file_t * const file) {
    bundle_file_map_node_t * const node = (bundle_file_map_node_t *)file;
    // check to make sure that we're probably dealing with a mapped file.. the likelihood that we're going to clash with our defined tag should be minimal.
    return is_bundle_mapped_file(file) ? node->bundle_fp : NULL;
}

// Mounts bundle on 'sb_app_root_directory'
bool adk_mount_bundle(bundle_t * const bundle) {
    VERIFY(bundle != NULL);
    if (statics.mounted_bundle) {
        return false; // a bundle is already mounted
    }
    init_bundle_map();
    statics.mounted_bundle = bundle;
    return true;
}

// Determines if given bundle is mounted
bool adk_is_mounted_bundle(bundle_t * const bundle) {
    return bundle == statics.mounted_bundle;
}

// Unmounts bundle on 'sb_app_root_directory'
bool adk_unmount_bundle(bundle_t * const bundle) {
    if (!statics.mounted_bundle) {
        return false; // no bundle mounted
    }
    if (bundle != statics.mounted_bundle) {
        return false; // this bundle not mounted
    }
    if (statics.bundle_map.active_head != NULL) {
        return false; // bundle files are open
    }
    statics.mounted_bundle = NULL;
    return true;
}

sb_file_t * adk_fopen(const sb_file_directory_e directory, const char * const path, const char * const mode) {
    if (bundle_fs_is_mounted(directory, path)) {
        if (strpbrk(mode, "wa+")) {
            return NULL; // bundle fs is read-only
        }
        // Check for required translation mode just to keep the API consistent -- zip_fopen() doesn't actually support
        // text mode, so bundled text files should use posix eol
        if (!strpbrk(mode, "bt")) {
            return NULL; // missing required eol translation mode
        }
        if (!have_free_bundle_fp()) {
            return NULL; // too many open bundle files
        }
        bundle_file_t * const bfp = bundle_fopen(statics.mounted_bundle, path);
        if (!bfp) {
            return NULL; // not found or open error
        }
        return (sb_file_t *)map_bundle_file(bfp);
    } else {
        return sb_fopen(directory, path, mode);
    }
}

bool adk_fclose(sb_file_t * const file) {
    if (!file) {
        return false;
    }
    if (is_bundle_mapped_file(file)) {
        bundle_file_map_node_t * const node = (bundle_file_map_node_t *)file;
        const bool close_status = bundle_fclose(node->bundle_fp);
        unmap_bundle_file(node);
        return close_status;
    } else {
        return sb_fclose(file);
    }
}

bool adk_delete_file(const sb_file_directory_e directory, const char * const filename) {
    return !is_app_root_directory(directory, filename) && sb_delete_file(directory, filename);
}

sb_directory_delete_error_e adk_delete_directory(const sb_file_directory_e directory, const char * const subpath) {
    return is_app_root_directory(directory, subpath) ? sb_directory_delete_invalid_input : sb_delete_directory(directory, subpath);
}

bool adk_create_directory_path(const sb_file_directory_e directory, const char * const input_path) {
    return !is_app_root_directory(directory, input_path) && sb_create_directory_path(directory, input_path);
}

sb_stat_result_t adk_stat(const sb_file_directory_e directory, const char * const subpath) {
    return bundle_fs_is_mounted(directory, subpath) ? bundle_stat(statics.mounted_bundle, subpath) : sb_stat(directory, subpath);
}

size_t adk_fread(
    void * const buffer,
    const size_t elem_size,
    const size_t elem_count,
    sb_file_t * const file) {
    bundle_file_t * const bfp = get_bundle_fp(file);
    return bfp ? bundle_fread(buffer, elem_size, elem_count, bfp) : sb_fread(buffer, elem_size, elem_count, file);
}

bool adk_feof(sb_file_t * const file) {
    bundle_file_t * const bfp = get_bundle_fp(file);
    return bfp ? bundle_feof(bfp) : sb_feof(file);
}

adk_fwrite_result_t adk_fwrite(const void * const buffer, const size_t elem_size, const size_t elem_count, sb_file_t * const file) {
    if (get_bundle_fp(file)) {
        return (adk_fwrite_result_t){.elements_written = 0, .error = adk_fwrite_error_read_only};
    }

    int32_t max_elements_can_write = (int32_t)elem_count;
    if (statics.rate_limiting.max_bytes_per_second > 0) {
        if (elem_size > (size_t)statics.rate_limiting.max_bytes_per_second) {
            return (adk_fwrite_result_t){.elements_written = 0, .error = adk_fwrite_error_element_too_large_for_rate_limit};
        }
        int32_t current_written_amount = 0;
        int32_t new_amount = 0;

        const int32_t i32_elem_size = (int32_t)elem_size;
        const int32_t i32_elem_count = (int32_t)elem_count;
        do {
            current_written_amount = sb_atomic_load(&statics.rate_limiting.amount_recently_written, memory_order_relaxed);
            ASSERT(current_written_amount <= statics.rate_limiting.max_bytes_per_second);
            max_elements_can_write = (statics.rate_limiting.max_bytes_per_second - current_written_amount) / i32_elem_size;
            if (max_elements_can_write == 0) {
                return (adk_fwrite_result_t){.elements_written = 0, .error = adk_fwrite_error_rate_limited};
            }
            new_amount = current_written_amount + min_int32_t(max_elements_can_write, i32_elem_count) * i32_elem_size;
            ASSERT(new_amount <= statics.rate_limiting.max_bytes_per_second);
        } while (sb_atomic_cas(&statics.rate_limiting.amount_recently_written, new_amount, current_written_amount, memory_order_relaxed) != current_written_amount);
    }
    return (adk_fwrite_result_t){.elements_written = (uint32_t)sb_fwrite(buffer, elem_size, (size_t)max_elements_can_write, file), .error = adk_fwrite_success};
}

void adk_file_set_write_limit(const int32_t persistent_storage_max_write_bytes_per_second) {
    ASSERT_IS_MAIN_THREAD();
    statics.rate_limiting.max_bytes_per_second = persistent_storage_max_write_bytes_per_second;
}

void adk_file_write_limit_drain(const int32_t bytes_to_drain) {
    ASSERT_IS_MAIN_THREAD();
    ASSERT(bytes_to_drain > 0);
    if (statics.rate_limiting.max_bytes_per_second <= 0) {
        return;
    }

    int32_t current_written_amount = 0;
    int32_t new_amount = 0;
    do {
        current_written_amount = sb_atomic_load(&statics.rate_limiting.amount_recently_written, memory_order_relaxed);
        new_amount = 0;
        if (current_written_amount >= bytes_to_drain) {
            new_amount = current_written_amount - bytes_to_drain;
        }
    } while (sb_atomic_cas(&statics.rate_limiting.amount_recently_written, new_amount, current_written_amount, memory_order_relaxed) != current_written_amount);
}

int get_file_size(sb_file_t * const file) {
    sb_fseek(file, 0, sb_seek_end);
    const long size = sb_ftell(file);
    sb_fseek(file, 0, sb_seek_set);
    return (int)size;
}

int get_artifact_size(const sb_file_directory_e directory, const char * const filename) {
    sb_file_t * const file = sb_fopen(directory, filename, "rb");
    if (file) {
        const int size = get_file_size(file);
        sb_fclose(file);
        return size;
    }
    return 0;
}

bool load_artifact_data(
    const sb_file_directory_e directory,
    const mem_region_t buffer,
    const char * const filename,
    const int ofs) {
    sb_file_t * const fp = sb_fopen(directory, filename, "rb");

    if (fp) {
        if (ofs > 0) {
            if (sb_fseek(fp, ofs, sb_seek_set)) {
                sb_fclose(fp);
                return false;
            }
        } else if (ofs < 0) {
            if (sb_fseek(fp, -ofs, sb_seek_end)) {
                sb_fclose(fp);
                return false;
            }
        }
        const size_t r = sb_fread(buffer.ptr, buffer.size, 1, fp);
        sb_fclose(fp);
        return r != 0;
    }

    return false;
}
