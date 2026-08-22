#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

#include "config/types.h"

typedef struct Collection Collection;

typedef struct {
    int  (*init)(Collection *collection);
    void (*reset)(const Collection *collection);
    void (*print)(const Collection *collection);
    void (*clean)(Collection *collection);
} CollectionOperations;

struct Collection {
    datatype                   *data;
    uint64                      size;
    uint64                      rows;
    uint64                      columns;
    const CollectionOperations *operations;
};

/* Prototipi delle funzioni pubbliche esposte */
Collection* collection_create(size_t size);
void        collection_destroy(Collection *collection);

#endif /* DATA_COLLECTION_H */