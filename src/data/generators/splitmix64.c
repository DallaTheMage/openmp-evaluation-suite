#include <stdlib.h>
#include <time.h>

#include "data/generator.h"
#include "data/rng.h"


typedef struct {

    datatype min;
    datatype max;

    RandomSource *rng;

} SplitMix64GeneratorConfig;


/*
 * ============================================================
 * Floating point conversion
 * ============================================================
 */

#if DATATYPE_IS_FLOAT

static float random_to_float(uint64 value)
{
    return (float)(value >> 40)
         * (1.0f / 16777216.0f);
}

#endif


#if DATATYPE_IS_DOUBLE

static double random_to_double(uint64 value)
{
    return (double)(value >> 11)
         * (1.0 / 9007199254740992.0);
}

#endif


/*
 * ============================================================
 * Init
 * ============================================================
 */

static int generator_splitmix64_init(
    DataGenerator *generator
)
{
    SplitMix64GeneratorConfig *config;

    if (generator == NULL ||
        generator->config == NULL) {
        return 0;
    }

    config =
        (SplitMix64GeneratorConfig *)generator->config;

    /*
     * RNG_TYPE selects the actual RNG.
     */
    config->rng =
        random_create((uint64)time(NULL));

    if (config->rng == NULL) {
        return 0;
    }

    return 1;
}


/*
 * ============================================================
 * Fill
 * ============================================================
 */

static int generator_splitmix64_fill(
    DataGenerator *generator,
    Collection *collection
)
{
    SplitMix64GeneratorConfig *config;
    uint64 i;

    if (generator == NULL ||
        generator->config == NULL ||
        collection == NULL ||
        collection->data == NULL) {
        return 0;
    }

    config =
        (SplitMix64GeneratorConfig *)generator->config;


#if DATATYPE_IS_DOUBLE

    {
        double min;
        double range;

        min = (double)config->min;
        range = (double)config->max - min;

        for (i = 0; i < collection->size; ++i) {

            double normalized;

            normalized =
                random_to_double(
                    random_next(config->rng)
                );

            collection->data[i] =
                (datatype)(
                    min + normalized * range
                );
        }
    }


#elif DATATYPE_IS_FLOAT

    {
        float min;
        float range;

        min = (float)config->min;
        range = (float)config->max - min;

        for (i = 0; i < collection->size; ++i) {

            float normalized;

            normalized =
                random_to_float(
                    random_next(config->rng)
                );

            collection->data[i] =
                (datatype)(
                    min + normalized * range
                );
        }
    }


#elif DATATYPE_IS_INTEGER

    {
        uint64 range;

        /*
         * Integer generation uses the RNG directly.
         *
         * This path intentionally avoids floating point.
         */
        range =
            (uint64)config->max -
            (uint64)config->min;

        if (range == 0) {

            for (i = 0; i < collection->size; ++i) {
                collection->data[i] =
                    config->min;
            }

        } else {

            for (i = 0; i < collection->size; ++i) {

                uint64 value;

                value =
                    random_next(config->rng);

                value %= range;

                collection->data[i] =
                    (datatype)(
                        (uint64)config->min + value
                    );
            }
        }
    }

#else

    return 0;

#endif


    return 1;
}


/*
 * ============================================================
 * Clean
 * ============================================================
 */

static void generator_splitmix64_clean(
    DataGenerator *generator
)
{
    SplitMix64GeneratorConfig *config;

    if (generator == NULL) {
        return;
    }

    config =
        (SplitMix64GeneratorConfig *)generator->config;

    if (config != NULL) {

        if (config->rng != NULL) {
            random_destroy(config->rng);
            config->rng = NULL;
        }

        free(config);
        generator->config = NULL;
    }

    free(generator);
}


/*
 * ============================================================
 * Factory
 * ============================================================
 */

DataGenerator *generator_splitmix64_create(
    datatype min,
    datatype max
)
{
    DataGenerator *generator;
    SplitMix64GeneratorConfig *config;

    generator =
        (DataGenerator *)malloc(
            sizeof(DataGenerator)
        );

    if (generator == NULL) {
        return NULL;
    }

    config =
        (SplitMix64GeneratorConfig *)malloc(
            sizeof(SplitMix64GeneratorConfig)
        );

    if (config == NULL) {
        free(generator);
        return NULL;
    }

    config->min = min;
    config->max = max;
    config->rng = NULL;

    generator->name =
        "random_splitmix64";

    generator->config =
        config;

    generator->operations.init =
        generator_splitmix64_init;

    generator->operations.fill =
        generator_splitmix64_fill;

    generator->operations.clean =
        generator_splitmix64_clean;

    if (!generator_init(generator)) {
        generator_destroy(generator);
        return NULL;
    }

    return generator;
}