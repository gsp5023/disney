/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
runtime.h

Main header for all translation units.

Declares types, common macros, and strings etc.
*/

#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#include <float.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
=======================================
Debug Page Memory Services

ADK applications are generally forbidden from allocating
dynamic memory and instead must allocate memory from their
own subsystem's reserved memory.

During development builds some "out of band" memory allocation
services can be enabled to support guard pages support in
heaps and memory pools or in custom system code.
=======================================
*/

//#define DEBUG_PAGE_MEMORY_SERVICES

/*
=======================================
Guard Page Support

If GUARD_PAGE_SUPPORT is defined, then specifying system_guard_pages_enabled in any memory allocator
will place all allocations at the end of a page, guarded by a NO_ACCESS page. This will cause an immediate
pagefault exception if a memory overrun occurs. This mode incurs significant memory overhead since every
allocation requires at least 2 pages of memory, and each page is typically 4k.

If GUARD_PAGE_SUPPORT is NOT defined then system_guard_pages_enabled acts like system_guard_pages_minimal
which bookends a given memory allocator by two NO_ACCESS pages.

system_guard_pages_disabled disables any extra-page allocations for overrun checks. All allocators internally
still have some minimal support to detect memory overwrite corruption but when it is detected it may not
be in a related part of the code where it is detected or causes side effects
(i.e. corruption could have happened anytime in the past).
=======================================
*/

#ifndef _SHIP
#define GUARD_PAGE_SUPPORT
#endif

#if defined(GUARD_PAGE_SUPPORT) && !defined(DEBUG_PAGE_MEMORY_SERVICES)
#define DEBUG_PAGE_MEMORY_SERVICES
#endif

/// Types of memory page protection
typedef enum system_page_protect_e {
    /// Read Only
    system_page_protect_read_only,
    /// Read & Write
    system_page_protect_read_write,
    /// Neither Read nor Right
    system_page_protect_no_access
} system_page_protect_e;

/// Types of guard page modes
typedef enum system_guard_page_mode_e {
    /// No guard pages used
    system_guard_page_mode_disabled,
    /// Minimal guard pages used
    system_guard_page_mode_minimal,
    /// Maximum guard pages used
    system_guard_page_mode_enabled
} system_guard_page_mode_e;

/*
===============================================================================
SYSTEM / COMPILER

Macros for architecture, DLL export, intrinsics
===============================================================================
*/

#if defined(__arm__) || defined(_M_ARM)
#define _ARM
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define _ARM64
#endif

#if defined(__thumb__) || defined(_M_ARMT)
#define _THUMB
#endif

#if defined(_MIPS_ARCH_MIPS3) || defined(_MIPS_ARCH_MIPS4) || defined(_MIPS_ARCH_MIPS64)
#define _MIPS_64
#elif defined(__mips__)
#define _MIPS
#endif

#if defined(__i386) || defined(_M_IX86)
#define _X86
#if _M_IX86_FP >= 2
#define _SSE2
#endif
#endif

#if defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64)
#define _X86_64
#define _SSE2
#endif

#if UINTPTR_MAX == UINT32_MAX
#define _MACHINE_SIZE 32
#else
#define _MACHINE_SIZE 64
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#define _NATIVE_WCHAR_T_DEFINED
#endif

#if defined(__GNUC__)
#if !(defined(_VADER) || defined(_LEIA))
#include <alloca.h>
#endif
#include <strings.h>

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH ((void)0)
#endif

#define STATIC_ASSERT(_expr) __attribute__((__unused__)) static const int TOKENPASTE(static_assert_array_, __COUNTER__)[(_expr) ? 1 : -1] = {}
#define PEDANTIC_CAST(_x) (_x)

#if defined(__clang__) || ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
typedef enum memory_order_e {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order_e;
#else
typedef enum memory_order_e {
    memory_order_relaxed,
    memory_order_acquire,
    memory_order_release,
    memory_order_seq_cst
} memory_order_e;
#endif

#define ALLOCA alloca
// 'ALIGN' collides with third-party headers
#define ALIGN_N(_x, _a) _x __attribute__((aligned(_a)))
#define PACK(_struct) _struct __attribute__((__packed__))
#define PURE __attribute__((const)) // referential transparency
#define THREAD_LOCAL __thread
#ifdef NDEBUG
#define DBG_BREAK()
#else
#define DBG_BREAK() TRAP("DBG_BREAK!")
#endif
#if defined(_X86) || defined(_X86_64)
#include <immintrin.h>
#include <x86intrin.h>
#endif

#define DLL_IMPORT
#if (defined(_VADER) || defined(_LEIA))
#define DLL_EXPORT __declspec(dllexport)
#elif __GNUC__ >= 4
#define DLL_EXPORT __attribute__((visibility("default")))
#else
#define DLL_EXPORT
#endif

#define NOVTABLE
#define ALIGN_OF(_x) __alignof__(_x)

#if !defined(__clang__) && ((__GNUC__ == 4) && (__GNUC_MINOR__ < 8))
// replace/fix __builtin_bswap16 not existing until gcc 4.8
PURE static inline uint16_t __builtin_bswap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}
#endif

#elif defined(_MSC_VER)
#include <immintrin.h>
#include <intrin.h>
#define ALLOCA _alloca
#define DLL_IMPORT __declspec(dllimport)
#define DLL_EXPORT __declspec(dllexport)
#define RDTSC() __rdtsc()
#define NOVTABLE __declspec(novtable)
#define THREAD_LOCAL __declspec(thread)
#define ALIGN_OF(_x) __alignof(_x)
// 'ALIGN' collides with third-party headers
#define ALIGN_N(_x, _a) __declspec(align(_a)) _x
#define PACK(_struct) __pragma(pack(push, 1)) _struct __pragma(pack(pop))
#define PURE // referential transparency

#define __builtin_popcount __popcnt

#define FALLTHROUGH __fallthrough
#define STATIC_ASSERT(_expr) static const int TOKENPASTE(static_assert_array_, __COUNTER__)[(_expr) ? 1 : -1] = {0}
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define PEDANTIC_CAST(_x)

PURE static inline uint32_t __builtin_popcountll(uint64_t x) {
    return __popcnt(x & 0xFFFFFFFFUL) + __popcnt(x >> 32);
}

PURE static inline uint32_t __builtin_clz(uint32_t x) {
    if (x) {
        unsigned long r = 0;
        _BitScanReverse(&r, x);
        return (31 - r);
    }

    return 32;
}

PURE static inline uint32_t __builtin_ctz(uint32_t x) {
    if (x) {
        unsigned long r = 0;
        _BitScanForward(&r, x);
        return r;
    }

    return 32;
}

PURE static inline uint32_t __builtin_ctzll(const uint64_t x) {
    uint32_t result = __builtin_ctz(x & 0xFFFFFFFFUL);
    if (result == 32) {
        result += __builtin_ctz(x >> 32);
    }
    return result;
}

PURE static inline uint32_t __builtin_clzll(const uint64_t x) {
    uint32_t result = __builtin_clz(x >> 32);
    if (result == 32) {
        result += __builtin_clz(x & 0xFFFFFFFFUL);
    }
    return result;
}

PURE static inline uint16_t __builtin_bswap16(uint16_t val) {
    return _byteswap_ushort(val);
}

PURE static inline uint32_t __builtin_bswap32(uint32_t val) {
    return _byteswap_ulong(val);
}

PURE static inline uint64_t __builtin_bswap64(uint64_t val) {
    return _byteswap_uint64(val);
}

/// specifies how memory accesses are to be ordered around an atomic operation.
typedef enum memory_order_e {
    /// no fence
    memory_order_relaxed,
    /// prevent stores from moving after this operation
    memory_order_acquire,
    /// prevent loads from moving before this operation
    memory_order_release,
    /// total order consistency
    memory_order_seq_cst
} memory_order_e;

#ifdef NDEBUG
#define DBG_BREAK()
#else
#define DBG_BREAK() __debugbreak()
#endif
#endif

#ifndef DLLAPI
#define DLLAPI
#endif

#define FORCE_ENUM_SIGNED(__name) TOKENPASTE(TOKENPASTE(__, __name), __force_signed) = -1
#define FORCE_ENUM_INT32(__name) TOKENPASTE(TOKENPASTE(__, __name), __force_int32) = INT32_MIN

#define TOKENPASTE2(_x, _y) _x##_y
#define TOKENPASTE(_x, _y) TOKENPASTE2(_x, _y)

/*
===============================================================================
ASSERT/VERIFY

ASSERT(expr):

	checks "expr", if it evaluates to false an assertion failure is triggered.
	ASSERT compiles to an empty statement when NDEBUG is defined. Do not
	place expressions with side effects inside an ASSERT.

VERIFY(expr):

	identical to assert except VERIFY persists in all builds, regardless of
	NDEBUG or other flags.

TRAP("message"): an unconditional assertion failure, persists in all builds.

TRAP_OUT_OF_MEMORY(expr): triggers an "out of memory" assertion failure if "expr"
is NULL.
===============================================================================
*/

static const char * vaprintf_macro(char * const buffer, const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, 0x7fffffff, msg, args);
    va_end(args);
    return buffer;
}

static int vaprintf_macro_count(const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    const int num = vsnprintf(NULL, 0, msg, args);
    va_end(args);
    return num;
}

#define VAPRINTF(...) vaprintf_macro((char * const)ALLOCA(vaprintf_macro_count(__VA_ARGS__) + 1), __VA_ARGS__)

EXT_EXPORT DLLAPI void assert_failed(const char * const message, const char * const filename, const char * const function, const int line);
DLLAPI void debug_write(const char * const msg, ...);
/// Use sparingly (e.g. for actual TTY output), in all other cases use LOG_* macros
DLLAPI void debug_write_line(const char * const msg, ...);

DLLAPI void sb_vadebug_write(const char * const msg, va_list args);
EXT_EXPORT DLLAPI void sb_vadebug_write_line(const char * const msg, va_list args);
DLLAPI void sb_unformatted_debug_write(const char * const msg);
EXT_EXPORT DLLAPI void sb_unformatted_debug_write_line(const char * const msg);

#define VERIFY_MSG(_expr, ...) (void)((!!(_expr)) || (assert_failed(VAPRINTF(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__), 0))
#define VERIFY(_expr) VERIFY_MSG(_expr, #_expr)

#ifdef NDEBUG
#define ASSERT(_expr) ((void)0)
#define ASSERT_MSG(_expr, ...) ((void)0)
#else
#define ASSERT(_expr) VERIFY(_expr)
#define ASSERT_MSG(_expr, ...) VERIFY_MSG(_expr, __VA_ARGS__)
#endif

#define TRAP(...) assert_failed(VAPRINTF(__VA_ARGS__), __FILE__, __FUNCTION__, __LINE__)
#define TRAP_OUT_OF_MEMORY(_expr) VERIFY_MSG(_expr, "Out of memory!");

#define UNIMPLEMENTED() TRAP("UNIMPLEMENTED")

/*
===============================================================================
Struct / member alignment

Usage:

struct ALIGN_N(mystruct, 8) {};
struct ALIGN_8(mystruct) {};

struct ALIGN_N(mystruct, 8) {
	int ALIGN_16(my_aligned_int);
};

===============================================================================
*/

#define ALIGN_4(_x) ALIGN_N(_x, 4)
#define ALIGN_8(_x) ALIGN_N(_x, 8)
#define ALIGN_16(_x) ALIGN_N(_x, 16)
#define ALIGN_32(_x) ALIGN_N(_x, 32)
#define ALIGN_64(_x) ALIGN_N(_x, 64)

/*
===============================================================================
Int/Ptr Alignment
===============================================================================
*/

#define FWD_ALIGN_INT(_x, _a) (((_x) + (_a)-1) & ~((_a)-1))
#define FWD_ALIGN_PTR(_x, _a) FWD_ALIGN_INT((uintptr_t)(_x), (uintptr_t)(_a))

#define REV_ALIGN_INT(_x, _a) ((_x) & ~((_a)-1))
#define REV_ALIGN_PTR(_x, _a) REV_ALIGN_INT(((uintptr_t)(_x)), ((uintptr_t)(_a)))

#define ALIGN_INT(_x, _a) FWD_ALIGN_INT(_x, _a)
#define ALIGN_PTR(_x, _a) FWD_ALIGN_PTR(_x, _a)

#define IS_ALIGNED(_x, _z) (((uintptr_t)_x) == FWD_ALIGN_INT((uintptr_t)_x, (uintptr_t)_z))
#define ASSERT_ALIGNED(_x, _z) ASSERT(IS_ALIGNED(_x, _z))
#define VERIFY_ALIGNED(_x, _z) VERIFY(IS_ALIGNED(_x, _z))

#define ASSERT_ALIGNED_4(_x) ASSERT_ALIGNED(_x, 4)
#define VERIFY_ALIGNED_4(_x) VERIFY_ALIGNED(_x, 4)
#define ASSERT_ALIGNED_8(_x) ASSERT_ALIGNED(_x, 8)
#define VERIFY_ALIGNED_8(_x) VERIFY_ALIGNED(_x, 8)
#define ASSERT_ALIGNED_16(_x) ASSERT_ALIGNED(_x, 16)
#define VERIFY_ALIGNED_16(_x) VERIFY_ALIGNED(_x, 16)
#define ASSERT_ALIGNED_32(_x) ASSERT_ALIGNED(_x, 32)
#define VERIFY_ALIGNED_32(_x) VERIFY_ALIGNED(_x, 32)
#define ASSERT_ALIGNED_64(_x) ASSERT_ALIGNED(_x, 64)
#define VERIFY_ALIGNED_64(_x) VERIFY_ALIGNED(_x, 64)

#define IS_POW2(_x) (((_x) > 0) && ((((_x) & ((_x)-1)) == 0)))
#define ASSERT_POW2(_x) ASSERT_MSG(IS_POW2(_x), "'" #_x "' is not a power of 2")
#define VERIFY_POW2(_x) VERIFY_MSG(IS_POW2(_x), "'" #_x "' is not a power of 2")

#define MEMBER_OFS(_type, _member) ((int)(uintptr_t)(&(((_type *)0)->_member)))

/*
===============================================================================
Common
===============================================================================
*/
/// Immutable block of continuous memory
typedef struct const_mem_region_t {
    /// Data
    union {
        /// Data as a void pointer
        const void * ptr;
        /// Data as a byte pointer
        const uint8_t * byte_ptr;
        /// address to the data
        uintptr_t adr;
    };
    /// Size of region in bytes
    size_t size;
} const_mem_region_t;

/// Mutable block of continuous memory
typedef struct mem_region_t {
    /// Data
    union {
        /// Data as an immutable block
        const_mem_region_t consted;
        struct {
            /// Data
            union {
                /// Data as a void pointer
                void * ptr;
                /// Data as a byte pointer
                uint8_t * byte_ptr;
                /// address to the data
                uintptr_t adr;
            };
            /// Size of region in bytes
            size_t size;
        };
    };
} mem_region_t;

#define MEM_REGION(_ptr, _size) \
    (mem_region_t) {            \
        {                       \
            {                   \
                {_ptr}, _size   \
            }                   \
        }                       \
    }

#define CONST_MEM_REGION(_ptr, _size) \
    (const_mem_region_t) {            \
        {_ptr}, _size                 \
    }

PURE static inline int log2i(int i) {
    ASSERT(i > 0);
    return 31 - __builtin_clz(i);
}

int findarg(const char * arg, const int argc, const char * const * const argv);
const char * getargarg(const char * arg, const int argc, const char * const * const argv);

#ifndef NULL
#define NULL ((void *)0)
#endif

// TODO The following collide with third-party headers but accepting their definitions could change
// the meaning of the code, needs to be fixed
#ifndef MIN
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)
#endif
#ifndef MAX
#define MAX(_x, _y) ((_x) > (_y)) ? (_x) : (_y)
#endif
#define CLAMP(_x, _min, _max) ((_x) < (_min)) ? (_min) : (_x > (_max)) ? (_max) : (_x)
#define IMPL_MIN_MAX_CLAMP(_type)                                                                      \
    static inline _type TOKENPASTE(min_, _type)(const _type x, const _type y) {                        \
        return MIN(x, y);                                                                              \
    }                                                                                                  \
    static inline _type TOKENPASTE(max_, _type)(const _type x, const _type y) {                        \
        return MAX(x, y);                                                                              \
    }                                                                                                  \
    static inline _type TOKENPASTE(clamp_, _type)(const _type x, const _type _min, const _type _max) { \
        return CLAMP(x, _min, _max);                                                                   \
    }

IMPL_MIN_MAX_CLAMP(int8_t)
IMPL_MIN_MAX_CLAMP(uint8_t)
IMPL_MIN_MAX_CLAMP(int16_t)
IMPL_MIN_MAX_CLAMP(uint16_t)
IMPL_MIN_MAX_CLAMP(int32_t)
IMPL_MIN_MAX_CLAMP(uint32_t)
IMPL_MIN_MAX_CLAMP(int64_t)
IMPL_MIN_MAX_CLAMP(uint64_t)
IMPL_MIN_MAX_CLAMP(size_t)
IMPL_MIN_MAX_CLAMP(intptr_t)
IMPL_MIN_MAX_CLAMP(uintptr_t)

IMPL_MIN_MAX_CLAMP(char)
IMPL_MIN_MAX_CLAMP(short)
IMPL_MIN_MAX_CLAMP(int)
IMPL_MIN_MAX_CLAMP(float)

#undef MIN
#undef MAX
#undef IMPL_MIN_MAX_CLAMP

#define ZEROMEM(_x) memset(_x, 0, sizeof(*(_x)));
#define ARRAY_SIZE(_x) ((int)(sizeof(_x) / sizeof(_x[0])))

#define PI 3.14159265358979323846264338327950288f
#define RAD2DEG (180.f / PI)
#define DEG2RAD (PI / 180.f)
#define RESTRICT __restrict

#define FOURCC(a, b, c, d) ((uint32_t)(((uint32_t)(a)) + (((uint32_t)(b)) << 8) + (((uint32_t)(c)) << 16) + (((uint32_t)(d)) << 24)))
#define FOURCC_SWAPPED(a, b, c, d) FOURCC(d, c, b, a)

#ifdef _BYTE_ORDER_LE
#define FOURCC_LE(a, b, c, d) FOURCC(a, b, c, d)
#define FOURCC_BE(a, b, c, d) FOURCC_SWAPPED(a, b, c, d)
#else
#define FOURCC_BE(a, b, c, d) FOURCC(a, b, c, d)
#define FOURCC_LE(a, b, c, d) FOURCC_SWAPPED(a, b, c, d)
#endif

#define STRINGIZE_INTERNAL(_x) #_x
#define STRINGIZE(_x) STRINGIZE_INTERNAL(_x)

#define MALLOC_TAG __FILE__ "(" STRINGIZE(__LINE__) ")"

#define LL_ASSERT_INVARIANTS(_msg, _prev, _next, _head, _tail)                                \
    ASSERT_MSG(((_head) && (_tail)) || !((_head) || (_tail)), _msg "Mislinked LL head/tail"); \
    ASSERT_MSG(!(_head) || (!(_head)->_prev), _msg "Mislinked LL head->prev");                \
    ASSERT_MSG(!(_tail) || (!(_tail)->_next), _msg "Mislinked LL tail->next");                \
    ASSERT_MSG(((_head) == (_tail)) || ((_head)->_next), _msg "Mislinked LL head->next");     \
    ASSERT_MSG(((_head) == (_tail)) || ((_tail)->_prev), _msg "Mislinked LL tail->prev")

#define LL_ADD(_item, _prev, _next, _head, _tail)                      \
    LL_ASSERT_INVARIANTS("LL_ADD(pre): ", _prev, _next, _head, _tail); \
    (_item)->_next = NULL;                                             \
    (_item)->_prev = _tail;                                            \
    if (_tail) {                                                       \
        (_tail)->_next = _item;                                        \
        _tail = _item;                                                 \
    } else {                                                           \
        _head = _tail = _item;                                         \
    }                                                                  \
    LL_ASSERT_INVARIANTS("LL_ADD(post): ", _prev, _next, _head, _tail)

#define LL_PUSH_FRONT(_item, _prev, _next, _head, _tail)                      \
    LL_ASSERT_INVARIANTS("LL_PUSH_FRONT(pre): ", _prev, _next, _head, _tail); \
    if (_head) {                                                              \
        (_item)->_next = _head;                                               \
        _head->prev = _item;                                                  \
        _head = _item;                                                        \
    } else {                                                                  \
        LL_ADD(_item, _prev, _next, _head, _tail);                            \
    }                                                                         \
    LL_ASSERT_INVARIANTS("LL_PUSH_FRONT(post): ", _prev, _next, _head, _tail)

#define LL_REMOVE(_item, _prev, _next, _head, _tail)                      \
    LL_ASSERT_INVARIANTS("LL_REMOVE(pre): ", _prev, _next, _head, _tail); \
    if ((_item)->_prev) {                                                 \
        (_item)->_prev->_next = (_item)->_next;                           \
        if ((_item) == (_tail)) {                                         \
            _tail = (_item)->_prev;                                       \
        }                                                                 \
    } else {                                                              \
        _head = (_item)->_next;                                           \
    }                                                                     \
    if ((_item)->_next) {                                                 \
        (_item)->_next->_prev = (_item)->_prev;                           \
        if ((_item) == (_head)) {                                         \
            _head = (_item)->_next;                                       \
        }                                                                 \
    } else {                                                              \
        _tail = (_item)->_prev;                                           \
    }                                                                     \
    (_item)->_next = (_item)->_prev = NULL;                               \
    LL_ASSERT_INVARIANTS("LL_REMOVE(post): ", _prev, _next, _head, _tail)

#define LL_POP_HEAD(_prev, _next, _head, _tail)                             \
    LL_ASSERT_INVARIANTS("LL_POP_HEAD(pre): ", _prev, _next, _head, _tail); \
    if (_head) {                                                            \
        if ((_head)->_next) {                                               \
            _head = (_head)->_next;                                         \
            ASSERT(((_head)->_prev) && ((_head)->_prev->_prev == NULL));    \
            (_head)->_prev->_next = NULL;                                   \
            (_head)->_prev = NULL;                                          \
        } else {                                                            \
            _head = NULL;                                                   \
            _tail = NULL;                                                   \
        }                                                                   \
    }                                                                       \
    LL_ASSERT_INVARIANTS("LL_POP_HEAD(post): ", _prev, _next, _head, _tail)

#define LL_POP_TAIL(_prev, _next, _head, _tail)                             \
    LL_ASSERT_INVARIANTS("LL_POP_TAIL(pre): ", _prev, _next, _head, _tail); \
    if (_tail) {                                                            \
        if ((_tail)->_prev) {                                               \
            _tail = (_tail)->_prev;                                         \
            ASSERT(((_tail)->_next) && ((_tail)->_next->_next == NULL));    \
            (_tail)->_next->_prev = NULL;                                   \
            (_tail)->_next = NULL;                                          \
        } else {                                                            \
            _head = NULL;                                                   \
            _tail = NULL                                                    \
        }                                                                   \
    }                                                                       \
    LL_ASSERT_INVARIANTS("LL_POP_TAIL(post): ", _prev, _next, _head, _tail)

#define LL_IN_LIST(_item, _prev, _head) ((_item)->_prev || ((_head) == (_item)))

/*
===============================================================================
_s function portability
===============================================================================
*/

#if !(defined(_WIN32) || defined(_VADER) || defined(_LEIA))
static inline int strcpy_s(char * dest, size_t dest_size, const char * src) {
    ASSERT(dest != NULL);
    ASSERT((src != NULL) && (strlen(src) < dest_size));

    strcpy(dest, src);
    return 0;
}

static inline int sprintf_s(char * buff, size_t buff_size, const char * fmt, ...) {
    ASSERT((buff != NULL) && (buff_size > 0));

    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buff, buff_size, fmt, args);
    va_end(args);

    ASSERT(ret != -1);
    return ret;
}

static inline int strcat_s(char * dest, size_t dest_size, const char * src) {
    ASSERT(dest != NULL);
    ASSERT((src != NULL) && (strlen(dest) + strlen(src) < dest_size));

    strcat(dest, src);
    return 0;
}
#endif

int sb_vsprintf_s(char * buffer, size_t size, const char * format, va_list argptr);
int sb_vsnprintf_s(char * buffer, size_t size, size_t count, const char * format, va_list argptr);

/// Returns the next power of 2 to an integer
static inline uint32_t next_power_of_2(uint32_t x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return ++x;
}

static inline uint32_t previous_power_of_2(uint32_t x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x - (x >> 1);
}

static inline uint32_t closest_power_of_2(uint32_t x) {
    const uint32_t nx = next_power_of_2(x);
    const uint32_t px = previous_power_of_2(x);
    return (nx - x) > (x - px) ? px : nx;
}

#ifdef __cplusplus
}
#endif
