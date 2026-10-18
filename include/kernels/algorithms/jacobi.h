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
 * @file jacobi.h
 * @brief Five-point Jacobi stencil with fixed time steps and double buffering.
 */
#ifndef KERNELS_ALGORITHMS_JACOBI_H
#define KERNELS_ALGORITHMS_JACOBI_H
#include "kernels/common/Kernel.h"

#define JACOBI_STEPS 8U

/**
 * @brief Measure Jacobi 2D on a VIEW_2D double-precision input.
 *
 * Scratch allocation, initialization and numerical checks are outside the
 * profiling interval. The input view is not modified. The benchmark runs a
 * fixed number of iterations, independent of the thread count.
 *
 * @param view Non-owning two-dimensional input (minimum 3-by-3 for Jacobi,
 *             2-by-2 for CG), stored contiguously in row-major order.
 * @param test_case Execution parameters, with nonzero thread and chunk counts.
 * @param profiler Initialized profiling context.
 * @param metric Output measurement populated by profiler_stop().
 * @return KERNEL_STATUS_OK on success; an appropriate failure status otherwise.
 * @note Generic proportional TestPlan sizes do not guarantee equal work per
 *       thread for these iterative algorithms.
 */
KernelStatus kernel_2d_jacobi(DataView *view, const struct TestCase *test_case,
                            struct Profiler *profiler, struct PerformanceMetric *metric);

static inline free_jacobi_components(double *a, double *b, double *ref, double *tmp) {
    free(a);
    free(b);
    free(ref);
    free(tmp);
}
#endif
