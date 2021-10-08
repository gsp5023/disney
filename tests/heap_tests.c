/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
heap_tests.c

memory heap library test fixture
*/
#include "source/adk/runtime/crc.h"
#include "source/adk/runtime/memory.h"
#include "testapi.h"

typedef enum alloc_fail_mode_e {
    alloc_fail_is_error,
    alloc_fail_is_ok
} alloc_fail_mode_e;

typedef struct alloc_crc_t {
    void * ptr;
    size_t size;
    uint32_t crc;
} alloc_crc_t;

typedef struct alloc_crcs_t {
    alloc_crc_t * allocs;
    int num_allocs;
    size_t total_cost;
} alloc_crcs_t;

static uint32_t rand_mem_crc(void * const p, const size_t size) {
    uint8_t * const b = (uint8_t *)p;
    for (size_t i = 0; i < size; ++i) {
        b[i] = (uint8_t)rand_int(0, 255);
    }

    return crc_32(b, size);
}

static void check_crc(const alloc_crc_t crc) {
    assert_true(crc_32((uint8_t *)crc.ptr, crc.size) == crc.crc);
}

static alloc_crc_t rand_alloc_crc(heap_t * const heap, const size_t size) {
    void * const p = heap_unchecked_alloc(heap, size, MALLOC_TAG);
    if (p) {
        return (alloc_crc_t){
            .ptr = p,
            .size = size,
            .crc = rand_mem_crc(p, size)};
    }

    return (alloc_crc_t){0};
}

static void assert_can_alloc(heap_t * const heap, const size_t size, const int alignment) {
    // heap_alloc dies if it fails
    void * const p = heap_unchecked_alloc(heap, size, MALLOC_TAG);
    assert_non_null(p);
    ASSERT_ALIGNED(p, alignment);
    heap_free(heap, p, MALLOC_TAG);
}

static size_t estimate_block_cost(const heap_t * const heap, const size_t size) {
    const size_t aligned_size = size ? ALIGN_INT(size, heap->internal.alignment) : heap->internal.alignment;
    return aligned_size + heap->internal.ptr_ofs;
}

static void verify_blocks(heap_t * heap, const alloc_crcs_t crcs) {
    size_t total_cost = 0;
    for (int i = 0; i < crcs.num_allocs; ++i) {
        const alloc_crc_t crc = crcs.allocs[i];
        check_crc(crc);
        total_cost += estimate_block_cost(heap, crc.size);
    }

    assert_true(total_cost == crcs.total_cost);
}

static void random_allocs(heap_t * heap, alloc_crcs_t * crcs, const size_t heap_size, const int min_allocation_size, const int max_allocation_size, const int alignment, const size_t num_allocations, alloc_fail_mode_e fail_mode) {
    crcs->allocs = (alloc_crc_t *)realloc(crcs->allocs, sizeof(alloc_crc_t) * (num_allocations + crcs->num_allocs));
    assert_non_null(crcs->allocs);
    memset(&crcs->allocs[crcs->num_allocs], 0, sizeof(alloc_crc_t) * num_allocations);

    size_t total_allocated_size = crcs->total_cost;

    for (size_t i = 0; i < num_allocations; ++i) {
        const size_t size = (i == 0) && (min_allocation_size == 0) ? 0 : rand_int(min_allocation_size, max_allocation_size);
        const size_t cost = estimate_block_cost(heap, size);
        const alloc_crc_t crc = rand_alloc_crc(heap, size);

        if (crc.ptr) {
            ASSERT_ALIGNED(crc.ptr, alignment);
            total_allocated_size += cost;
            crcs->allocs[crcs->num_allocs++] = crc;
        } else {
            if ((cost + total_allocated_size) <= heap_size) {
                assert_true(fail_mode != alloc_fail_is_error);
            }
        }
    }

    crcs->total_cost = total_allocated_size;
    crcs->allocs = (alloc_crc_t *)realloc(crcs->allocs, sizeof(alloc_crc_t) * crcs->num_allocs);
    assert_non_null(crcs->allocs);
    verify_blocks(heap, *crcs);
}

static void random_reallocs(heap_t * heap, alloc_crcs_t * allocs, const int min_allocation_size, const int max_allocation_size, const int alignment) {
    const int num_allocations = allocs->num_allocs;

    for (int i = 0; i < num_allocations; ++i) {
        alloc_crc_t crc = allocs->allocs[i];
        check_crc(crc);
        uint32_t partial_crc = 0;

        const size_t new_size = rand_int(min_allocation_size, max_allocation_size);
        if (new_size < crc.size) {
            partial_crc = crc_32((uint8_t *)crc.ptr, new_size);
        }

        void * const p = heap_unchecked_realloc(heap, crc.ptr, new_size, MALLOC_TAG);

        // shrinking an allocation can never fail
        assert_true(p || (new_size > crc.size));

        if (p) {
            ASSERT_ALIGNED(p, alignment);
            crc.ptr = p;

            const size_t old_cost = estimate_block_cost(heap, crc.size);
            const size_t new_cost = estimate_block_cost(heap, new_size);

            if (new_size < crc.size) {
                crc.size = new_size;
                crc.crc = crc_32((uint8_t *)p, new_size);
                // make sure we didn't corrupt any data when we shrunk
                assert_true(partial_crc == crc.crc);
                assert_true(old_cost >= new_cost);
                const size_t delta = old_cost - new_cost;
                assert_true(allocs->total_cost >= delta);
                allocs->total_cost -= delta;
            } else {
                // crc should not have changed!
                check_crc(crc);
                crc.size = new_size;
                crc.crc = rand_mem_crc(p, new_size);

                assert_true(new_cost >= old_cost);
                const size_t delta = new_cost - old_cost;
                allocs->total_cost += delta;
            }

            allocs->allocs[i] = crc;
        }
    }

    verify_blocks(heap, *allocs);
}

static void random_free(heap_t * const heap, alloc_crcs_t * crcs, int num_to_free) {
    assert_true(num_to_free <= crcs->num_allocs);

    while (--num_to_free >= 0) {
        const int i = rand_int(0, crcs->num_allocs - 1);

        if (crcs->allocs[i].ptr) {
            const size_t block_cost = estimate_block_cost(heap, crcs->allocs[i].size);
            assert_true(crcs->total_cost >= block_cost);
            crcs->total_cost -= block_cost;
            heap_free(heap, crcs->allocs[i].ptr, MALLOC_TAG);
        }

        if (i < crcs->num_allocs - 1) {
            memmove(&crcs->allocs[i], &crcs->allocs[i + 1], sizeof(alloc_crc_t) * (crcs->num_allocs - i - 1));
        }

        --crcs->num_allocs;
    }

    crcs->allocs = realloc(crcs->allocs, sizeof(alloc_crc_t) * crcs->num_allocs);
    assert_non_null(crcs->allocs);
    verify_blocks(heap, *crcs);
}

static void heap_stress_test(
    heap_t * const heap,
    const size_t heap_size,
    const int alignment,
    const int block_header_size,
    const size_t num_iterations) {
    heap_enable_debug_checks(heap, true);

    const int max_block_size = (int)ALIGN_INT(heap_size - heap->internal.ptr_ofs, alignment);

    // should not be able to allocate a block of this size
    assert_null(heap_unchecked_alloc(heap, heap_size, MALLOC_TAG));
    // check for integer overflow whenever calculating aligned size
    assert_null(heap_unchecked_alloc(heap, SIZE_MAX, MALLOC_TAG));
    assert_true(heap->internal.num_used_blocks == 0);

    assert_can_alloc(heap, max_block_size, alignment);

    alloc_crcs_t crcs;
    ZEROMEM(&crcs);
    assert_true(max_block_size / 4 > 0);
    assert_true(max_block_size / 2 != max_block_size / 4);

    const size_t min_block_cost = estimate_block_cost(heap, 0);
    const size_t num_allocations = (heap_size / min_block_cost) / 2 ? (heap_size / min_block_cost) / 2 : (heap_size / min_block_cost);
    assert_true(num_allocations > 0);

    random_allocs(heap, &crcs, heap_size, 0, max_block_size / 2 + max_block_size / 4, alignment, num_allocations, alloc_fail_is_error);
    verify_blocks(heap, crcs);

    for (size_t i = 0; i < num_iterations; ++i) {
        random_reallocs(heap, &crcs, 0, max_block_size / 4, alignment);

        // free half of the allocations
        if (crcs.num_allocs > 1) {
            random_free(heap, &crcs, crcs.num_allocs / 2);
        }

        random_reallocs(heap, &crcs, 0, max_block_size / 4, alignment);

        // 50% chance to free 75% of them
        if ((crcs.num_allocs > 1) && (rand_float() > 0.5f)) {
            random_free(heap, &crcs, crcs.num_allocs / 2);
        }

        // heap is now likely fragmented and some allocations will probably fail
        random_allocs(heap, &crcs, heap_size, 0, max_block_size / 4, alignment, num_allocations, alloc_fail_is_ok);
    }

    for (int i = 0; i < crcs.num_allocs; ++i) {
        heap_free(heap, crcs.allocs[i].ptr, MALLOC_TAG);
    }

    free(crcs.allocs);

    // check perfect fragmentation recovery
    assert_can_alloc(heap, max_block_size, alignment);
    assert_true(heap->internal.num_used_blocks == 0);
}

static void heap_test_with_parameters(
    system_guard_page_mode_e guard_pages,
    const size_t heap_size,
    const int alignment,
    const int block_header_size,
    const int num_iterations) {
    ASSERT(!alignment || IS_POW2(alignment));
    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const size_t aligned_heap_struct_size = ALIGN_INT(sizeof(heap_t), clamped_alignment);
    assert_true(heap_size > aligned_heap_struct_size);
    const size_t page_aligned_heap_size = PAGE_ALIGN_INT(heap_size);

#ifdef DEBUG_PAGE_MEMORY_SERVICES
    static const char * guard_page_strings[] = {
        "system_guard_page_mode_disabled",
        "system_guard_page_mode_minimal",
        "system_guard_page_mode_enabled"};

    if (guard_pages) {
        const size_t emplace_heap_size = page_aligned_heap_size - aligned_heap_struct_size;
        {
            print_message("testing: debug_heap_emplace_init(size=%zu, align=%d, header=%d, iter=%d, %s)...\n", heap_size, alignment, block_header_size, num_iterations, guard_page_strings[guard_pages]);
            heap_t * const heap = debug_heap_emplace_init(heap_size, alignment, block_header_size, "heap_tests", guard_pages, MALLOC_TAG);
            assert_non_null(heap);
            heap_stress_test(heap, emplace_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(heap, MALLOC_TAG);
        }
        {
            print_message("testing: debug_heap_init(size=%zu, align=%d, header=%d, iter=%d, %s)...\n", heap_size, alignment, block_header_size, num_iterations, guard_page_strings[guard_pages]);
            heap_t heap;
            debug_heap_init(&heap, heap_size, alignment, block_header_size, "heap_tests", guard_pages, MALLOC_TAG);
            heap_stress_test(&heap, page_aligned_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(&heap, MALLOC_TAG);
        }
        return;
    }
#endif

    // page aligned heap memory test
    {
        const size_t emplace_heap_size = page_aligned_heap_size - aligned_heap_struct_size;
        void * const p = malloc(page_aligned_heap_size + get_sys_page_size() - 1);
        TRAP_OUT_OF_MEMORY(p);
        void * const aligned_p = (void *)PAGE_ALIGN_PTR(p);

        const mem_region_t heap_memory = MEM_REGION(.ptr = aligned_p, .size = page_aligned_heap_size);

        {
            print_message("testing: heap_emplace_init_with_block(size=%zu, align=%d, header=%d, iter=%d)...\n", page_aligned_heap_size, alignment, block_header_size, num_iterations);
            heap_t * const heap = heap_emplace_init_with_region(heap_memory, alignment, block_header_size, "heap_tests");
            assert_non_null(heap);
            heap_stress_test(heap, emplace_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(heap, MALLOC_TAG);
        }
        {
            print_message("testing: heap_init_with_block(size=%zu, align=%d, header=%d, iter=%d)...\n", page_aligned_heap_size, alignment, block_header_size, num_iterations);
            heap_t heap;
            heap_init_with_region(&heap, heap_memory, alignment, block_header_size, "heap_tests");
            heap_stress_test(&heap, page_aligned_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(&heap, MALLOC_TAG);
        }

        free(p);
    }

    // non-page aligned heap memory test
    {
        const size_t aligned_heap_size = ALIGN_INT(heap_size, clamped_alignment);
        const size_t emplace_heap_size = aligned_heap_size - aligned_heap_struct_size;
        void * const p = malloc(aligned_heap_size + clamped_alignment - 1);
        TRAP_OUT_OF_MEMORY(p);
        void * const aligned_p = (void *)ALIGN_PTR(p, clamped_alignment);

        const mem_region_t heap_memory = MEM_REGION(.ptr = aligned_p, .size = aligned_heap_size);

        const int clamped_block_size = max_int(block_header_size, sizeof(heap_block_header_t));
        const size_t estimated_min_block_size = ALIGN_INT(clamped_block_size, clamped_alignment);

        // make sure our heap has enough room to hold at least one free block
        if (estimated_min_block_size < emplace_heap_size) {
            print_message("testing: heap_emplace_init_with_block(size=%zu, align=%d, header=%d, iter=%d)...\n", aligned_heap_size, alignment, block_header_size, num_iterations);
            heap_t * const heap = heap_emplace_init_with_region(heap_memory, alignment, block_header_size, "heap_tests");
            assert_non_null(heap);
            heap_stress_test(heap, emplace_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(heap, MALLOC_TAG);
        }
        // make sure our heap has enough room to hold at least one free block
        if (estimated_min_block_size < aligned_heap_size) {
            print_message("testing: heap_init_with_block(size=%zu, align=%d, header=%d, iter=%d)...\n", aligned_heap_size, alignment, block_header_size, num_iterations);
            heap_t heap;
            heap_init_with_region(&heap, heap_memory, alignment, block_header_size, "heap_tests");
            heap_stress_test(&heap, aligned_heap_size, clamped_alignment, block_header_size, num_iterations);
            heap_destroy(&heap, MALLOC_TAG);
        }

        free(p);
    }
}

static void heap_test_variants(
    const size_t heap_size,
    const int * const alignments,
    const int * const block_header_sizes,
    const int num_alignment_variants,
    const int num_block_header_variants,
    const int num_iterations) {
    for (int i = 0; i < num_alignment_variants; ++i) {
        for (int k = 0; k < num_block_header_variants; ++k) {
#ifdef DEBUG_PAGE_MEMORY_SERVICES
#ifdef GUARD_PAGE_SUPPORT
            heap_test_with_parameters(system_guard_page_mode_enabled, heap_size, alignments[i], block_header_sizes[k], num_iterations);
#endif
            heap_test_with_parameters(system_guard_page_mode_minimal, heap_size, alignments[i], block_header_sizes[k], num_iterations);
#endif
            heap_test_with_parameters(system_guard_page_mode_disabled, heap_size, alignments[i], block_header_sizes[k], num_iterations);
        }
    }
}

static void heap_test(size_t heap_size) {
    static const int alignments[] = {
        0, 16, 32};
    static const int block_header_sizes[] = {
        0, sizeof(heap_block_header_t), sizeof(heap_block_header_t) + 16, sizeof(heap_block_header_t) + 32};
    static const int num_alignment_variants = ARRAY_SIZE(alignments);
    static const int num_block_header_variants = ARRAY_SIZE(block_header_sizes);

    heap_test_variants(heap_size, alignments, block_header_sizes, num_alignment_variants, num_block_header_variants, 4);
}

static void heap_unit_test(void ** state) {
    int s = 8 * 1024 * 1024;
    {
        const char * arg = test_getargarg("-test_heap_size");
        if (arg) {
            s = atoi(arg);
            // make pow2
            s = 1 << (32 - __builtin_clz(max_int(s, 1)));
        }
    }

    for (; s >= 256; s /= 2) {
        heap_test(s);
        // test odd size
        if (s > 256) {
            heap_test(rand_int(s / 2 + 1, s - 1));
        }
    }
}

int test_heap() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(heap_unit_test, NULL, NULL),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
