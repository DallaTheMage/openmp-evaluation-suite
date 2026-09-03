#include <stdlib.h>
#include <time.h>

#include "data/generator.h"
#include "data/rng.h"

typedef struct Xoshiro256GeneratorConfig Xoshiro256GeneratorConfig;

struct Xoshiro256GeneratorConfig {
    datatype min;
    datatype max;
    RandomSource *rng;
};

/*
 * ============================================================
 * Random uint64 -> double [0, 1)
 * ============================================================
 */
static double random_to_double(uint64 value)
{
    return (double)(value >> 11)
         * (1.0 / 9007199254740992.0);
}


/*
 * ============================================================
 * Init
 * ============================================================
 */
static int generator_xoshiro256_init(DataGenerator *generator) {
    Xoshiro256GeneratorConfig *config;
    if (generator == NULL ||
        generator->config == NULL) {
        return 0;
    }
    config = (Xoshiro256GeneratorConfig *)generator->config;
    config->rng = random_create((uint64)time(NULL));
    if (config->rng == NULL) { return 0; }
    return 1;
}


/*
 * ============================================================
 * Fill
 * ============================================================
 */
static int generator_xoshiro256_fill(
    DataGenerator *generator,
    Collection *collection) {
    Xoshiro256GeneratorConfig *config;
    uint64 i;
    double normalized;
    double min;
    double max;
    if (generator == NULL ||
        generator->config == NULL ||
        collection == NULL ||
        collection->data == NULL) {
        return 0;
    }
    config = (Xoshiro256GeneratorConfig *)generator->config;
    min = (double)config->min;
    max = (double)config->max;
    for (i = 0; i < collection->size; ++i) {
        normalized = random_to_double(random_next(config->rng));
        collection->data[i] = (datatype)(min + normalized * (max - min));
    }
    return 1;
}

/*
 * ============================================================
 * Clean
 * ============================================================
 */
static void generator_xoshiro256_clean(DataGenerator *generator) {
    Xoshiro256GeneratorConfig *config;
    if (generator == NULL) { return; }
    config = (Xoshiro256GeneratorConfig *)generator->config;
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
DataGenerator *generator_xoshiro256_create(
    datatype min,
    datatype max) {
    DataGenerator *generator;
    Xoshiro256GeneratorConfig *config;
    generator = (DataGenerator *)malloc(sizeof(DataGenerator));
    if (generator == NULL) {
        return NULL;
    }
    config =
        (Xoshiro256GeneratorConfig *)malloc(
            sizeof(Xoshiro256GeneratorConfig)
        );
    if (config == NULL) {
        free(generator);
        return NULL;
    }
    config->min = min;
    config->max = max;
    config->rng = NULL;
    generator->name = "random_xoshiro256";
    generator->config = config;
    generator->operations.init =
        generator_xoshiro256_init;

    generator->operations.fill =
        generator_xoshiro256_fill;

    generator->operations.clean =
        generator_xoshiro256_clean;

    if (!generator_init(generator)) {
        generator_destroy(generator);
        return NULL;
    }
    return generator;
}
