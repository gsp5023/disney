/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "extern/wasm3/source/m3_env.h"
#include "extern/wasm3/source/m3_exception.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/telemetry/telemetry.h"
#include "source/adk/wasm3/private/wasm3.h"

#define WASM3_TAG FOURCC('W', 'S', 'M', '3')

enum {
    functions_buffer_size = 2048,
    stack_trace_buffer_size = functions_buffer_size * 2,
};

static struct {
    char stack_trace_buffer[stack_trace_buffer_size];
} statics;

static inline m3ret_t vectorcall wasm3_call(d_m3OpSig) {
    const m3ret_t possible_trap = m3_Yield();
    if (UNLIKELY(possible_trap)) {
        return possible_trap;
    }

    return ((IM3Operation)(*_pc))(_pc + 1, d_m3OpArgs); // nextOpDirect()
}

/* custom entry point that will pick up the last saved stack pointer when calling from m5 callback */
M3Result m3_CallIntoRunningProgram(IM3Function i_function, void * const ret, uint32_t argc, const void * const * const argv) {
    M3Result result = m3Err_none;

    if (i_function->compiled) {
        IM3Module module = i_function->module;
        IM3Runtime runtime = module->runtime;
        IM3FuncType ftype = i_function->funcType;

        if (argc != ftype->numArgs)
            _throw(m3Err_argumentCountMismatch);

        u64 * stack = runtime->stackPointerAtLastRawFunctionCall
                          ? runtime->stackPointerAtLastRawFunctionCall
                          : runtime->stack;

        for (u32 i = 0; i < ftype->numArgs; ++i) {
            u64 * s = &stack[i];
            const void * const arg = argv[i];
            switch (ftype->argTypes[i]) {
                case c_m3Type_i32:
                    *(i32 *)(s) = *(i32 *)arg;
                    break;
                case c_m3Type_i64:
                    *(i64 *)(s) = *(i64 *)arg;
                    break;
                case c_m3Type_f32:
                    *(f32 *)(s) = *(f32 *)arg;
                    break;
                case c_m3Type_f64:
                    *(f64 *)(s) = *(f64 *)arg;
                    break;
                default:
                    _throw("unknown argument type");
            }
        }

        m3StackCheckInit();

        const bool reentrant_ctx = runtime->ctx != NULL;

        m3_exec_ctx ctx = {0};
        if (!reentrant_ctx) {
            runtime->ctx = &ctx;
        }

        _((M3Result)wasm3_call(i_function->compiled, (m3stack_t)stack, runtime->memory.mallocated, d_m3OpDefaultArgs, runtime->ctx));

        if (!reentrant_ctx) {
            runtime->ctx = NULL;
        }

        switch (ftype->returnType) {
            case c_m3Type_none:
                break;
            case c_m3Type_i32:
                *(i32 *)ret = *(i32 *)(stack);
                break;
            case c_m3Type_i64:
                *(i64 *)ret = *(i64 *)(stack);
                break;
            case c_m3Type_f32:
                *(f32 *)ret = *(f32 *)(stack);
                break;
            case c_m3Type_f64:
                *(f64 *)ret = *(f64 *)(stack);
                break;
            default:
                _throw("unknown return type");
        }

        runtime->stackPointerAtLastRawFunctionCall = stack;
    } else
        _throw(m3Err_missingCompiledCode);

_catch:
    if (result) {
        LOG_ERROR(WASM3_TAG, "%s", result);
    }
    return result;
}

M3Result m3_CallByName(IM3Runtime runtime, const char * const name, void * const ret, uint32_t argc, const void * const * const argv) {
    IM3Function func = NULL;
    M3Result find_function_result = m3_FindFunction(&func, runtime, name);
    if (find_function_result) {
        return find_function_result;
    }

    return m3_CallIntoRunningProgram(func, ret, argc, argv);
}

uint8_t * m3_GetMemoryBase(IM3Runtime runtime) {
    uint32_t size = 0;
    return m3_GetMemory(runtime, &size, 0);
}

uint32_t m3_GetMemorySize(IM3Runtime runtime) {
    uint32_t size = 0;
    m3_GetMemory(runtime, &size, 0);
    return size;
}

uint32_t m3_ConvertNativePtrToWasmPtr(IM3Runtime runtime, uint8_t * const ptr) {
    uint8_t * const base = m3_GetMemoryBase(runtime);

    if (ptr == base) {
        return 0;
    }

    uintptr_t diff = ((uintptr_t)ptr - (uintptr_t)base);
    return (uint32_t)diff;
}

uint8_t * m3_ConvertWasmPtrToNativePtr(IM3Runtime runtime, uint32_t ptr) {
    if (ptr == 0) {
        return NULL;
    }

    uint8_t * const base = m3_GetMemoryBase(runtime);
    return &base[ptr];
}

M3Result m3_Call_void(IM3Runtime runtime, const char * const name) {
    return m3_CallByName(runtime, name, NULL, 0, NULL);
}

M3Result m3_Call_ri(IM3Runtime runtime, const char * const name, uint32_t * const ret) {
    return m3_CallByName(runtime, name, ret, 0, NULL);
}

M3Result m3_Call_rI(IM3Runtime runtime, const char * const name, uint64_t * const ret) {
    return m3_CallByName(runtime, name, ret, 0, NULL);
}

M3Result m3_Call_i(IM3Runtime runtime, const char * const name, const uint32_t arg0) {
    const void * const args[] = {&arg0};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1) {
    const void * const args[] = {&arg0, &arg1};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2) {
    const void * const args[] = {&arg0, &arg1, &arg2};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iiii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iiiii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3, const uint32_t arg4) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3, &arg4};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_i(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0) {
    const void * const args[] = {&arg0};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iI(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1) {
    const void * const args[] = {&arg0, &arg1};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_iI(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1) {
    const void * const args[] = {&arg0, &arg1};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_Ii(IM3Runtime runtime, const char * const name, const uint64_t arg0, const uint32_t arg1) {
    const void * const args[] = {&arg0, &arg1};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint32_t arg2) {
    const void * const args[] = {&arg0, &arg1, &arg2};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iIIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_iIIi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iIIii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3, &arg4};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_iIIii(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3, &arg4};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_iiIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_iiIi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3) {
    const void * const args[] = {&arg0, &arg1, &arg2, &arg3};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ifI(IM3Runtime runtime, const char * const name, const uint32_t arg0, const float arg1, const uint64_t arg2) {
    const void * const args[] = {&arg0, &arg1, &arg2};
    return m3_CallByName(runtime, name, NULL, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ri_ifI(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, const uint64_t arg2) {
    const void * const args[] = {&arg0, &arg1, &arg2};
    return m3_CallByName(runtime, name, ret, ARRAY_SIZE(args), args);
}

M3Result m3_Call_ip(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1) {
    return m3_Call_iI(runtime, name, arg0, (uint64_t)(uintptr_t)arg1);
}

M3Result m3_Call_ri_ip(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1) {
    return m3_Call_ri_iI(runtime, name, ret, arg0, (uint64_t)(uintptr_t)arg1);
}

M3Result m3_Call_pi(IM3Runtime runtime, const char * const name, void * const arg0, const uint32_t arg1) {
    return m3_Call_Ii(runtime, name, (uint64_t)(uintptr_t)arg0, arg1);
}

M3Result m3_Call_ipi(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, const uint32_t arg2) {
    return m3_Call_iIi(runtime, name, arg0, (uint64_t)(uintptr_t)arg1, arg2);
}

M3Result m3_Call_ippi(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3) {
    return m3_Call_iIIi(runtime, name, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

M3Result m3_Call_ri_ippi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3) {
    return m3_Call_ri_iIIi(runtime, name, ret, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

M3Result m3_Call_ippii(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4) {
    return m3_Call_iIIii(runtime, name, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3, arg4);
}

M3Result m3_Call_ri_ippii(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4) {
    return m3_Call_ri_iIIii(runtime, name, ret, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3, arg4);
}

M3Result m3_Call_iipi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3) {
    return m3_Call_iiIi(runtime, name, arg0, arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

M3Result m3_Call_ri_iipi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3) {
    return m3_Call_ri_iiIi(runtime, name, ret, arg0, arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

M3Result m3_Call_ifp(IM3Runtime runtime, const char * const name, const uint32_t arg0, const float arg1, void * const arg2) {
    return m3_Call_ifI(runtime, name, arg0, arg1, (uint64_t)(uintptr_t)arg2);
}

M3Result m3_Call_ri_ifp(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, void * const arg2) {
    return m3_Call_ri_ifI(runtime, name, ret, arg0, arg1, (uint64_t)(uintptr_t)arg2);
}

const char * m3_GetCallStack(IM3Runtime runtime) {
    const m3_exec_ctx * const ctx = runtime->ctx;

    enum {
        functions_buffer_size = 2048
    };

    static char functions[functions_buffer_size + 1];
    memset(functions, 0, sizeof(functions));

    size_t position = 0;

    for (size_t i = ctx->callstack_len; i > 0; i--) {
        if (i < ctx->callstack_len) {
            strcpy(&functions[position], "\x1F");
            position++;
        }

        const size_t index = i - 1;

        const IM3Function func = ctx->callstack[index];
        const cstr_t func_name = GetFunctionName(func);
        const size_t func_name_len = strlen(func_name);

        if (position + func_name_len < sizeof(functions) - 2) {
            strcpy(&functions[position], func_name);
            position += func_name_len;
        } else {
            break;
        }
    }

    return functions;
}

const char * m3_StoreErrorAndStackTrace(IM3Runtime runtime, const char * const wasm3_message) {
    sprintf_s(statics.stack_trace_buffer, ARRAY_SIZE(statics.stack_trace_buffer), "%s\n%s", wasm3_message, m3_GetCallStack(runtime));
    char * curr_stack = statics.stack_trace_buffer;
    while (curr_stack) {
        curr_stack = strchr(curr_stack, '\x1F');
        if (curr_stack) {
            *curr_stack = '\n';
        }
    }
    return statics.stack_trace_buffer;
}

const char * adk_get_wasm_error_and_stack_trace() {
    return *statics.stack_trace_buffer != 0 ? statics.stack_trace_buffer : NULL;
}

void adk_clear_wasm_error_and_stack_trace() {
    ZEROMEM(&statics.stack_trace_buffer);
}

void m3_exec_ctx_push_call(m3_exec_ctx * const ctx, IM3Function function) {
    WASM_FN_TRACE_PUSH(GetFunctionName(function));
    assert(ctx->callstack_len <= M3_CALLSTACK_MAX_DEPTH);
    ctx->callstack[ctx->callstack_len++] = function;
}

void m3_exec_ctx_pop_call(m3_exec_ctx * const ctx) {
    WASM_FN_TRACE_POP();
    ctx->callstack_len -= 1;
}
