/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  cg_gzip.c

  support for gzipped files
  */
#include "cg_gzip.h"

#include "extern/zlib/zlib.h"
#include "source/adk/canvas/cg.h"
#include "source/adk/imagelib/imagelib.h"

typedef PACK(struct gzip_header_t {
    uint16_t magic;
    char compression;
    char flags;
    uint32_t timestamp;
    char xfl;
    char os;
}) gzip_header_t;

STATIC_ASSERT(sizeof(gzip_header_t) == 10);

typedef enum gzip_header_magic_code_e {
    gzip_header_magic_code = 0x8b1f,
} gzip_header_magic_code_e;

typedef struct gzip_alloc_user_t {
    cg_allocation_t allocation;
    size_t amount_used;
} gzip_alloc_user_t;

static void * gzip_alloc(void * const void_user, const uint32_t count, const uint32_t size) {
    gzip_alloc_user_t * const user = void_user;
    user->amount_used += count * size;
    VERIFY(user->amount_used <= user->allocation.region.size);
    return user->allocation.region.ptr;
}

static void gzip_free(void * const void_user, void * const ptr) {
    gzip_alloc_user_t * const user = void_user;
    VERIFY_MSG(user->amount_used, "Gzip tried to free 2 allocations whenever we expected only a single free from it.");
    cg_free_alloc(user->allocation, MALLOC_TAG);
    ZEROMEM(user);
}

inflate_gzip_from_memory_errors_e inflate_gzip_from_memory(cg_heap_t * resource_heap, const cg_context_t * context, const mem_region_t in_bytes, cg_allocation_t * const out_allocation) {
    int ret;
    z_stream strm;
    *out_allocation = (cg_allocation_t){0};
    const gzip_header_t header = *(const gzip_header_t *)in_bytes.consted.ptr;
    if (header.magic != gzip_header_magic_code) {
        return inflate_gzip_from_memory_invalid_header;
    }

    // uncompressed size stored in last 4 bytes of file
    const uint32_t out_file_size = *(uint32_t *)(in_bytes.adr + in_bytes.size - 4);
    gzip_alloc_user_t gzip_alloc_user = {.allocation = cg_unchecked_alloc(resource_heap, context->config.gzip_limits.working_space, MALLOC_TAG)};
    if (!gzip_alloc_user.allocation.region.ptr) {
        return inflate_gzip_from_memory_out_of_memory;
    }

    *out_allocation = cg_unchecked_alloc(resource_heap, out_file_size, MALLOC_TAG);
    if (!out_allocation->region.ptr) {
        cg_free_alloc(gzip_alloc_user.allocation, MALLOC_TAG);
        return inflate_gzip_from_memory_out_of_memory;
    }
    strm.zalloc = gzip_alloc;
    strm.zfree = gzip_free;
    strm.opaque = &gzip_alloc_user;

    ret = inflateInit2(&strm, 32);
    if (ret != Z_OK) {
        (void)inflateEnd(&strm);
        cg_free_alloc(*out_allocation, MALLOC_TAG);
        return inflate_gzip_from_memory_decode_error;
    }

    // data to decompress is in strm.next_in
    strm.avail_in = (uInt)in_bytes.size;
    strm.next_in = (Bytef *)in_bytes.consted.ptr;
    strm.avail_out = (uInt)out_file_size;
    strm.next_out = (Bytef *)out_allocation->region.ptr;

    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret == Z_STREAM_ERROR) {
        (void)inflateEnd(&strm);
        cg_free_alloc(*out_allocation, MALLOC_TAG);
        return inflate_gzip_from_memory_decode_error;
    }

    switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
            // fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            cg_free_alloc(*out_allocation, MALLOC_TAG);
            return inflate_gzip_from_memory_decode_error;
    }

    (void)inflateEnd(&strm);
    return inflate_gzip_from_memory_success;
}
