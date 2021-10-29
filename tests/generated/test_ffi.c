/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

#include "tests/generated/wasm_ffi.c"
#include "tests/generated/type_assertions.h"
#include "source/adk/interpreter/interp_common.h"

/* Beginning of Wasm3 */

#ifdef _WASM3

#include "source/adk/wasm3/wasm3_link.h"

#define FFI_WASM
#define FFI_WASM_PTR wasm_ptr_t
#define FFI_PIN_WASM_PTR(_x) (wasm_translate_ptr_wasm_to_native(_x))
#define FFI_ASSERT_ALIGNED_WASM_PTR(_x, _t) ASSERT_MSG(IS_ALIGNED((_x).ofs, ALIGN_OF(_t)), "%s (0x%x) requires %zu-byte alignment", #_x, (_x).ofs, ALIGN_OF(_t))
#define FFI_WASM_PTR_OFFSET(_x) ((void *)((uintptr_t)(_x).ofs))
#define FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR(_x) (_x)(((_x) > 0) ? (wasm_translate_ptr_wasm_to_native(_x) : NULL)
#define FFI_SELECT_NATIVE_OR_WASM_VALUE(_native, _wasm) _wasm
#define FFI_BOOL wasm_bool_t
#define FFI_GET_BOOL(_x) (!!(_x).b)
#define FFI_SET_BOOL(_b)  \
    (wasm_bool_t) {       \
        .b = (_b) ? 1 : 0 \
    }
#define FFI_ENUM(_e) int32_t

#define FFI_NATIVE_PTR(_x) native_ptr_t
#define FFI_GET_NATIVE_PTR(_t, _x) ((_t)(uintptr_t)((_x)))
#define FFI_SET_NATIVE_PTR(_x) (uint64_t)(uintptr_t)(_x)

#define FFI_THUNK(_sig, _ret, _name, _args, _body) static _ret TOKENPASTE(_wasm3_thunk_, _name) _args _body
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK

static void * wasm3_dispatch(
    IM3Runtime runtime,
    uint64_t * sp,
    void * mem,
    const char * const name,
    const wasm_sig_mask sig,
    uintptr_t func);

#define FFI_THUNK(_sig, _ret, _name, _args, _body)                                                                        \
    static const void * TOKENPASTE(_wasm3_prethunk_, _name)(IM3Runtime runtime, uint64_t * sp, void * mem) {              \
        WASM_FFI_TRACE_PUSH("ffi_" #_name);                                                                               \
        void * const value = wasm3_dispatch(runtime, sp, mem, #_name, _sig, (uintptr_t)TOKENPASTE(_wasm3_thunk_, _name)); \
        WASM_FFI_TRACE_POP();                                                                                             \
        return value;                                                                                                     \
    }
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#define FFI_ASSERT_ALIGNED_WASM_PTR(_x, _t)
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK

extender_status_e wasm3_link_tests(IM3Module wasm_app_module) {
#define FFI_THUNK(_sig, _ret, _name, _args, _body) \
    wasm3_export_native_function(wasm_app_module, #_name, _sig, TOKENPASTE(_wasm3_prethunk_, _name));
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK
    return extender_status_success;
}

static void * wasm3_dispatch(
    IM3Runtime runtime,
    uint64_t * sp,
    void * mem,
    const char * const name,
    const wasm_sig_mask sig,
    uintptr_t func) {
#define ARG_I32(_i) *(int32_t *)&sp[_i]
#define ARG_I64(_i) *(int64_t *)&sp[_i]
#define ARG_F32(_i) *(float *)&sp[_i]
#define ARG_F64(_i) *(double *)&sp[_i]
#define ARG_WASM_PTR(_i)   \
    (FFI_WASM_PTR) {       \
        .ofs = ARG_I32(_i) \
    }
#define RET_VOID(_v) _v
#define RET_I32(_v) (*(int32_t *)&sp[0]) = _v
#define RET_I64(_v) (*(int64_t *)&sp[0]) = _v
#define RET_F32(_v) (*(float *)&sp[0]) = _v
#define RET_F64(_v) (*(double *)&sp[0]) = _v
#define RET_WASM_PTR(_v) RET_I32(_v)

    switch (sig) {
#include "tests/generated/wasm_dispatcher.h"
        default: {
            char textual_sig[32] = {0};
            wasm_sig(sig, textual_sig);
            TRAP("Signature [%s](0x%X) not implemented -> function: [%s]", textual_sig, sig, name);
        }
    }

    return NULL;
#undef ARG_I32
#undef ARG_I64
#undef ARG_F32
#undef ARG_F64
#undef ARG_WASM_PTR
#undef ARG_NATIVE_PTR
#undef RET_VOID
#undef RET_I32
#undef RET_I64
#undef RET_F32
#undef RET_F64
#undef RET_WASM_PTR
#undef RET_NATIVE_PTR
}

#undef FFI_BOOL
#undef FFI_GET_BOOL
#undef FFI_SET_BOOL
#undef FFI_WASM_PTR
#undef FFI_TYPE
#undef FFI_PIN_WASM_PTR
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#undef FFI_WASM_PTR_OFFSET
#undef FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR
#undef FFI_SELECT_NATIVE_OR_WASM_VALUE
#undef FFI_NATIVE_PTR
#undef FFI_GET_NATIVE_PTR
#undef FFI_SET_NATIVE_PTR
#undef FFI_ENUM
#undef FFI_WASM

#endif // _WASM3

/* End of Wasm3 */

/* Beginning of WAMR */

#ifdef _WAMR

#include "source/adk/wamr/wamr_link.h"

#define FFI_WASM
#define FFI_WASM_PTR wasm_ptr_t
#define FFI_PIN_WASM_PTR(_x) (wasm_translate_ptr_wasm_to_native(_x))
#define FFI_ASSERT_ALIGNED_WASM_PTR(_x, _t) ASSERT_MSG(IS_ALIGNED((_x).ofs, ALIGN_OF(_t)), "%s (0x%x) requires %zu-byte alignment", #_x, (_x).ofs, ALIGN_OF(_t))
#define FFI_WASM_PTR_OFFSET(_x) ((void *)((uintptr_t)(_x).ofs))
#define FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR(_x) (_x)(((_x) > 0) ? (wasm_translate_ptr_wasm_to_native(_x) : NULL)
#define FFI_SELECT_NATIVE_OR_WASM_VALUE(_native, _wasm) _wasm
#define FFI_BOOL wasm_bool_t
#define FFI_GET_BOOL(_x) (!!(_x).b)
#define FFI_SET_BOOL(_b)  \
    (wasm_bool_t) {       \
        .b = (_b) ? 1 : 0 \
    }
#define FFI_ENUM(_e) int32_t

#define FFI_NATIVE_PTR(_x) native_ptr_t
#define FFI_GET_NATIVE_PTR(_t, _x) ((_t)(uintptr_t)((_x)))
#define FFI_SET_NATIVE_PTR(_x) (uint64_t)(uintptr_t)(_x)

#define FFI_THUNK(_sig, _ret, _name, _args, _body) static _ret TOKENPASTE(_wamr_thunk_, _name) _args _body
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK

#undef FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR
#undef FFI_WASM_PTR_OFFSET
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#undef FFI_PIN_WASM_PTR
#define FFI_PIN_WASM_PTR(_x) (_x)
#define FFI_ASSERT_ALIGNED_WASM_PTR(_x, _t)
#define FFI_WASM_PTR_OFFSET(_x) (_x)
#define FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR(_x) (_x)

static void * wamr_dispatch(
    wasm_exec_env_t runtime,
    uint64_t * sp,
    void * mem,
    const char * const name,
    const wasm_sig_mask sig,
    uintptr_t func) {
#define ARG_I32(_i) *(int32_t *)&sp[_i]
#define ARG_I64(_i) *(int64_t *)&sp[_i]
#define ARG_F32(_i) *(float *)&sp[_i]
#define ARG_F64(_i) *(double *)&sp[_i]
#define ARG_WASM_PTR(_i)   \
    (FFI_WASM_PTR) {       \
        .ofs = ARG_I32(_i) \
    }
#define RET_VOID(_v) _v
#define RET_I32(_v) (*(int32_t *)&sp[0]) = _v
#define RET_I64(_v) (*(int64_t *)&sp[0]) = _v
#define RET_F32(_v) (*(float *)&sp[0]) = _v
#define RET_F64(_v) (*(double *)&sp[0]) = _v
#define RET_WASM_PTR(_v) RET_I32(_v)

    switch (sig) {
#include "tests/generated/wasm_dispatcher.h"
    default: {
        char textual_sig[32] = { 0 };
        wasm_sig(sig, textual_sig);
        TRAP("Signature [%s](0x%X) not implemented -> function: [%s]", textual_sig, sig, name);
    }
    }

    return NULL;
#undef ARG_I32
#undef ARG_I64
#undef ARG_F32
#undef ARG_F64
#undef ARG_WASM_PTR
#undef ARG_NATIVE_PTR
#undef RET_VOID
#undef RET_I32
#undef RET_I64
#undef RET_F32
#undef RET_F64
#undef RET_WASM_PTR
#undef RET_NATIVE_PTR
}

#define FFI_THUNK(_sig, _ret, _name, _args, _body)                                                           \
    static const void * TOKENPASTE(_wamr_prethunk_, _name)(wasm_exec_env_t runtime, uint64_t * sp) { \
        return wamr_dispatch(runtime, sp, NULL, #_name, _sig, (uintptr_t)TOKENPASTE(_wamr_thunk_, _name));  \
    }
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK

extender_status_e wamr_link_tests(wasm_exec_env_t env) {
#define FFI_THUNK(_sig, _ret, _name, _args, _body) \
    wamr_export_native_function(env, #_name, _sig, (uintptr_t)TOKENPASTE(_wamr_prethunk_, _name));
#include "tests/generated/ffi_thunks.h"
#undef FFI_THUNK
    return extender_status_success;
}

#undef FFI_BOOL
#undef FFI_GET_BOOL
#undef FFI_SET_BOOL
#undef FFI_WASM_PTR
#undef FFI_TYPE
#undef FFI_PIN_WASM_PTR
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#undef FFI_WASM_PTR_OFFSET
#undef FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR
#undef FFI_SELECT_NATIVE_OR_WASM_VALUE
#undef FFI_NATIVE_PTR
#undef FFI_GET_NATIVE_PTR
#undef FFI_SET_NATIVE_PTR
#undef FFI_ENUM
#undef FFI_WASM

#endif // WAMR

/* End of WAMR */

#ifdef _NATIVE_FFI

typedef struct ffi_ptr_t {
    void * ptr;
} ffi_ptr_t;
typedef struct ffi_bool_t {
    uint8_t b;
} ffi_bool_t;

#define FFI_NATIVE
#define FFI_WASM_PTR ffi_ptr_t
#define FFI_PIN_WASM_PTR(_x) (_x).ptr
#define FFI_ASSERT_ALIGNED_WASM_PTR(_x, _t)
#define FFI_WASM_PTR_OFFSET(_x) FFI_PIN_WASM_PTR(_x)
#define FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR(_x) (_x)
#define FFI_SELECT_NATIVE_OR_WASM_VALUE(_native, _wasm) _native
#define FFI_BOOL ffi_bool_t
#define FFI_GET_BOOL(_x) (!!(_x).b)
#define FFI_SET_BOOL(_b) \
    (ffi_bool_t) {       \
        .b = (_b)        \
    }
#define FFI_NATIVE_PTR(_x) _x
#define FFI_GET_NATIVE_PTR(_t, _x) (_x)
#define FFI_SET_NATIVE_PTR(_x) (_x)
#define FFI_ENUM(_e) _e

#define FFI_THUNK(_sig, _ret, _name, _args, _body) static _ret TOKENPASTE(_ffi_thunk_, _name) _args _body

#include "tests/generated/ffi_thunks.h"

#undef FFI_THUNK
#define FFI_THUNK(_sig, _ret, _name, _args, _body) _ret(*_name) _args;

typedef struct ffi_exports_t {
#include "tests/generated/ffi_thunks.h"
} ffi_exports_t;

#undef FFI_THUNK
#define FFI_THUNK(_sig, _ret, _name, _args, _body) ._name = TOKENPASTE(_ffi_thunk_, _name),

#ifdef _WIN32
DLL_EXPORT const ffi_exports_t test_ffi_exports = {
#include "tests/generated/ffi_thunks.h"
};
DLL_EXPORT const ffi_exports_t * get_test_ffi_exports() {
    return &test_ffi_exports;
}
#else
static const ffi_exports_t exports = {
#include "tests/generated/ffi_thunks.h"
};
DLL_EXPORT const ffi_exports_t * test_ffi_exports = &exports;
DLL_EXPORT const ffi_exports_t * get_test_ffi_exports() {
    return &exports;
}
#endif

#undef FFI_THUNK

#undef FFI_BOOL
#undef FFI_GET_BOOL
#undef FFI_SET_BOOL
#undef FFI_WASM_PTR
#undef FFI_PIN_WASM_PTR
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#undef FFI_WASM_PTR_OFFSET
#undef FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR
#undef FFI_SELECT_NATIVE_OR_WASM_VALUE
#undef FFI_NATIVE_PTR
#undef FFI_GET_NATIVE_PTR
#undef FFI_SET_NATIVE_PTR
#undef FFI_ENUM
#undef FFI_NATIVE

#endif
