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
 * @file gemm.h
 * @brief Dense GEMM benchmark implemented through the common kernel API.
 */
#ifndef KERNELS_ALGORITHMS_GEMM_H
#define KERNELS_ALGORITHMS_GEMM_H
#include "kernels/common/Kernel.h"
#include <stdlib.h>
/**
 * @brief Measure a row-parallel dense matrix product C = A * B.
 *
 * A is the m-by-n VIEW_2D input, B is an n-by-n matrix generated before
 * profiling, and C is an m-by-n private scratch buffer. Input is unchanged.
 * Input preparation, allocation and numerical validation are not timed.
 * The implementation intentionally uses an unblocked scalar inner product
 * and is not a replacement for an optimized BLAS implementation.
 *
 * @param view Non-owning VIEW_2D of doubles, m by n, m,n > 0.
 * @param test_case Run parameters, including positive num_threads.
 * @param profiler Initialized profiler used to measure only the matrix product.
 * @param metric Output metrics populated on successful profiling.
 * @return KERNEL_STATUS_OK on successful computation and validation, or an
 *         appropriate argument, allocation, profiling or execution error.
 * @note Dimension selection in the existing proportional TestPlan does not
 *       implement FLOP-proportional weak scaling for GEMM.
 */
KernelStatus kernel_2d_gemm(DataView *view, const struct TestCase *test_case,
                           struct Profiler *profiler, struct PerformanceMetric *metric);

static inline void free_gemm_components(double *b, double *c) {
    free(b);
    free(c);
}
#endif
