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
 * @file DataView.c
 * @brief Non-owning views over logical data layouts.
 *
 * This module constructs DataView objects over existing DataBuffer objects.
 *
 * DataView creation is a setup-time operation. No allocation or I/O is
 * performed here, and no function belongs in a benchmark measurement
 * region.
 */

#include "data/DataView.h"
#include "data/views/ViewShapeFns.h"

#include <stdint.h>


/* ========================================================================= */
/* --- Internal helpers --------------------------------------------------- */
/* ========================================================================= */

/**
 * @brief Check whether a logical window is contained in a DataBuffer.
 *
 * The subtraction-based check avoids evaluating offset + size directly
 * and therefore avoids unsigned integer overflow.
 *
 * @param buffer DataBuffer to inspect.
 * @param offset Window starting offset in elements.
 * @param size Window length in elements.
 *
 * @return Non-zero if the complete window is contained in the buffer,
 *         zero otherwise.
 */
static int window_is_valid(
    const DataBuffer *buffer,
    uint64_t offset,
    uint64_t size
)
{
    uint64_t element_count;

    if (buffer == NULL) {
        return 0;
    }

    element_count = (uint64_t)buffer->element_count;

    if (offset > element_count) {
        return 0;
    }

    return size <= element_count - offset;
}


/**
 * @brief Initialize metadata from a view creation descriptor.
 *
 * Only configuration parameters are copied here. Derived quantities are
 * computed afterwards by recompute_metadata().
 *
 * @param view DataView being initialized.
 * @param type Requested view type.
 * @param config View creation parameters.
 */
static void initialize_metadata(
    DataView *view,
    ViewType type,
    const ViewCreateConfig *config
)
{
    switch (type) {
        case VIEW_1D:
            view->meta.v1d.length = config->v1d.length;
            break;

        case VIEW_2D:
            view->meta.v2d.cols = config->v2d.cols;
            break;

        case VIEW_3D:
            view->meta.v3d.depth = config->v3d.depth;
            view->meta.v3d.cols = config->v3d.cols;
            break;

        case VIEW_AOS:
            view->meta.aos.struct_size =
                config->aos.struct_size;
            break;

        case VIEW_SOA:
            view->meta.soa.num_fields =
                config->soa.num_fields;
            break;

        case VIEW_AOSOA:
            view->meta.aosoa.vector_length =
                config->aosoa.vector_length;
            view->meta.aosoa.num_fields =
                config->aosoa.num_fields;
            break;

        case VIEW_TYPE_COUNT:
        default:
            break;
    }
}


/**
 * @brief Recompute all derived metadata for a view.
 *
 * @param view DataView whose metadata must be recomputed.
 */
static void recompute_metadata(DataView *view)
{
    switch (view->type) {
        case VIEW_1D:
            shape_1d_recompute(view);
            break;

        case VIEW_2D:
            shape_2d_recompute(view);
            break;

        case VIEW_3D:
            shape_3d_recompute(view);
            break;

        case VIEW_AOS:
            shape_aos_recompute(view);
            break;

        case VIEW_SOA:
            shape_soa_recompute(view);
            break;

        case VIEW_AOSOA:
            shape_aosoa_recompute(view);
            break;

        case VIEW_TYPE_COUNT:
        default:
            break;
    }
}


/* ========================================================================= */
/* --- Public API --------------------------------------------------------- */
/* ========================================================================= */

DataView create_data_view(
    DataBuffer *buffer,
    ViewType type,
    const ViewCreateConfig *config,
    uint64_t offset,
    uint64_t size
)
{
    DataView view = {0};

    if (buffer == NULL || config == NULL) {
        return view;
    }

    if (type < VIEW_1D || type >= VIEW_TYPE_COUNT) {
        return view;
    }

    if (!window_is_valid(buffer, offset, size)) {
        return view;
    }

    view.buffer = buffer;
    view.type = type;
    view.window.offset = offset;
    view.window.size = size;

    initialize_metadata(&view, type, config);
    recompute_metadata(&view);

    return view;
}