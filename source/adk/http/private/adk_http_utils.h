/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

#include <ctype.h>

/// Parses header (key & value) according to RFC, returns mem-region of 'value'
/// RFC 7230 states: "Each header field consists of a case-insensitive field name followed by a colon (":"), optional leading whitespace, the field value, and optional trailing whitespace."
/// Requires the header to end with a CRLF
static inline const_mem_region_t http_parse_header_for_key(const char * const key, const const_mem_region_t header) {
    // Identify header:value sections as being separated by `\r\n` sections.
    // Do not assume `header` refers to a single complete header
    size_t start = 0;
    size_t end = 0;
    size_t key_length = strlen(key);
    const char * const header_buffer = (const char *)header.ptr;

    while (start < header.size) {
        size_t key_end = 0;
        bool found_colon = false;

        // look forward for \r\n to designate the end of the key: value pair
        while (end < header.size + 1 && header_buffer[end] != '\r' && header_buffer[end + 1] != '\n') {
            end++;
            if (header_buffer[end] == ':') {
                if (!found_colon) {
                    key_end = end - 1;
                }

                found_colon = true;
            }
        }

        if (found_colon == false) {
            // Not a properly formatted header value for this function. Typical with the phrase/status value at the beginning
            // of the header buffer. Just skip past it
            start += (end - start + 2);
            end = start;
            continue;
        }

        const size_t parsed_key_length = key_end - start + 1;

        if (key_length != parsed_key_length) {
            // not the key we're looking for, move on
            start += (end - start + 2);
            end = start;
            continue;
        }

        const char * const parsed_key = header_buffer + start;

        if (strncasecmp(parsed_key, key, parsed_key_length) != 0) {
            // not the key we're looking for, move on
            start += (end - start + 2);
            end = start;
            continue;
        }

        // Got the right key, parse out the value

        size_t value_start = key_end + 1;

        // Move along past any colon and spaces
        while ((header_buffer[value_start] == ':' || isspace(header_buffer[value_start])) && value_start < header.size - 1) {
            value_start++;
        }

        if (value_start == header.size - 1) {
            // Blown past the end of the header buffer, must be incomplete or invalid
            return (const_mem_region_t){0};
        }

        // Header values are terminated with \r\n, so just move  value_end along until we find it
        size_t value_end = value_start;

        while (header_buffer[value_end] != '\r' && header_buffer[value_end] != '\n' && value_end < header.size - 2) {
            value_end++;
        }

        if (value_end == header.size - 1) {
            // Malformed header, no \r\n termination
            return (const_mem_region_t){0};
        }

        return CONST_MEM_REGION(.byte_ptr = header.byte_ptr + value_start, .size = value_end - value_start);
    }

    return (const_mem_region_t){0};
}

// the first header line is the status line and will contain the 3DIGIT status code, per RFC as: HTTP-version SP status-code SP reason-phrase CRLF
static inline long http_parse_header_for_status_code(const const_mem_region_t header) {
    const char * const header_buffer = (const char *)header.ptr;

    size_t i = 1;

    // minus 5 represents the minimum length still needed past the first space (of 3 digits, a 2nd space, and at least one phrase character)
    while ((i < (header.size - 5)) && (header_buffer[i] != ' ')) {
        i++;
    }

    if ((i < (header.size - 5)) && (header_buffer[i] == ' ') && isdigit(header_buffer[i + 1]) && isdigit(header_buffer[i + 2]) && isdigit(header_buffer[i + 3]) && (header_buffer[i + 4] == ' ')) {
        char status_code_buffer[4];
        memcpy(status_code_buffer, &header_buffer[i + 1], 3);
        status_code_buffer[3] = '\0';

        return strtol(status_code_buffer, NULL, 10);
    }

    return -1;
}