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
 * @file ordered.c
 * @brief OpenMP synchronization execution for the ordered construct.
 */

#include "kernels/synchronization/ordered.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_2_0

KernelStatus kernel_1d_ordered(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t length      = view->meta.v1d.length;
    double ordered_sum         = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        ordered \
        shared(pool, length, chunk, ordered_sum)
    for (uint64_t r = 0; r < length; r++) {
        double local_sum = 0.0;
        const double value = pool[r];
        pool[r] = value * 2.0 + 1.0;
        local_sum += value;

        #pragma omp ordered
        {
            ordered_sum += local_sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


KernelStatus kernel_2d_ordered(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    double ordered_sum         = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        ordered \
        default(none) \
        shared(pool, rows, cols, chunk, ordered_sum)
    for (uint64_t r = 0; r < rows; r++) {
        const uint64_t offset = r * cols;
        double local_sum = 0.0;
        for (uint64_t c = 0; c < cols; c++) {
            const double value = pool[offset + c];
            pool[offset + c] = value * 2.0 + 1.0;
            local_sum += value;
        }

        #pragma omp ordered
        {
            ordered_sum += local_sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_2_0 */