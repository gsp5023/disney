/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
metrics.c

Low overhead publishing of values over the cncbus
*/

#include "source/adk/metrics/metrics.h"

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/steamboat/sb_platform.h"

typedef struct metrics_bus_info_t {
    cncbus_t * bus;
    cncbus_address_t address;
    cncbus_address_t subnet_mask;
} metrics_bus_info_t;

static metrics_bus_info_t bus_info = {
    .bus = NULL,
};

void metrics_init(cncbus_t * const bus, const cncbus_address_t address, const cncbus_address_t subnet_mask) {
    ZEROMEM(&bus_info);
    bus_info.bus = bus;
    bus_info.address = address;
    bus_info.subnet_mask = subnet_mask;
}

void publish_metric(metric_types_e metric_type, const void * const value, const size_t value_size) {
    if (bus_info.bus) {
        const metric_msg_header_t metric_header = {
            .time_since_epoch = sb_get_time_since_epoch(),
            .type = metric_type,
            .value_size = (int32_t)value_size};

        cncbus_msg_t * const cncbus_msg = cncbus_msg_begin_unchecked(bus_info.bus, cncbus_msg_type_metric_v2);
        cncbus_msg_write_checked(cncbus_msg, &metric_header, sizeof(metric_header));
        cncbus_msg_write_checked(cncbus_msg, value, (int32_t)value_size);
        cncbus_send_async(cncbus_msg, CNCBUS_INVALID_ADDRESS, bus_info.address, bus_info.subnet_mask, NULL);
    }
}