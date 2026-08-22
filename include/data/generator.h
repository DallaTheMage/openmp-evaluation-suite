#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "collection.h"

typedef struct DataGenerator DataGenerator;

typedef struct {
    int (*init)(DataGenerator *gen);
    int (*fill)(DataGenerator *gen, Collection *collection);
    void (*clean)(DataGenerator *gen);
} DataGeneratorOperations;

struct DataGenerator {
    const char *name;
    void *config;
    DataGeneratorOperations operations;
};

DataGenerator* generator_xoshiro256_create(double min, double max);

#endif /* DATA_GENERATOR_H */