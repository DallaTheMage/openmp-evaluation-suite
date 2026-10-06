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
 * @brief Generic interface shared by benchmark kernel implementations.
 *
 * The kernel interface is deliberately small. A kernel receives a non-owning
 * DataView, the immutable execution parameters for one TestCase, and an
 * already initialized Profiler. The kernel is responsible for measuring only
 * its benchmark region and returning the collected PerformanceMetric.
 *
 * The header uses forward declarations for TestCase, Profiler and
 * PerformanceMetric so that the kernel API does not depend on the complete
 * definitions of the core or profiling layers. This keeps the dependency
 * direction one-way and avoids pulling profiling implementation details into
 * every kernel header.
 */

#ifndef KERNELS_COMMON_KERNEL_H
#define KERNELS_COMMON_KERNEL_H

#include "data/DataView.h"


/*
 * Opaque types owned by other architectural layers.
 *
 * The complete definitions are intentionally not included here. Kernel
 * implementations include the appropriate headers when they need access
 * to the members of these structures.
 */
struct TestCase;
struct Profiler;
struct PerformanceMetric;


/**
 * @brief Generic benchmark kernel function.
 *
 * A kernel operates on the supplied DataView using the specified TestCase
 * configuration, while recording measurements through the supplied
 * Profiler.
 *
 * @param view
 *     Non-owning DataView processed by the kernel.
 *
 * @param test_case
 *     Execution parameters for the current test invocation.
 *
 * @param profiler
 *     Initialized profiler used for the measurement interval.
 *
 * @param metric
 *     Output structure receiving the collected performance metrics.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The profiler must have been initialized before the call.
 * @pre The selected TestCase must be valid for the supplied DataView.
 *
 * @note Kernel functions do not take ownership of any argument.
 *
 * @note The OpenMP scheduling policy is a compile-time property of the
 *       benchmark build and is not selected through this interface.
 *
 * @note The concrete kernel implementation is selected by the kernel
 *       registry using the DataView type and KernelType.
 */
typedef void (*KernelFunc)(
    DataView *view,
    const struct TestCase *test_case,
    struct Profiler *profiler,
    struct PerformanceMetric *metric
);


#endif /* KERNELS_COMMON_KERNEL_H */
