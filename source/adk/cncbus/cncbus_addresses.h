/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/cncbus/cncbus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CNCBUS_ADDRESS_LOGGER CNCBUS_MAKE_ADDRESS(42, 42, 1, 0)
#define CNCBUS_SUBNET_MASK_CORE CNCBUS_MAKE_ADDRESS(255, 255, 0, 0)

#ifdef __cplusplus
}
#endif