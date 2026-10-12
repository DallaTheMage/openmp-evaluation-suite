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
 * @file two_pass.c
 * @brief Two-pass parallel prefix-scan kernels.
 *
 * The implementation computes local inclusive scans in parallel, scans the
 * per-thread totals to obtain exclusive offsets, and applies those offsets
 * to each thread's assigned interval.
 */

#include "kernels/scan/two_pass.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#include <stdlib.h>
#include <omp.h>

#if OPENMP_HAS_TWO_PASS_SCAN

/* ============================================================================
 * Internal two-pass implementation
 * ========================================================================== */

/**
 * @brief Execute a parallel inclusive scan using two passes.
 *
 * The first pass computes a local inclusive scan for each thread.
 * The thread-local totals are then converted into exclusive offsets.
 * The second pass applies those offsets to the corresponding intervals.
 *
 * Memory allocation takes place before profiling starts.
 *
 * @param pool Data buffer to scan in place.
 * @param total Number of elements.
 * @param num_threads Requested number of OpenMP threads.
 * @param profiler Profiler used to measure execution.
 * @param metric Destination for the performance metric.
 *
 * @return KERNEL_STATUS_OK on success, or an appropriate error status.
 */
static KernelStatus run_twopass_omp(
    double *restrict pool,
    uint64_t total,
    uint16_t num_threads,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    double *offsets;
    int stop_status;

    if (pool == NULL ||
        total == 0U ||
        profiler == NULL ||
        metric == NULL) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }

    if (num_threads == 0U) {
        num_threads = 1U;
    }

    /*
     * Allocate the offsets outside the measured region.
     *
     * Offset-management overhead must not contaminate the benchmark
     * measurement of the scan algorithm.
     */
    offsets = calloc((size_t)num_threads, sizeof(*offsets));

    if (offsets == NULL) {
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }

    if (profiler_start(profiler) != 0) {
        free(offsets);
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, offsets)
    {
        const uint64_t tid =
            (uint64_t)omp_get_thread_num();

        const uint64_t nth =
            (uint64_t)omp_get_num_threads();

        const uint64_t items_per_thread =
            total / nth;

        const uint64_t remainder =
            total % nth;

        const uint64_t start =
            tid * items_per_thread +
            (tid < remainder ? tid : remainder);

        const uint64_t count =
            items_per_thread +
            (tid < remainder ? UINT64_C(1) : UINT64_C(0));

        const uint64_t end = start + count;

        /*
         * PASS 1
         *
         * Each thread performs a local inclusive scan over its assigned
         * interval and stores its local total in offsets[].
         */
        double acc = 0.0;

        for (uint64_t i = start; i < end; ++i) {
            acc += pool[i];
            pool[i] = acc;
        }

        offsets[tid] = acc;

        /*
         * All threads must publish their local totals before the single
         * thread accesses the offsets array.
         */
        #pragma omp barrier

        /*
         * OFFSET SCAN
         *
         * Thread 0 converts the local totals into exclusive offsets.
         *
         * Example:
         *
         *   offsets = [10, 20, 30, 40]
         *
         * becomes:
         *
         *   offsets = [0, 10, 30, 60]
         */
        #pragma omp single
        {
            double running = 0.0;

            for (uint64_t t = 0U; t < nth; ++t) {
                const double tmp = offsets[t];

                offsets[t] = running;
                running += tmp;
            }
        }

        /*
         * PASS 2
         *
         * Each thread adds its exclusive global offset to its assigned
         * interval, producing the final inclusive scan.
         */
        {
            const double thread_offset = offsets[tid];

            if (thread_offset != 0.0) {
                for (uint64_t i = start; i < end; ++i) {
                    pool[i] += thread_offset;
                }
            }
        }
    }

    stop_status = profiler_stop(profiler, metric);

    free(offsets);

    if (stop_status != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    return KERNEL_STATUS_OK;
}

/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

/**
 * @brief Execute a two-pass scan over a 2D view.
 */
KernelStatus kernel_2d_two_pass_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    const uint64_t total =
        view->meta.v2d.rows * view->meta.v2d.cols;

    return run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        profiler,
        metric
    );
}

/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

/**
 * @brief Execute a two-pass scan over a 3D view.
 */
KernelStatus kernel_3d_two_pass_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    const uint64_t total =
        view->meta.v3d.height *
        view->meta.v3d.cols *
        view->meta.v3d.depth;

    return run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        profiler,
        metric
    );
}

/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

/**
 * @brief Execute a two-pass scan over an array-of-structures view.
 */
KernelStatus kernel_aos_two_pass_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    const uint64_t total =
        view->meta.aos.num_structs *
        view->meta.aos.struct_size;

    return run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        profiler,
        metric
    );
}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */

/**
 * @brief Execute a two-pass scan over a structure-of-arrays view.
 */
KernelStatus kernel_soa_two_pass_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    const uint64_t total =
        view->meta.soa.field_length *
        view->meta.soa.num_fields;

    return run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        profiler,
        metric
    );
}

/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */

/**
 * @brief Execute a two-pass scan over an array-of-structures-of-arrays view.
 */
KernelStatus kernel_aosoa_two_pass_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric)
{
    const uint64_t vl =
        view->meta.aosoa.vector_length;

    if (vl == 0U) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }

    const uint64_t total =
        view->meta.aosoa.num_blocks *
        view->meta.aosoa.num_fields *
        vl;

    return run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        profiler,
        metric
    );
}

#endif /* OPENMP_HAS_TWO_PASS_SCAN */