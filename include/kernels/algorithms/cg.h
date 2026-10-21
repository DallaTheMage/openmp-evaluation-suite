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
 * @file cg.h
 * @brief Fixed-iteration matrix-free Conjugate Gradient on the 2D Poisson operator.
 */
#ifndef KERNELS_ALGORITHMS_CG_H
#define KERNELS_ALGORITHMS_CG_H
#include <stdlib.h>
#include "kernels/common/Kernel.h"

#define CG_STEPS 12U

/**
 * @brief Measure Conjugate Gradient on a VIEW_2D double-precision input.
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
 */
KernelStatus kernel_2d_cg(
    DataView *view,
    const struct  TestCase *test_case,
    struct Profiler *profiler,
    struct PerformanceMetric *metric);

static inline void spd_five_point_dirichlet(
    size_t i,
    size_t j,
    size_t rows,
    size_t cols,
    const double *x,
    double *y) {
    size_t z = (i * cols) + j;
    double v = 4.0 * x[z];
    if(i) {
        v -= x[z - cols];
    }
    if((i + 1) < rows) {
        v -= x[z + cols];
    }
    if(j) {
        v -= x[z - 1];
    }
    if((j + 1) < cols) {
        v -= x[z + 1];
    }
    y[z] = v;
}

static inline void free_cg_components(
    double* x,
    double* r,
    double* p,
    double* ap,
    double* check) {
    free(x);
    free(r);
    free(p);
    free(ap);
    free(check);
}
#endif
