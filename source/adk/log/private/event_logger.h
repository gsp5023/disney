/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  private/event_logger.h

  Event Logging

  System event logger via cnc_buss connection.
  */

#pragma once

#include "source/adk/cncbus/cncbus.h"

#ifdef __cplusplus
extern "C" {
#endif

void event_logger_init(cncbus_t * const bus, const cncbus_address_t address);

void event_logger_shutdown(cncbus_t * const bus);

#ifdef __cplusplus
}
#endif
