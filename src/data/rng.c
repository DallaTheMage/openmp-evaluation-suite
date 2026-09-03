#include <stddef.h>
#include "data/rng.h"
#include "config/rng.h"

/*
 * ============================================================
 * Generic RandomSource API
 * ============================================================
 */

uint64 random_next(RandomSource *rng)
{
    if (rng == NULL || rng->next == NULL) {
        return UINT64_C(0);
    }

    return rng->next(rng);
}


void random_seed(
    RandomSource *rng,
    uint64 seed
)
{
    if (rng == NULL || rng->seed == NULL) {
        return;
    }

    rng->seed(rng, seed);
}


void random_destroy(RandomSource *rng)
{
    if (rng == NULL || rng->destroy == NULL) {
        return;
    }

    rng->destroy(rng);
}


/*
 * ============================================================
 * RNG factory
 * ============================================================
 */

RandomSource *random_create(uint64 seed)
{
#if RNG_TYPE == RNG_XOSHIRO256

    return random_xoshiro256_create(seed);

#elif RNG_TYPE == RNG_SPLITMIX64

    return random_splitmix64_create(seed);

#else

    #error "Unsupported RNG_TYPE"

#endif
}
