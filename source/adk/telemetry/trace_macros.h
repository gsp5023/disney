/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
TRACE FUNCTIONS: Add tracing to function. NOTE: _signature need to be comma separated.
    * eg: int func(int a, int b) =>
        TRACE_FN(, int, func, (int, a, int, b))

VARIATIONS:
    * TRACE_FN: non-void function with 0 or more arguments
    * TRACE_FN_VOID: void function with 0 or more arguments
*/

#ifdef _TELEMETRY
#include "source/adk/telemetry/telemetry.h"
#define TRACE_FN(_visibility, _return_type, _function_name, _signature, _mask)                    \
    static inline _return_type _function_name##_traced(MAP(CONCAT, PASS_PARAMETERS(_signature))); \
    _visibility _return_type _function_name(MAP(CONCAT, PASS_PARAMETERS(_signature))) {           \
        TRACE_PUSH_FN(_mask);                                                                     \
        _return_type ___r = _function_name##_traced(MAP(SECOND, PASS_PARAMETERS(_signature)));    \
        TRACE_POP(_mask);                                                                         \
        return ___r;                                                                              \
    }                                                                                             \
    static inline _return_type _function_name##_traced(MAP(CONCAT, PASS_PARAMETERS(_signature)))

#define TRACE_FN_VOID(_visibility, _function_name, _signature, _mask)                     \
    static inline void _function_name##_traced(MAP(CONCAT, PASS_PARAMETERS(_signature))); \
    _visibility void _function_name(MAP(CONCAT, PASS_PARAMETERS(_signature))) {           \
        TRACE_PUSH_FN(_mask);                                                             \
        _function_name##_traced(MAP(SECOND, PASS_PARAMETERS(_signature)));                \
        TRACE_POP(_mask);                                                                 \
    }                                                                                     \
    static inline void _function_name##_traced(MAP(CONCAT, PASS_PARAMETERS(_signature)))
#else
#define TRACE_FN(_visibility, _return_type, _function_name, _signature, _mask) _visibility _return_type _function_name(MAP(CONCAT, PASS_PARAMETERS(_signature)))
#define TRACE_FN_VOID(_visibility, _function_name, _signature, _mask) _visibility void _function_name(MAP(CONCAT, PASS_PARAMETERS(_signature)))
#endif

//** MACRO helpers below. **//

// Strip parenthesis
#define _Args(...) __VA_ARGS__
#define STRIP_PARENS(X) X
#define PASS_PARAMETERS(X) STRIP_PARENS(_Args X)

#define CONCAT(x, y) x y
#define SECOND(x, y) y

// Currently have support for up to 6 arguments (5 pairs)
#define GET_MAP_MACRO(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, NAME, ...) NAME

// Apply function f to each PAIR of arguments.
#define EXPAND(x) x
#define MAP(f, ...) EXPAND(EXPAND(GET_MAP_MACRO(_0, __VA_ARGS__, MAP12, MAP11, MAP10, MAP9, MAP8, MAP7, MAP6, MAP5, MAP4, MAP3, MAP2, MAP1, MAP0))(f, __VA_ARGS__))

#define INVALID_NUM_ARGS // Expect pairs (even)
#define MAP0(f) INVALID_NUM_ARGS
#define MAP1(f, ...) // Special case for no arguments, eg. ()
#define MAP2(f, _1, _2) f(_1, _2)
#define MAP3(...) INVALID_NUM_ARGS
#define MAP4(f, _1, _2, _3, _4) f(_1, _2), f(_3, _4)
#define MAP5(...) INVALID_NUM_ARGS
#define MAP6(f, _1, _2, _3, _4, _5, _6) f(_1, _2), f(_3, _4), f(_5, _6)
#define MAP7(...) INVALID_NUM_ARGS
#define MAP8(f, _1, _2, _3, _4, _5, _6, _7, _8) f(_1, _2), f(_3, _4), f(_5, _6), f(_7, _8)
#define MAP9(...) INVALID_NUM_ARGS
#define MAP10(f, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) f(_1, _2), f(_3, _4), f(_5, _6), f(_7, _8), f(_9, _10)
#define MAP11(...) INVALID_NUM_ARGS
#define MAP12(f, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) f(_1, _2), f(_3, _4), f(_5, _6), f(_7, _8), f(_9, _10), f(_11, _12)

#ifdef __cplusplus
}
#endif