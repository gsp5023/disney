/*  Written in 2015,2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
 * Additions and changes Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

// Modifications to this software (c) 2021 Disney

#pragma once

#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#include <stdint.h>

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html
   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state; otherwise, we
   rather suggest to use a xoroshiro128+ (for moderately parallel
   computations) or xorshift1024* (for massively parallel computations)
   generator. */

typedef struct splitmix64_t {
    uint64_t x; /* The state can be seeded with any value. */
} splitmix64_t;

splitmix64_t splitmix64_init(const uint64_t seed);
uint64_t splitmix64_next(splitmix64_t * const generator);

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

FFI_EXPORT FFI_PUB_CRATE typedef struct xoroshiro256plusplus_t {
    uint64_t s[4];
} xoroshiro256plusplus_t;

xoroshiro256plusplus_t xoroshiro256plusplus_init(splitmix64_t * const seed_gen);

uint64_t xoroshiro256plusplus_next(xoroshiro256plusplus_t * const generator);

/* This is the jump function for the generator. It is equivalent
   to 2^128 calls to next(); it can be used to generate 2^128
   non-overlapping subsequences for parallel computations. */
void xoroshiro256plusplus_jump(xoroshiro256plusplus_t * const generator);

/* This is the long-jump function for the generator. It is equivalent to
   2^192 calls to next(); it can be used to generate 2^64 starting points,
   from each of which jump() will generate 2^64 non-overlapping
   subsequences for parallel distributed computations. */
void xoroshiro256plusplus_long_jump(xoroshiro256plusplus_t * const generator);