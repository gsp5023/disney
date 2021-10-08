/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/samples/extension/lib/extension.h"

int32_t extension_squared(int32_t value) {
    return value * value;
}

void extension_println(const char * msg) {
    sb_unformatted_debug_write_line(msg);
}
