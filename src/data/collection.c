#include <stdio.h>
#include <stdlib.h>
#include "config/types.h"
#include "defines.h"
#include "data/collection.h"

/* Forward declaration per C89 */
void collection_destroy(Collection *collection);

/* --- METODI INTERNI PRIVATI PER LA VTABLE DI COLLECTION --- */

static int collection_init(Collection *collection) {
    if (collection == NULL) return 0;
    return 1;
}

static void collection_reset(const Collection *collection) {
    if (collection == NULL || collection->data == NULL) return;
}

static void collection_print(const Collection *collection) {
    if (collection == NULL || collection->data == NULL) {
        printf("Collection [NULL]\n");
        return;
    }

    printf("Collection (%lu x %lu):\n  ",
           (unsigned long)collection->rows,
           (unsigned long)collection->columns);
}

static void collection_clean(Collection *collection) {
    if (collection == NULL) return;

    if (collection->data != NULL) {
        free(collection->data);
        collection->data = NULL;
    }
    collection->size = 0;
    collection->rows = 0;
    collection->columns = 0;
}

/* Inizializzazione della VTable di default */
static const CollectionOperations DEFAULT_COLLECTION_OPERATIONS = {
    collection_init,
    collection_reset,
    collection_print,
    collection_clean
};

/* --- FUNZIONI PUBBLICHE PER IL CICLO DI VITA --- */

Collection* collection_create(size_t log2_n) {
    Collection *collection;
    size_t total_elements;
    size_t matrix_dim;
    size_t i;

    if (log2_n == 0) return NULL;

    /*
     * Calcola il numero totale di elementi N (2^log2_n)
     * e la dimensione della matrice quadrata sqrt(N) via macro
     */
    total_elements = GET_ELEMENT_COUNT(log2_n);
    matrix_dim = GET_MATRIX_DIM(log2_n);

    collection = (Collection *)malloc(sizeof(Collection));
    if (collection == NULL) return NULL;

    collection->data = (datatype *)malloc(total_elements * sizeof(datatype));
    if (collection->data == NULL) {
        free(collection);
        return NULL;
    }

    /* Assegnazione corretta delle metriche dimensionali */
    collection->size = total_elements;
    collection->rows = matrix_dim;
    collection->columns = matrix_dim;
    collection->operations = &DEFAULT_COLLECTION_OPERATIONS;

    /*
     * First Touch / Pre-warming:
     * Inizializziamo i dati per allocare fisicamente le pagine RAM
     * ed evitare trappole FPU (denormalized numbers / NaN)
     */
    for (i = 0; i < total_elements; ++i) {
        collection->data[i] = 1.0;
    }

    if (!collection->operations->init(collection)) {
        collection_destroy(collection);
        return NULL;
    }

    return collection;
}

void collection_destroy(Collection *collection) {
    if (collection == NULL) return;

    if (collection->operations != NULL && collection->operations->clean != NULL) {
        collection->operations->clean(collection);
    } else if (collection->data != NULL) {
        free(collection->data);
    }
    free(collection);
}