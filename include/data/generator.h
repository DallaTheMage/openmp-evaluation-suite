#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "config/types.h"
#include "data/collection.h"
#include "data/rng.h"


typedef struct DataGenerator DataGenerator;


typedef struct {

    int (*init)(
        DataGenerator *generator
    );

    int (*fill)(
        DataGenerator *generator,
        Collection *collection
    );

    void (*clean)(
        DataGenerator *generator
    );

} DataGeneratorOperations;


struct DataGenerator {

    const char *name;

    void *config;

    DataGeneratorOperations operations;
};


/*
 * Generic API
 */

int generator_init(
    DataGenerator *generator
);

int generator_fill(
    DataGenerator *generator,
    Collection *collection
);

void generator_destroy(
    DataGenerator *generator
);


/*
 * Generic factory
 */

DataGenerator *generator_random_create(
    datatype min,
    datatype max
);


/*
 * Concrete generators
 */

DataGenerator *generator_xoshiro256_create(
    datatype min,
    datatype max
);

DataGenerator *generator_splitmix64_create(
    datatype min,
    datatype max
);


#endif /* DATA_GENERATOR_H */
