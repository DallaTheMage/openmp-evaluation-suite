/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
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
 * DataView objects do not own the underlying DataBuffer.
 * The referenced DataBuffer must remain alive for the entire lifetime
 * of the view.
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
 *
 * All fields are derived from the logical DataView window.
 */
typedef struct {
    uint64_t length;
} View1DMetadata;


/**
 * @brief Metadata describing a two-dimensional view.
 *
 * cols is a layout parameter while rows is derived from the logical
 * DataView window.
 */
typedef struct {
    uint64_t rows;
    uint64_t cols;
} View2DMetadata;


/**
 * @brief Metadata describing a three-dimensional view.
 *
 * depth and cols are layout parameters while height is derived from
 * the logical DataView window.
 */
typedef struct {
    uint64_t height;
    uint64_t depth;
    uint64_t cols;
} View3DMetadata;


/**
 * @brief Metadata describing an Array-of-Structures view.
 *
 * struct_size is a layout parameter while num_structs is derived from
 * the logical DataView window.
 */
typedef struct {
    uint64_t num_structs;
    uint64_t struct_size;
} ViewAoSMetadata;


/**
 * @brief Metadata describing a Structure-of-Arrays view.
 *
 * num_fields is a layout parameter while field_length is derived from
 * the logical DataView window.
 */
typedef struct {
    uint64_t num_fields;
    uint64_t field_length;
} ViewSoAMetadata;


/**
 * @brief Metadata describing an Array-of-Structures-of-Arrays view.
 *
 * vector_length and num_fields are layout parameters while num_blocks
 * is derived from the logical DataView window.
 */
typedef struct {
    uint64_t num_blocks;
    uint64_t vector_length;
    uint64_t num_fields;
} ViewAoSoAMetadata;


/**
 * @brief Layout-specific metadata associated with a DataView.
 *
 * Only the member corresponding to DataView::type is valid.
 *
 * The metadata combines configuration parameters with quantities derived
 * from the logical DataView window.
 */
typedef union {
    View1DMetadata     v1d;
    View2DMetadata     v2d;
    View3DMetadata     v3d;
    ViewAoSMetadata    aos;
    ViewSoAMetadata    soa;
    ViewAoSoAMetadata  aosoa;
} ViewMetadata;


/**
 * @brief Runtime descriptor used to construct a DataView.
 *
 * The descriptor contains only the parameters that define the selected
 * layout. Quantities derived from the logical window are computed by
 * the view-shape functions.
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
        uint64_t cols;
    } v2d;

    /**
     * @brief Parameters for a three-dimensional view.
     */
    struct {
        uint64_t depth;
        uint64_t cols;
    } v3d;

    /**
     * @brief Parameters for an Array-of-Structures view.
     */
    struct {
        uint64_t struct_size;
    } aos;

    /**
     * @brief Parameters for a Structure-of-Arrays view.
     */
    struct {
        uint64_t num_fields;
    } soa;

    /**
     * @brief Parameters for an Array-of-Structures-of-Arrays view.
     */
    struct {
        uint64_t vector_length;
        uint64_t num_fields;
    } aosoa;
} ViewCreateConfig;


/**
 * @brief Non-owning logical view over a DataBuffer.
 *
 * A DataView combines a data buffer, a logical window and the metadata
 * required to interpret that window according to a specific layout.
 *
 * Ownership:
 * - the DataBuffer is not owned.
 *
 * The referenced DataBuffer must remain alive for the entire lifetime
 * of the DataView.
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
 * @return An initialized DataView. A zero-initialized DataView is returned
 *         if the input arguments are invalid.
 *
 * @pre buffer must not be NULL.
 * @pre config must not be NULL.
 * @pre offset and size must describe a region contained in buffer.
 *
 * @warning The returned DataView does not take ownership of buffer.
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