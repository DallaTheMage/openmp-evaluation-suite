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
 * @file native.c
 * @brief OpenMP scan execution for the native construct.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 */

#include "kernels/scan/native.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_NATIVE_SCAN
/* ============================================================================
 * VIEW_2D
 * ========================================================================== */
/**
 * @brief Run the native scan benchmark using the 2D data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_2d_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v2d.rows * view->meta.v2d.cols;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */
/**
 * @brief Run the native scan benchmark using the 3D data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_3d_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v3d.height * view->meta.v3d.cols * view->meta.v3d.depth;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

/**
 * @brief Run the native scan benchmark using the AOS data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_aos_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.aos.num_structs * view->meta.aos.struct_size;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */
/**
 * @brief Run the native scan benchmark using the SOA data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_soa_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.soa.field_length * view->meta.soa.num_fields;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */
/**
 * @brief Run the native scan benchmark using the AOSOA data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_aosoa_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t vl          = view->meta.aosoa.vector_length;
    if (vl == 0) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const uint64_t total = view->meta.aosoa.num_blocks * view->meta.aosoa.num_fields * vl;
    double sum           = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_NATIVE_SCAN */