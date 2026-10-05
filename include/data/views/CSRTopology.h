#ifndef CORE_CSR_TOPOLOGY_H
#define CORE_CSR_TOPOLOGY_H

#include <stddef.h>
#include <stdint.h>


/* ========================================================================= */
/* CSR index storage                                                        */
/* ========================================================================= */

typedef struct {
    uint64_t *row_ptr;
    uint64_t *col_ind;

    size_t nrows;
    size_t nnz;
    size_t max_nnz_capacity;

} CSRIndexBuffer;


/* ========================================================================= */
/* Lifecycle                                                                */
/* ========================================================================= */

/**
 * Allocates storage for a CSR topology.
 *
 * row_ptr contains nrows + 1 entries.
 * col_ind contains up to max_nnz_capacity entries.
 */
CSRIndexBuffer *create_csr_index_buffer(
    size_t nrows,
    size_t max_nnz_capacity
);


/**
 * Releases the CSR topology storage.
 */
void destroy_csr_index_buffer(
    CSRIndexBuffer *buffer
);


/* ========================================================================= */
/* Topology generation                                                      */
/* ========================================================================= */

/**
 * Generates a CSR topology for a window.
 *
 * The generated topology represents approximately `density` of the
 * elements in the window.
 *
 * The actual number of entries is stored in buffer->nnz.
 *
 * row_ptr and col_ind remain owned by CSRIndexBuffer.
 */
int generate_csr_topology(
    CSRIndexBuffer *buffer,
    uint64_t window_size,
    double density
);

#endif /* CORE_CSR_TOPOLOGY_H */
