/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
metrics.h

low overhead publishing of values over the cncbus
*/

#pragma once

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct metric_time_to_first_interaction_t {
    milliseconds_t main_timestamp;
    milliseconds_t app_init_timestamp;
    milliseconds_t dimiss_system_splash_timestamp;
} metric_time_to_first_interaction_t;

typedef struct metric_memory_footprint_t {
    uint32_t low_memory_size;
    uint32_t high_memory_size;
} metric_memory_footprint_t;

typedef enum metric_types_e {
    metric_type_int,
    metric_type_float,
    metric_type_delta_time_in_ms, // milliseconds_t
    metric_type_time_to_first_interaction, // metric_time_to_first_interaction_t
    metric_type_memory_footprint, // metric_memory_footprint_t
    metric_types_last, // this must be the last element in the enum
    FORCE_ENUM_INT32(metric_types_e)
} metric_types_e;

typedef struct metric_msg_header_t {
    sb_time_since_epoch_t time_since_epoch;
    metric_types_e type;
    int32_t value_size;
    // the specific type of `metric_types_e` will follow the header.
} metric_msg_header_t;

void metrics_init(cncbus_t * const bus, const cncbus_address_t address, const cncbus_address_t subnet_mask);

// any listener on the attached cncbus should read the messages off as follows:
// read out a metric_msg_header_t, check the type of metric reported
// create memory to read the specified type in
// read from the cncbus the full message as the type specifies
// the data in the bus is thus: [msg_header][message_type_bytes...] as 2 separate cncbus messages.
void publish_metric(metric_types_e metric_type, const void * const value, const size_t value_size);

#ifdef __cplusplus
}
#endif
