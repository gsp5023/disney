/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

case 0x10:
    RET_VOID(((void(*)())func)());
    break;

case 0x104:
    RET_VOID(((void(*)(int32_t))func)(ARG_I32(0)));
    break;

case 0x108:
    RET_VOID(((void(*)(int64_t))func)(ARG_I64(0)));
    break;

case 0x1084:
    RET_VOID(((void(*)(int64_t, int32_t))func)(ARG_I64(0), ARG_I32(1)));
    break;

case 0x10844:
    RET_VOID(((void(*)(int64_t, int32_t, int32_t))func)(ARG_I64(0), ARG_I32(1), ARG_I32(2)));
    break;

case 0x1088:
    RET_VOID(((void(*)(int64_t, int64_t))func)(ARG_I64(0), ARG_I64(1)));
    break;

case 0x10888:
    RET_VOID(((void(*)(int64_t, int64_t, int64_t))func)(ARG_I64(0), ARG_I64(1), ARG_I64(2)));
    break;

case 0x108F:
    RET_VOID(((void(*)(int64_t, float))func)(ARG_I64(0), ARG_F32(1)));
    break;

case 0x108D:
    RET_VOID(((void(*)(int64_t, double))func)(ARG_I64(0), ARG_F64(1)));
    break;

case 0x108A:
    RET_VOID(((void(*)(int64_t, FFI_WASM_PTR))func)(ARG_I64(0), ARG_WASM_PTR(1)));
    break;

case 0x108AA:
    RET_VOID(((void(*)(int64_t, FFI_WASM_PTR, FFI_WASM_PTR))func)(ARG_I64(0), ARG_WASM_PTR(1), ARG_WASM_PTR(2)));
    break;

case 0x108AAAA:
    RET_VOID(((void(*)(int64_t, FFI_WASM_PTR, FFI_WASM_PTR, FFI_WASM_PTR, FFI_WASM_PTR))func)(ARG_I64(0), ARG_WASM_PTR(1), ARG_WASM_PTR(2), ARG_WASM_PTR(3), ARG_WASM_PTR(4)));
    break;

case 0x10A:
    RET_VOID(((void(*)(FFI_WASM_PTR))func)(ARG_WASM_PTR(0)));
    break;

case 0x408:
    RET_I32(((int32_t(*)(int64_t))func)(ARG_I64(0)));
    break;

case 0x4084:
    RET_I32(((int32_t(*)(int64_t, int32_t))func)(ARG_I64(0), ARG_I32(1)));
    break;

case 0x4084A:
    RET_I32(((int32_t(*)(int64_t, int32_t, FFI_WASM_PTR))func)(ARG_I64(0), ARG_I32(1), ARG_WASM_PTR(2)));
    break;

case 0x4088:
    RET_I32(((int32_t(*)(int64_t, int64_t))func)(ARG_I64(0), ARG_I64(1)));
    break;

case 0x408844:
    RET_I32(((int32_t(*)(int64_t, int64_t, int32_t, int32_t))func)(ARG_I64(0), ARG_I64(1), ARG_I32(2), ARG_I32(3)));
    break;

case 0x408A:
    RET_I32(((int32_t(*)(int64_t, FFI_WASM_PTR))func)(ARG_I64(0), ARG_WASM_PTR(1)));
    break;

case 0x408A4:
    RET_I32(((int32_t(*)(int64_t, FFI_WASM_PTR, int32_t))func)(ARG_I64(0), ARG_WASM_PTR(1), ARG_I32(2)));
    break;

case 0x408A48:
    RET_I32(((int32_t(*)(int64_t, FFI_WASM_PTR, int32_t, int64_t))func)(ARG_I64(0), ARG_WASM_PTR(1), ARG_I32(2), ARG_I64(3)));
    break;

case 0x408AA:
    RET_I32(((int32_t(*)(int64_t, FFI_WASM_PTR, FFI_WASM_PTR))func)(ARG_I64(0), ARG_WASM_PTR(1), ARG_WASM_PTR(2)));
    break;

case 0x40A:
    RET_I32(((int32_t(*)(FFI_WASM_PTR))func)(ARG_WASM_PTR(0)));
    break;

case 0x80:
    RET_I64(((int64_t(*)())func)());
    break;

case 0x808:
    RET_I64(((int64_t(*)(int64_t))func)(ARG_I64(0)));
    break;

case 0x8084444:
    RET_I64(((int64_t(*)(int64_t, int32_t, int32_t, int32_t, int32_t))func)(ARG_I64(0), ARG_I32(1), ARG_I32(2), ARG_I32(3), ARG_I32(4)));
    break;

case 0x80888:
    RET_I64(((int64_t(*)(int64_t, int64_t, int64_t))func)(ARG_I64(0), ARG_I64(1), ARG_I64(2)));
    break;

case 0x80A:
    RET_I64(((int64_t(*)(FFI_WASM_PTR))func)(ARG_WASM_PTR(0)));
    break;

case 0x80A4:
    RET_I64(((int64_t(*)(FFI_WASM_PTR, int32_t))func)(ARG_WASM_PTR(0), ARG_I32(1)));
    break;

case 0x80A48:
    RET_I64(((int64_t(*)(FFI_WASM_PTR, int32_t, int64_t))func)(ARG_WASM_PTR(0), ARG_I32(1), ARG_I64(2)));
    break;

case 0xD08:
    RET_F64(((double(*)(int64_t))func)(ARG_I64(0)));
    break;

