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
 * @file masked.c
 * @brief OpenMP synchronization execution for the masked construct.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 */

#include "kernels/synchronization/master_masked.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_5_1
/* ============================================================================
- VIEW_2D
- ========================================================================== */
/**
 * @brief Run the masked benchmark using the 2D data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_2d_masked(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, rows, cols)
    {
        #pragma omp masked
        {
            if (rows > 0 && cols > 0) {
                pool[0] = pool[0] * 2.0 + 1.0;
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_5_1 */