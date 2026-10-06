/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file CSRTopology.h
 * @brief Ownership and generation contract for CSR index topology.
 *
 * This header defines the storage object used to own the index arrays of
 * a compressed sparse row (CSR) topology.
 *
 * A CSRIndexBuffer owns:
 *
 * - row_ptr, containing nrows + 1 row offsets;
 * - col_ind, containing up to max_nnz_capacity column indices.
 *
 * The topology is intentionally kept separate from DataBuffer and DataView:
 * DataBuffer owns numerical data, CSRIndexBuffer owns CSR indices, and
 * DataView only references both objects without taking ownership.
 *
 * Topology generation and validation are setup-time operations and must not
 * be performed inside benchmark kernel hot paths.
 */

#ifndef CORE_CSR_TOPOLOGY_H
#define CORE_CSR_TOPOLOGY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* ========================================================================= */
/* CSR index storage                                                         */
/* ========================================================================= */

/**
 * @brief Owned storage for a CSR sparse topology.
 *
 * The buffer owns both dynamically allocated index arrays.
 *
 * @par Ownership
 * - @c row_ptr has @c nrows + 1 entries.
 * - @c col_ind has capacity for @c max_nnz_capacity entries.
 *
 * @par Invariants
 * After successful topology generation:
 *
 * - @c row_ptr[0] == 0;
 * - @c row_ptr[nrows] == nnz;
 * - @c row_ptr is monotonically non-decreasing;
 * - @c nnz <= max_nnz_capacity.
 */
typedef struct {
    uint64_t *row_ptr;
    uint64_t *col_ind;

    size_t nrows;
    size_t nnz;
    size_t max_nnz_capacity;
} CSRIndexBuffer;


/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/**
 * @brief Allocates an empty CSR index buffer.
 *
 * The function allocates:
 *
 * - @c nrows + 1 entries for @c row_ptr;
 * - @c max_nnz_capacity entries for @c col_ind.
 *
 * The returned buffer owns both allocations.
 *
 * @param nrows Number of rows in the CSR topology.
 * @param max_nnz_capacity Maximum number of non-zero entries that can be
 *        stored.
 *
 * @return A newly allocated CSRIndexBuffer on success, or @c NULL if the
 *         arguments are invalid or allocation fails.
 *
 * @pre @p nrows must be greater than zero.
 * @pre @p max_nnz_capacity must be greater than zero.
 *
 * @post The returned buffer has @c nnz == 0.
 * @post The returned buffer owns its index arrays.
 */
CSRIndexBuffer *create_csr_index_buffer(
    size_t nrows,
    size_t max_nnz_capacity
);


/**
 * @brief Releases all storage owned by a CSR index buffer.
 *
 * Passing @c NULL is allowed and has no effect.
 *
 * @param buffer CSR index buffer to destroy.
 */
void destroy_csr_index_buffer(
    CSRIndexBuffer *buffer
);


/* ========================================================================= */
/* Topology generation                                                       */
/* ========================================================================= */

/**
 * @brief Generates a CSR topology for a benchmark window.
 *
 * The generated topology contains approximately @p density of the possible
 * entries in the requested window. The exact number of generated entries is
 * stored in @c buffer->nnz.
 *
 * No memory is allocated or resized by this function. The generated topology
 * must fit within @c buffer->max_nnz_capacity.
 *
 * @param buffer CSR index buffer receiving the generated topology.
 * @param window_size Logical size of the generated window.
 * @param density Fraction of possible entries to generate.
 *
 * @return Zero on success, non-zero on failure.
 *
 * @pre @p buffer must not be NULL.
 * @pre @p window_size must be greater than zero.
 * @pre @p density must be in the interval (0.0, 1.0].
 *
 * @post On success, @c buffer->row_ptr describes exactly @c buffer->nnz
 *       entries in @c buffer->col_ind.
 * @post On success, @c buffer->nnz <=
 *       @c buffer->max_nnz_capacity.
 *
 * @note This function is intended for benchmark setup and data generation.
 *       It must not be called from a benchmark kernel.
 */
int generate_csr_topology(
    CSRIndexBuffer *buffer,
    uint64_t window_size,
    double density
);


/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/**
 * @brief Validates the structural invariants of a CSR topology.
 *
 * This function performs setup-time correctness checks and must not be used
 * inside benchmark kernel hot paths.
 *
 * At minimum, validation checks:
 *
 * - non-null index arrays;
 * - valid row offsets;
 * - monotonicity of @c row_ptr;
 * - consistency between @c row_ptr[nrows] and @c nnz;
 * - capacity bounds.
 *
 * @param buffer CSR index buffer to validate.
 *
 * @return @c true if the topology is structurally valid, otherwise @c false.
 *
 * @note This function validates CSR storage invariants. It does not measure
 *       benchmark performance and does not belong in the timed region.
 */
bool validate_csr_topology(
    const CSRIndexBuffer *buffer
);


#endif /* CORE_CSR_TOPOLOGY_H */
