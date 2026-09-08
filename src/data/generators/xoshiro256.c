#include <stdlib.h>
#include <time.h>

#include "data/generator.h"
#include "data/rng.h"

typedef struct Xoshiro256GeneratorConfig Xoshiro256GeneratorConfig;

struct Xoshiro256GeneratorConfig {
    double min;
    double max;
    RandomSource *rng;
};

/*
 * Convert uint64_t to double in range [0, 1)
 */
static double random_to_double(uint64_t value) {
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}

/*
 * Init
 */
static int generator_xoshiro256_init(DataGenerator *generator) {
    Xoshiro256GeneratorConfig *config;
    if (generator == NULL || generator->config == NULL) {
        return 0;
    }
    config = (Xoshiro256GeneratorConfig *)generator->config;
    config->rng = random_create((uint64_t)time(NULL));
    return config->rng != NULL;
}

/*
 * Fill
 */
static int generator_xoshiro256_fill(DataGenerator *generator, Collection *collection) {
    Xoshiro256GeneratorConfig *config;
    if (generator == NULL ||
        generator->config == NULL ||
        collection == NULL ||
        collection->data == NULL) {
        return 0;
    }

    config = (Xoshiro256GeneratorConfig *)generator->config;
    double min = config->min;
    double range = config->max - min;

    for (uint64_t i = 0; i < collection->size; ++i) {
        double normalized = random_to_double(random_next(config->rng));
        collection->data[i] = min + normalized * range;
    }

    return 1;
}

/*
 * Clean
 */
static void generator_xoshiro256_clean(DataGenerator *generator) {
    Xoshiro256GeneratorConfig *config;
    if (generator == NULL) {
        return;
    }
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
 * Factory
 */
DataGenerator *generator_xoshiro256_create(double min, double max) {
    DataGenerator *generator = (DataGenerator *)malloc(sizeof(DataGenerator));
    if (generator == NULL) {
        return NULL;
    }

    Xoshiro256GeneratorConfig *config = (Xoshiro256GeneratorConfig *)malloc(sizeof(Xoshiro256GeneratorConfig));
    if (config == NULL) {
        free(generator);
        return NULL;
    }

    config->min = min;
    config->max = max;
    config->rng = NULL;

    generator->name = "random_xoshiro256";
    generator->config = config;
    generator->operations.init = generator_xoshiro256_init;
    generator->operations.fill = generator_xoshiro256_fill;
    generator->operations.clean = generator_xoshiro256_clean;

    if (!generator_init(generator)) {
        generator_destroy(generator);
        return NULL;
    }
    return generator;
}
