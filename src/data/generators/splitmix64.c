#include <stdlib.h>
#include <time.h>
#include "data/generator.h"
#include "data/rng.h"

typedef struct {
    double min;
    double max;
    RandomSource *rng;
} SplitMix64GeneratorConfig;

/*
 * Convert uint64_t to double in range [0, 1)
 */
static double random_to_double(uint64_t value) {
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}

static int generator_splitmix64_init(DataGenerator *generator) {
    SplitMix64GeneratorConfig *config = generator ? generator->config : NULL;
    if (config == NULL) {
        return 0;
    }
    config->rng = random_create((uint64_t)time(NULL));
    return config->rng != NULL;
}

static int generator_splitmix64_fill(DataGenerator *generator, Collection *collection) {
    SplitMix64GeneratorConfig *config;
    if (generator == NULL ||
        collection == NULL ||
        collection->data == NULL ||
        (config = generator->config) == NULL ||
        config->rng == NULL) {
        return 0;
    }

    double min = config->min;
    double range = config->max - min;

    for (uint64_t i = 0; i < collection->size; ++i) {
        double normalized = random_to_double(random_next(config->rng));
        collection->data[i] = min + normalized * range;
    }

    return 1;
}

static void generator_splitmix64_clean(DataGenerator *generator) {
    SplitMix64GeneratorConfig *config;
    if (generator == NULL) {
        return;
    }
    config = generator->config;
    if (config != NULL) {
        if (config->rng != NULL) {
            random_destroy(config->rng);
        }
        free(config);
    }
    free(generator);
}

DataGenerator *generator_splitmix64_create(double min, double max) {
    DataGenerator *generator = malloc(sizeof(*generator));
    if (generator == NULL) {
        return NULL;
    }
    SplitMix64GeneratorConfig *config = malloc(sizeof(*config));
    if (config == NULL) {
        free(generator);
        return NULL;
    }
    *config = (SplitMix64GeneratorConfig) {
        .min = min,
        .max = max,
        .rng = NULL
    };
    *generator = (DataGenerator) {
        .name = "random_splitmix64",
        .config = config,
        .operations = {
            .init  = generator_splitmix64_init,
            .fill  = generator_splitmix64_fill,
            .clean = generator_splitmix64_clean
        }
    };
    if (!generator_init(generator)) {
        generator_destroy(generator);
        return NULL;
    }
    return generator;
}
