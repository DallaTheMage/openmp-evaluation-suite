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
 * @file ViewType.h
 * @brief Enumeration of supported logical data-view layouts.
 *
 * ViewType identifies how a DataView interprets the underlying DataBuffer.
 *
 * The enumeration values are used as stable dispatch identifiers by
 * view-shape functions, kernel dispatch code and other layout-dependent
 * components.
 */

#ifndef DATA_VIEW_TYPE_H
#define DATA_VIEW_TYPE_H


/**
 * @brief Identifies a supported logical data-view layout.
 *
 * @note VIEW_TYPE_COUNT is not a valid view type. It is a sentinel used
 *       to determine the number of supported view types and to size
 *       dispatch tables.
 */
typedef enum {
    /**
     * @brief One-dimensional contiguous view.
     */
    VIEW_1D = 0,

    /**
     * @brief Two-dimensional row-major view.
     */
    VIEW_2D,

    /**
     * @brief Three-dimensional row-major view.
     */
    VIEW_3D,

    /**
     * @brief Compressed Sparse Row view.
     */
    VIEW_CSR,

    /**
     * @brief Array-of-Structures view.
     */
    VIEW_AOS,

    /**
     * @brief Structure-of-Arrays view.
     */
    VIEW_SOA,

    /**
     * @brief Array-of-Structures-of-Arrays view.
     */
    VIEW_AOSOA,

    /**
     * @brief Number of supported view types.
     *
     * This value is a sentinel and must not be used as a ViewType.
     */
    VIEW_TYPE_COUNT
} ViewType;


#endif /* DATA_VIEW_TYPE_H */
