#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#include <stddef.h>

#include "config/types.h"


typedef struct Collection Collection;


/*
 * ============================================================
 * Collection operations
 * ============================================================
 */

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


/*
 * ============================================================
 * Collection
 * ============================================================
 */

struct Collection {

    datatype *data;

    uint64 size;

    uint64 rows;
    uint64 columns;

    const CollectionOperations *operations;
};


/*
 * ============================================================
 * Public lifecycle
 * ============================================================
 */

/*
 * log2_n represents:
 *
 *     size = 2^log2_n
 *
 * and the collection is a square matrix.
 */
Collection *collection_create(
    size_t log2_n
);

void collection_destroy(
    Collection *collection
);


#endif /* DATA_COLLECTION_H */