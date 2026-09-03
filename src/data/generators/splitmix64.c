#include <stdlib.h>
#include <time.h>
#include "data/generator.h"
#include "data/rng.h"

typedef struct {
    datatype min;
    datatype max;
    RandomSource *rng;
} SplitMix64GeneratorConfig;

#if DATATYPE_IS_FLOAT
static float random_to_float(uint64 value) {
    return (float)(value >> 40) * (1.0f / 16777216.0f);
}
#elif DATATYPE_IS_DOUBLE
static double random_to_double(uint64 value) {
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}
#endif

static int generator_splitmix64_init(DataGenerator *generator) {
    SplitMix64GeneratorConfig *config = generator ? generator->config : NULL;
    if (config == NULL) {
        return 0;
    }
    config->rng = random_create((uint64)time(NULL));
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
#if DATATYPE_IS_FLOAT || DATATYPE_IS_DOUBLE
    {
#if DATATYPE_IS_FLOAT
        float min = (float)config->min;
        float range = (float)config->max - min;
#else
        double min = (double)config->min;
        double range = (double)config->max - min;
#endif
        for (uint64 i = 0; i < collection->size; ++i) {
#if DATATYPE_IS_FLOAT
            float normalized = random_to_float(random_next(config->rng));
#else
            double normalized = random_to_double(random_next(config->rng));
#endif
            collection->data[i] =
                (datatype)(min + normalized * range);
        }
    }
#elif DATATYPE_IS_INTEGER
    {
        uint64 range = (uint64)config->max - (uint64)config->min;
        for (uint64 i = 0; i < collection->size; ++i) {
            uint64 value = range ? random_next(config->rng) % range : 0;
            collection->data[i] = (datatype)((uint64)config->min + value);
        }
    }
#else
    return 0;
#endif
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

DataGenerator *generator_splitmix64_create(datatype min, datatype max) {
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