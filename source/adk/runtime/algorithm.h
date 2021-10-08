/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
algorithm.h

various container algorithms.

Usage:

// my_type.h
typedef struct my_type_t {
	int a, b;
} my_type;

#define ALGO_TYPE my_type_t
#include "algorithm.h"

// my_type.c
#define ALGO_IMPL my_type_t
static bool compare_less_my_type_t(const my_type_t a, const my_type_t b) {
	return a.a < b.a;
}

#include "algorithm.h"

-----------------------------------------
You can also define the algorithms for your type statically
for private types.
-----------------------------------------

#define ALGO_STATIC my_type_t
#include "algorithm.h"

*/

#include "runtime.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ALGO_STATIC
#define ALGO_TYPE ALGO_STATIC
#endif

#ifdef ALGO_TYPE
#define ALGO_MANGLE(_prefix) TOKENPASTE(_prefix, ALGO_TYPE)
#else
#define ALGO_MANGLE(_prefix) TOKENPASTE(_prefix, ALGO_IMPL)
#endif

#if !defined(ALGO_IMPL) || defined(ALGO_STATIC)

#ifndef ALGO_TYPE
#error "ALGO_TYPE not defined"
#endif

static void ALGO_MANGLE(swap_)(ALGO_TYPE * const a, ALGO_TYPE * const b) {
    ALGO_TYPE t = *a;
    *a = *b;
    *b = t;
}

#ifndef ALGO_STATIC
#define ALGO_PREDICATE bool (*less)(const ALGO_TYPE a, const ALGO_TYPE b)

void ALGO_MANGLE(make_heap_)(ALGO_TYPE * const begin, ALGO_TYPE * const end);
void ALGO_MANGLE(sort_heap_)(ALGO_TYPE * const begin, ALGO_TYPE * const end);
void ALGO_MANGLE(make_heap_with_predicate_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, ALGO_PREDICATE);
void ALGO_MANGLE(sort_heap_with_predicate_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, ALGO_PREDICATE);

void ALGO_MANGLE(sort_)(ALGO_TYPE * const begin, ALGO_TYPE * const end);
void ALGO_MANGLE(sort_with_predicate_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, ALGO_PREDICATE);

// Returns pointer to the first element in the range [begin, end) that is
// _not less_ than (i.e. greater or equal to) value, or end if no such element is found.
ALGO_TYPE * ALGO_MANGLE(lower_bound_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, const ALGO_TYPE value);
ALGO_TYPE * ALGO_MANGLE(lower_bound_with_predicate_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, const ALGO_TYPE value, ALGO_PREDICATE);
const ALGO_TYPE * ALGO_MANGLE(lower_bound_const_)(const ALGO_TYPE * const begin, const ALGO_TYPE * const end, const ALGO_TYPE value);
const ALGO_TYPE * ALGO_MANGLE(lower_bound_const_with_predicate_)(const ALGO_TYPE * const begin, const ALGO_TYPE * const end, const ALGO_TYPE value, ALGO_PREDICATE);

// Returns an iterator pointing to the first element in the range [begin, end)
// that is _greater_ than value, or last if no such element is found.
ALGO_TYPE * ALGO_MANGLE(upper_bound_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, const ALGO_TYPE value);
ALGO_TYPE * ALGO_MANGLE(upper_bound_with_predicate_)(ALGO_TYPE * const begin, ALGO_TYPE * const end, const ALGO_TYPE value, ALGO_PREDICATE);
const ALGO_TYPE * ALGO_MANGLE(upper_bound_const_)(const ALGO_TYPE * const begin, const ALGO_TYPE * const end, const ALGO_TYPE value);
const ALGO_TYPE * ALGO_MANGLE(upper_bound_const_with_predicate_)(const ALGO_TYPE * const begin, const ALGO_TYPE * const end, const ALGO_TYPE value, ALGO_PREDICATE);

#undef ALGO_PREDICATE
#endif

#undef ALGO_TYPE

#endif

#if defined(ALGO_IMPL) || defined(ALGO_STATIC)

#ifdef ALGO_STATIC
#define ALGO_IMPL ALGO_STATIC
#define ALGO_CALL static
#else
#define ALGO_CALL
#endif

#define ALGO_PREDICATE bool (*less)(const ALGO_IMPL a, const ALGO_IMPL b)
#define ALGO_SWAP TOKENPASTE(swap_, ALGO_IMPL)
#define ALGO_COMPARE_LESS TOKENPASTE(compare_less_, ALGO_IMPL)

static void TOKENPASTE(__heapify_, ALGO_IMPL)(ALGO_IMPL * const array, int ofs, const int len, ALGO_PREDICATE) {
    ASSERT(ofs < len);

push_down:;

    const int left = ofs * 2 + 1;
    const int right = ofs * 2 + 2;
    int largest = ofs;

    if ((left < len) && less(array[largest], array[left])) {
        ASSERT(!less(array[left], array[largest]));
        largest = left;
    }
    if ((right < len) && less(array[largest], array[right])) {
        ASSERT(!less(array[right], array[largest]));
        largest = right;
    }
    if (largest != ofs) {
        ALGO_SWAP(&array[ofs], &array[largest]);
        ofs = largest;
        goto push_down;
    }
}

ALGO_CALL void TOKENPASTE(make_heap_with_predicate_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, ALGO_PREDICATE) {
    const int len = (int)(end - begin);
    const int half_len = len / 2;
    for (int i = half_len - 1; i >= 0; --i) {
        TOKENPASTE(__heapify_, ALGO_IMPL)
        (begin, i, len, less);
    }
}

ALGO_CALL void TOKENPASTE(sort_heap_with_predicate_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, ALGO_PREDICATE) {
    // take a heap and converts it into a sorted array
    const int len = (int)(end - begin);
    for (int i = len - 1; i > 0; --i) {
        ASSERT(!less(begin[0], begin[i])); // heap invariant
        ALGO_SWAP(&begin[i], &begin[0]); // move largest value to end of array
        // the value at root is now the smalles value, breaking the heap property
        // resort this element
        TOKENPASTE(__heapify_, ALGO_IMPL)
        (begin, 0, i, less);
    }
}

ALGO_CALL void TOKENPASTE(make_heap_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end) {
    TOKENPASTE(make_heap_with_predicate_, ALGO_IMPL)
    (begin, end, ALGO_COMPARE_LESS);
}

ALGO_CALL void TOKENPASTE(sort_heap_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end) {
    TOKENPASTE(sort_heap_with_predicate_, ALGO_IMPL)
    (begin, end, ALGO_COMPARE_LESS);
}

static void TOKENPASTE(__insertion_sort_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, ALGO_PREDICATE) {
    const int len = (int)(end - begin);
    for (int i = 1; i < len; ++i) {
        ALGO_IMPL x = begin[i];
        int j = i - 1;
        while ((j >= 0) && less(x, begin[j])) {
            begin[j + 1] = begin[j];
            --j;
        }
        begin[j + 1] = x;
    }
}

static ALGO_IMPL * TOKENPASTE(__partition_, ALGO_IMPL)(ALGO_IMPL * const low, ALGO_IMPL * const high, ALGO_PREDICATE) {
    ALGO_IMPL pivot = *high;
    ALGO_IMPL * i = low - 1;

    for (ALGO_IMPL * j = low; j <= high - 1; ++j) {
        if (!less(pivot, *j)) {
            ++i;
            ALGO_SWAP(i, j);
        }
    }

    ALGO_SWAP(i + 1, high);
    return i + 1;
}

static ALGO_IMPL * TOKENPASTE(__midpoint_, ALGO_IMPL)(ALGO_IMPL * const a, ALGO_IMPL * const b, ALGO_IMPL * const c, ALGO_PREDICATE) {
    const bool a_less_b = less(*a, *b);
    const bool a_less_c = less(*a, *c);

    const bool b_less_c = less(*b, *c);
    const bool b_lequal_a = less(*b, *a) || !a_less_b;

    const bool c_lequal_a = less(*c, *a) || !a_less_c;
    const bool c_lequal_b = less(*c, *b) || !b_less_c;

    if (a_less_b && b_less_c) {
        return b;
    }

    if (a_less_c && c_lequal_b) {
        return c;
    }

    if (b_lequal_a && a_less_c) {
        return a;
    }

    if (b_less_c && c_lequal_a) {
        return c;
    }

    if (c_lequal_a && a_less_b) {
        return a;
    }

    return b;
}

ALGO_CALL void TOKENPASTE(__introsort_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, const int maxdepth, ALGO_PREDICATE) {
    if (begin < end - 1) {
        if (maxdepth < 1) {
            TOKENPASTE(make_heap_with_predicate_, ALGO_IMPL)
            (begin, end, less);
            TOKENPASTE(sort_heap_with_predicate_, ALGO_IMPL)
            (begin, end, less);
        } else {
            {
                ALGO_IMPL * pivot = TOKENPASTE(__midpoint_, ALGO_IMPL)(begin, begin + (end - begin - 1) / 2, end - 1, less);
                ALGO_SWAP(pivot, end - 1);
            }
            ALGO_IMPL * const p = TOKENPASTE(__partition_, ALGO_IMPL)(begin, end - 1, less);
            TOKENPASTE(__introsort_, ALGO_IMPL)
            (begin, p, maxdepth - 1, less);
            TOKENPASTE(__introsort_, ALGO_IMPL)
            (p + 1, end, maxdepth - 1, less);
        }
    }
}

ALGO_CALL void TOKENPASTE(sort_with_predicate_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, ALGO_PREDICATE) {
    if (begin < end - 1) {
        const int maxdepth = 2 * log2i((int)(end - begin));
        TOKENPASTE(__introsort_, ALGO_IMPL)
        (begin, end, maxdepth, less);
    }
}

ALGO_CALL void TOKENPASTE(sort_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end) {
    TOKENPASTE(sort_with_predicate_, ALGO_IMPL)
    (begin, end, ALGO_COMPARE_LESS);
}

// Returns pointer to the first element in the range [begin, end) that is
// _not less_ than (i.e. greater or equal to) value, or end if no such element is found.

ALGO_CALL const ALGO_IMPL * TOKENPASTE(lower_bound_const_with_predicate_, ALGO_IMPL)(const ALGO_IMPL * const begin, const ALGO_IMPL * const end, const ALGO_IMPL value, ALGO_PREDICATE) {
    const int len = (int)(end - begin);

    int left = 0;
    int right = len;
    int mid = right / 2;

    while (left < right) {
        ASSERT(mid < len);

        const ALGO_IMPL check = begin[mid];

        if (less(check, value)) {
            left = mid + 1;
        } else {
            right = mid;
        }

        mid = (left + right) / 2;
    }

    ASSERT(right <= len);
    return begin + right;
}

ALGO_CALL const ALGO_IMPL * TOKENPASTE(lower_bound_const_, ALGO_IMPL)(const ALGO_IMPL * const begin, const ALGO_IMPL * const end, const ALGO_IMPL value) {
    return TOKENPASTE(lower_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, ALGO_COMPARE_LESS);
}

ALGO_CALL ALGO_IMPL * TOKENPASTE(lower_bound_with_predicate_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, const ALGO_IMPL value, ALGO_PREDICATE) {
    return (ALGO_IMPL *)TOKENPASTE(lower_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, less);
}

ALGO_CALL ALGO_IMPL * TOKENPASTE(lower_bound_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, const ALGO_IMPL value) {
    return (ALGO_IMPL *)TOKENPASTE(lower_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, ALGO_COMPARE_LESS);
}

// Returns an iterator pointing to the first element in the range [begin, end)
// that is _greater_ than value, or last if no such element is found.

ALGO_CALL const ALGO_IMPL * TOKENPASTE(upper_bound_const_with_predicate_, ALGO_IMPL)(const ALGO_IMPL * const begin, const ALGO_IMPL * const end, const ALGO_IMPL value, ALGO_PREDICATE) {
    const int len = (int)(end - begin);

    int left = 0;
    int right = len;
    int mid = right / 2;

    while (left < right) {
        ASSERT(mid < len);

        const ALGO_IMPL check = begin[mid];

        if (less(value, check)) {
            right = mid;
        } else {
            left = mid + 1;
        }

        mid = (left + right) / 2;
    }

    ASSERT(right <= len);
    return begin + right;
}

ALGO_CALL const ALGO_IMPL * TOKENPASTE(upper_bound_const_, ALGO_IMPL)(const ALGO_IMPL * const begin, const ALGO_IMPL * const end, const ALGO_IMPL value) {
    return TOKENPASTE(upper_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, ALGO_COMPARE_LESS);
}

ALGO_CALL ALGO_IMPL * TOKENPASTE(upper_bound_with_predicate_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, const ALGO_IMPL value, ALGO_PREDICATE) {
    return (ALGO_IMPL *)TOKENPASTE(upper_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, less);
}

ALGO_CALL ALGO_IMPL * TOKENPASTE(upper_bound_, ALGO_IMPL)(ALGO_IMPL * const begin, ALGO_IMPL * const end, const ALGO_IMPL value) {
    return (ALGO_IMPL *)TOKENPASTE(upper_bound_const_with_predicate_, ALGO_IMPL)(begin, end, value, ALGO_COMPARE_LESS);
}

#undef ALGO_IMPL
#undef ALGO_SWAP
#undef ALGO_COMPARE_LESS
#undef ALGO_PREDICATE
#undef ALGO_STATIC
#undef ALGO_CALL

#endif

#undef ALGO_MANGLE

#ifdef __cplusplus
}
#endif
