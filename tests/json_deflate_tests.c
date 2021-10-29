/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_tests.c

 unit tests for JSON deflate
*/

#include "extern/cjson/cJSON.h"
#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/json_deflate_tool/json_deflate_tool.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_socket.h"
#include "testapi.h"

#ifdef _WASM3
#include "source/adk/wasm3/wasm3.h"
#include "source/adk/wasm3/wasm3_link.h"
#endif // _WASM3

#ifdef _WAMR
#include "source/adk/wamr/wamr.h"
#include "source/adk/wamr/wamr_link.h"
#endif // _WAMR

#if _RESTRICTED && !(defined(_STB_NATIVE) || defined(_RPI) || defined(_CONSOLE_NATIVE))
#define HAS_DEFLATE_GEN 1
#endif

wasm_interpreter_t * wasm;

enum {
    max_argv = 256,
};

// The command line parameter that will be passed in to set the number of parallel deflates that will be tested.
// Raising this value will require raising the Wasm heap size to avoid Rust OOM panics
const char * const parallel_deflate_count_param = "8";

typedef enum json_deflate_run_tests_for_e {
    json_deflate_run_tests_for_native = 0x1,
    json_deflate_run_tests_for_wasm = 0x2,
    json_deflate_run_tests_for_both = json_deflate_run_tests_for_native | json_deflate_run_tests_for_wasm,
} json_deflate_run_tests_for_e;

static struct {
    mem_region_t json_deflate_region;
    mem_region_t thread_pool_region;
    mem_region_t curl_region;
    mem_region_t fragment_buffers_region;
    thread_pool_t * thread_pool;
} statics;

#define JD_TEST_CASE_FILE(_varname, _suite, _filename, _test_case) \
    char _varname[1024] = {0};                                     \
    sprintf_s(_varname, sizeof(_varname), "tests/rust_json_deflate/json/" _suite "/%s/" _filename, _test_case);

static void copy_file(const char * const target, const char * const source) {
    assert_non_null(target);
    assert_non_null(source);

    VERIFY(sb_create_directory_path(sb_app_config_directory, target));

    sb_file_t * const source_handle = sb_fopen(sb_app_root_directory, source, "rb");
    sb_file_t * const target_handle = sb_fopen(sb_app_config_directory, target, "wb");
    VERIFY(target_handle);
    assert_non_null(source_handle);
    sb_fseek(source_handle, 0, sb_seek_end);
    size_t length = sb_ftell(source_handle);
    sb_fseek(source_handle, 0, sb_seek_set);
    void * buff = calloc(length, sizeof(char));
    TRAP_OUT_OF_MEMORY(buff);
    sb_fread(buff, sizeof(char), length, source_handle);
    sb_fwrite(buff, sizeof(char), length, target_handle);
    free(buff);
    sb_fclose(source_handle);
    sb_fclose(target_handle);
}

static size_t get_file_size_by_filename(const char * const filename) {
    sb_file_t * const file = sb_fopen(sb_app_root_directory, filename, "rb");
    sb_fseek(file, 0, sb_seek_end);
    const size_t file_size = (size_t)sb_ftell(file);
    sb_fclose(file);
    return file_size;
}

static void read_bytes(const char * const filename, const mem_region_t region, const size_t offset) {
    sb_file_t * const file = sb_fopen(sb_app_root_directory, filename, "rb");
    sb_fread(region.byte_ptr + offset, sizeof(char), region.size, file);
    sb_fclose(file);
}

static mem_region_t read_all_bytes(const char * const filename) {
    const size_t file_size = get_file_size_by_filename(filename);
    mem_region_t region = MEM_REGION(.ptr = json_deflate_calloc(file_size, 1), .size = file_size);
    read_bytes(filename, region, 0);
    return region;
}

static void read_line(char * const buffer, const size_t length, sb_file_t * const file) {
    char * buf = buffer;
    while (!sb_feof(file) && length > (uintptr_t)(buf - buffer)) {
        sb_fread(buf, 1, 1, file);
        if (*buf == '\n') {
            *buf = 0;
            buf--;
            if (*buf == '\r') {
                *buf = 0;
            }
            break;
        }
        buf++;
    }
}

static void assert_text_files_equal(const sb_file_directory_e f1_dir, const char * const f1, const char * const f2) {
    assert_non_null(f1);
    assert_non_null(f2);

    sb_file_t * const f1_handle = sb_fopen(f1_dir, f1, "rt");
    sb_file_t * const f2_handle = sb_fopen(sb_app_root_directory, f2, "rt");
    VERIFY(f1_handle);
    VERIFY(f2_handle);

    const size_t max_line = 1024 * 1024;
    char * const f1_line = calloc(max_line, sizeof(char));
    char * const f2_line = calloc(max_line, sizeof(char));
    while (!sb_feof(f1_handle) && !sb_feof(f2_handle)) {
        memset(f1_line, 0, max_line);
        memset(f2_line, 0, max_line);

        read_line(f1_line, max_line, f1_handle);
        read_line(f2_line, max_line, f2_handle);

        VERIFY_MSG(!strcmp(f1_line, f2_line), "Lines do not match:\n%s\n%s\n(%s - %s)\n", f1_line, f2_line, f1, f2);
    }

    free(f1_line);
    free(f2_line);

    if (!sb_feof(f1_handle) || !sb_feof(f2_handle)) {
        TRAP("EOF");
    }

    sb_fclose(f1_handle);
    sb_fclose(f2_handle);
}

static void run_wasm_test_code_event_loop(void) {
    milliseconds_t time;
    milliseconds_t last_time = {0};

    uint32_t running = 1;

    the_app.event_head = the_app.event_tail = NULL;

    while (running) {
        thread_pool_run_completion_callbacks(statics.thread_pool);
        adk_curl_run_callbacks();

        ASSERT_MSG(the_app.event_head == the_app.event_tail, "Not all events handled!");
        sb_tick(PEDANTIC_CAST(const adk_event_t **) & the_app.event_head, PEDANTIC_CAST(const adk_event_t **) & the_app.event_tail);
        ASSERT(the_app.event_head < the_app.event_tail);

        // last event should be time;
        {
            const adk_event_t * time_event = (the_app.event_tail - 1);
            ASSERT(time_event->event_data.type == adk_time_event);
            time = time_event->time;
            if ((last_time.ms == 0) || ((time.ms - last_time.ms) > 1000)) {
                last_time.ms = time.ms - 1;
            }
        }

        --the_app.event_tail;

        const milliseconds_t delta_time = {time.ms - last_time.ms};
        ASSERT(delta_time.ms <= 1000);

        last_time = time;

        ++the_app.fps.num_frames;
        the_app.fps.time.ms += delta_time.ms;

        if (the_app.fps.time.ms >= 1000) {
            const milliseconds_t ms_per_frame = {the_app.fps.time.ms / the_app.fps.num_frames};
            debug_write_line("[%4d] FPS: [%dms/frame]", (ms_per_frame.ms > 0) ? 1000 / ms_per_frame.ms : 1000, ms_per_frame.ms);
            the_app.fps.time.ms = 0;
            the_app.fps.num_frames = 0;
        }

        const wasm_call_result_t tick_call_result = wasm->call_ri_ifp("app_tick", &running, time.ms, delta_time.ms / 1000.f, NULL);
        VERIFY_MSG(!tick_call_result.status, tick_call_result.details);
    }
}

static void run_wasm(const char * const artifact_name, int argc, const char ** argv) {
    const uint32_t wasm_low_heap_size = 16 * 1024 * 1024;
    const uint32_t wasm_high_heap_size = 80 * 1024 * 1024;
    wasm_memory_region_t wasm_memory = wasm->load(sb_app_root_directory, artifact_name, wasm_low_heap_size, wasm_high_heap_size, 100 * 1024);
    VERIFY_MSG(wasm_memory.wasm_mem_region.region.consted.ptr != NULL, "deflate: Failed to load wasm app: %s", artifact_name);

    uint32_t init_ret = 0;
    const wasm_call_result_t init_call_result = wasm->call_ri_argv("__app_init", &init_ret, (uint32_t)argc, argv);
    VERIFY_MSG(!init_call_result.status, "app_init returned a failure value");

    run_wasm_test_code_event_loop();

    uint32_t shutdown_ret = 0;
    const wasm_call_result_t shutdown_call_result = wasm->call_ri("app_shutdown", &shutdown_ret);
    VERIFY_MSG(!shutdown_call_result.status, "app_shutdown returned a failure value");

    wasm->unload(wasm_memory);
}

static void run_wasm_test_code_async(
    const char * const artifact_name,
    const char * const url,
    const char * const test_name,
    const char * const parallel_param,
    const bool perf) {
    if (url != NULL) {
        const char * argv[] = {"--name", test_name, "--from", url, "--parallel", parallel_param};
        run_wasm(artifact_name, ARRAY_SIZE(argv), argv);

// TODO: Uncomment once Leia/Vader have httpx support
#if !defined(_LEIA) && !defined(_VADER)
        {
            // Test HTTPx request deflate
            const char * httpx_argv[] = {"--name", test_name, "--from", url, "--parallel", parallel_param, "--httpx"};
            run_wasm(artifact_name, ARRAY_SIZE(httpx_argv), httpx_argv);
        }

        {
            // Test HTTPx response deflate
            const char * httpx_argv[] = {"--name", test_name, "--from", url, "--parallel", parallel_param, "--httpx-deflate-response"};
            run_wasm(artifact_name, ARRAY_SIZE(httpx_argv), httpx_argv);
        }
#endif
    } else if (perf) {
        const char * argv[] = {"--name", test_name, "--perf"};
        run_wasm(artifact_name, ARRAY_SIZE(argv), argv);
    } else {
        const char * argv[] = {"--name", test_name, "--parallel", parallel_param};
        run_wasm(artifact_name, ARRAY_SIZE(argv), argv);
    }
}

static void json_deflate_test_wasm(
    const char * const output_wasm_file,
    const char * const reference_output_file,
    const char * const wasm_bytecode_file,
    const char * const url,
    const char * const test_name,
    const char * const parallel_param,
    const bool perf) {
    run_wasm_test_code_async(wasm_bytecode_file, url, test_name, parallel_param, perf);

    if (!perf) {
        assert_text_files_equal(sb_app_config_directory, output_wasm_file, reference_output_file);
    }
}

static void json_deflate_test_native(
    const char * const output_native_file,
    const char * const reference_output_file,
    const char * const url,
    const char * const test_name,
    const char * const parallel_param,
    const bool perf) {
#ifdef HAS_DEFLATE_GEN

    char system_args[json_deflate_max_string_length] = {0};
    strcpy_s(
        system_args,
        sizeof(system_args),
        "cargo run --manifest-path tests/rust_json_deflate/Cargo.toml --features test ");

#ifdef NDEBUG
    strcat_s(system_args, sizeof(system_args), " --release ");
#endif

    strcat_s(system_args, sizeof(system_args), " -- --bundle tests/rust_json_deflate/json_deflate_bundle.json ");

    strcat_s(system_args, sizeof(system_args), " -- --headless");

    strcat_s(system_args, sizeof(system_args), " --name ");
    strcat_s(system_args, sizeof(system_args), test_name);

    strcat_s(system_args, sizeof(system_args), " --parallel ");
    strcat_s(system_args, sizeof(system_args), parallel_param);

    if (url) {
        strcat_s(system_args, sizeof(system_args), " --from ");
        strcat_s(system_args, sizeof(system_args), url);
    }

    if (perf) {
        strcat_s(system_args, sizeof(system_args), " --perf ");
    }

    assert_int_equal(system(system_args), 0);

    if (!perf) {
        assert_text_files_equal(sb_app_config_directory, output_native_file, reference_output_file);
    }

    if (url) {
        // Test HTTPx system deflating request and response from URL

        // First, delete previous output file
        sb_delete_file(sb_app_config_directory, output_native_file);

        strcat_s(system_args, sizeof(system_args), " --httpx");
        assert_int_equal(system(system_args), 0);
        assert_text_files_equal(sb_app_config_directory, output_native_file, reference_output_file);

        strcat_s(system_args, sizeof(system_args), "-deflate-response");
        assert_int_equal(system(system_args), 0);
        assert_text_files_equal(sb_app_config_directory, output_native_file, reference_output_file);

        if (!perf) {
            assert_text_files_equal(sb_app_config_directory, output_native_file, reference_output_file);
        }
    }
#endif
}

#ifdef HAS_DEFLATE_GEN
static void infer_test_case_with_options(
    const char * const test_case,
    json_deflate_schema_type_group_t * const groups,
    const int group_count,
    const char * const * const maps,
    const int map_count,
    const json_deflate_infer_options_t * const options) {
    JD_TEST_CASE_FILE(json_data_file, "testcases-infer", "data.json", test_case);
    JD_TEST_CASE_FILE(output_file, "testcases-infer", "schema-output.json", test_case);
    JD_TEST_CASE_FILE(reference_output, "testcases-infer", "schema-reference.json", test_case);

    sb_delete_file(sb_app_config_directory, output_file);

    json_deflate_infer_schema(json_data_file, output_file, groups, group_count, maps, map_count, options);

    assert_text_files_equal(sb_app_root_directory, output_file, reference_output);
}

static void infer_test_case(const char * const test_case, json_deflate_schema_type_group_t * const groups, const int group_count, const char * const * const maps, const int map_count) {
    const json_deflate_infer_options_t options = {0};
    infer_test_case_with_options(test_case, groups, group_count, maps, map_count, &options);
}
#endif

static void deflate_test_case_with_options(const char * const test_case, json_deflate_schema_options_t options) {
    JD_TEST_CASE_FILE(json_schema_file, "testcases", "schema.json", test_case);
    JD_TEST_CASE_FILE(binary_schema_c_file, "testcases", "schema.dat", test_case);
    JD_TEST_CASE_FILE(rust_file, "testcases", "schema.rs", test_case);

#ifdef HAS_DEFLATE_GEN
    options.directory = "schema.dat";
    json_deflate_prepare_schema(json_schema_file, binary_schema_c_file, rust_file, &options);
#endif
}

static void generate_test_case_with_invalid_input() {
    const char * const test_case = "all";

    JD_TEST_CASE_FILE(json_schema_file, "testcases-parse", "schema.json", test_case);
    JD_TEST_CASE_FILE(binary_schema_native_file, "testcases-parse", "schema-bare.dat", test_case);
    JD_TEST_CASE_FILE(rust_file, "testcases-parse", "schema.rs", test_case);

#ifdef HAS_DEFLATE_GEN
    json_deflate_schema_options_t options = {0};
    options.root_name = "Root";
    options.directory = "schema.dat";

    json_deflate_prepare_schema(json_schema_file, binary_schema_native_file, rust_file, &options);
#endif
}

static void test_invalid_input() {
    const char * const test_case = "all";

    JD_TEST_CASE_FILE(binary_schema_native_file, "testcases-parse", "schema-bare.dat", test_case);
    JD_TEST_CASE_FILE(json_data_file, "testcases-parse", "data.json", test_case);

    const mem_region_t layout = read_all_bytes(binary_schema_native_file);
    const size_t json_data_full_size = get_file_size_by_filename(json_data_file);

    const uint32_t schema_hash = *(((uint32_t *)layout.byte_ptr) + 1);
    const uint32_t expected_size = 0x70;

    for (size_t size = 0; size <= json_data_full_size; size++) {
        const mem_region_t json_data = MEM_REGION(.ptr = json_deflate_calloc(size, sizeof(char)), .size = size);

        read_bytes(json_data_file, json_data, 0);

        const mem_region_t target = MEM_REGION(.ptr = json_deflate_calloc(json_data.size * 4, sizeof(char)), .size = json_data.size * 4 * sizeof(char));

        json_deflate_parse_data(layout.consted, json_data.consted, target, json_deflate_parse_target_native, expected_size, schema_hash);

        json_deflate_free(target.ptr);
        json_deflate_free(json_data.ptr);
    }

    json_deflate_free(layout.ptr);
}

static void generate_test_case_without_key_conversion(const char * const test_case) {
    json_deflate_schema_options_t options = {0};
    options.root_name = "Root";
    options.skip_key_conversion = true;
    deflate_test_case_with_options(test_case, options);
}

static void generate_test_case(const char * const test_case) {
    json_deflate_schema_options_t options = {0};
    options.root_name = "Root";
    options.skip_key_conversion = false;
    deflate_test_case_with_options(test_case, options);
}

static void generate_test_case_with_slice(const char * const test_case, const char * const slice) {
    json_deflate_schema_options_t options = {0};
    options.root_name = "Root";
    options.slice = slice;
    deflate_test_case_with_options(test_case, options);
}

#ifdef HAS_DEFLATE_GEN
static void test_infer_groups(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "Alpha";
    groups->length = 2;
    groups->paths[0] = "/alpha";
    groups->paths[1] = "/other-alpha";
    infer_test_case("groups", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_wildcard(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "Alpha";
    groups->length = 1;
    groups->paths[0] = "/*";
    infer_test_case("groups", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_inner(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "Alpha";
    groups->length = 3;
    groups->paths[0] = "/*/alpha-1";
    groups->paths[1] = "/*/alpha-2";
    groups->paths[2] = "/*/alpha-3";
    infer_test_case("groups-inner", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_inner_wildcard(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "Alpha";
    groups->length = 2;
    groups->paths[0] = "/alpha/*";
    groups->paths[1] = "/other-alpha/*";
    infer_test_case("groups-inner", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_inner_wildcards(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "Alpha";
    groups->length = 1;
    groups->paths[0] = "/*/*";
    infer_test_case("groups-inner", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_multi(void ** state) {
    json_deflate_schema_type_group_t groups[2] = {0};
    groups[0].name = "Out";
    groups[0].length = 1;
    groups[0].paths[0] = "/*";
    groups[1].name = "In";
    groups[1].length = 1;
    groups[1].paths[0] = "/*/*";
    infer_test_case("groups-multiple", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_groups_multi_real(void ** state) {
    json_deflate_schema_type_group_t groups[2] = {0};
    groups[0].name = "CollectionConfig";
    groups[0].length = 1;
    groups[0].paths[0] = "/configs/*";
    groups[1].name = "CollectionConfigSets";
    groups[1].length = 1;
    groups[1].paths[0] = "/configs/*/sets/*";
    infer_test_case("groups-multiple-real", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_numbers_integers(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "NumbersGroup";
    groups->length = 2;
    groups->paths[0] = "/numbers-group-1";
    groups->paths[1] = "/numbers-group-2";

    infer_test_case("numbers-integers", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_null_none(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "A";
    groups->length = 1;
    groups->paths[0] = "/*";

    infer_test_case("null-none", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_null_one(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "A";
    groups->length = 1;
    groups->paths[0] = "/*";

    infer_test_case("null-one", groups, ARRAY_SIZE(groups), NULL, 0);
}

static void test_infer_catch_required(void ** state) {
    json_deflate_schema_type_group_t groups[1] = {0};
    groups->name = "A";
    groups->length = 1;
    groups->paths[0] = "/*";

    const json_deflate_infer_options_t options = {
        .catch_required = 1};

    infer_test_case_with_options("catch-required", groups, ARRAY_SIZE(groups), NULL, 0, &options);
}

static void test_infer_no_array_items(void ** state) {
    infer_test_case("no-array-items", NULL, 0, NULL, 0);
}

static void test_infer_maps(void ** state) {
    const char * maps[] = {
        "/a",
        "/b",
        "/c",
        "/e",
        "/f",
        "/g",
        "/h/*",
        "/h/e/*",
    };

    const json_deflate_infer_options_t options = {0};
    infer_test_case_with_options("map", NULL, 0, maps, ARRAY_SIZE(maps), &options);
}

static void test_infer_real_1(void ** state) {
    infer_test_case("real-1", NULL, 0, NULL, 0);
}

static void test_infer_real_2(void ** state) {
    infer_test_case("real-2", NULL, 0, NULL, 0);
}
#endif

static void test_deflate_by_name_with_options(
    const char * const name,
    const json_deflate_run_tests_for_e run_tests_for,
    const char * const url,
    const char * const parallel_param,
    const bool perf) {
    // NOTE: output paths are hard-coded in rust_json_deflate Rust app
    const char output_file_wasm[] = "tests/rust_json_deflate/json/output-wasm.txt";
    const char output_file_native[] = "tests/rust_json_deflate/json/output-native.txt";

#ifndef NDEBUG
    const char wasm_bytecode_file[] = "target/wasm32-unknown-unknown/debug/rust_json_deflate.wasm";
#else
    const char wasm_bytecode_file[] = "target/wasm32-unknown-unknown/release/rust_json_deflate.wasm";
#endif

    JD_TEST_CASE_FILE(reference_output_file, "testcases", "output-reference.txt", name);

    if (run_tests_for & json_deflate_run_tests_for_wasm) {
        json_deflate_test_wasm(output_file_wasm, reference_output_file, wasm_bytecode_file, url, name, parallel_param, perf);
    }

    if (run_tests_for & json_deflate_run_tests_for_native) {
        json_deflate_test_native(output_file_native, reference_output_file, url, name, parallel_param, perf);
    }
}

static void run_test_case(const char * const name) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_both, NULL, "1", false);
}

static void run_test_case_native_only(const char * const name) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_native, NULL, "1", false);
}

static void run_test_case_from_url(const char * const name, const char * const url) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_both, url, "1", false);
}

static void run_test_case_in_parallel(const char * const name) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_both, NULL, parallel_deflate_count_param, false);
}

static void run_test_case_from_url_in_parallel(const char * const name, const char * const url) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_both, url, parallel_deflate_count_param, false);
}

static void run_perf_test_case(const char * const name) {
    test_deflate_by_name_with_options(name, json_deflate_run_tests_for_both, NULL, "1", true);
}

static void run_all_test_cases(void ** state) {
    // To test deflation - generated artifacts are processed via Rust code (see rust_json_deflate)
    // - the output of the deflation is written to a file via Rust and verified here via asserts

    run_test_case("booleans");
    run_test_case("integers");
    run_test_case("numbers");
    run_test_case("strings");
    run_test_case("strings-alt");
    run_test_case("strings-alt-2");
    run_test_case("mismatch");
    run_test_case("keywords");
    run_test_case("snake_keys");
    run_test_case("variants");
    run_test_case("array-in-variant");
    run_test_case("object-in-variant");
    run_test_case("results");
    run_test_case("arrays");
    run_test_case("zst");
    run_test_case("groups");
    run_test_case("groups-collections");
    run_test_case("groups-inner");
    run_test_case("names");
    run_test_case("colon");
    run_test_case("field-name");
    run_test_case("tag-in-boolean");
    run_test_case("tag-in-vector");
    run_test_case("tag-in-boolean-or-vector");
    run_test_case("tag-in-option");
    run_test_case("tag-pool-1");
    run_test_case("wrong-enum");
    run_test_case("map");
    run_test_case("top-array");
    run_test_case("top-map");
    run_test_case("huge");
    run_test_case("real");
    run_test_case("real-2");
    run_test_case("real-2-groupped");
    run_test_case("real-2-groupped-alt");
    run_test_case("real-2-groupped-alt-2");
    run_test_case("real-3");
    run_test_case_native_only("real-4"); // NOTE: wasm3 fails to run this test due to the size of the `println!` formatting code
    run_test_case("keys");
    run_test_case("slice");

    run_test_case_from_url("real-2-groupped-alt-3", "https://prod-static.disney-plus.net/fed-container-configs/prod/connected/connected/3.0.2/output.json");

    test_invalid_input();

    run_test_case_in_parallel("real");
    run_test_case_in_parallel("real-2");
    run_test_case_in_parallel("real-3");
    run_test_case_from_url_in_parallel("real-2-groupped-alt-3", "https://prod-static.disney-plus.net/fed-container-configs/prod/connected/connected/3.0.2/output.json");

    run_perf_test_case("real-3");
}

static int test_json_deflate_setup(void ** state) {
    sb_create_directory_path(sb_app_config_directory, "tests/rust_json_deflate/json/");
    statics.thread_pool_region.size = 256 * 1024;
    statics.thread_pool_region.ptr = malloc(statics.thread_pool_region.size);
    TRAP_OUT_OF_MEMORY(statics.thread_pool_region.ptr);
    statics.thread_pool = thread_pool_emplace_init(statics.thread_pool_region, thread_pool_max_threads, "json_pool_", MALLOC_TAG);

    statics.json_deflate_region.size = 24 * 1024 * 1024;
    statics.json_deflate_region.ptr = malloc(statics.json_deflate_region.size);
    TRAP_OUT_OF_MEMORY(statics.json_deflate_region.ptr);
    json_deflate_init(statics.json_deflate_region, unit_test_guard_page_mode, statics.thread_pool);

    statics.curl_region.size = 2 * 1024 * 1024;
    statics.curl_region.ptr = malloc(statics.curl_region.size);

    statics.fragment_buffers_region.size = 4 * 1024 * 1024;
    statics.fragment_buffers_region.ptr = malloc(statics.fragment_buffers_region.size);
    TRAP_OUT_OF_MEMORY(statics.fragment_buffers_region.ptr);
    adk_curl_api_init(statics.curl_region, statics.fragment_buffers_region, network_pump_fragment_size, network_pump_sleep_period, unit_test_guard_page_mode, adk_curl_http_init_normal);

#ifdef _WASM3
    if (!wasm) {
        wasm = get_wasm3_interpreter();
        extern extender_status_e wasm3_link_adk(void * const m);
        wasm3_register_linker(wasm3_link_adk);
    }
#endif // _WASM3

#ifdef _WAMR
    if (!wasm) {
        wasm = get_wamr_interpreter();
        extern extender_status_e wamr_link_adk(void * const env);
        wamr_register_linker(wamr_link_adk);
    }
#endif // _WAMR

    set_active_wasm_interpreter(wasm);
    ASSERT_MSG(get_active_wasm_interpreter(), "No interpreter selected to run JSON deflate tests");

    return 0;
}

static int test_json_deflate_teardown(void ** state) {
    adk_curl_api_shutdown();
    json_deflate_shutdown();
    thread_pool_shutdown(statics.thread_pool, MALLOC_TAG);

    free(statics.json_deflate_region.ptr);
    free(statics.thread_pool_region.ptr);
    free(statics.curl_region.ptr);

    return 0;
}

static void generate_all_test_cases(void ** state) {
#if !(defined(_STB_NATIVE) || defined(_RPI) || defined(_CONSOLE_NATIVE))
    generate_test_case("booleans");
    generate_test_case("integers");
    generate_test_case("numbers");
    generate_test_case("strings");
    generate_test_case("strings-alt");
    generate_test_case("strings-alt-2");
    generate_test_case("mismatch");
    generate_test_case("keywords");
    generate_test_case("snake_keys");
    generate_test_case("variants");
    generate_test_case("array-in-variant");
    generate_test_case("object-in-variant");
    generate_test_case("results");
    generate_test_case("arrays");
    generate_test_case("zst");
    generate_test_case("groups");
    generate_test_case("groups-collections");
    generate_test_case("groups-inner");
    generate_test_case("names");
    generate_test_case("colon");
    generate_test_case("field-name");
    generate_test_case("tag-in-boolean");
    generate_test_case("tag-in-vector");
    generate_test_case("tag-in-boolean-or-vector");
    generate_test_case("tag-in-option");
    generate_test_case("tag-pool-1");
    generate_test_case("wrong-enum");
    generate_test_case("map");
    generate_test_case("top-array");
    generate_test_case("top-map");
    generate_test_case("huge");
    generate_test_case("real");
    generate_test_case("real-2");
    generate_test_case("real-2-groupped");
    generate_test_case("real-2-groupped-alt");
    generate_test_case("real-2-groupped-alt-2");
    generate_test_case("real-2-groupped-alt-3");
    generate_test_case("real-3");
    generate_test_case("real-4");

    generate_test_case_without_key_conversion("keys");
    generate_test_case_with_slice("slice", "/i/am/root");

    generate_test_case_with_invalid_input();

#ifndef NDEBUG
    assert_int_equal(system("cargo build --manifest-path tests/rust_json_deflate/Cargo.toml --features test --target wasm32-unknown-unknown"), 0);
#else
    assert_int_equal(system("cargo build --manifest-path tests/rust_json_deflate/Cargo.toml --features test --target wasm32-unknown-unknown --release"), 0);
#endif // !NDEBUG
#endif
}

int test_json_deflate() {
    // Specifying the --artifacts CLI option will run only the `generate` test suite (which generates the deflate artifacts for testing)
    const bool artifacts_only = test_findarg("--artifacts") != -1;
    if (artifacts_only) {
        const struct CMUnitTest tests[] = {cmocka_unit_test(generate_all_test_cases)};
        return cmocka_run_group_tests(tests, test_json_deflate_setup, test_json_deflate_teardown);
    }

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(generate_all_test_cases),
        cmocka_unit_test(run_all_test_cases),

#ifdef HAS_DEFLATE_GEN
        cmocka_unit_test(test_infer_groups),
        cmocka_unit_test(test_infer_groups_wildcard),
        cmocka_unit_test(test_infer_groups_inner),
        cmocka_unit_test(test_infer_groups_inner_wildcard),
        cmocka_unit_test(test_infer_groups_inner_wildcards),
        cmocka_unit_test(test_infer_groups_multi),
        cmocka_unit_test(test_infer_groups_multi_real),
        cmocka_unit_test(test_infer_numbers_integers),
        cmocka_unit_test(test_infer_null_none),
        cmocka_unit_test(test_infer_null_one),
        cmocka_unit_test(test_infer_catch_required),
        cmocka_unit_test(test_infer_no_array_items),
        cmocka_unit_test(test_infer_maps),
        cmocka_unit_test(test_infer_real_1),
        cmocka_unit_test(test_infer_real_2),
#endif
    };

    return cmocka_run_group_tests(tests, test_json_deflate_setup, test_json_deflate_teardown);
}
