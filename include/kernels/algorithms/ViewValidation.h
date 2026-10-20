/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2026 Lorenzo Dall'Ara
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite,
 * developed as part of a Bachelor's thesis at the
 * Alma Mater Studiorum - University of Bologna.
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
 * @file ViewValidation.h
 * @brief Shared validation for two-dimensional algorithm kernels.
 */

#ifndef KERNELS_ALGORITHMS_VIEW_VALIDATION_H
#define KERNELS_ALGORITHMS_VIEW_VALIDATION_H

#include <stddef.h>
#include "core/TestPlan.h"
#include "profiling/Profiler.h"

/**
 * @brief Validate a two-dimensional double-precision kernel view.
 * @param view Input data view.
 * @param test_case Test-case scheduling parameters.
 * @param profiler Profiler required for the invocation.
 * @param metric Destination for performance metrics.
 * @param min_side Minimum permitted rows and columns.
 * @return Nonzero for a valid window, zero otherwise.
 */
int kernel_valid_2d_view(const DataView *view, const TestCase *test_case,
                         const Profiler *profiler, const PerformanceMetric *metric,
                         size_t min_side);

#endif /* KERNELS_ALGORITHMS_VIEW_VALIDATION_H */
