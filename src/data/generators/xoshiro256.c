#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "data/generator.h"
#include "config/types.h"

/* Rotazione dei bit a sinistra per uint64 */
static uint64 rotl(const uint64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* Stato del generatore xoshiro256** */
typedef struct {
    uint64 s[4];
} Xoshiro256State;

typedef struct {
    double min;
    double max;
    Xoshiro256State rng_state;
} RandomConfig;

/* Splitmix64 adattato a uint64 per il seeding */
static uint64 splitmix64_next(uint64 *state) {
    uint64 z = (*state += 0x9E3779B97F4A7C15UL);
    z = (z ^ (z >> 30)) * 0xBF58476d1CE4E5B9UL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBUL;
    return z ^ (z >> 31);
}

static void xoshiro256_seed(Xoshiro256State *state, uint64 seed) {
    uint64 sm_state = seed;
    state->s[0] = splitmix64_next(&sm_state);
    state->s[1] = splitmix64_next(&sm_state);
    state->s[2] = splitmix64_next(&sm_state);
    state->s[3] = splitmix64_next(&sm_state);
}

static uint64 xoshiro256_next(Xoshiro256State *state) {
    const uint64 result = rotl(state->s[1] * 5, 7) * 9;
    const uint64 t = state->s[1] << 17;

    state->s[2] ^= state->s[0];
    state->s[3] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[0] ^= state->s[3];

    state->s[2] ^= t;
    state->s[0] = rotl(state->s[0], 45);

    return result;
}

/* Conversione a double usando i 53 bit della mantissa */
static double xoshiro_to_double(uint64 x) {
    return (double)(x >> 11) * (1.0 / 9007199254740992.0);
}

static int xoshiro256_init(DataGenerator *gen) {
    RandomConfig *cfg;
    if (gen == NULL || gen->config == NULL) return 0;
    cfg = (RandomConfig *)gen->config;

    xoshiro256_seed(&cfg->rng_state, (uint64)time(NULL));
    return 1;
}

static int xoshiro256_fill(DataGenerator *gen, Collection *collection) {
    RandomConfig *cfg;
    double *data;
    uint64 i;
    double range;

    if (gen == NULL || gen->config == NULL || collection == NULL) return 0;
    cfg = (RandomConfig *)gen->config;

    if (IS_DOUBLE && collection->data != NULL) {
        data = (double *)collection->data;
        range = cfg->max - cfg->min;

        for (i = 0; i < collection->size; ++i) {
            uint64 raw_bits = xoshiro256_next(&cfg->rng_state);
            double norm = xoshiro_to_double(raw_bits);
            data[i] = cfg->min + (norm * range);
        }
        return 1;
    }
    return 0;
}

static void xoshiro256_clean(DataGenerator *gen) {
    if (gen != NULL) {
        if (gen->config != NULL) {
            free(gen->config);
        }
        free(gen);
    }
}

DataGenerator* generator_xoshiro256_create(double min, double max) {
    DataGenerator *gen;
    RandomConfig *cfg;

    gen = (DataGenerator *)malloc(sizeof(DataGenerator));
    if (gen == NULL) return NULL;

    cfg = (RandomConfig *)malloc(sizeof(RandomConfig));
    if (cfg == NULL) {
        free(gen);
        return NULL;
    }

    cfg->min = min;
    cfg->max = max;

    gen->name = "random_xoshiro256";
    gen->config = cfg;
    gen->operations.init = xoshiro256_init;
    gen->operations.fill = xoshiro256_fill;
    gen->operations.clean = xoshiro256_clean;

    if (gen->operations.init(gen) != 1) {
        xoshiro256_clean(gen);
        return NULL;
    }

    return gen;
}