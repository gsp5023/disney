/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
memory_pool_tests.c

memory pool library test fixture
*/
#include "source/adk/runtime/crc.h"
#include "source/adk/runtime/memory.h"
#include "testapi.h"

typedef struct test_memory_pool_node_t {
    struct test_memory_pool_node_t * next;
    char data[256];
    uint32_t crc;
} test_memory_pool_node_t;

static uint32_t rand_mem_crc(void * const p, const size_t size) {
    uint8_t * const b = (uint8_t *)p;
    for (size_t i = 0; i < size; ++i) {
        b[i] = (uint8_t)rand_int(0, 255);
    }

    return crc_32(b, size);
}

static void memory_pool_stress_test(memory_pool_t * const pool, const size_t expected_block_count, const int alignment) {
    memory_pool_verify(pool);
    assert_true(pool->internal.num_used_blocks == 0);

    size_t count = 0;
    test_memory_pool_node_t * head = NULL;
    test_memory_pool_node_t * tail = NULL;

    for (;;) {
        test_memory_pool_node_t * node = memory_pool_unchecked_calloc(pool, MALLOC_TAG);
        if (!node) {
            break;
        }
        ASSERT_ALIGNED(node, alignment);

        memory_pool_verify_ptr(pool, node);
        node->crc = rand_mem_crc(node->data, ARRAY_SIZE(node->data));
        if (tail) {
            assert_non_null(head);
            tail->next = node;
            tail = node;
        } else {
            head = tail = node;
        }
        ++count;
    }

    memory_pool_verify(pool);

    assert_null(pool->internal.free);
    assert_true(pool->internal.num_free_blocks == 0);
    assert_non_null(head);
    assert_non_null(tail);
    assert_true(count == expected_block_count);

    while (head) {
        test_memory_pool_node_t * next = head->next;
        memory_pool_verify_ptr(pool, head);
        assert_true(head->crc == crc_32((uint8_t *)head->data, ARRAY_SIZE(head->data)));
        memory_pool_free(pool, head, MALLOC_TAG);
        head = next;
    }

    memory_pool_verify(pool);
    assert_true(pool->internal.num_used_blocks == 0);
}

static void memory_pool_test_with_parameters(
    system_guard_page_mode_e guard_pages,
    const size_t pool_size,
    const int alignment,
    const int block_header_size) {
    ASSERT(!alignment || IS_POW2(alignment));

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const size_t aligned_pool_struct_size = ALIGN_INT(sizeof(memory_pool_t), clamped_alignment);
    assert_true(pool_size > aligned_pool_struct_size);
    const size_t page_aligned_pool_size = PAGE_ALIGN_INT(pool_size);

#ifdef DEBUG_PAGE_MEMORY_SERVICES
    static const char * guard_page_strings[] = {
        "system_guard_page_mode_disabled",
        "system_guard_page_mode_minimal",
        "system_guard_page_mode_enabled"};

    if (guard_pages) {
        {
            const size_t emplace_pool_size = page_aligned_pool_size - aligned_pool_struct_size;
            const size_t expected_block_count = memory_pool_get_block_count(emplace_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            print_message("testing: debug_memory_pool_emplace_init(size=%zu, bcnt=%zu, align=%d, header=%d, %s)...\n", pool_size, expected_block_count, alignment, block_header_size, guard_page_strings[guard_pages]);
            memory_pool_t * const pool = debug_memory_pool_emplace_init(pool_size, sizeof(test_memory_pool_node_t), alignment, block_header_size, guard_pages, MALLOC_TAG);
            assert_non_null(pool);
            memory_pool_stress_test(pool, expected_block_count, clamped_alignment);
            memory_pool_destroy(pool, MALLOC_TAG);
        }
        {
            const size_t expected_block_count = memory_pool_get_block_count(page_aligned_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            print_message("testing: debug_memory_pool_init(size=%zu, bcnt=%zu, align=%d, header=%d, %s)...\n", pool_size, expected_block_count, alignment, block_header_size, guard_page_strings[guard_pages]);
            memory_pool_t pool;
            debug_memory_pool_init(&pool, pool_size, sizeof(test_memory_pool_node_t), alignment, block_header_size, guard_pages, MALLOC_TAG);
            memory_pool_stress_test(&pool, expected_block_count, clamped_alignment);
            memory_pool_destroy(&pool, MALLOC_TAG);
        }
        return;
    }
#endif

    // page aligned memory pool test
    {
        void * const p = malloc(page_aligned_pool_size + get_sys_page_size() - 1);
        TRAP_OUT_OF_MEMORY(p);
        void * const aligned_p = (void *)PAGE_ALIGN_PTR(p);

        const mem_region_t pool_memory = MEM_REGION(.ptr = aligned_p, .size = page_aligned_pool_size);

        {
            const size_t emplace_pool_size = page_aligned_pool_size - aligned_pool_struct_size;
            const size_t expected_block_count = memory_pool_get_block_count(emplace_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            print_message("testing: memory_pool_emplace_init_with_block(size=%zu, bcnt=%zu, align=%d, header=%d)...\n", pool_size, expected_block_count, alignment, block_header_size);
            memory_pool_t * const pool = memory_pool_emplace_init_with_region(pool_memory, sizeof(test_memory_pool_node_t), alignment, block_header_size);
            assert_non_null(pool);
            memory_pool_stress_test(pool, expected_block_count, clamped_alignment);
            memory_pool_destroy(pool, MALLOC_TAG);
        }
        {
            const size_t expected_block_count = memory_pool_get_block_count(page_aligned_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            print_message("testing: memory_pool_init_with_block(size=%zu, bcnt=%zu, align=%d, header=%d)...\n", pool_size, expected_block_count, alignment, block_header_size);
            memory_pool_t pool;
            memory_pool_init_with_region(&pool, pool_memory, sizeof(test_memory_pool_node_t), alignment, block_header_size);
            memory_pool_stress_test(&pool, expected_block_count, clamped_alignment);
            memory_pool_destroy(&pool, MALLOC_TAG);
        }

        free(p);
    }

    // non-page aligned memory pool test
    {
        const size_t aligned_pool_size = ALIGN_INT(pool_size, clamped_alignment);
        const size_t emplace_pool_size = aligned_pool_size - aligned_pool_struct_size;
        void * const p = malloc(aligned_pool_size + clamped_alignment - 1);
        TRAP_OUT_OF_MEMORY(p);
        void * const aligned_p = (void *)ALIGN_PTR(p, clamped_alignment);

        const mem_region_t pool_memory = MEM_REGION(.ptr = aligned_p, .size = aligned_pool_size);

        const int clamped_block_header_size = max_int(block_header_size, sizeof(memory_pool_block_header_t));
        const size_t estimated_min_block_size = ALIGN_INT(clamped_block_header_size, clamped_alignment);

        // make sure our memory pool has enough room to hold at least one free block
        if (estimated_min_block_size < emplace_pool_size) {
            const size_t expected_block_count = memory_pool_get_block_count(emplace_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            if (expected_block_count) {
                print_message("testing: memory_pool_emplace_init_with_block(size=%zu, bcnt=%zu, align=%d, header=%d)...\n", pool_size, expected_block_count, alignment, block_header_size);
                memory_pool_t * const pool = memory_pool_emplace_init_with_region(pool_memory, sizeof(test_memory_pool_node_t), alignment, block_header_size);
                assert_non_null(pool);
                memory_pool_stress_test(pool, expected_block_count, clamped_alignment);
                memory_pool_destroy(pool, MALLOC_TAG);
            }
        }
        // make sure our memory pool has enough room to hold at least one free block
        if (estimated_min_block_size < aligned_pool_size) {
            const size_t expected_block_count = memory_pool_get_block_count(aligned_pool_size, sizeof(test_memory_pool_node_t), clamped_alignment, block_header_size);
            if (expected_block_count) {
                print_message("testing: memory_pool_init_with_block(size=%zu, bcnt=%zu, align=%d, header=%d)...\n", pool_size, expected_block_count, alignment, block_header_size);
                memory_pool_t pool;
                memory_pool_init_with_region(&pool, pool_memory, sizeof(test_memory_pool_node_t), alignment, block_header_size);
                memory_pool_stress_test(&pool, expected_block_count, clamped_alignment);
                memory_pool_destroy(&pool, MALLOC_TAG);
            }
        }

        free(p);
    }
}

static void memory_pool_test_variants(
    const size_t pool_size,
    const int * const alignments,
    const int * const block_header_sizes,
    const int num_alignment_variants,
    const int num_block_header_variants) {
    for (int i = 0; i < num_alignment_variants; ++i) {
        for (int k = 0; k < num_block_header_variants; ++k) {
#ifdef DEBUG_PAGE_MEMORY_SERVICES
#ifdef GUARD_PAGE_SUPPORT
            memory_pool_test_with_parameters(system_guard_page_mode_enabled, pool_size, alignments[i], block_header_sizes[k]);
#endif
            memory_pool_test_with_parameters(system_guard_page_mode_minimal, pool_size, alignments[i], block_header_sizes[k]);
#endif
            memory_pool_test_with_parameters(system_guard_page_mode_disabled, pool_size, alignments[i], block_header_sizes[k]);
        }
    }
}

static void memory_pool_test(size_t pool_size) {
    static const int alignments[] = {
        0, 16, 32};
    static const int block_header_sizes[] = {
        0, sizeof(memory_pool_block_header_t), sizeof(memory_pool_block_header_t) + 16, sizeof(memory_pool_block_header_t) + 32};
    static const int num_alignment_variants = ARRAY_SIZE(alignments);
    static const int num_block_header_variants = ARRAY_SIZE(block_header_sizes);

    memory_pool_test_variants(pool_size, alignments, block_header_sizes, num_alignment_variants, num_block_header_variants);
}

static void memory_pool_unit_test(void ** state) {
    int s = 1 * 1024 * 1024;
    {
        const char * arg = test_getargarg("-test_mem_pool_size");
        if (arg) {
            s = atoi(arg);
            // make pow2
            s = 1 << (32 - __builtin_clz(max_int(s, 1)));
        }
    }

    for (; s >= 256; s /= 2) {
        memory_pool_test(s);
        // test odd size
        if (s > 256) {
            memory_pool_test(rand_int(s / 2 + 1, s - 1));
        }
    }
}

int test_memory_pool() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(memory_pool_unit_test, NULL, NULL),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
