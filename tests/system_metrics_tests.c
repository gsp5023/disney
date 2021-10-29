/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 system_metrics_tests.c

 system metrics test fixture
 */

#include "source/adk/runtime/app/app.h"
#include "testapi.h"

static bool is_alphanumeric(const char c) {
    return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9'));
}

static bool is_alphanumeric_with_dashes(const char * const str, const size_t size) {
    for (size_t i = 0; (i < size) && str[i]; ++i) {
        if (!(is_alphanumeric(str[i]) || (str[i] == '-'))) {
            return false;
        }
    }
    return true;
}

static bool is_alphanumeric_with_underscores(const char * const str, const size_t size) {
    for (size_t i = 0; (i < size) && str[i]; ++i) {
        if (!(is_alphanumeric(str[i]) || (str[i] == '_'))) {
            return false;
        }
    }
    return true;
}

void adk_runtime_override_system_metrics(adk_system_metrics_t * const out) {
    strcpy(out->partner, "launchpad");
    strcpy(out->partner_guid, "123");
}

void system_metrics_unit_test(void ** state) {
    adk_system_metrics_t static_system_info = {0};
    adk_get_system_metrics(&static_system_info);

    // TODO: validate that these conform to TRCs

    assert_true(static_system_info.main_memory_mbytes > 0);
    assert_true(static_system_info.video_memory_mbytes > 0);
    assert_true(static_system_info.num_hardware_threads > 0);
    assert_true(static_system_info.num_cores > 0);
    assert_true(static_system_info.device_class > 0);
    assert_true(static_system_info.device_class <= adk_device_class_minature_sbc);

    assert_true(static_system_info.core_version[0]);
    assert_true(static_system_info.config[0]);

    assert_true(static_system_info.vendor[0]);
    assert_true(static_system_info.partner[0]);
    assert_true(static_system_info.device[0]);
    assert_true(static_system_info.firmware[0]);
    assert_true(static_system_info.software[0]);
    assert_true(static_system_info.revision[0]);
    assert_true(static_system_info.gpu[0]);
    assert_true(static_system_info.cpu[0]);
    assert_true(static_system_info.device_id.bytes[0]);
    assert_true(static_system_info.device_region[0]);
    assert_true(static_system_info.tenancy[0]);
    assert_true(static_system_info.partner_guid[0]);
    assert_true(static_system_info.advertising_id[0]);

    print_message("Core Version: %s\n", static_system_info.core_version);
    print_message("Configuration: %s\n", static_system_info.config);

    print_message("Vendor: %s\n", static_system_info.vendor);
    print_message("Partner: %s\n", static_system_info.partner);
    print_message("Device: %s\n", static_system_info.device);
    print_message("Firmware: %s\n", static_system_info.firmware);
    print_message("Software: %s\n", static_system_info.software);
    print_message("Revision: %s\n", static_system_info.revision);
    print_message("GPU: %s\n", static_system_info.gpu);
    print_message("CPU: %s\n", static_system_info.cpu);
    print_message("Device ID: %s\n", (const char *)static_system_info.device_id.bytes);
    print_message("Device Region: %s\n", static_system_info.device_region);
    print_message("Tenancy: %s\n", static_system_info.tenancy);
    print_message("Partner Guid: %s\n", static_system_info.partner_guid);
    print_message("Advertising ID: %s\n", static_system_info.advertising_id);

    print_message("Main Memory Bytes: %d\n", static_system_info.main_memory_mbytes);
    print_message("Video Memory Bytes: %d\n", static_system_info.video_memory_mbytes);
    print_message("Num Hardware Threads: %d\n", static_system_info.num_hardware_threads);
    print_message("Num Cores: %d\n", static_system_info.num_cores);
    print_message("GPU Ready Formats: %d\n", (int)static_system_info.gpu_ready_texture_formats);

    print_message("Persistant Storage Available Bytes: %d\n", static_system_info.persistent_storage_available_bytes);
    print_message("Persistant Storage Write Bytes Per Second: %d\n", static_system_info.persistent_storage_max_write_bytes_per_second);

    assert_true(is_alphanumeric_with_underscores(static_system_info.vendor, ARRAY_SIZE(static_system_info.vendor)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.partner, ARRAY_SIZE(static_system_info.partner)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.device, ARRAY_SIZE(static_system_info.device)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.software, ARRAY_SIZE(static_system_info.software)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.gpu, ARRAY_SIZE(static_system_info.gpu)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.cpu, ARRAY_SIZE(static_system_info.cpu)));
    assert_true(is_alphanumeric_with_underscores((const char *)static_system_info.device_id.bytes, ARRAY_SIZE(static_system_info.device_id.bytes)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.device_region, ARRAY_SIZE(static_system_info.device_region)));
    assert_true(is_alphanumeric_with_underscores(static_system_info.tenancy, ARRAY_SIZE(static_system_info.tenancy)));
    assert_true(is_alphanumeric_with_dashes(static_system_info.partner_guid, ARRAY_SIZE(static_system_info.partner_guid)));
}

int test_system_metrics() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(system_metrics_unit_test, NULL, NULL)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
