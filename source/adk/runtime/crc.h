/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "runtime.h"

PURE unsigned char * checksum_NMEA(const unsigned char * input_str, unsigned char * result);
PURE uint8_t crc_8(const unsigned char * input_str, size_t num_bytes);
PURE uint16_t crc_16(const unsigned char * input_str, size_t num_bytes);
PURE uint32_t crc_32(const unsigned char * input_str, size_t num_bytes);
FFI_EXPORT FFI_NAME(adk_crc_str_32) PURE uint32_t crc_str_32(FFI_PTR_WASM const char * str);
PURE uint64_t crc_64_ecma(const unsigned char * input_str, size_t num_bytes);
PURE uint64_t crc_64_we(const unsigned char * input_str, size_t num_bytes);
PURE uint8_t update_crc_8(uint8_t crc, unsigned char c);
PURE uint16_t update_crc_16(uint16_t crc, unsigned char c);
PURE uint32_t update_crc_32(const uint32_t crc, const unsigned char * c, const size_t num_bytes);
PURE uint32_t update_crc_str_32(uint32_t crc, const char * str);

PURE static uint32_t crc_name_check(const char * const str, const uint32_t crc) {
    VERIFY(crc_str_32(str) == crc);
    return crc;
}

// use https://crccalc.com/ (CRC-32) algorithm

#ifdef _SHIP
#define CRC_NAME(_name, _crc) (_crc)
#else
#define CRC_NAME(_name, _crc) crc_name_check(#_name, _crc)
#endif
