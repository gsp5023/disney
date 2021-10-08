/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
file_tests.c

file library test fixture
*/

#include "source/adk/file/file.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/private/file.h"
#include "source/adk/steamboat/sb_file.h"
#include "testapi.h"

#include <stdio.h>

// Printed message types
#define MT_I "info:     "
#define MT_T "testing:  "

enum {
    rand_string_len = 4,
    rand_string_buff = rand_string_len + 2,
    adk_file_max_bytes_per_second = 1024 * 1024,
};

extern const adk_api_t * api;

// only reason build is appended here is so we don't have to perform special edits and can use runtime_win32.c values..
// aka we can use our normal program defaults.
static const char test_sub_directory[] = "build/file_test_dir";
static const char directory_test_sub_directory[] = "directory_tests";
static bool test_prefix_set = false;

static void fill_rand_string(char buff[rand_string_buff]) {
    const int min = 1 << ((rand_string_len - 1) * 4);
    const int max = 1 << (rand_string_len * 4);
    const int rand_num = rand_int(min, max);
    sprintf_s(buff, rand_string_buff, "%04X", rand_num);
}

static void print_message_with_dir_and_file(const char * const msg, const sb_file_directory_e directory, const char * const file) {
    char buff[sb_max_path_length];
    sprintf_s(buff, sb_max_path_length, "%s/%s", adk_get_file_directory_path(directory), file);
    print_message(msg, buff);
}

static void cleanup_previous_test(const sb_file_directory_e directory) {
    (void)sb_delete_directory(directory, "");
}

static void test_adk_is_valid_sub_path() {
    print_message(MT_T "adk_is_valid_sub_path()\n");
    assert_false(adk_is_valid_sub_path("\\"));
    assert_false(adk_is_valid_sub_path("some_path/that_is_nearly\\right.txt"));
    assert_false(adk_is_valid_sub_path("path/this%cannot:be_a<_actual>file_sanely\"|?.test"));
    assert_false(adk_is_valid_sub_path("/"));
    assert_false(adk_is_valid_sub_path(":/"));
    assert_false(adk_is_valid_sub_path("/some_path/asd.txt"));

    // white space check
    assert_false(adk_is_valid_sub_path("/some_path/ that has white spaces at the start/           lots of white space/[file].txt"));

    assert_true(adk_is_valid_sub_path("other_paths/some folder/file.txt"));
    assert_true(adk_is_valid_sub_path("long_path/with_lots of/various sub dirs/of questionable  spacing/$file.dat"));
}

static void test_non_existent_file(const sb_file_directory_e directory) {
    static const char this_file_should_not_exist[] = "this file does not exist 123454123143ADFSFD.txt";

    print_message_with_dir_and_file(MT_T "non existent file read: %s\n", directory, this_file_should_not_exist);

    sb_file_t * const file = sb_fopen(directory, this_file_should_not_exist, "rb");
    assert_null(file);

    // if by some case someone actually makes the above file..
    sb_fclose(file);
}

static void test_file_create_and_deletes(const sb_file_directory_e directory, const int num_files) {
    char file_name[rand_string_buff];
    for (int i = 0; i < num_files; ++i) {
        fill_rand_string(file_name);

        print_message_with_dir_and_file(MT_T "creation and deletion of file: %s\n", directory, file_name);

        sb_file_t * const file_ptr = sb_fopen(directory, file_name, "wb");
        if (directory == sb_app_root_directory) {
            assert_null(file_ptr);
            continue;
        }
        assert_non_null(file_ptr);

        assert_true(sb_fclose(file_ptr));

        assert_true(sb_delete_file(directory, file_name));

        sb_file_t * const file_ptr2 = sb_fopen(directory, file_name, "rb");
        assert_null(file_ptr2);

        sb_fclose(file_ptr2);
    }
}

static void test_write_to_file(
    const sb_file_directory_e directory,
    const char * const file_name,
    void * buff,
    const int elem_size,
    const int elem_count) {
    print_message_with_dir_and_file(MT_T "writing to file: %s\n", directory, file_name);
    sb_file_t * const file_ptr = sb_fopen(directory, file_name, "wb");

    assert_non_null(file_ptr);

    assert_int_equal(sb_fwrite(buff, elem_size, elem_count, file_ptr), elem_count);

    assert_true(sb_fclose(file_ptr));
}

static void test_read_from_file(
    const sb_file_directory_e directory,
    const char * const file_name,
    void * buff,
    const int elem_size,
    const int elem_count) {
    print_message_with_dir_and_file(MT_T "reading from file: %s\n", directory, file_name);
    sb_file_t * const file_ptr = sb_fopen(directory, file_name, "rb");
    assert_non_null(file_ptr);

    assert_int_equal(sb_fread(buff, elem_size, elem_count, file_ptr), elem_count);

    assert_true(sb_fclose(file_ptr));
}

static void test_write_large_file(
    const sb_file_directory_e directory,
    const char * const file_name,
    const void * chunk,
    const int elem_size,
    const int elem_count,
    const int num_chunks) {
    const size_t file_size = elem_size * elem_count * num_chunks;

    char msg[128];
    sprintf_s(msg, sizeof(msg), MT_T "writing %zd byte file: %%s\n", file_size);

    print_message_with_dir_and_file(msg, directory, file_name);
    sb_file_t * const file_ptr = sb_fopen(directory, file_name, "wb");
    assert_non_null(file_ptr);

    for (int n = 0; n < num_chunks; ++n) {
        assert_int_equal(sb_fwrite(chunk, elem_size, elem_count, file_ptr), elem_count);
    }

    assert_true(sb_fclose(file_ptr));
}

static void test_read_large_file(
    const sb_file_directory_e directory,
    const char * const file_name,
    const void * chunk,
    const int elem_size,
    const int elem_count,
    const int num_chunks) {
    const size_t file_size = elem_size * elem_count * num_chunks;

    char msg[128];
    sprintf_s(msg, sizeof(msg), MT_T "reading %zd byte file: %%s\n", file_size);

    print_message_with_dir_and_file(msg, directory, file_name);
    sb_file_t * const file_ptr = sb_fopen(directory, file_name, "rb");
    assert_non_null(file_ptr);

    void * buff = calloc(elem_count, elem_size);
    assert_non_null(buff);

    const int chunk_size = elem_count * elem_size;

    for (int n = 0; n < num_chunks; ++n) {
        assert_int_equal(sb_fread(buff, elem_size, elem_count, file_ptr), elem_count);
        assert_true(memcmp(chunk, buff, chunk_size) == 0);
    }

    assert_true(sb_fclose(file_ptr));

    free(buff);
}

static void test_file_write_and_read(const sb_file_directory_e directory, const int num_files) {
    static const int special_secret_num = 0xdeadbeef;
    int special_num_from_read = 0;
    char file_name[rand_string_buff];

    for (int i = 0; i < num_files; ++i) {
        fill_rand_string(file_name);

        print_message_with_dir_and_file(MT_T "write and read of file: %s\n", directory, file_name);

        test_write_to_file(directory, file_name, (void *)&special_secret_num, sizeof(special_secret_num), 1);

        test_read_from_file(directory, file_name, &special_num_from_read, sizeof(special_num_from_read), 1);

        assert_int_equal(special_secret_num, special_num_from_read);

        assert_true(sb_delete_file(directory, file_name));
    }
}

static void test_create_directory(const sb_file_directory_e directory, const int rand_directories_count) {
    // test just directory creation
    print_message_with_dir_and_file(MT_T "no additional directories: %s\n", directory, "");
    const bool directory_create_status = sb_create_directory_path(directory, "");
    if (directory == sb_app_root_directory) {
        assert_false(directory_create_status);
        return;
    } else {
        assert_true(directory_create_status);
    }

    // test creating a normal sub directory. e.g. dir/test
    print_message_with_dir_and_file(MT_T "creation of test group directory: %s\n", directory, directory_test_sub_directory);

    assert_true(sb_create_directory_path(directory, directory_test_sub_directory));

    char file_name[rand_string_buff];

    char dir_name[ARRAY_SIZE(directory_test_sub_directory) + ARRAY_SIZE(file_name) + 1];
    char combined_path[ARRAY_SIZE(dir_name) + ARRAY_SIZE(file_name)];

    // then test all the random names (throwing stuff under /test to ease deleting.. since we don't have rmdir in currently)
    for (int i = 0; i < rand_directories_count; ++i) {
        fill_rand_string(file_name);
        sprintf_s(dir_name, ARRAY_SIZE(dir_name), "%s/%s/", directory_test_sub_directory, file_name);

        print_message_with_dir_and_file(MT_T "directory creation: %s\n", directory, dir_name);
        assert_true(sb_create_directory_path(directory, dir_name));

        // then test directory creation (in the above loop?) for generating random named files.
        sprintf_s(combined_path, ARRAY_SIZE(combined_path), "%s%s", dir_name, file_name);

        print_message_with_dir_and_file(MT_T "directory creation with file specifed: %s\n", directory, combined_path);
        assert_true(sb_create_directory_path(directory, combined_path));
    }
}

static void test_seek_and_tell(const sb_file_directory_e directory, const int num_files) {
    for (int i = 0; i < num_files; ++i) {
        static const char some_test_str[] = "file_seek_test_with_not_just_int_length";

        char file_name[rand_string_buff];
        fill_rand_string(file_name);

        print_message_with_dir_and_file(MT_T "seek and tell of file: %s\n", directory, file_name);

        assert_true(sb_create_directory_path(directory, file_name));

        sb_file_t * file = sb_fopen(directory, file_name, "wb");

        assert_int_equal(sb_ftell(file), 0);

        assert_true(sb_fseek(file, 0, sb_seek_end));

        assert_int_equal(sb_ftell(file), 0);

        // write to a file.
        const size_t write_len = sb_fwrite(some_test_str, sizeof(some_test_str[0]), ARRAY_SIZE(some_test_str), file);
        assert_int_equal(write_len, ARRAY_SIZE(some_test_str));

        // seek to end
        assert_true(sb_fseek(file, 0, sb_seek_end));

        // tell to determine length
        assert_int_equal(sb_ftell(file), ARRAY_SIZE(some_test_str));

        sb_fclose(file);
        file = sb_fopen(directory, file_name, "rb");

        // seek out of bounds
        sb_fseek(file, 100000, sb_seek_end);

        int read_fail_num = 0;
        size_t read_count = sb_fread(&read_fail_num, sizeof(read_fail_num), 1, file);

        assert_true((read_count == 0) && sb_feof(file));

        sb_fseek(file, -100000, sb_seek_set);

        read_count = sb_fread(&read_fail_num, sizeof(read_fail_num), 1, file);

        assert_true((read_count == 0) && sb_feof(file));

        // close and delete delete file
        assert_true(sb_fclose(file));

        assert_true(sb_delete_file(directory, file_name));
    }
}

static void append_test_folder_to_directories(const sb_file_directory_e directory) {
    char buff[sb_max_path_length];

    sprintf_s(buff, sb_max_path_length, "%s/%s", adk_get_file_directory_path(directory), test_sub_directory);

    adk_set_directory_path(directory, buff);
}

static void remove_test_folder_prefix(const sb_file_directory_e directory) {
    char buff[sb_max_path_length];

    const char * dir = adk_get_file_directory_path(directory);
    sprintf_s(buff, sb_max_path_length, "%.*s", (int)(strlen(dir) - strlen(test_sub_directory) - 1), dir);

    adk_set_directory_path(directory, buff);
}

static void directory_deletion_test() {
    print_message("deleting [sb_app_cache_directory] recursively\n");
    sb_directory_delete_error_e status = sb_delete_directory(sb_app_cache_directory, NULL);
    assert_int_equal(status, sb_directory_delete_success);

    status = sb_delete_directory(sb_app_cache_directory, NULL);
    assert_int_equal(status, sb_directory_delete_success);

    print_message("attempting to delete something in a read only directory\n");
    status = sb_delete_directory(sb_app_root_directory, test_sub_directory);
    assert_int_equal(status, sb_directory_delete_invalid_input);

    print_message("attempting invalid deletions\n");
    status = sb_delete_directory(sb_app_cache_directory, "..");
    assert_int_equal(status, sb_directory_delete_invalid_input);

    status = sb_delete_directory(sb_app_root_directory, NULL);
    assert_int_equal(status, sb_directory_delete_invalid_input);

    status = sb_delete_directory(sb_app_root_directory, "");
    assert_int_equal(status, sb_directory_delete_invalid_input);
}

static void directory_iterator_test() {
    sb_directory_t * const dir = sb_open_directory(sb_app_root_directory, "");
    while (true) {
        const sb_read_directory_result_t read_result = sb_read_directory(dir);
        if (read_result.entry_type == sb_directory_entry_null) {
            break;
        }
        const char * const entry_name = read_result.entry ? sb_get_directory_entry_name(read_result.entry) : NULL;
        switch (read_result.entry_type) {
            case sb_directory_entry_file:
                print_message("found file: [%s]\n", entry_name);
                break;
            case sb_directory_entry_directory:
                print_message("found directory: [%s]\n", entry_name);
                break;
            case sb_directory_entry_sym_link:
                print_message("found sym-link: [%s]\n", entry_name);
                break;
            case sb_directory_entry_unknown:
                print_message("found unknown file type: [%s]\n", entry_name);
                break;
            default:
                break;
        }
    }
    sb_close_directory(dir);

    const sb_stat_result_t result = sb_stat(sb_app_root_directory, ".gitignore");
    VERIFY(result.error == sb_stat_success);
    print_message("\n.gitignore stat:\n");
    print_message(
        "\tmode:                 [%s%s%s%s]\n",
        result.stat.mode & sb_file_mode_readable ? "readable " : "",
        result.stat.mode & sb_file_mode_writable ? "writable " : "",
        result.stat.mode & sb_file_mode_executable ? "executable " : "",
        result.stat.mode & sb_file_mode_directory ? "directory" : (result.stat.mode & sb_file_mode_regular_file ? "file" : "sym_link"));

    print_message("\taccess_time:          [%" PRIu64 "]\n", result.stat.access_time_s);
    print_message("\tmodification_time_s:  [%" PRIu64 "]\n", result.stat.modification_time_s);
    print_message("\tcreate_time_s:        [%" PRIu64 "]\n", result.stat.create_time_s);

    print_message("\thard_links:           [%" PRIu32 "]\n", result.stat.hard_links);
    print_message("\tsize:                 [%" PRIu64 "]\n", result.stat.size);
    print_message("\tuser_id:              [%" PRIu32 "]\n", result.stat.user_id);
    print_message("\tgroup_id:             [%" PRIu32 "]\n", result.stat.group_id);
}

static void write_limit_test() {
    adk_file_set_write_limit(adk_file_max_bytes_per_second);

    sb_file_t * const fp = adk_fopen(sb_app_config_directory, "write_limit_file", "wb");
    assert_non_null(fp);
    const char write_buff[adk_file_max_bytes_per_second] = {0};
    {
        const adk_fwrite_result_t write_result = adk_fwrite(write_buff, 1, ARRAY_SIZE(write_buff), fp);

        assert_int_equal(write_result.error, adk_fwrite_success);
        assert_int_equal(write_result.elements_written, ARRAY_SIZE(write_buff));
    }
    {
        const adk_fwrite_result_t write_result = adk_fwrite(write_buff, 1, 100, fp);
        assert_int_equal(write_result.error, adk_fwrite_error_rate_limited);
        assert_int_equal(write_result.elements_written, 0);
    }
    {
        const int32_t bytes_to_drain = 100;
        adk_file_write_limit_drain(100);
        const int32_t elem_size = 4;
        const int32_t expected = bytes_to_drain / elem_size;
        const adk_fwrite_result_t write_result = adk_fwrite(write_buff, elem_size, bytes_to_drain, fp);
        assert_int_equal(write_result.error, adk_fwrite_success);
        assert_int_equal(write_result.elements_written, expected);
    }
    {
        const adk_fwrite_result_t write_result = adk_fwrite(write_buff, adk_file_max_bytes_per_second + 1, 1, fp);
        assert_int_equal(write_result.error, adk_fwrite_error_element_too_large_for_rate_limit);
        assert_int_equal(write_result.elements_written, 0);
    }
    adk_fclose(fp);
    adk_file_set_write_limit(0);
}

static void file_unit_test(void ** state) {
    directory_iterator_test();
    test_adk_is_valid_sub_path();

    const int num_files_to_test = 20;
    static const sb_file_directory_e directories[] = {sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory};

    static const sb_file_directory_e writable_directories[] = {sb_app_config_directory, sb_app_cache_directory};

    // append the test folder before doing the cleanup for previous tests so folders not belonging to this test aren't deleted
    for (int i = 0; i < ARRAY_SIZE(directories); ++i) {
        append_test_folder_to_directories(directories[i]);
    }

    test_prefix_set = true;

    for (int i = 0; i < ARRAY_SIZE(writable_directories); ++i) {
        cleanup_previous_test(writable_directories[i]);
    }

    for (int i = 0; i < ARRAY_SIZE(writable_directories); ++i) {
        test_create_directory(writable_directories[i], num_files_to_test);
        test_file_create_and_deletes(writable_directories[i], num_files_to_test);
    }

    for (int i = 0; i < ARRAY_SIZE(writable_directories); ++i) {
        const sb_file_directory_e curr_dir = writable_directories[i];

        test_file_write_and_read(curr_dir, num_files_to_test);
        test_seek_and_tell(curr_dir, num_files_to_test);
    }

    for (int i = 0; i < ARRAY_SIZE(directories); ++i) {
        test_non_existent_file(directories[i]);
    }

    const char large_file_name[] = "large_file.bin";

    const uint32_t elem_count = 8 * 1024; // * sizeof(uint32_t) = 32 KiB
    uint32_t * const chunk = (uint32_t *)calloc(elem_count, sizeof(uint32_t));
    assert_non_null(chunk);
    for (uint32_t w = 0; w < elem_count; ++w) {
        chunk[w] = w;
    }

    // Keeping "large" file kinda small due to STB limits.  For better stress test, size could be passed in through CLI.
    test_write_large_file(sb_app_cache_directory, large_file_name, chunk, sizeof(uint32_t), elem_count, 1);
    test_read_large_file(sb_app_cache_directory, large_file_name, chunk, sizeof(uint32_t), elem_count, 1);
    assert_true(sb_delete_file(sb_app_cache_directory, large_file_name));

    free(chunk);

    write_limit_test();

    directory_deletion_test();

    for (int i = 0; i < ARRAY_SIZE(directories); ++i) {
        remove_test_folder_prefix(directories[i]);
    }

    test_prefix_set = false;
}

static int teardown(void ** s) {
    static const sb_file_directory_e directories[] = {sb_app_root_directory, sb_app_config_directory, sb_app_cache_directory};

    // always clean up test folder prefix to not affect subsequent tests if a failure occurs
    if (test_prefix_set) {
        test_prefix_set = false;

        for (int i = 0; i < ARRAY_SIZE(directories); ++i) {
            remove_test_folder_prefix(directories[i]);
        }
    }

    return 0;
}

int test_files() {
    const struct CMUnitTest tests[] = {cmocka_unit_test_setup_teardown(file_unit_test, NULL, teardown)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
