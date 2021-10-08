/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 wamr_tests.c

 integration for running rust tests in wamr
 */

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/log/log.h"
#include "testapi.h"

#ifdef _WAMR
#include "source/adk/wamr/wamr.h"
#include "source/adk/wamr/wamr_link.h"
#endif // _WAMR

static void wamr_unit_test(void ** state) {
#ifdef _WAMR
    wasm_interpreter_t * const wamr = get_wamr_interpreter();
    set_active_wasm_interpreter(wamr);

    extender_status_e wamr_link_adk(void * const exec_env);
    extender_status_e wamr_link_tests(void * const wasm_app_module);
    wamr_register_linker(wamr_link_adk);
    wamr_register_linker(wamr_link_tests);

    wamr->load(sb_app_root_directory, "target/wasm32-unknown-unknown/release/wasm_tests.wasm", 0x03000000);

    const wasm_call_result_t ffi_test = wamr->call_i("exercise", 0);
    VERIFY_MSG(!ffi_test.status, ffi_test.details);

    uint64_t ret1 = 0xDEADBEEF;
    const wasm_call_result_t r1 = wamr->call_rI("test_interpreter_1", &ret1);
    VERIFY_MSG(r1.status == wasm_call_success, r1.details);
    assert_int_equal(ret1, 42);

    uint32_t ret2 = 0;
    const wasm_call_result_t r2 = wamr->call_ri("test_interpreter_2", &ret2);
    VERIFY_MSG(r2.status == wasm_call_success, r2.details);
    assert_int_not_equal(ret2, 0);

    uint32_t ret3 = 0;
    const wasm_call_result_t r3 = wamr->call_ri("test_interpreter_panic", &ret3);
    VERIFY(r3.status == wasm_call_unreachable_executed);

    uint32_t ret4 = 0;
    const wasm_call_result_t r4 = wamr->call_ri("invalid_memory_access_example", &ret4);
    VERIFY(r4.status == wasm_call_out_of_bounds_memory_access);

#endif // _WAMR
}

static int wamr_setup(void ** state) {
    return 0;
}

static int wamr_teardown(void ** state) {
    return 0;
}

int test_wamr() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(wamr_unit_test)};

    return cmocka_run_group_tests(tests, wamr_setup, wamr_teardown);
}
