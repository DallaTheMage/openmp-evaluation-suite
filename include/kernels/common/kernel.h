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
 * @file Kernel.h
 * @brief Generic interface implemented by benchmark kernels.
 *
 * This header defines the minimal execution contract shared by all
 * benchmark kernels.
 *
 * Kernel implementations operate on a DataView, a test case configuration,
 * and interact with the profiler and performance metrics layer.
 */

#ifndef KERNELS_COMMON_KERNEL_H
#define KERNELS_COMMON_KERNEL_H

#include <stdint.h>

#include "data/DataView.h"


/**
 * @brief Generic benchmark kernel function.
 *
 * A kernel operates on the supplied DataView using the specified test case
 * configuration, while recording metrics through the profiler.
 *
 * @param view DataView processed by the kernel.
 * @param test_case Test case configuration and execution parameters for this invocation.
 * @param profiler Profiler instance used for performance tracking.
 * @param metric Performance metric container for results.
 *
 * @pre view must not be NULL.
 * @pre test_case must not be NULL.
 * @pre profiler must not be NULL.
 * @pre metric must not be NULL.
 *
 * @note The selected OpenMP schedule is a compile-time property of the
 *       benchmark build and is not supplied through this interface.
 */
typedef void (*KernelFunc)(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);


#endif /* KERNELS_COMMON_KERNEL_H */