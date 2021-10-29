/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "extern/wasm3/source/wasm3.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/interpreter/interp_common.h"
#include "source/adk/wasm3/wasm3.h"

extern IM3Environment wasm3_global_environment;
extern IM3Runtime wasm3_global_runtime;
extern IM3Module wasm3_app_module;

IM3Environment m3_NewEnvironment(void);
void m3_FreeEnvironment(IM3Environment i_environment);

IM3Runtime m3_NewRuntime(IM3Environment io_environment, uint32_t i_stackSizeInBytes, void * unused);
void m3_FreeRuntime(IM3Runtime i_runtime);

M3Result m3_ParseModule(IM3Environment i_environment, IM3Module * o_module, const uint8_t * const i_wasmBytes, uint32_t i_numWasmBytes);
M3Result m3_LoadModule(IM3Runtime io_runtime, IM3Module io_module);

M3Result m3_FindFunction(IM3Function * o_function, IM3Runtime i_runtime, const char * const i_functionName);
M3Result m3_LinkRawFunction(IM3Module io_module, const char * const i_moduleName, const char * const i_functionName, const char * const i_signature, M3RawCall i_function);

M3Result m3_CallIntoRunningProgram(IM3Function i_function, void * const ret, uint32_t argc, const void * const * const argv);
M3Result m3_CallByName(IM3Runtime runtime, const char * const name, void * const ret, uint32_t argc, const void * const * const argv);

uint8_t * m3_GetMemoryBase(IM3Runtime runtime);
uint32_t m3_GetMemorySize(IM3Runtime runtime);
uint32_t m3_ConvertNativePtrToWasmPtr(IM3Runtime runtime, uint8_t * const ptr);
uint8_t * m3_ConvertWasmPtrToNativePtr(IM3Runtime runtime, uint32_t ptr);

const char * m3_GetCallStack(IM3Runtime runtime);

M3Result m3_Call_void(IM3Runtime runtime, const char * const name);
M3Result m3_Call_rI(IM3Runtime runtime, const char * const name, uint64_t * const ret);
M3Result m3_Call_ri(IM3Runtime runtime, const char * const name, uint32_t * const ret);
M3Result m3_Call_i(IM3Runtime runtime, const char * const name, const uint32_t arg0);
M3Result m3_Call_ii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1);
M3Result m3_Call_iii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2);
M3Result m3_Call_iiii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3);
M3Result m3_Call_iiiii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3, const uint32_t arg4);
M3Result m3_Call_iI(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1);
M3Result m3_Call_ri_iI(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1);
M3Result m3_Call_Ii(IM3Runtime runtime, const char * const name, const uint64_t arg0, const uint32_t arg1);
M3Result m3_Call_iIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint32_t arg2);
M3Result m3_Call_iIIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3);
M3Result m3_Call_ri_iIIi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3);
M3Result m3_Call_iiIi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3);
M3Result m3_Call_ri_iiIi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3);
M3Result m3_Call_iIIii(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4);
M3Result m3_Call_ifI(IM3Runtime runtime, const char * const name, const uint32_t arg0, const float arg1, const uint64_t arg2);
M3Result m3_Call_ri_ifI(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, const uint64_t arg2);
M3Result m3_Call_ri_iIIii(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4);
M3Result m3_Call_ri_i(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0);
M3Result m3_Call_ip(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1);
M3Result m3_Call_ri_ip(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1);
M3Result m3_Call_pi(IM3Runtime runtime, const char * const name, void * const arg0, const uint32_t arg1);
M3Result m3_Call_ipi(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, const uint32_t arg2);
M3Result m3_Call_ippi(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3);
M3Result m3_Call_ri_ippi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3);
M3Result m3_Call_ippii(IM3Runtime runtime, const char * const name, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4);
M3Result m3_Call_ri_ippii(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4);
M3Result m3_Call_iipi(IM3Runtime runtime, const char * const name, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3);
M3Result m3_Call_ri_iipi(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3);
M3Result m3_Call_ifp(IM3Runtime runtime, const char * const name, const uint32_t arg0, const float arg1, void * const arg2);
M3Result m3_Call_ri_ifp(IM3Runtime runtime, const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, void * const arg2);

const char * m3_GetCallStack(IM3Runtime runtime);

// Loads wasm3 from named file
wasm_memory_region_t load_wasm3(
    const sb_file_directory_e directory,
    const char * const wasm_filename,
    const size_t wasm_low_heap_size,
    const size_t wasm_high_heap_size,
    const size_t alloction_threshold);

// Loads wasm3 via given read function
wasm_memory_region_t load_wasm3_fp(
    void * const file,
    const wasm_fread_t fread_func,
    const size_t wasm_file_content_size,
    const size_t wasm_low_heap_size,
    const size_t wasm_high_heap_size,
    const size_t alloction_threshold);

void unload_wasm3(wasm_memory_region_t wasm_memory);

void wasm3_run_all_linkers(void);
