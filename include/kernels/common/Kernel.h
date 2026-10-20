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
 * @file Kernel.h
 * @brief Generic interface shared by benchmark kernel implementations.
 *
 * This header defines the common function signature and execution status
 * used by all benchmark kernels.
 *
 * A kernel receives a non-owning DataView, the execution parameters for
 * one TestCase, an initialized Profiler, and an output PerformanceMetric.
 * It returns a KernelStatus indicating whether the execution completed
 * successfully.
 *
 * The header uses forward declarations for TestCase, Profiler, and
 * PerformanceMetric to avoid unnecessary dependencies on the core and
 * profiling implementation layers.
 */

#ifndef KERNELS_COMMON_KERNEL_H
#define KERNELS_COMMON_KERNEL_H

#include "data/DataView.h"

/**
 * @brief Opaque declaration of the test-case configuration.
 */
struct TestCase;

/**
 * @brief Opaque declaration of the profiler.
 */
struct Profiler;

/**
 * @brief Opaque declaration of the performance metric structure.
 */
struct PerformanceMetric;

/**
 * @brief Status returned by a benchmark kernel.
 *
 * The returned status allows the dispatcher to distinguish successful
 * executions from invalid arguments, allocation failures, profiling
 * failures, and other execution errors.
 *
 * A kernel must return KERNEL_STATUS_OK only when its execution and
 * measurement have completed successfully.
 */
typedef enum KernelStatus {
    /** The kernel completed successfully and produced a valid measurement. */
    KERNEL_STATUS_OK = 0,

    /** One or more required arguments are invalid. */
    KERNEL_STATUS_INVALID_ARGUMENT,

    /** A required memory allocation failed. */
    KERNEL_STATUS_ALLOCATION_FAILURE,

    /** The profiler failed to start or complete the measurement. */
    KERNEL_STATUS_PROFILER_FAILURE,

    /** The kernel could not complete its execution for another reason. */
    KERNEL_STATUS_EXECUTION_FAILURE
} KernelStatus;

/**
 * @brief Generic benchmark kernel function signature.
 *
 * Each kernel processes the supplied DataView according to the specified
 * TestCase and records its performance metrics through the supplied
 * Profiler.
 *
 * The kernel does not own any of its arguments and must not release them.
 * Temporary resources allocated by the kernel must be released before
 * returning, including when an error occurs.
 *
 * The dispatcher is responsible for preparing the working buffer before
 * invocation and for deciding whether the returned metric can be accepted.
 *
 * @param view
 *     Non-owning view of the data processed by the kernel.
 *
 * @param test_case
 *     Immutable execution parameters for the current test invocation.
 *
 * @param profiler
 *     Initialized profiler used to measure the kernel execution.
 *
 * @param metric
 *     Output structure receiving the collected performance metrics.
 *
 * @return KERNEL_STATUS_OK if execution and measurement succeed;
 *         an appropriate error status otherwise.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The profiler must have been initialized before the call.
 * @pre The TestCase must be valid for the supplied DataView.
 *
 * @note Kernel functions do not take ownership of their arguments.
 *
 * @note The kernel must not report success if profiling fails or if the
 *       measurement is incomplete.
 *
 * @note The OpenMP scheduling policy is a compile-time property of the
 *       benchmark build and is not selected through this interface.
 *
 * @note The concrete kernel implementation is selected by the kernel
 *       registry using the DataView type and KernelType.
 */
typedef KernelStatus (*KernelFunc)(
    DataView *view,
    const struct TestCase *test_case,
    struct Profiler *profiler,
    struct PerformanceMetric *metric
);

#endif /* KERNELS_COMMON_KERNEL_H */