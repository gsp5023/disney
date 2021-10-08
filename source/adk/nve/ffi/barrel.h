/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#include <stdbool.h>

// NVE FFI (required even for null player for no-op methods)
#include "extern/nve-api/include/nve.h"
#include "extern/nve-api/include/nve_abr_params.h"
#include "extern/nve-api/include/nve_buffer_params.h"
#include "extern/nve-api/include/nve_capabilities.h"
#include "extern/nve-api/include/nve_text_format.h"

#ifdef _NVE
#include "source/adk/nve/psdk/M5View.h"
#endif
