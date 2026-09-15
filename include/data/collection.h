#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#include <stddef.h>
#include <stdint.h>

typedef struct Collection Collection;

typedef struct {
    int  (*init)(Collection *collection);
    void (*reset)(
        const Collection *collection
    );
    void (*print)(
        const Collection *collection
    );
    void (*clean)(
        Collection *collection
    );

} CollectionOperations;

struct Collection {
    double *data;
    uint64_t size;
    uint64_t rows;
    uint64_t columns;
    const CollectionOperations *operations;
};

/*
 * log2_n represents:
 *     size = 2^log2_n
 * and the collection is a square matrix.
 */
Collection *collection_create(
    size_t log2_n
);

void collection_destroy(
    Collection *collection
);

void collection_update_matrix_dimensions(Collection *col);

#endif /* DATA_COLLECTION_H */
