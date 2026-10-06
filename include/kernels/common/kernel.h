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
 * Kernel implementations operate on a DataView and an execution
 * configuration. Performance measurement is intentionally handled outside
 * the kernel interface by the dispatcher/profiler layer.
 */

#ifndef KERNELS_COMMON_KERNEL_H
#define KERNELS_COMMON_KERNEL_H

#include <stdint.h>

#include "data/DataView.h"


/**
 * @brief Execution parameters supplied to a kernel.
 *
 * This structure contains only parameters that affect kernel execution.
 * Benchmark infrastructure such as profiling and result storage does not
 * belong here.
 */
typedef struct KernelExecution {
    uint32_t num_threads;
    uint64_t chunk_size;
} KernelExecution;


/**
 * @brief Generic benchmark kernel function.
 *
 * A kernel operates on the supplied DataView using the specified execution
 * parameters.
 *
 * @param view DataView processed by the kernel.
 * @param execution Execution parameters for this invocation.
 *
 * @pre view must not be NULL.
 * @pre execution must not be NULL.
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