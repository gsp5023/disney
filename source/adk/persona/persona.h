/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
persona.h

Support for running multiple apps with the same m5 core
*/

#pragma once

#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_file.h"

enum {
    // Default message contains unicode characters and it's expected that message
    // may include non-ascii symbols in it.
    adk_max_message_length = 512
};

typedef struct persona_mapping_t {
    char file[sb_max_path_length];
    char id[adk_metrics_string_max];
    char manifest_url[sb_max_path_length];
    char fallback_error_message[adk_max_message_length];
} persona_mapping_t;

bool get_persona_mapping(persona_mapping_t * mapping);
