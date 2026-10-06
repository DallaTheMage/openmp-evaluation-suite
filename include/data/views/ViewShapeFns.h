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
 * @file ViewShapeFns.h
 * @brief Shape computation and validation functions for DataView objects.
 *
 * The functions declared in this header compute and validate the
 * layout-specific metadata stored in a DataView.
 *
 * Shape recomputation is intended to be performed when a view is created
 * or when its logical window changes. These functions must not be called
 * from the hot loop of a benchmark kernel.
 *
 * Validation functions are intended for correctness checks and debugging.
 * They are not part of the benchmark hot path.
 */

#ifndef DATA_VIEW_SHAPE_FNS_H
#define DATA_VIEW_SHAPE_FNS_H

#include <stdbool.h>

#include "data/DataView.h"


/**
 * @brief Recomputes metadata for a one-dimensional view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_1D.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_1d_recompute(DataView *view);


/**
 * @brief Recomputes metadata for a two-dimensional view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_2D.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_2d_recompute(DataView *view);


/**
 * @brief Recomputes metadata for a three-dimensional view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_3D.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_3d_recompute(DataView *view);


/**
 * @brief Recomputes metadata for a CSR view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_CSR.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_csr_recompute(DataView *view);


/**
 * @brief Recomputes metadata for an Array-of-Structures view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_AOS.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_aos_recompute(DataView *view);


/**
 * @brief Recomputes metadata for a Structure-of-Arrays view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_SOA.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_soa_recompute(DataView *view);


/**
 * @brief Recomputes metadata for an Array-of-Structures-of-Arrays view.
 *
 * @param view DataView whose metadata must be recomputed.
 *
 * @pre view must not be NULL.
 * @pre view->type must be VIEW_AOSOA.
 *
 * @note Intended for setup/binding operations, never for a kernel hot loop.
 */
void shape_aosoa_recompute(DataView *view);


/**
 * @brief Validates metadata for a one-dimensional view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_1d_validate(const DataView *view);


/**
 * @brief Validates metadata for a two-dimensional view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_2d_validate(const DataView *view);


/**
 * @brief Validates metadata for a three-dimensional view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_3d_validate(const DataView *view);


/**
 * @brief Validates metadata for a CSR view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_csr_validate(const DataView *view);


/**
 * @brief Validates metadata for an Array-of-Structures view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_aos_validate(const DataView *view);


/**
 * @brief Validates metadata for a Structure-of-Arrays view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_soa_validate(const DataView *view);


/**
 * @brief Validates metadata for an Array-of-Structures-of-Arrays view.
 *
 * @param view DataView to validate.
 *
 * @return true if the view metadata is consistent, false otherwise.
 *
 * @pre view must not be NULL.
 *
 * @note Intended for correctness checks and debugging, not benchmarking.
 */
bool shape_aosoa_validate(const DataView *view);


#endif /* DATA_VIEW_SHAPE_FNS_H */
