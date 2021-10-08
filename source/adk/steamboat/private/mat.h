/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 mat.h

 steamboat for MAT Analyzer
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MAT_ENABLED

#define MAT_REGISTER_MAT_GROUP(_group, _group_name, _group_parent)   \
    do {                                                             \
        if (!_group) {                                               \
            _group = mat_register_group(_group_name, _group_parent); \
        }                                                            \
    } while (false)

#define MAT_REGISTER_CORE_MAT_GROUP(_group, _group_name) MAT_REGISTER_MAT_GROUP(_group, _group_name, mat_get_default_group())
#define MAT_REGISTER_CPU_MAT_GROUP(_group, _group_name) MAT_REGISTER_MAT_GROUP(_group, _group_name, mat_get_cpu_group())
#define MAT_REGISTER_GPU_MAT_GROUP(_group, _group_name) MAT_REGISTER_MAT_GROUP(_group, _group_name, mat_get_gpu_group())
#define MAT_REGISTER_NVE_MAT_GROUP(_group, _group_name) MAT_REGISTER_MAT_GROUP(_group, _group_name, mat_get_nve_group())

#define MAT_ALLOC(_pointer, _mem_size, _padding, _group)  \
    do {                                                  \
        mat_alloc(_pointer, _mem_size, _padding, _group); \
    } while (false)

#define MAT_ALLOC_POOL(_name, _pointer, _size, _group, _pool)  \
    do {                                                       \
        mat_alloc_pool(_name, _pointer, _size, _group, _pool); \
    } while (false)

#define MAT_MAP_FLEXIBLE(_pointer, _size, _protection, _flags)  \
    do {                                                        \
        mat_map_flexible(_pointer, _size, _protection, _flags); \
    } while (false)

#define MAT_UNMAP_FLEXIBLE(_pointer, _size)  \
    do {                                     \
        mat_unmap_flexible(_pointer, _size); \
    } while (false)

#define MAT_FREE(_pointer)  \
    do {                    \
        mat_free(_pointer); \
    } while (false)

#define MAT_FREE_POOL(_pool)  \
    do {                      \
        mat_free_pool(_pool); \
    } while (false)

#define MAT_BOOK_MARK(_label, _description)  \
    do {                                     \
        mat_book_mark(_label, _description); \
    } while (false)

#define MAT_ERROR_BOOK_MARK(_description) MAT_BOOK_MARK("[ERROR]", _description)
#define MAT_WARNING_MARK(_description) MAT_BOOK_MARK("[WARNING]", _description)
#define MAT_INFO_MARK(_description) MAT_BOOK_MARK("[INFO]", _description)

#define MAT_TAG_VIRTUAL(_pointer, _mem_size, _tag)  \
    do {                                            \
        mat_tag_virtual(_pointer, _mem_size, _tag); \
    } while (false)
#define MAT_ALLOC_PHYSICAL(_pointer, _mem_size, _alignment, _mem_type)  \
    do {                                                                \
        mat_alloc_physical(_pointer, _mem_size, _alignment, _mem_type); \
    } while (false)
#define MAT_FREE_PHYSICAL(_pointer, _mem_size)  \
    do {                                        \
        mat_free_physical(_pointer, _mem_size); \
    } while (false)
#define MAT_MAP_DIRECT(_virtual_addr, _mem_size, _protection, _flags, _physical_addr, _alignment, _mem_type)  \
    do {                                                                                                      \
        mat_map_direct(_virtual_addr, _mem_size, _protection, _flags, _physical_addr, _alignment, _mem_type); \
    } while (false)
#define MAT_UNMAP_DIRECT(_pointer, _mem_size)  \
    do {                                       \
        mat_unmap_direct(_pointer, _mem_size); \
    } while (false)
#define MAT_ALLOC_MSPACE(_space, _name, _pointer, _mem_size, _flag, _group)  \
    do {                                                                     \
        mat_alloc_mspace(_space, _name, _pointer, _mem_size, _flag, _group); \
    } while (false)
#define MAT_FREE_MSPACE(_space)  \
    do {                         \
        mat_free_mspace(_space); \
    } while (false)
#else
#define MAT_VOID_STUB ((void)0)

#define MAT_REGISTER_MAT_GROUP(_group, _group_name, _group_parent) MAT_VOID_STUB

#define MAT_REGISTER_CORE_MAT_GROUP(_group, _group_name) MAT_VOID_STUB
#define MAT_REGISTER_CPU_MAT_GROUP(_group, _group_name) MAT_VOID_STUB
#define MAT_REGISTER_GPU_MAT_GROUP(_group, _group_name) MAT_VOID_STUB
#define MAT_REGISTER_NVE_MAT_GROUP(_group, _group_name) MAT_VOID_STUB

#define MAT_ALLOC(_pointer, _mem_size, _padding, _group) MAT_VOID_STUB
#define MAT_ALLOC_POOL(name, p, s, _group, _pool) MAT_VOID_STUB

#define MAT_FREE(_pointer) MAT_VOID_STUB
#define MAT_FREE_POOL(_pool) MAT_VOID_STUB
#define MAT_BOOK_MARK(_label, _description) MAT_VOID_STUB

#define MAT_ERROR_BOOK_MARK(_description) MAT_VOID_STUB
#define MAT_WARNING_MARK(_description) MAT_VOID_STUB
#define MAT_INFO_MARK(_description) MAT_VOID_STUB

#define MAT_TAG_VIRTUAL(_pointer, _mem_size, _tag) MAT_VOID_STUB
#define MAT_ALLOC_PHYSICAL(_pointer, _mem_size, _alignment, _mem_type) MAT_VOID_STUB
#define MAT_FREE_PHYSICAL(_pointer, _mem_size) MAT_VOID_STUB
#define MAT_MAP_DIRECT(_virtual_addr, _mem_size, _protection, _flags, _physical_addr, _alignment, _mem_type) MAT_VOID_STUB
#define MAT_UNMAP_DIRECT(_pointer, _mem_size) MAT_VOID_STUB
#define MAT_ALLOC_MSPACE(_space, _name, _pointer, _mem_size, _flag, _group) MAT_VOID_STUB
#define MAT_FREE_MSPACE(_space) MAT_VOID_STUB
#define MAT_MAP_FLEXIBLE(_pointer, _size, _protection, _flags) MAT_VOID_STUB
#define MAT_UNMAP_FLEXIBLE(_pointer, _size) MAT_VOID_STUB

#endif

typedef uint16_t mat_group_t;
typedef uint64_t mat_pool_t;

mat_group_t mat_register_group(const char * group_name, mat_group_t parent);
void mat_alloc(void * p, uint64_t size, uint32_t padding, mat_group_t group);
void mat_alloc_pool(const char * label, void * p, uint64_t size, mat_group_t group, mat_pool_t * pool);
void mat_free(void * p);
void mat_free_pool(mat_pool_t p);
void mat_init();
void mat_shut_down();

void mat_book_mark(const char * label, const char * description);
void mat_tag_virtual(const void * p, uint64_t size, const char * tag);
void mat_alloc_physical(uint64_t p, uint64_t size, uint64_t alignment, int32_t type);
void mat_free_physical(uint64_t p, uint64_t size);
void mat_map_direct(
    const void * virtualAddress,
    uint64_t size,
    int32_t protection,
    int32_t flags,
    uint64_t physicalAddress,
    uint64_t alignment,
    int32_t memoryType);

void mat_unmap_direct(const void * virtualAddress, uint64_t size);
void mat_alloc_mspace(void * mspace, const char * name, void * p, uint64_t size, unsigned flag, mat_group_t group);
void mat_free_mspace(void * mspace);

void mat_map_flexible(void * virtualAddress, uint64_t size, int32_t protection, int32_t flags);
void mat_unmap_flexible(void * virtualAddress, uint64_t size);

mat_group_t mat_get_root_group();
mat_group_t mat_get_default_group();
mat_group_t mat_get_cpu_group();
mat_group_t mat_get_gpu_group();
mat_group_t mat_get_nve_group();

#ifdef __cplusplus
}
#endif
