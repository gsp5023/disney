/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
vm_map_tests.c

vm map counts test fixture
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/runtime/hosted_app.h"
#include "source/adk/runtime/memory.h"
#include "testapi.h"

extern const adk_api_t * api;

static int init_cycles;

static int vm_map_test_shutdown() {
    adk_app_metrics_shutdown();
    adk_httpx_client_free(the_app.httpx_client);
    adk_mbedtls_shutdown(MALLOC_TAG);
    adk_shutdown(MALLOC_TAG);
    return 0;
}

static int setup(void ** state) {
    init_cycles = 0;

    // note that shutdown needs to be run during setup because test/main.c
    // initializes the core before running tests, so immediately trying to
    // re-initialize would result in errors
    return vm_map_test_shutdown();
}

static int teardown(void ** state) {
    // On teardown, we replicate the initialization done in test/main.c, thus
    // allowing other tests to run after this one
    return tests_init_core(0, NULL);
}

static void vm_map_stress_test(const int max_cycles) {
    init_cycles++;
    print_message("beginning init cycle (%i)...\n", init_cycles);

    // run a simplified init/shutdown cycle
    for (int i = 0; i < max_cycles; ++i) {
        tests_init_core(0, NULL);
        vm_map_test_shutdown();
    }

    for (int i = 0; i < max_cycles; ++i) {
        // run a full initialization of all subsystems
        sb_preinit(0, NULL);
        app_init_subsystems(get_default_runtime_configuration());

        // run a full shutdown of all subsystems
        app_shutdown_thunk();
    }
}

static void vm_map_test_with_parameters(const int max_cycles, system_guard_page_mode_e guard_pages) {
#ifdef DEBUG_PAGE_MEMORY_SERVICES
    static const char * guard_page_strings[] = {
        "system_guard_page_mode_disabled",
        "system_guard_page_mode_minimal",
        "system_guard_page_mode_enabled"};

    if (guard_pages) {
        print_message("testing: vm maps with guard pages (%s)...\n", guard_page_strings[guard_pages]);
        vm_map_stress_test(max_cycles);
        return;
    }
#endif

    // testing without guard pages enabled
    vm_map_stress_test(max_cycles);
}

static void vm_map_test_variants(const int max_cycles) {
#ifdef DEBUG_PAGE_MEMORY_SERVICES
#ifdef GUARD_PAGE_SUPPORT
    vm_map_test_with_parameters(max_cycles, system_guard_page_mode_enabled);
#endif
    vm_map_test_with_parameters(max_cycles, system_guard_page_mode_minimal);
#endif
    vm_map_test_with_parameters(max_cycles, system_guard_page_mode_disabled);
}

static void vm_map_test(int pool_size) {
    vm_map_test_variants(pool_size);
}

static void vm_map_unit_test(void ** state) {
    int max_cycles = 10;
    vm_map_test(max_cycles);
}

int test_vm_map() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(vm_map_unit_test)};

    return cmocka_run_group_tests(tests, setup, teardown);
}
