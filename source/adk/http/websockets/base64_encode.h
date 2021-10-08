// Modifications to this software Copyright (c) 2021 Disney

/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 * source: http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
 */

#include <stdbool.h>
#include <stddef.h>

bool base64_encode(const unsigned char * const src, const size_t len, unsigned char * const out_buf, size_t * const out_len);