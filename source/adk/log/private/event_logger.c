/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 private/event_logger.c

 Event Logging

 Structured event log messages via cnc_bus connection.
 */

#include "source/adk/log/private/event_logger.h"

#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/app/events.h"

#define TAG_LOGGER FOURCC('E', 'V', 'N', 'T')

static inline void log_event(const adk_event_t * const event);

static struct {
    cncbus_receiver_t receiver;
    cncbus_receiver_vtable_t vtable;
} event_logger;

static inline bool is_hid_event(const adk_event_t * const event) {
    bool result = (event->event_data.type == adk_key_event || event->event_data.type == adk_char_event || event->event_data.type == adk_mouse_event || event->event_data.type == adk_gamepad_event || event->event_data.type == adk_stb_input_event || event->event_data.type == adk_stb_rich_input_event);

    return result;
}

static int on_event_msg_received(cncbus_receiver_t * const receiver, const cncbus_msg_header_t header, cncbus_msg_t * const msg) {
    if (header.msg_type != cncbus_msg_type_event) {
        return 1;
    }

    size_t batch_size = 0;
    cncbus_msg_read(msg, &batch_size, sizeof(size_t));

    ASSERT(batch_size > 0);
    ASSERT((header.msg_size - sizeof(size_t)) == batch_size * sizeof(adk_event_t));

    adk_event_t event;
    for (size_t events_processed = batch_size; events_processed > 0; events_processed--) {
        cncbus_msg_read(msg, &event, sizeof(adk_event_t));

        if (!is_hid_event(&event)) {
            continue;
        }

        log_event(&event);
    }

    return 0;
}

void event_logger_init(cncbus_t * const bus, const cncbus_address_t address) {
    event_logger.vtable.on_msg_recv = on_event_msg_received;

    cncbus_init_receiver(&event_logger.receiver, &event_logger.vtable, address);
    cncbus_connect(bus, &event_logger.receiver);
}

void event_logger_shutdown(cncbus_t * const bus) {
    cncbus_disconnect(bus, &event_logger.receiver);
}

static inline void log_event(const adk_event_t * const event) {
    switch (event->event_data.type) {
        case adk_application_event: {
            LOG_INFO(TAG_LOGGER, "Event received: adk_application_event {}");
            break;
        }
        case adk_window_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_window_event {}");
            break;
        }
        case adk_key_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_key_event { action: %s, key: %i, mod_keys: %i, repeat: %i }", event->event_data.key.event == adk_key_event_key_down ? "DOWN" : "UP", event->event_data.key.key, event->event_data.key.mod_keys, event->event_data.key.repeat);
            break;
        }
        case adk_char_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_char_event { codepoint: %i }", event->event_data.ch.codepoint_utf32);
            break;
        }
        case adk_mouse_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_mouse_event { window: %p, mouse_state: { x: %i, y: %i }, event_data: { action: %s } }", event->event_data.mouse.window.ptr, event->event_data.mouse.mouse_state.x, event->event_data.mouse.mouse_state.y, event->event_data.mouse.event_data.event == adk_mouse_event_button ? "button_press" : "motion");
            break;
        }
        case adk_gamepad_event: {
            const char * event_type_msg = NULL;

            switch (event->event_data.gamepad.event_data.event) {
                case adk_gamepad_event_connect: {
                    event_type_msg = "connect";
                    break;
                }
                case adk_gamepad_event_disconnect: {
                    event_type_msg = "disconnect";
                    break;
                }
                case adk_gamepad_event_button: {
                    event_type_msg = "button_press";
                    break;
                }
                case adk_gamepad_event_axis: {
                    event_type_msg = "axis_change";
                    break;
                }
                default: {
                    break;
                }
            }

            LOG_INFO(TAG_LOGGER, "Event Received: adk_gamepad_event { index: %i, action: %s }", event->event_data.gamepad.gamepad_index, event_type_msg);
            break;
        }
        case adk_stb_input_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_stb_input_event { key: %i, repeat: %i }", event->event_data.stb_input.stb_key, event->event_data.stb_input.repeat);
            break;
        }
        case adk_stb_rich_input_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_stb_rich_input_event { key: %i, action: %s, repeat: %i}", event->event_data.stb_rich_input.stb_key, event->event_data.stb_rich_input.event, event->event_data.stb_rich_input.repeat);
            break;
        }
        case adk_time_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_time_event");
            break;
        }
        case adk_deeplink_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_deeplink_event");
            break;
        }
        case adk_power_mode_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_power_mode_event");
            break;
        }
        case adk_system_overlay_event: {
            LOG_INFO(TAG_LOGGER, "Event Received: adk_system_overlay_event");
            break;
        }
        default: {
            break;
        }
    }
}
