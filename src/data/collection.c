#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "config/types.h"
#include "defines.h"
#include "data/collection.h"


/*
 * ============================================================
 * Private operations
 * ============================================================
 */

static int collection_init(
    Collection *collection
)
{
    if (collection == NULL) {
        return 0;
    }

    return 1;
}


static void collection_reset(
    const Collection *collection
)
{
    uint64 i;

    if (collection == NULL ||
        collection->data == NULL) {
        return;
    }

    for (i = 0; i < collection->size; ++i) {
        collection->data[i] = (datatype)0;
    }
}


static void collection_print(
    const Collection *collection
)
{
    if (collection == NULL ||
        collection->data == NULL) {

        printf("Collection [NULL]\n");
        return;
    }

    printf(
        "Collection (%" PRIu64 " x %" PRIu64 "):\n",
        collection->rows,
        collection->columns
    );
}


static void collection_clean(
    Collection *collection
)
{
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


/*
 * ============================================================
 * Default vtable
 * ============================================================
 */

static const CollectionOperations
DEFAULT_COLLECTION_OPERATIONS = {

    collection_init,
    collection_reset,
    collection_print,
    collection_clean
};


/*
 * ============================================================
 * Public lifecycle
 * ============================================================
 */

Collection *collection_create(
    size_t log2_n
)
{
    Collection *collection;

    size_t total_elements;
    size_t matrix_dim;

    size_t i;


    /*
     * log2_n == 0 is not a valid collection
     * in the current design.
     */
    if (log2_n == 0) {
        return NULL;
    }


    /*
     * Calculate:
     *
     *     N = 2^log2_n
     *
     *     dimension = sqrt(N)
     *
     * using your project macros.
     */
    total_elements = GET_ELEMENT_COUNT(log2_n);

    matrix_dim = GET_MATRIX_DIM(log2_n);


    /*
     * Allocate Collection object.
     */
    collection = (Collection *)malloc(
        sizeof(Collection)
    );

    if (collection == NULL) {
        return NULL;
    }


    /*
     * Allocate data.
     */
    collection->data =
        (datatype *)malloc(
            total_elements *
            sizeof(datatype)
        );

    if (collection->data == NULL) {

        free(collection);

        return NULL;
    }


    /*
     * Set metadata.
     */
    collection->size =
        (uint64)total_elements;

    collection->rows =
        (uint64)matrix_dim;

    collection->columns =
        (uint64)matrix_dim;

    collection->operations =
        &DEFAULT_COLLECTION_OPERATIONS;


    /*
     * First touch / page pre-warming.
     *
     * Important:
     *
     * We explicitly cast to datatype so that this
     * remains correct for float/integer datatypes.
     */
    for (i = 0; i < total_elements; ++i) {

        collection->data[i] =
            (datatype)1;
    }


    /*
     * Execute virtual init().
     */
    if (!collection->operations->init(
            collection)) {

        collection_destroy(collection);

        return NULL;
    }


    return collection;
}


void collection_destroy(
    Collection *collection
)
{
    if (collection == NULL) {
        return;
    }


    if (collection->operations != NULL &&
        collection->operations->clean != NULL) {

        collection->operations->clean(
            collection
        );

    } else if (collection->data != NULL) {

        free(collection->data);

        collection->data = NULL;
    }


    free(collection);
}