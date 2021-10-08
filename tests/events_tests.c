/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/runtime/app/events.h"
#include "source/adk/runtime/private/events.h"
#include "testapi.h"

static void assert_equal_overlay_events(adk_event_t e0, adk_event_t e1) {
    VERIFY(e0.event_data.type == adk_system_overlay_event);
    VERIFY(e1.event_data.type == adk_system_overlay_event);

    VERIFY(e0.time.ms == e1.time.ms);
    VERIFY(e0.event_data.overlay.overlay_type == e1.event_data.overlay.overlay_type);
    VERIFY(e0.event_data.overlay.state == e1.event_data.overlay.state);
    VERIFY(e0.event_data.overlay.n_x == e1.event_data.overlay.n_x);
    VERIFY(e0.event_data.overlay.n_y == e1.event_data.overlay.n_y);
    VERIFY(e0.event_data.overlay.n_width == e1.event_data.overlay.n_width);
    VERIFY(e0.event_data.overlay.n_height == e1.event_data.overlay.n_height);
    VERIFY(0 == memcmp(e0.event_data.overlay.reserved, e1.event_data.overlay.reserved, ARRAY_SIZE(e0.event_data.overlay.reserved)));
}

static void overlay_event_unit_test(void ** state) {
    adk_event_t events[] = {
        {.time = {.ms = 10000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_alert, .state = adk_system_overlay_begin, .n_x = .25f, .n_y = .25f, .n_width = .5f, .n_height = .5f}}}},
        {.time = {.ms = 10000 + 2000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_alert, .state = adk_system_overlay_end, .n_x = .25f, .n_y = .25f, .n_width = .5f, .n_height = .5f}}}},
        {.time = {.ms = 10000 + 4000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_reminder, .state = adk_system_overlay_begin, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 6000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_reminder, .state = adk_system_overlay_end, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 8000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_text_search, .state = adk_system_overlay_begin, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 10000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_text_search, .state = adk_system_overlay_end, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 12000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_voice_search, .state = adk_system_overlay_begin, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 14000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_voice_search, .state = adk_system_overlay_end, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f}}}},
        {.time = {.ms = 10000 + 16000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_reserved, .state = adk_system_overlay_begin, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f, .reserved = "Test Reserve 1"}}}},
        {.time = {.ms = 10000 + 18000}, .event_data = {.type = adk_system_overlay_event, {.overlay = {.overlay_type = adk_system_overlay_reserved, .state = adk_system_overlay_end, .n_x = .35f, .n_y = .35f, .n_width = .3f, .n_height = .3f, .reserved = "Test Reserve 2"}}}}};

    for (int i = 0; i < 4; i++) {
        adk_post_event_async(events[i]);
    }

    const adk_event_t * first;
    const adk_event_t * last;

    adk_get_events_swap_and_unlock(&first, &last);

    assert_equal_overlay_events(events[0], *first);
    // FYI: events[4] will not be last - it will be an app event

    for (int i = 0; i < 4; i++) {
        assert_equal_overlay_events(events[i], first[i]);
    }
}

int test_events() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(overlay_event_unit_test, NULL, NULL)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
