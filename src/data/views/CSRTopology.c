#include "data/views/CSRTopology.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


/* ========================================================================= */
/* --- CSR Index Buffer ---------------------------------------------------- */
/* ========================================================================= */

CSRIndexBuffer *create_csr_index_buffer(size_t max_nnz_capacity)
{
    CSRIndexBuffer *buf =
        (CSRIndexBuffer *)malloc(sizeof(CSRIndexBuffer));

    if (!buf)
        return NULL;

    buf->max_nnz_capacity = max_nnz_capacity;

    /*
     * row_ptr contains nrows + 1 entries.
     *
     * At allocation time we only know the maximum number of nnz,
     * so max_nnz_capacity + 1 is the safe upper bound for row_ptr.
     */
    if (max_nnz_capacity == SIZE_MAX) {
        free(buf);
        return NULL;
    }

    buf->row_ptr =
        (uint64_t *)malloc(
            sizeof(uint64_t) * (max_nnz_capacity + 1));

    buf->col_ind =
        (uint64_t *)malloc(
            sizeof(uint64_t) * max_nnz_capacity);

    if (!buf->row_ptr || !buf->col_ind) {
        free(buf->row_ptr);
        free(buf->col_ind);
        free(buf);
        return NULL;
    }

    return buf;
}


void destroy_csr_index_buffer(CSRIndexBuffer *buf)
{
    if (!buf)
        return;

    free(buf->row_ptr);
    free(buf->col_ind);
    free(buf);
}


/* ========================================================================= */
/* --- CSR Topology Generation -------------------------------------------- */
/* ========================================================================= */

DataView create_csr_view_for_nnz(
    DataBuffer *buffer,
    uint64_t offset,
    uint64_t nnz_size,
    CSRIndexBuffer *idx_buf,
    uint32_t nnz_per_row
) {
    assert(buffer != NULL);
    assert(idx_buf != NULL);
    assert(nnz_per_row > 0);
    assert(nnz_size <= idx_buf->max_nnz_capacity);

    /*
     * Empty CSR view.
     */
    if (nnz_size == 0) {
        return create_data_view_csr(
            buffer,
            offset,
            0,
            0,
            0,
            NULL,
            NULL
        );
    }

    /*
     * Number of rows needed to contain nnz_size elements,
     * given the configured number of non-zeros per row.
     *
     * Written without:
     *
     *     nnz_size + nnz_per_row - 1
     *
     * to avoid possible unsigned overflow.
     */
    uint64_t nrows =
        nnz_size / nnz_per_row +
        (nnz_size % nnz_per_row != 0);

    /*
     * V1 keeps the synthetic CSR topology square.
     *
     * This is a topology-generation choice, not a property of
     * the generic CSR shape logic.
     */
    uint64_t ncols = nrows;

    uint64_t current_nnz = 0;

    idx_buf->row_ptr[0] = 0;

    for (uint64_t r = 0; r < nrows; ++r) {

        uint32_t row_nnz = nnz_per_row;

        /*
         * The last row may contain fewer elements.
         */
        uint64_t remaining =
            nnz_size - current_nnz;

        if (remaining < row_nnz)
            row_nnz = (uint32_t)remaining;

        for (uint32_t k = 0; k < row_nnz; ++k) {

            /*
             * Deterministic synthetic topology.
             *
             * Columns wrap around the square matrix.
             */
            idx_buf->col_ind[current_nnz] =
                (r + k) % ncols;

            ++current_nnz;
        }

        idx_buf->row_ptr[r + 1] =
            current_nnz;
    }

    /*
     * Sanity check: topology must contain exactly the requested nnz.
     */
    assert(current_nnz == nnz_size);

    return create_data_view_csr(
        buffer,
        offset,
        nnz_size,
        nrows,
        ncols,
        idx_buf->row_ptr,
        idx_buf->col_ind
    );
}
