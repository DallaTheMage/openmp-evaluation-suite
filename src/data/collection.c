#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "core/utils.h"
#include "data/collection.h"

static int collection_init(Collection *collection) {
    if (collection == NULL) {
        return 0;
    }
    return 1;
}

static void collection_reset(const Collection *collection) {
    if (collection == NULL || collection->data == NULL) {
        return;
    }

    for (size_t i = 0; i < collection->size; ++i) {
        collection->data[i] = 0.0;
    }
}

static void collection_print(const Collection *collection) {
    if (collection == NULL || collection->data == NULL) {
        printf("Collection [NULL]\n");
        return;
    }

    printf(
        "Collection (%lu x %lu):\n",
        (unsigned long)collection->rows,
        (unsigned long)collection->columns
    );
}

static void collection_clean(Collection *collection) {
    if (collection == NULL) {
        return;
    }

    if (collection->data != NULL) {
        free(collection->data);
        collection->data = NULL;
    }

    collection->size = 0;
    collection->rows = 0;
    collection->columns = 0;
}

static const CollectionOperations DEFAULT_COLLECTION_OPERATIONS = {
    collection_init,
    collection_reset,
    collection_print,
    collection_clean
};

Collection *collection_create(size_t log2_n) {
    if (log2_n == 0) {
        return NULL;
    }

    size_t total_elements = GET_ELEMENT_COUNT(log2_n);
    size_t matrix_dim = GET_MATRIX_DIM(log2_n);

    Collection *collection = (Collection *)malloc(sizeof(Collection));
    if (collection == NULL) {
        return NULL;
    }

    collection->data = (double *)malloc(total_elements * sizeof(double));
    if (collection->data == NULL) {
        free(collection);
        return NULL;
    }

    collection->size = (uint64_t)total_elements;
    collection->rows = (uint64_t)matrix_dim;
    collection->columns = (uint64_t)matrix_dim;
    collection->operations = &DEFAULT_COLLECTION_OPERATIONS;

    /* First touch / page pre-warming */
    for (size_t i = 0; i < total_elements; ++i) {
        collection->data[i] = 1.0;
    }

    if (!collection->operations->init(collection)) {
        collection_destroy(collection);
        return NULL;
    }

    return collection;
}

void collection_destroy(Collection *collection) {
    if (collection == NULL) {
        return;
    }

    if (collection->operations != NULL && collection->operations->clean != NULL) {
        collection->operations->clean(collection);
    } else if (collection->data != NULL) {
        free(collection->data);
        collection->data = NULL;
    }

    free(collection);
}

void collection_update_matrix_dimensions(Collection *col) {
    if (col == NULL || col->size == 0) return;

    // Trattando il dataset come una matrice quadrata N x N
    // N = sqrt(size)
    size_t dim = (size_t)sqrt((double)col->size);

    col->rows = dim;
    col->columns = dim;
}
