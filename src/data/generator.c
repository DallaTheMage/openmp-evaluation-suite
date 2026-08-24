#include <stdlib.h>

#include "data/generator.h"
#include "config/rng.h"


/*
 * ============================================================
 * Generic DataGenerator API
 * ============================================================
 */

int generator_init(DataGenerator *generator)
{
    if (generator == NULL) {
        return 0;
    }

    if (generator->operations.init == NULL) {
        return 0;
    }

    return generator->operations.init(generator);
}


int generator_fill(
    DataGenerator *generator,
    Collection *collection
)
{
    if (generator == NULL ||
        collection == NULL) {
        return 0;
    }

    if (generator->operations.fill == NULL) {
        return 0;
    }

    return generator->operations.fill(
        generator,
        collection
    );
}


void generator_destroy(DataGenerator *generator)
{
    if (generator == NULL) {
        return;
    }

    if (generator->operations.clean != NULL) {
        generator->operations.clean(generator);
    }
}


/*
 * ============================================================
 * Random generator factory
 * ============================================================
 */

DataGenerator *generator_random_create(
    datatype min,
    datatype max
)
{
#if RNG_TYPE == RNG_XOSHIRO256

    return generator_xoshiro256_create(
        min,
        max
    );

#elif RNG_TYPE == RNG_SPLITMIX64

    return generator_splitmix64_create(
        min,
        max
    );

#else

    return NULL;

#endif
}