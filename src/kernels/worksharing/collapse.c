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
 * @file collapse.c
 * @brief OpenMP worksharing execution for the collapse construct.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 */

#include "kernels/worksharing/collapse.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"
#include "kernels/common/Slowdown.h"

#if OPENMP_HAS_3_0
/* ============================================================================
- VIEW_2D
- ========================================================================== */
KernelStatus kernel_2d_collapse(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        collapse(2) \
        shared(chunk, cols, pool, rows)
    for (uint64_t r = 0; r < rows; ++r) {
        for (uint64_t c = 0; c < cols; ++c) {
            const uint64_t offset = r * cols + c;
            pool[offset] = pool[offset] * 2.0 + 1.0;
            SLOWDOWN_AFTER_UPDATE(offset);
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
- VIEW_3D
- ========================================================================== */
KernelStatus kernel_3d_collapse(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v3d.height;
    const uint64_t cols        = view->meta.v3d.cols;
    const uint64_t depth       = view->meta.v3d.depth;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        collapse(3) \
        shared(chunk, cols, depth, pool, rows)
    for (uint64_t r = 0; r < rows; ++r) {
        for (uint64_t c = 0; c < cols; ++c) {
            for (uint64_t d = 0; d < depth; ++d) {
                const uint64_t offset = (r * cols + c) * depth + d;
                pool[offset] = pool[offset] * 2.0 + 1.0;
                SLOWDOWN_AFTER_UPDATE(offset);
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_3_0 */