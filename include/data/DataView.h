/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP Benchmark Suite.
 *
 * The OpenMP Benchmark Suite is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The OpenMP Benchmark Suite is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the OpenMP Benchmark Suite. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 * @file DataView.h
 * @brief Non-owning views over logical data layouts.
 *
 * A DataView provides a lightweight description of how a region of a
 * DataBuffer must be interpreted by a benchmark kernel.
 *
 * DataView objects do not own the underlying DataBuffer or CSR topology.
 * Referenced objects must remain alive for the entire lifetime of the view.
 */

#ifndef DATA_DATAVIEW_H
#define DATA_DATAVIEW_H

#include <stdint.h>

#include "data/DataBuffer.h"
#include "data/views/ViewType.h"


/**
 * @brief Logical region of a DataBuffer.
 *
 * The offset and size are expressed in number of double elements.
 *
 * The represented interval is:
 *
 *     [offset, offset + size)
 *
 * @note DataWindow does not own any memory.
 */
typedef struct {
    uint64_t offset;
    uint64_t size;
} DataWindow;


/**
 * @brief Metadata describing a one-dimensional view.
 */
typedef struct {
    uint64_t length;
} View1DMetadata;


/**
 * @brief Metadata describing a two-dimensional view.
 */
typedef struct {
    uint64_t rows;
    uint64_t cols;
} View2DMetadata;


/**
 * @brief Metadata describing a three-dimensional view.
 */
typedef struct {
    uint64_t height;
    uint64_t depth;
    uint64_t cols;
} View3DMetadata;


/**
 * @brief Metadata describing an Array-of-Structures view.
 */
typedef struct {
    uint64_t num_structs;
    uint64_t struct_size;
} ViewAoSMetadata;


/**
 * @brief Metadata describing a Structure-of-Arrays view.
 */
typedef struct {
    uint64_t num_fields;
    uint64_t field_length;
} ViewSoAMetadata;


/**
 * @brief Metadata describing an Array-of-Structures-of-Arrays view.
 */
typedef struct {
    uint64_t num_blocks;
    uint64_t vector_length;
    uint64_t num_fields;
} ViewAoSoAMetadata;


/**
 * @brief Metadata describing a Compressed Sparse Row view.
 *
 * The CSR topology is referenced but not owned by the DataView.
 */
typedef struct {
    uint64_t nrows;
    uint64_t ncols;
    uint64_t nnz;

    /**
     * @brief Non-owning pointer to the CSR row-pointer array.
     */
    const uint64_t *row_ptr;

    /**
     * @brief Non-owning pointer to the CSR column-index array.
     */
    const uint64_t *col_ind;
} ViewCSRMetadata;


/**
 * @brief Layout-specific metadata associated with a DataView.
 *
 * Only the member corresponding to DataView::type is valid.
 */
typedef union {
    View1DMetadata     v1d;
    View2DMetadata     v2d;
    View3DMetadata     v3d;
    ViewAoSMetadata    aos;
    ViewSoAMetadata    soa;
    ViewAoSoAMetadata  aosoa;
    ViewCSRMetadata    csr;
} ViewMetadata;


/**
 * @brief Runtime descriptor used to construct a DataView.
 *
 * The descriptor contains the parameters required to initialize the
 * metadata associated with a specific view type.
 *
 * CSR views additionally reference externally managed topology arrays.
 * This descriptor does not take ownership of any referenced memory.
 *
 * Only the member corresponding to the requested ViewType is valid.
 */
typedef union {
    /**
     * @brief Parameters for a one-dimensional view.
     */
    struct {
        uint64_t length;
    } v1d;

    /**
     * @brief Parameters for a two-dimensional view.
     */
    struct {
        uint64_t rows;
        uint64_t cols;
    } v2d;

    /**
     * @brief Parameters for a three-dimensional view.
     */
    struct {
        uint64_t height;
        uint64_t depth;
        uint64_t cols;
    } v3d;

    /**
     * @brief Parameters for an Array-of-Structures view.
     */
    struct {
        uint64_t num_structs;
        uint64_t struct_size;
    } aos;

    /**
     * @brief Parameters for a Structure-of-Arrays view.
     */
    struct {
        uint64_t num_fields;
        uint64_t field_length;
    } soa;

    /**
     * @brief Parameters for an Array-of-Structures-of-Arrays view.
     */
    struct {
        uint64_t num_blocks;
        uint64_t vector_length;
        uint64_t num_fields;
    } aosoa;

    /**
     * @brief Parameters for a Compressed Sparse Row view.
     */
    struct {
        uint64_t nrows;
        uint64_t ncols;
        uint64_t nnz;

        const uint64_t *row_ptr;
        const uint64_t *col_ind;
    } csr;
} ViewCreateConfig;


/**
 * @brief Non-owning logical view over a DataBuffer.
 *
 * A DataView combines a data buffer, a logical window and the metadata
 * required to interpret that window according to a specific layout.
 *
 * Ownership:
 * - the DataBuffer is not owned;
 * - CSR row_ptr is not owned;
 * - CSR col_ind is not owned.
 *
 * All referenced objects must remain alive for the entire lifetime of
 * the DataView.
 */
typedef struct {
    DataBuffer  *buffer;
    ViewType     type;
    DataWindow   window;
    ViewMetadata meta;
} DataView;


/**
 * @brief Creates a DataView over an existing DataBuffer.
 *
 * The function initializes the view metadata according to the requested
 * ViewType and the supplied creation descriptor.
 *
 * @param buffer DataBuffer referenced by the view.
 * @param type Logical layout used to interpret the view.
 * @param config Layout-specific construction parameters.
 * @param offset Offset of the logical window in double elements.
 * @param size Number of double elements covered by the logical window.
 *
 * @return An initialized DataView.
 *
 * @pre buffer must not be NULL.
 * @pre config must not be NULL.
 * @pre offset + size must not exceed the DataBuffer capacity.
 *
 * @warning The returned DataView does not take ownership of buffer or
 *          any CSR topology referenced by config.
 *
 * @note The function does not allocate storage for the DataView itself.
 */
DataView create_data_view(
    DataBuffer *buffer,
    ViewType type,
    const ViewCreateConfig *config,
    uint64_t offset,
    uint64_t size
);


#endif /* DATA_DATAVIEW_H */