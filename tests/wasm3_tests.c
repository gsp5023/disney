/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
wasm3_tests.c

integration for running rust tests in wasm3
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/log/log.h"
#include "testapi.h"

#ifdef _WASM3
#include "source/adk/wasm3/wasm3.h"
#include "source/adk/wasm3/wasm3_link.h"
#endif // _WASM3

static void wasm3_unit_test(void ** state) {
#ifdef _WASM3
    wasm_interpreter_t * const wasm3 = get_wasm3_interpreter();
    set_active_wasm_interpreter(wasm3);

    extender_status_e wasm3_link_adk(void * const wasm_app_module);
    extender_status_e wasm3_link_tests(void * const wasm_app_module);
    wasm3_register_linker(wasm3_link_adk);
    wasm3_register_linker(wasm3_link_tests);

    const wasm_memory_region_t region = wasm3->load(sb_app_root_directory, "target/wasm32-unknown-unknown/release/wasm_tests.wasm", 0x03000000);
    VERIFY_MSG(region.wasm_bytecode_size, "Failed to load Wasm file");

    const wasm_call_result_t ffi_test = wasm3->call_i("exercise", 0);
    VERIFY_MSG(!ffi_test.status, ffi_test.details);

    uint64_t ret1 = 0xDEADBEEF;
    const wasm_call_result_t r1 = wasm3->call_rI("test_interpreter_1", &ret1);
    VERIFY_MSG(r1.status == wasm_call_success, r1.details);
    assert_int_equal(ret1, 42);

    uint32_t ret2 = 0;
    const wasm_call_result_t r2 = wasm3->call_ri("test_interpreter_2", &ret2);
    VERIFY_MSG(r2.status == wasm_call_success, r2.details);
    assert_int_not_equal(ret2, 0);

    uint32_t ret3 = 0;
    const wasm_call_result_t r3 = wasm3->call_ri("test_interpreter_panic", &ret3);
    VERIFY(r3.status == wasm_call_unreachable_executed);
    assert_true(strstr(r3.details, "test_interpreter_panic") != NULL);

    adk_clear_wasm_error_and_stack_trace();
    assert_null(adk_get_wasm_error_and_stack_trace());

    uint32_t ret4 = 0;
    const wasm_call_result_t r4 = wasm3->call_ri("invalid_memory_access_example", &ret4);
    VERIFY(r4.status == wasm_call_out_of_bounds_memory_access);
    print_message("%s\n", r4.details);
    assert_int_equal(ret4, 0);
    assert_int_equal(strcmp(r4.details,
                            "[trap] out of bounds memory access\n"
                            "second_nesting_layer\n"
                            "nested_call\n"
                            "invalid_memory_access_example"),
                     0);

    adk_clear_wasm_error_and_stack_trace();
    assert_null(adk_get_wasm_error_and_stack_trace());

    uint32_t ret5 = 0;
    const wasm_call_result_t r5 = wasm3->call_ri("test_out_of_memory", &ret5);
    assert_int_equal(ret5, 0);

    /* NOTE: The string used to identify native Rust functions can change from
     * platform to platform, but the name error string and the user-defined
     * functions should be stable across platforms.  Therefore, we are only
     * testing for error (head of the results) and the user-defined functions
     * of the stack (tail of the results).  */
    const char * const expected_head = "memory allocation failed";
    const char * const expected_tail =
        "stack_depth_2\n"
        "stack_depth_1\n"
        "stack_depth_0\n"
        "test_out_of_memory";
    assert_int_equal(strncmp(r5.details, expected_head, strlen(expected_head)), 0);
    assert_int_equal(strcmp(r5.details + (strlen(r5.details) - strlen(expected_tail)), expected_tail), 0);

    adk_clear_wasm_error_and_stack_trace();
    assert_null(adk_get_wasm_error_and_stack_trace());
#endif // _WASM3
}

static int wasm3_setup(void ** state) {
    return 0;
}

static int wasm3_teardown(void ** state) {
    return 0;
}

int test_wasm3() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(wasm3_unit_test)};

    return cmocka_run_group_tests(tests, wasm3_setup, wasm3_teardown);
}
