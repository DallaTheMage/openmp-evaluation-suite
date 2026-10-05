#include "data/generator.h"

static uint64_t splitmix64_next(uint64_t *sm_state) {
    uint64_t z = (*sm_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static void generator_init(xoshiro256_state *state, uint64_t seed) {
    uint64_t sm_state = seed;
    state->s[0] = splitmix64_next(&sm_state);
    state->s[1] = splitmix64_next(&sm_state);
    state->s[2] = splitmix64_next(&sm_state);
    state->s[3] = splitmix64_next(&sm_state);
}

void generator_jump(xoshiro256_state *state) {
    static const uint64_t JUMP[] = {
        0x180ec6d33cfd0eba, 0xd5161d507089b4f1,
        0x3ff9b3f3f6b9ed8e, 0xe0b628341f480766
    };

    uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;

    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); i++) {
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & (UINT64_C(1) << b)) {
                s0 ^= state->s[0];
                s1 ^= state->s[1];
                s2 ^= state->s[2];
                s3 ^= state->s[3];
            }
            generator_next_double(state);
        }
    }

    state->s[0] = s0;
    state->s[1] = s1;
    state->s[2] = s2;
    state->s[3] = s3;
}

void generator_fill_pool(uint64_t seed, double *restrict pool, size_t count) {
    xoshiro256_state state;
    generator_init(&state, seed);
    for (size_t i = 0; i < count; i++) {
        double val = generator_next_double(&state);
        pool[i] = (val == 0.0) ? 0.001 : val;
    }
}