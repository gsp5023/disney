/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  private/log_receiver.h

  Core Logging

  Receiver sink for log messages sent via cnc_bus connection
  */

#pragma once

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

EXT_EXPORT void log_receiver_init(cncbus_t * const bus, const cncbus_address_t address);

EXT_EXPORT void log_receiver_shutdown(cncbus_t * const bus);

void log_receiver_reload_tty_options();

#ifdef __cplusplus
}
#endif
