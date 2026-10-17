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
 * @file gemm.c
 * @brief Deterministic dense GEMM workload with untimed numerical checks.
 */
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config/openmp.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"
#include "kernels/algorithms/gemm.h"

#if OPENMP_HAS_2_0
/** @brief Compute a reproducible, bounded right-hand matrix element. */
static double right_element(size_t k, size_t j) {
    return (double)((((k * 17U) + (j * 13U)) % 29U) + 1U) / 29.0;
}

/**
 * @brief Check several complete output rows against a sequential reference.
 *
 * The check is independent of the timed OpenMP region. It detects incomplete
 * or incorrect parallel execution without making validation O(n^3).
 */
static int validate_rows(const double *a, const double *b,
                         const double *c, size_t m, size_t n) {
    const size_t rows[3] = {0U, m / 2U, m - 1U};
    for (size_t sample = 0; sample < 3U; ++sample) {
        size_t i = rows[sample];
        for (size_t j = 0; j < n; ++j) {
            double expected = 0.0;
            for (size_t k = 0; k < n; ++k) {
                expected += a[i * n + k] * b[k * n + j];
            }
            double actual = c[i * n + j];
            if (!isfinite(actual) ||
                fabs(actual - expected) > 1e-10 * fmax(1.0, fabs(expected))) {
                return 0;
            }
        }
    }
    return 1;
}

KernelStatus kernel_2d_gemm(DataView *view, const TestCase *test_case,
                           Profiler *profiler, PerformanceMetric *metric) {
    if (view == NULL || test_case == NULL || profiler == NULL || metric == NULL ||
        view->buffer == NULL || view->buffer->pool == NULL ||
        view->type != VIEW_2D || test_case->num_threads == 0U ||
        view->meta.v2d.rows == 0U || view->meta.v2d.cols == 0U) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const uint64_t rows = view->meta.v2d.rows;
    const uint64_t cols = view->meta.v2d.cols;
    if (rows > SIZE_MAX || cols > SIZE_MAX ||
        cols > SIZE_MAX / sizeof(double) ||
        rows > SIZE_MAX / cols ||
        cols > SIZE_MAX / cols ||
        rows * cols > SIZE_MAX / sizeof(double) ||
        cols * cols > SIZE_MAX / sizeof(double) ||
        view->window.size < rows * cols ||
        view->window.offset > view->buffer->element_count ||
        rows * cols > view->buffer->element_count - view->window.offset) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const size_t m = (size_t)rows;
    const size_t n = (size_t)cols;
    const size_t b_elements = n * n;
    const size_t c_elements = m * n;
    const double *a = view->buffer->pool + view->window.offset;
    double *b = malloc(b_elements * sizeof(*b));
    double *c = malloc(c_elements * sizeof(*c));
    if (b == NULL || c == NULL) {
        free(b);
        free(c);
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }
    for (size_t k = 0; k < n; ++k) {
        for (size_t j = 0; j < n; ++j) {
            b[k * n + j] = right_element(k, j);
        }
    }
    memset(c, 0, c_elements * sizeof(*c));
    if (profiler_start(profiler) != 0) {
        free(b);
        free(c);
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    /* Each output row has one writer. Traversing B and C by rows improves
     * spatial locality without introducing inter-thread write conflicts.
     * C was zero-initialized before profiling.
     */
    #pragma omp parallel for num_threads(test_case->num_threads) schedule(CHOSEN_SCHEDULE, test_case->chunk_size)
    for (size_t i = 0; i < m; ++i) {
        double *restrict c_row = c + i * n;
        const double *restrict a_row = a + i * n;
        for (size_t k = 0; k < n; ++k) {
            const double aik = a_row[k];
            const double *restrict b_row = b + k * n;
            for (size_t j = 0; j < n; ++j) {
                c_row[j] += aik * b_row[j];
            }
        }
    }
    const int stop_result = profiler_stop(profiler, metric);
    int valid = stop_result == 0 && validate_rows(a, b, c, m, n);
    free(b);
    free(c);
    if (stop_result != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return valid ? KERNEL_STATUS_OK : KERNEL_STATUS_EXECUTION_FAILURE;
}
#endif
