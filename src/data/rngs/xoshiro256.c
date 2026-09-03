#include <stdlib.h>
#include "data/rng.h"


/*
 * ============================================================
 * State
 * ============================================================
 */

typedef struct {
    uint64 s[4];
} Xoshiro256State;


/*
 * ============================================================
 * Rotate left
 * ============================================================
 */

static uint64 rotl64(
    uint64 x,
    int k
)
{
    return (x << k) | (x >> (64 - k));
}


/*
 * ============================================================
 * SplitMix64 used ONLY for seeding xoshiro
 * ============================================================
 */

static uint64 splitmix64_seed_next(
    uint64 *state
)
{
    uint64 z;

    *state += UINT64_C(0x9E3779B97F4A7C15);

    z = *state;

    z = (z ^ (z >> 30))
      * UINT64_C(0xBF58476D1CE4E5B9);

    z = (z ^ (z >> 27))
      * UINT64_C(0x94D049BB133111EB);

    return z ^ (z >> 31);
}


/*
 * ============================================================
 * Seed
 * ============================================================
 */

static void xoshiro256_seed(
    RandomSource *self,
    uint64 seed
)
{
    Xoshiro256State *state;
    uint64 sm_state;

    state = (Xoshiro256State *)self->state;

    sm_state = seed;

    state->s[0] = splitmix64_seed_next(&sm_state);
    state->s[1] = splitmix64_seed_next(&sm_state);
    state->s[2] = splitmix64_seed_next(&sm_state);
    state->s[3] = splitmix64_seed_next(&sm_state);
}


/*
 * ============================================================
 * next()
 * ============================================================
 */

static uint64 xoshiro256_next(
    RandomSource *self
)
{
    Xoshiro256State *state;
    uint64 result;
    uint64 t;

    state = (Xoshiro256State *)self->state;

    result =
        rotl64(
            state->s[1] * UINT64_C(5),
            7
        )
        * UINT64_C(9);

    t = state->s[1] << 17;

    state->s[2] ^= state->s[0];
    state->s[3] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[0] ^= state->s[3];

    state->s[2] ^= t;

    state->s[0] = rotl64(
        state->s[0],
        45
    );

    return result;
}


/*
 * ============================================================
 * destroy()
 * ============================================================
 */

static void xoshiro256_destroy(
    RandomSource *self
)
{
    if (self == NULL) {
        return;
    }

    if (self->state != NULL) {
        free(self->state);
    }

    free(self);
}


/*
 * ============================================================
 * Factory
 * ============================================================
 */

RandomSource *random_xoshiro256_create(
    uint64 seed
)
{
    RandomSource *rng;
    Xoshiro256State *state;

    rng = (RandomSource *)malloc(
        sizeof(RandomSource)
    );

    if (rng == NULL) {
        return NULL;
    }

    state = (Xoshiro256State *)malloc(
        sizeof(Xoshiro256State)
    );

    if (state == NULL) {
        free(rng);
        return NULL;
    }

    rng->next = xoshiro256_next;
    rng->seed = xoshiro256_seed;
    rng->destroy = xoshiro256_destroy;
    rng->state = state;

    xoshiro256_seed(
        rng,
        seed
    );

    return rng;
}
