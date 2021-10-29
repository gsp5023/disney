/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

FFI_THUNK(0x108, void, wasm_test_func_I, (int64_t arg0), {
    wasm_test_func_I(arg0);
})

FFI_THUNK(0x108D, void, wasm_test_func_IF, (int64_t arg0, double arg1), {
    wasm_test_func_IF(arg0, arg1);
})

FFI_THUNK(0x1088, void, wasm_test_func_II, (int64_t arg0, int64_t arg1), {
    wasm_test_func_II(arg0, arg1);
})

FFI_THUNK(0x10888, void, wasm_test_func_III, (int64_t arg0, int64_t arg1, int64_t arg2), {
    wasm_test_func_III(arg0, arg1, arg2);
})

FFI_THUNK(0x1088444A, void, wasm_test_func_IIiiiw, (int64_t arg0, int64_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, FFI_WASM_PTR const arg5), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg5, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg5), "arg5 cannot be NULL");
    wasm_test_func_IIiiiw(arg0, arg1, arg2, arg3, arg4, FFI_PIN_WASM_PTR(arg5));
})

FFI_THUNK(0x1088AA, void, wasm_test_func_IIww, (int64_t arg0, int64_t arg1, FFI_WASM_PTR const arg2, FFI_WASM_PTR const arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_IIww(arg0, arg1, FFI_PIN_WASM_PTR(arg2), FFI_PIN_WASM_PTR(arg3));
})

FFI_THUNK(0x108F, void, wasm_test_func_If, (int64_t arg0, float arg1), {
    wasm_test_func_If(arg0, arg1);
})

FFI_THUNK(0x1084, void, wasm_test_func_Ii, (int64_t arg0, int32_t arg1), {
    wasm_test_func_Ii(arg0, arg1);
})

FFI_THUNK(0x10848, void, wasm_test_func_IiI, (int64_t arg0, int32_t arg1, int64_t arg2), {
    wasm_test_func_IiI(arg0, arg1, arg2);
})

FFI_THUNK(0x10844, void, wasm_test_func_Iii, (int64_t arg0, int32_t arg1, int32_t arg2), {
    wasm_test_func_Iii(arg0, arg1, arg2);
})

FFI_THUNK(0x10844A, void, wasm_test_func_Iiiw, (int64_t arg0, int32_t arg1, int32_t arg2, FFI_WASM_PTR const arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_Iiiw(arg0, arg1, arg2, FFI_PIN_WASM_PTR(arg3));
})

FFI_THUNK(0x1084A, void, wasm_test_func_Iiw, (int64_t arg0, int32_t arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_Iiw(arg0, arg1, FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x108A, void, wasm_test_func_Iw, (int64_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_Iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x108A8A8444, void, wasm_test_func_IwIwIiii, (int64_t arg0, FFI_WASM_PTR const arg1, int64_t arg2, FFI_WASM_PTR const arg3, int64_t arg4, int32_t arg5, int32_t arg6, int32_t arg7), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_IwIwIiii(arg0, FFI_PIN_WASM_PTR(arg1), arg2, FFI_PIN_WASM_PTR(arg3), arg4, arg5, arg6, arg7);
})

FFI_THUNK(0x108AFFA4A, void, wasm_test_func_Iwffwiw, (int64_t arg0, FFI_WASM_PTR const arg1, float arg2, float arg3, FFI_WASM_PTR const arg4, int32_t arg5, FFI_WASM_PTR const arg6), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg6, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg6), "arg6 cannot be NULL");
    wasm_test_func_Iwffwiw(arg0, FFI_PIN_WASM_PTR(arg1), arg2, arg3, FFI_PIN_WASM_PTR(arg4), arg5, FFI_PIN_WASM_PTR(arg6));
})

FFI_THUNK(0x108AFFAA4A, void, wasm_test_func_Iwffwwiw, (int64_t arg0, FFI_WASM_PTR const arg1, float arg2, float arg3, FFI_WASM_PTR const arg4, FFI_WASM_PTR const arg5, int32_t arg6, FFI_WASM_PTR const arg7), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg5, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg5), "arg5 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg7, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg7), "arg7 cannot be NULL");
    wasm_test_func_Iwffwwiw(arg0, FFI_PIN_WASM_PTR(arg1), arg2, arg3, FFI_PIN_WASM_PTR(arg4), FFI_PIN_WASM_PTR(arg5), arg6, FFI_PIN_WASM_PTR(arg7));
})

FFI_THUNK(0x108A4, void, wasm_test_func_Iwi, (int64_t arg0, FFI_WASM_PTR const arg1, int32_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_Iwi(arg0, FFI_PIN_WASM_PTR(arg1), arg2);
})

FFI_THUNK(0x108AA, void, wasm_test_func_Iww, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_Iww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x108AA4, void, wasm_test_func_Iwwi, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2, int32_t arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_Iwwi(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2), arg3);
})

FFI_THUNK(0x108AA4A, void, wasm_test_func_Iwwiw, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2, int32_t arg3, FFI_WASM_PTR const arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    wasm_test_func_Iwwiw(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2), arg3, FFI_PIN_WASM_PTR(arg4));
})

FFI_THUNK(0x108AAA, void, wasm_test_func_Iwww, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2, FFI_WASM_PTR const arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_Iwww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2), FFI_PIN_WASM_PTR(arg3));
})

FFI_THUNK(0x108AAAA, void, wasm_test_func_Iwwww, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2, FFI_WASM_PTR const arg3, FFI_WASM_PTR const arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    wasm_test_func_Iwwww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2), FFI_PIN_WASM_PTR(arg3), FFI_PIN_WASM_PTR(arg4));
})

FFI_THUNK(0x10F, void, wasm_test_func_f, (float arg0), {
    wasm_test_func_f(arg0);
})

FFI_THUNK(0x10FFFF, void, wasm_test_func_ffff, (float arg0, float arg1, float arg2, float arg3), {
    wasm_test_func_ffff(arg0, arg1, arg2, arg3);
})

FFI_THUNK(0x104, void, wasm_test_func_i, (int32_t arg0), {
    wasm_test_func_i(arg0);
})

FFI_THUNK(0x1048, void, wasm_test_func_iI, (int32_t arg0, int64_t arg1), {
    wasm_test_func_iI(arg0, arg1);
})

FFI_THUNK(0x104F, void, wasm_test_func_if, (int32_t arg0, float arg1), {
    wasm_test_func_if(arg0, arg1);
})

FFI_THUNK(0x1044, void, wasm_test_func_ii, (int32_t arg0, int32_t arg1), {
    wasm_test_func_ii(arg0, arg1);
})

FFI_THUNK(0x10444, void, wasm_test_func_iii, (int32_t arg0, int32_t arg1, int32_t arg2), {
    wasm_test_func_iii(arg0, arg1, arg2);
})

FFI_THUNK(0x1044A, void, wasm_test_func_iiw, (int32_t arg0, int32_t arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_iiw(arg0, arg1, FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x1044AA, void, wasm_test_func_iiww, (int32_t arg0, int32_t arg1, FFI_WASM_PTR const arg2, FFI_WASM_PTR const arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_iiww(arg0, arg1, FFI_PIN_WASM_PTR(arg2), FFI_PIN_WASM_PTR(arg3));
})

FFI_THUNK(0x104A, void, wasm_test_func_iw, (int32_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x104A4, void, wasm_test_func_iwi, (int32_t arg0, FFI_WASM_PTR const arg1, int32_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_iwi(arg0, FFI_PIN_WASM_PTR(arg1), arg2);
})

FFI_THUNK(0x104AA, void, wasm_test_func_iww, (int32_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_iww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0xD08, double, wasm_test_func_rF_I, (int64_t arg0), {
    return wasm_test_func_rF_I(arg0);
})

FFI_THUNK(0x80, int64_t, wasm_test_func_rI, (), {
    return wasm_test_func_rI();
})

FFI_THUNK(0x808, int64_t, wasm_test_func_rI_I, (int64_t arg0), {
    return wasm_test_func_rI_I(arg0);
})

FFI_THUNK(0x80888, int64_t, wasm_test_func_rI_III, (int64_t arg0, int64_t arg1, int64_t arg2), {
    return wasm_test_func_rI_III(arg0, arg1, arg2);
})

FFI_THUNK(0x808F4, int64_t, wasm_test_func_rI_Ifi, (int64_t arg0, float arg1, int32_t arg2), {
    return wasm_test_func_rI_Ifi(arg0, arg1, arg2);
})

FFI_THUNK(0x80844, int64_t, wasm_test_func_rI_Iii, (int64_t arg0, int32_t arg1, int32_t arg2), {
    return wasm_test_func_rI_Iii(arg0, arg1, arg2);
})

FFI_THUNK(0x8084444, int64_t, wasm_test_func_rI_Iiiii, (int64_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4), {
    return wasm_test_func_rI_Iiiii(arg0, arg1, arg2, arg3, arg4);
})

FFI_THUNK(0x8084A, int64_t, wasm_test_func_rI_Iiw, (int64_t arg0, int32_t arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_rI_Iiw(arg0, arg1, FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x808A, int64_t, wasm_test_func_rI_Iw, (int64_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_rI_Iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x808AA, int64_t, wasm_test_func_rI_Iww, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_rI_Iww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x804, int64_t, wasm_test_func_rI_i, (int32_t arg0), {
    return wasm_test_func_rI_i(arg0);
})

FFI_THUNK(0x804A, int64_t, wasm_test_func_rI_iw, (int32_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_rI_iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x804AA, int64_t, wasm_test_func_rI_iww, (int32_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_rI_iww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x80A, int64_t, wasm_test_func_rI_w, (FFI_WASM_PTR const arg0), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_rI_w(FFI_PIN_WASM_PTR(arg0));
})

FFI_THUNK(0x80A88A8444, int64_t, wasm_test_func_rI_wIIwIiii, (FFI_WASM_PTR const arg0, int64_t arg1, int64_t arg2, FFI_WASM_PTR const arg3, int64_t arg4, int32_t arg5, int32_t arg6, int32_t arg7), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    return wasm_test_func_rI_wIIwIiii(FFI_PIN_WASM_PTR(arg0), arg1, arg2, FFI_PIN_WASM_PTR(arg3), arg4, arg5, arg6, arg7);
})

FFI_THUNK(0x80A4, int64_t, wasm_test_func_rI_wi, (FFI_WASM_PTR const arg0, int32_t arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_rI_wi(FFI_PIN_WASM_PTR(arg0), arg1);
})

FFI_THUNK(0x80A48, int64_t, wasm_test_func_rI_wiI, (FFI_WASM_PTR const arg0, int32_t arg1, int64_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_rI_wiI(FFI_PIN_WASM_PTR(arg0), arg1, arg2);
})

FFI_THUNK(0x80A44, int64_t, wasm_test_func_rI_wii, (FFI_WASM_PTR const arg0, int32_t arg1, int32_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_rI_wii(FFI_PIN_WASM_PTR(arg0), arg1, arg2);
})

FFI_THUNK(0x80AA8AA, int64_t, wasm_test_func_rI_wwIww, (FFI_WASM_PTR const arg0, FFI_WASM_PTR const arg1, int64_t arg2, FFI_WASM_PTR const arg3, FFI_WASM_PTR const arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    return wasm_test_func_rI_wwIww(FFI_PIN_WASM_PTR(arg0), FFI_PIN_WASM_PTR(arg1), arg2, FFI_PIN_WASM_PTR(arg3), FFI_PIN_WASM_PTR(arg4));
})

FFI_THUNK(0xF0, float, wasm_test_func_rf, (), {
    return wasm_test_func_rf();
})

FFI_THUNK(0xF08FFA4, float, wasm_test_func_rf_Iffwi, (int64_t arg0, float arg1, float arg2, FFI_WASM_PTR const arg3, int32_t arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    return wasm_test_func_rf_Iffwi(arg0, arg1, arg2, FFI_PIN_WASM_PTR(arg3), arg4);
})

FFI_THUNK(0xF04, float, wasm_test_func_rf_i, (int32_t arg0), {
    return wasm_test_func_rf_i(arg0);
})

FFI_THUNK(0x40, int32_t, wasm_test_func_ri, (), {
    return wasm_test_func_ri();
})

FFI_THUNK(0x408, int32_t, wasm_test_func_ri_I, (int64_t arg0), {
    return wasm_test_func_ri_I(arg0);
})

FFI_THUNK(0x4088, int32_t, wasm_test_func_ri_II, (int64_t arg0, int64_t arg1), {
    return wasm_test_func_ri_II(arg0, arg1);
})

FFI_THUNK(0x40884, int32_t, wasm_test_func_ri_IIi, (int64_t arg0, int64_t arg1, int32_t arg2), {
    return wasm_test_func_ri_IIi(arg0, arg1, arg2);
})

FFI_THUNK(0x408844, int32_t, wasm_test_func_ri_IIii, (int64_t arg0, int64_t arg1, int32_t arg2, int32_t arg3), {
    return wasm_test_func_ri_IIii(arg0, arg1, arg2, arg3);
})

FFI_THUNK(0x4084, int32_t, wasm_test_func_ri_Ii, (int64_t arg0, int32_t arg1), {
    return wasm_test_func_ri_Ii(arg0, arg1);
})

FFI_THUNK(0x4084A, int32_t, wasm_test_func_ri_Iiw, (int64_t arg0, int32_t arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_ri_Iiw(arg0, arg1, FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x408A, int32_t, wasm_test_func_ri_Iw, (int64_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_ri_Iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x408A4, int32_t, wasm_test_func_ri_Iwi, (int64_t arg0, FFI_WASM_PTR const arg1, int32_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_ri_Iwi(arg0, FFI_PIN_WASM_PTR(arg1), arg2);
})

FFI_THUNK(0x408A48, int32_t, wasm_test_func_ri_IwiI, (int64_t arg0, FFI_WASM_PTR const arg1, int32_t arg2, int64_t arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_ri_IwiI(arg0, FFI_PIN_WASM_PTR(arg1), arg2, arg3);
})

FFI_THUNK(0x408A44AA, int32_t, wasm_test_func_ri_Iwiiww, (int64_t arg0, FFI_WASM_PTR const arg1, int32_t arg2, int32_t arg3, FFI_WASM_PTR const arg4, FFI_WASM_PTR const arg5), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg5, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg5), "arg5 cannot be NULL");
    return wasm_test_func_ri_Iwiiww(arg0, FFI_PIN_WASM_PTR(arg1), arg2, arg3, FFI_PIN_WASM_PTR(arg4), FFI_PIN_WASM_PTR(arg5));
})

FFI_THUNK(0x408AA, int32_t, wasm_test_func_ri_Iww, (int64_t arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_ri_Iww(arg0, FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x404, int32_t, wasm_test_func_ri_i, (int32_t arg0), {
    return wasm_test_func_ri_i(arg0);
})

FFI_THUNK(0x4044, int32_t, wasm_test_func_ri_ii, (int32_t arg0, int32_t arg1), {
    return wasm_test_func_ri_ii(arg0, arg1);
})

FFI_THUNK(0x4044A, int32_t, wasm_test_func_ri_iiw, (int32_t arg0, int32_t arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    return wasm_test_func_ri_iiw(arg0, arg1, FFI_PIN_WASM_PTR(arg2));
})

FFI_THUNK(0x404A, int32_t, wasm_test_func_ri_iw, (int32_t arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    return wasm_test_func_ri_iw(arg0, FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x40A, int32_t, wasm_test_func_ri_w, (FFI_WASM_PTR const arg0), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_ri_w(FFI_PIN_WASM_PTR(arg0));
})

FFI_THUNK(0x40A48, int32_t, wasm_test_func_ri_wiI, (FFI_WASM_PTR const arg0, int32_t arg1, int64_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_ri_wiI(FFI_PIN_WASM_PTR(arg0), arg1, arg2);
})

FFI_THUNK(0x40A484, int32_t, wasm_test_func_ri_wiIi, (FFI_WASM_PTR const arg0, int32_t arg1, int64_t arg2, int32_t arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_ri_wiIi(FFI_PIN_WASM_PTR(arg0), arg1, arg2, arg3);
})

FFI_THUNK(0x40A44, int32_t, wasm_test_func_ri_wii, (FFI_WASM_PTR const arg0, int32_t arg1, int32_t arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_ri_wii(FFI_PIN_WASM_PTR(arg0), arg1, arg2);
})

FFI_THUNK(0x40A448, int32_t, wasm_test_func_ri_wiiI, (FFI_WASM_PTR const arg0, int32_t arg1, int32_t arg2, int64_t arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    return wasm_test_func_ri_wiiI(FFI_PIN_WASM_PTR(arg0), arg1, arg2, arg3);
})

FFI_THUNK(0x10, void, wasm_test_func_void, (), {
    wasm_test_func_void();
})

FFI_THUNK(0x10A, void, wasm_test_func_w, (FFI_WASM_PTR const arg0), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    wasm_test_func_w(FFI_PIN_WASM_PTR(arg0));
})

FFI_THUNK(0x10A8, void, wasm_test_func_wI, (FFI_WASM_PTR const arg0, int64_t arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    wasm_test_func_wI(FFI_PIN_WASM_PTR(arg0), arg1);
})

FFI_THUNK(0x10A844, void, wasm_test_func_wIii, (FFI_WASM_PTR const arg0, int64_t arg1, int32_t arg2, int32_t arg3), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    wasm_test_func_wIii(FFI_PIN_WASM_PTR(arg0), arg1, arg2, arg3);
})

FFI_THUNK(0x10AF, void, wasm_test_func_wf, (FFI_WASM_PTR const arg0, float arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    wasm_test_func_wf(FFI_PIN_WASM_PTR(arg0), arg1);
})

FFI_THUNK(0x10AFAA4, void, wasm_test_func_wfwwi, (FFI_WASM_PTR const arg0, float arg1, FFI_WASM_PTR const arg2, FFI_WASM_PTR const arg3, int32_t arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    wasm_test_func_wfwwi(FFI_PIN_WASM_PTR(arg0), arg1, FFI_PIN_WASM_PTR(arg2), FFI_PIN_WASM_PTR(arg3), arg4);
})

FFI_THUNK(0x10A484A4444A, void, wasm_test_func_wiIiwiiiiw, (FFI_WASM_PTR const arg0, int32_t arg1, int64_t arg2, int32_t arg3, FFI_WASM_PTR const arg4, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8, FFI_WASM_PTR const arg9), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg9, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg9), "arg9 cannot be NULL");
    wasm_test_func_wiIiwiiiiw(FFI_PIN_WASM_PTR(arg0), arg1, arg2, arg3, FFI_PIN_WASM_PTR(arg4), arg5, arg6, arg7, arg8, FFI_PIN_WASM_PTR(arg9));
})

FFI_THUNK(0x10A48A4444A, void, wasm_test_func_wiIwiiiiw, (FFI_WASM_PTR const arg0, int32_t arg1, int64_t arg2, FFI_WASM_PTR const arg3, int32_t arg4, int32_t arg5, int32_t arg6, int32_t arg7, FFI_WASM_PTR const arg8), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg3, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg3), "arg3 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg8, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg8), "arg8 cannot be NULL");
    wasm_test_func_wiIwiiiiw(FFI_PIN_WASM_PTR(arg0), arg1, arg2, FFI_PIN_WASM_PTR(arg3), arg4, arg5, arg6, arg7, FFI_PIN_WASM_PTR(arg8));
})

FFI_THUNK(0x10A448A, void, wasm_test_func_wiiIw, (FFI_WASM_PTR const arg0, int32_t arg1, int32_t arg2, int64_t arg3, FFI_WASM_PTR const arg4), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    wasm_test_func_wiiIw(FFI_PIN_WASM_PTR(arg0), arg1, arg2, arg3, FFI_PIN_WASM_PTR(arg4));
})

FFI_THUNK(0x10A4A44A, void, wasm_test_func_wiwiiw, (FFI_WASM_PTR const arg0, int32_t arg1, FFI_WASM_PTR const arg2, int32_t arg3, int32_t arg4, FFI_WASM_PTR const arg5), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg5, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg5), "arg5 cannot be NULL");
    wasm_test_func_wiwiiw(FFI_PIN_WASM_PTR(arg0), arg1, FFI_PIN_WASM_PTR(arg2), arg3, arg4, FFI_PIN_WASM_PTR(arg5));
})

FFI_THUNK(0x10A4A4A4444A, void, wasm_test_func_wiwiwiiiiw, (FFI_WASM_PTR const arg0, int32_t arg1, FFI_WASM_PTR const arg2, int32_t arg3, FFI_WASM_PTR const arg4, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8, FFI_WASM_PTR const arg9), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg4, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg4), "arg4 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg9, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg9), "arg9 cannot be NULL");
    wasm_test_func_wiwiwiiiiw(FFI_PIN_WASM_PTR(arg0), arg1, FFI_PIN_WASM_PTR(arg2), arg3, FFI_PIN_WASM_PTR(arg4), arg5, arg6, arg7, arg8, FFI_PIN_WASM_PTR(arg9));
})

FFI_THUNK(0x10AA, void, wasm_test_func_ww, (FFI_WASM_PTR const arg0, FFI_WASM_PTR const arg1), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_ww(FFI_PIN_WASM_PTR(arg0), FFI_PIN_WASM_PTR(arg1));
})

FFI_THUNK(0x10AAF, void, wasm_test_func_wwf, (FFI_WASM_PTR const arg0, FFI_WASM_PTR const arg1, float arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    wasm_test_func_wwf(FFI_PIN_WASM_PTR(arg0), FFI_PIN_WASM_PTR(arg1), arg2);
})

FFI_THUNK(0x10AAA, void, wasm_test_func_www, (FFI_WASM_PTR const arg0, FFI_WASM_PTR const arg1, FFI_WASM_PTR const arg2), {
    FFI_ASSERT_ALIGNED_WASM_PTR(arg0, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg0), "arg0 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg1, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg1), "arg1 cannot be NULL");
    FFI_ASSERT_ALIGNED_WASM_PTR(arg2, const int32_t);
    ASSERT_MSG(FFI_PIN_WASM_PTR(arg2), "arg2 cannot be NULL");
    wasm_test_func_www(FFI_PIN_WASM_PTR(arg0), FFI_PIN_WASM_PTR(arg1), FFI_PIN_WASM_PTR(arg2));
})

