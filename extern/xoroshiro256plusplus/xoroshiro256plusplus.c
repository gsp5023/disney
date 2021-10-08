/*  Written in 2015,2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
 * Additions and changes Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include "xoroshiro256plusplus.h"
#include <stddef.h>

#define ARRAY_SIZE(_x) ((int)(sizeof(_x) / sizeof(_x[0])))

splitmix64_t splitmix64_init(const uint64_t seed) {
    return (splitmix64_t){seed};
}

uint64_t splitmix64_next(splitmix64_t * const generator) {
    uint64_t z = (generator->x += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

xoroshiro256plusplus_t xoroshiro256plusplus_init(splitmix64_t * const seed_gen) {
    return (xoroshiro256plusplus_t){
        .s = {
            splitmix64_next(seed_gen),
            splitmix64_next(seed_gen),
            splitmix64_next(seed_gen),
            splitmix64_next(seed_gen),
        }};
}

uint64_t xoroshiro256plusplus_next(xoroshiro256plusplus_t * const generator) {
    const uint64_t result = rotl(generator->s[0] + generator->s[3], 23) + generator->s[0];

    const uint64_t t = generator->s[1] << 17;

    generator->s[2] ^= generator->s[0];
    generator->s[3] ^= generator->s[1];
    generator->s[1] ^= generator->s[2];
    generator->s[0] ^= generator->s[3];

    generator->s[2] ^= t;

    generator->s[3] = rotl(generator->s[3], 45);

    return result;
}

void xoroshiro256plusplus_jump(xoroshiro256plusplus_t * const generator) {
    static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;
    for (size_t i = 0; i < ARRAY_SIZE(JUMP); i++)
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & UINT64_C(1) << b) {
                s0 ^= generator->s[0];
                s1 ^= generator->s[1];
                s2 ^= generator->s[2];
                s3 ^= generator->s[3];
            }
            xoroshiro256plusplus_next(generator);
        }

    generator->s[0] = s0;
    generator->s[1] = s1;
    generator->s[2] = s2;
    generator->s[3] = s3;
}

void xoroshiro256plusplus_long_jump(xoroshiro256plusplus_t * const generator) {
    static const uint64_t LONG_JUMP[] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;
    for (size_t i = 0; i < ARRAY_SIZE(LONG_JUMP); i++)
        for (int b = 0; b < 64; b++) {
            if (LONG_JUMP[i] & UINT64_C(1) << b) {
                s0 ^= generator->s[0];
                s1 ^= generator->s[1];
                s2 ^= generator->s[2];
                s3 ^= generator->s[3];
            }
            xoroshiro256plusplus_next(generator);
        }

    generator->s[0] = s0;
    generator->s[1] = s1;
    generator->s[2] = s2;
    generator->s[3] = s3;
}
