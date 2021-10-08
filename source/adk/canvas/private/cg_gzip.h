/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  cg_gzip.c

  support for gzipped files
  */

#include "source/adk/canvas/cg.h"
#include "source/adk/runtime/runtime.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum inflate_gzip_from_memory_errors_e {
    inflate_gzip_from_memory_decode_error = -3,
    inflate_gzip_from_memory_invalid_header = -2,
    inflate_gzip_from_memory_out_of_memory = -1,
    inflate_gzip_from_memory_success = 0,
} inflate_gzip_from_memory_errors_e;

inflate_gzip_from_memory_errors_e inflate_gzip_from_memory(cg_heap_t * resource_heap, const cg_context_t * context, const mem_region_t in_bytes, cg_allocation_t * const out_allocation);

#ifdef __cplusplus
}
#endif
