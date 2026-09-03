#include <stdlib.h>
#include "data/rng.h"


/*
 * ============================================================
 * SplitMix64 state
 * ============================================================
 */

typedef struct {
    uint64 state;
} SplitMix64State;


/*
 * ============================================================
 * next()
 * ============================================================
 */

static uint64 splitmix64_next(RandomSource *self) {
    SplitMix64State *state;
    uint64 z;
    state = (SplitMix64State *)self->state;
    state->state += UINT64_C(0x9E3779B97F4A7C15);
    z = state->state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/*
 * ============================================================
 * seed()
 * ============================================================
 */

static void splitmix64_seed(RandomSource *self, uint64 seed) {
    SplitMix64State *state;
    state = (SplitMix64State *)self->state;
    state->state = seed;
}


/*
 * ============================================================
 * destroy()
 * ============================================================
 */

static void splitmix64_destroy(RandomSource *self) {
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

RandomSource *random_splitmix64_create(uint64 seed) {
    RandomSource *rng;
    SplitMix64State *state;
    rng = (RandomSource *)malloc(
        sizeof(RandomSource)
    );
    if (rng == NULL) {
        return NULL;
    }
    state = (SplitMix64State *)malloc(sizeof(SplitMix64State));
    if (state == NULL) {
        free(rng);
        return NULL;
    }
    state->state = seed;
    rng->next = splitmix64_next;
    rng->seed = splitmix64_seed;
    rng->destroy = splitmix64_destroy;
    rng->state = state;
    return rng;
}
