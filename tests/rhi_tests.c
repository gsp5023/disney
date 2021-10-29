/* ===========================================================================
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 * ==========================================================================*/

#include "source/adk/renderer/private/rbcmd.h"
#include "source/adk/renderer/private/rhi_device_api.h"
#include "source/adk/steamboat/sb_platform.h"
#include "testapi.h"

void rb_cmd_buf_execute(rhi_device_t * const device, rb_cmd_buf_t * const cmd_buf); // from rbcmd.c

// 'Subclass' of `rhi_device_t` that contains values for unit tests
typedef struct rhi_test_device_t {
    rhi_device_t device;
    struct {
        size_t counter;
        size_t sum;
    } test;
} rhi_test_device_t;

static void rhi_test_device_present(rhi_device_t * const device, const rhi_swap_interval_t swap_interval) {
    rhi_test_device_t * const test_device = (rhi_test_device_t *)device;

    test_device->test.counter += 1;
    test_device->test.sum += swap_interval.interval;
}

static const rhi_device_vtable_t test_device_vtable = {
    .present = rhi_test_device_present};

static void test_rhi_command_diffing(void ** state) {
    render_cmd_config_t render_cmd_config = {0};
    render_cmd_config.rhi_command_diffing.enabled = true;
    render_cmd_init(render_cmd_config);

    rhi_test_device_t device = {.device = {.resource = {0}, .vtable = &test_device_vtable}, .test = {0}};

    rb_cmd_buf_t cmd_buf = {0};
    mem_region_t region = sb_map_pages(PAGE_ALIGN_INT(4096), system_page_protect_read_write);
    hlba_init(&cmd_buf.hlba, region.ptr, (int)region.size);

    // Write a 'present' command and execute the buffer twice - expecting no action on the second execute (as hash will match)

    assert_true(render_cmd_buf_write_present(&cmd_buf, (rhi_swap_interval_t){.interval = 1}, MALLOC_TAG));

    rb_cmd_buf_execute((rhi_device_t *)&device, &cmd_buf);
    assert_int_equal(device.test.counter, 1);
    assert_int_equal(device.test.sum, 1);

    rb_cmd_buf_execute((rhi_device_t *)&device, &cmd_buf);
    assert_int_equal(device.test.counter, 1);
    assert_int_equal(device.test.sum, 1);

    // Write a 'present' command with a different `interval` and execute the buffer - expecting execution of command as hash will differ

    assert_true(render_cmd_buf_write_present(&cmd_buf, (rhi_swap_interval_t){.interval = 2}, MALLOC_TAG));

    rb_cmd_buf_execute((rhi_device_t *)&device, &cmd_buf);
    assert_int_equal(device.test.counter, 2);
    assert_int_equal(device.test.sum, 3);

    sb_unmap_pages(region);
}

int test_rhi() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_rhi_command_diffing)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
