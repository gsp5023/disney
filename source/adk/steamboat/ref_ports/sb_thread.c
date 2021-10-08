/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#include "source/adk/steamboat/sb_thread.h"

sb_thread_id_t sb_get_main_thread_id() {
    extern sb_thread_id_t sb_main_thread_id;
    return sb_main_thread_id;
}
