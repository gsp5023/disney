/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
main.c

ADK tests main entry point
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/hosted_app.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"
#include "testapi.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996) // allow use of strtok in place of strtok_s
#endif

static int _argc;
static const char * const * _argv;

#ifdef _VADER

#include <sys/signal.h>

#endif

int test_findarg(const char * arg) {
    return findarg(arg, _argc, _argv);
}

const char * test_getargarg(const char * arg) {
    return getargarg(arg, _argc, _argv);
}

extern void (*__assert_failed_hook)(const char * const message, const char * const filename, const char * const function, const int line);

static void invoke_mock_assert(const char * const message, const char * const filename, const char * const function, const int line) {
    if (sb_is_main_thread()) {
        mock_assert(0, message, filename, line);
    } else {
        print_error("%s (%d): ASSERT FAILED [%s]\n", filename, line, message);
        abort();
    }
}

void init_http2(bool use_proxy, const char * proxy_address);

int test_algorithms();
int test_bundle();
int test_cache();
int test_canvas();
int test_cncbus();
int test_coredump();
int test_crypto();
int test_events();
int test_extender();
int test_files();
int test_heap();
int test_http();
int test_http2();
int test_httpx();
int test_inputs();
int test_json_deflate();
int test_locale();
int test_log();
int test_manifest();
int test_memory_pool();
int test_metrics();
int test_persona();
int test_pvr();
int test_reporting();
int test_socket();
int test_system_metrics();
int test_text_to_speech();
int test_thread_pool();
int test_uuid();
int test_wamr();
int test_wasm3();
int test_websockets();

#ifdef _RESTRICTED
int test_unwind();
#endif

int test_blank_test() {
    return 0;
}

const system_guard_page_mode_e app_guard_page_mode = unit_test_guard_page_mode;

// just for completing symbols..
int app_main(const int argc, const char * const * const argv) {
    return -1;
}

// Same as above, not used in tests...
void verify_wasm_call_and_halt_on_failure(const struct wasm_call_result_t result) {
    (void)result;
}

const adk_api_t * api;
const char * const adk_app_name = NULL;

typedef struct test_t {
    const char * name;
    int (*function)();
    bool enabled;
    bool failed;
    bool skipped;
} test_t;

#define TEST_IF(NAME, CONDITION) \
    { .name = #NAME, .function = test_##NAME, .enabled = (CONDITION) }

#define TEST(NAME) TEST_IF(NAME, true)

bool tests_toggle_by_name(test_t * const tests, size_t tests_len, const char * names, bool enabled) {
    char * tokens = (char *)names;
    for (char * name = strtok(tokens, ", "); name != NULL; name = strtok(NULL, ", ")) {
        bool found_test = false;
        for (size_t i = 0; i < tests_len; ++i) {
            if (strcmp(tests[i].name, name) == 0) {
                tests[i].enabled = enabled;
                found_test = true;
                break;
            }
        }

        if (!found_test) {
            print_error("Failed to find the test '%s'\n\tExpected one of the following:", name);
            for (size_t i = 0; i < tests_len; ++i) {
                print_error((i == 0) ? " %s" : ", %s", tests[i].name);
            }
            print_error("\n");

            return false;
        }
    }

    return true;
}

int main(const int argc, const char * const * const argv) {
    _argc = argc;
    _argv = argv;

    __assert_failed_hook = invoke_mock_assert;

    log_set_min_level(log_level_warn);

    if (!sb_preinit(argc, argv)) {
        return -1;
    }

    adk_cjson_context_initialize();

    the_app.guard_page_mode = unit_test_guard_page_mode;
    adk_memory_reservations_t mem_reservations = adk_get_default_memory_reservations();
    mem_reservations.low.canvas += 7 * 1024 * 1024; // additional memory required to load CJK fonts
    mem_reservations.high.canvas += 12 * 1024 * 1024;
    mem_reservations.low.json_deflate = 12 * 1024 * 1024; // json deflate tests expect 12mb
    api = adk_init(
        argc,
        argv,
        unit_test_guard_page_mode,
        mem_reservations,
        MALLOC_TAG);

    VERIFY(api);

    the_app.api = api;

    runtime_configuration_t runtime_config = get_default_runtime_configuration();

    the_app.httpx = adk_httpx_api_create(
        the_app.api->mmap.httpx.region,
        the_app.api->mmap.httpx_fragment_buffers.region,
        runtime_config.network_pump_fragment_size,
        the_app.guard_page_mode,
        adk_httpx_init_normal,
        "app-httpx");

    adk_app_metrics_init();

    const bool help = test_findarg("--help") != -1;
    const bool quick = test_findarg("--quick") != -1;
    const bool headless = test_findarg("--headless") != -1;
    const bool input = test_findarg("--input") != -1;
    (void)headless;

    const bool http2_use_proxy = test_findarg("--test_http_proxy") != -1;
    init_http2(http2_use_proxy, http2_use_proxy ? test_getargarg("--test_http_proxy") : NULL);

    test_t tests[] = {
        TEST(algorithms),
        TEST(bundle),
        TEST(cache),
        TEST_IF(canvas, !headless),
        TEST(cncbus),
        TEST(coredump),
        TEST(crypto),
        TEST(events),
        TEST(extender),
        TEST(files),
        TEST_IF(heap, !quick),
        TEST(http),
        TEST(http2),
        TEST(httpx),
        TEST_IF(inputs, input),
        TEST(json_deflate),
        TEST(locale),
        TEST(log),
        TEST(manifest),
        TEST_IF(memory_pool, !quick),
        TEST(metrics),
        TEST(persona),
        TEST(pvr),
        TEST(reporting),
        TEST(socket),
        TEST(system_metrics),
        TEST(text_to_speech),
        TEST(thread_pool),
        TEST(uuid),
        TEST(wamr),
        TEST(wasm3),
        TEST(websockets),

#ifdef _RESTRICTED
        TEST(unwind),
#endif
    };

    const size_t tests_size = ARRAY_SIZE(tests);

    if (help) {
        print_message(
            "m5 unit test runner\n"
            "\n"
            "USAGE:\n"
            "    tests [OPTIONS]\n"
            "\n"
            "OPTIONS:\n"
            "    --headless          Skip tests that use the rendering backend\n"
            "    --run <TESTS>       Run only TESTS (comma-separated list)\n"
            "    --skip <TESTS>      Run all tests but TESTS (comma-separated list)\n"
            "    --help              Prints help information\n"
            "    --test_http_proxy   Run http2 tests with proxy on (requires a proxy server running on 127.0.0.1:8888)\n"
            "    --input             Run input test");

        // Print out a list of the available tests (by name)
        print_message("\n\nTESTS:\n");
        for (size_t i = 0; i < tests_size; ++i) {
            print_message("    %s\n", tests[i].name);
        }
        print_message("\n");

        return 0;
    }

    const char * const includes = test_getargarg("--run");
    const char * const excludes = test_getargarg("--skip");
    if (includes != NULL) {
        for (size_t i = 0; i < tests_size; ++i) {
            tests[i].enabled = false;
        }

        if (!tests_toggle_by_name(tests, tests_size, includes, true)) {
            return -1;
        }
    } else if (excludes != NULL) {
        if (!tests_toggle_by_name(tests, tests_size, excludes, false)) {
            return -1;
        }
    }

    srand(adk_read_millisecond_clock().ms);

    int num_skipped = 0;
    int num_failed = 0;
    int num_completed = 0;
    for (size_t i = 0; i < tests_size; ++i) {
        if (tests[i].enabled) {
            if (tests[i].function()) {
                ++num_failed;
                tests[i].failed = true;
            } else {
                ++num_completed;
            }
        } else {
            tests[i].skipped = true;
            ++num_skipped;
        }
    }

    if (num_skipped > 0) {
        print_message("Skipped[%d]:", num_skipped);
        for (size_t i = 0; i < tests_size; ++i) {
            if (tests[i].skipped) {
                print_message(" %s", tests[i].name);
            }
        }
        print_message("\n");
    }
    if (num_failed > 0) {
        print_message("Failed[%d]:", num_failed);
        for (size_t i = 0; i < tests_size; ++i) {
            if (tests[i].failed) {
                print_message(" %s", tests[i].name);
            }
        }
        print_message("\n");
    }
    if (num_completed > 0) {
        print_message("Completed[%d]:", num_completed);
        for (size_t i = 0; i < tests_size; ++i) {
            if (!tests[i].failed && tests[i].enabled) {
                print_message(" %s", tests[i].name);
            }
        }
        print_message("\n");
    }

    adk_app_metrics_shutdown();
    adk_httpx_api_free(the_app.httpx);
    adk_shutdown(MALLOC_TAG);

    return num_failed ? 1 : 0;
}
