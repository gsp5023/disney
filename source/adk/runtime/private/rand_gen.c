/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#include "source/adk/runtime/rand_gen.h"

#include "extern/xoroshiro256plusplus/xoroshiro256plusplus.h"
#include "source/adk/steamboat/sb_platform.h"

adk_rand_generator_t adk_rand_create_generator() {
    sb_time_since_epoch_t timestamp = sb_get_time_since_epoch();
    return adk_rand_create_generator_with_seed(((uint64_t)timestamp.seconds * 1000) + ((uint64_t)timestamp.microseconds / 1000));
}

adk_rand_generator_t adk_rand_create_generator_with_seed(const uint64_t seed) {
    splitmix64_t seed_gen = splitmix64_init(seed);
    return (adk_rand_generator_t){.state = xoroshiro256plusplus_init(&seed_gen)};
}

uint64_t adk_rand_next(adk_rand_generator_t * const generator) {
    return xoroshiro256plusplus_next(&generator->state);
}