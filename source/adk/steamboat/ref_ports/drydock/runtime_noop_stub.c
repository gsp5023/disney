/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "impl_tracking.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_platform.h"

// A memory-related global variable relied-upon by m5
int sys_page_size;

void sb_unformatted_debug_write_line(const char * const msg) {
    UNUSED(msg);

    NOT_IMPLEMENTED_EX;
}

void sb_unformatted_debug_write(const char * const msg) {
    UNUSED(msg);

    NOT_IMPLEMENTED_EX;
}

void sb_vadebug_write(const char * const msg, va_list args) {
    UNUSED(msg);
    UNUSED(args);

    NOT_IMPLEMENTED_EX;
}

void sb_vadebug_write_line(const char * const msg, va_list args) {
    UNUSED(msg);
    UNUSED(args);

    NOT_IMPLEMENTED_EX;
}

void sb_seconds_since_epoch_to_localtime(const time_t seconds, struct tm * const _tm) {
    UNUSED(seconds);
    UNUSED(_tm);

    NOT_IMPLEMENTED_EX;
}

sb_cpu_mem_status_t sb_get_cpu_mem_status() {
    NOT_IMPLEMENTED_EX;
    return (sb_cpu_mem_status_t){0};
}
