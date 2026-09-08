#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

/*
 * ============================================================
 * RandomSource
 * ============================================================
 */
typedef struct RandomSource RandomSource;


struct RandomSource {

    /*
     * Generate the next 64 random bits.
     */
    uint64_t (*next)(RandomSource *self);

    /*
     * Reset the generator with a new seed.
     */
    void (*seed)(RandomSource *self, uint64_t seed);

    /*
     * Destroy the RNG.
     */
    void (*destroy)(RandomSource *self);

    /*
     * Implementation-specific state.
     */
    void *state;
};


/*
 * ============================================================
 * Generic API
 * ============================================================
 */

uint64_t random_next(RandomSource *rng);

void random_seed(
    RandomSource *rng,
    uint64_t seed
);

void random_destroy(RandomSource *rng);


/*
 * ============================================================
 * Factory
 * ============================================================
 *
 * The selected implementation depends on RNG_TYPE.
 */
RandomSource *random_create(uint64_t seed);


/*
 * ============================================================
 * Concrete factories
 * ============================================================
 */

RandomSource *random_xoshiro256_create(
    uint64_t seed
);

RandomSource *random_splitmix64_create(
    uint64_t seed
);


#endif /* RANDOM_H */
