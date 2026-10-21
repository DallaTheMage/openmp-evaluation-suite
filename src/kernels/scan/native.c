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
/* inscan belongs to the loop; OpenMP forbids a schedule clause on this loop. */
/* ============================================================================
 * VIEW_1D
 * ========================================================================== */
KernelStatus kernel_1d_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v1d.length;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* VIEW_2D */
KernelStatus kernel_2d_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v2d.rows * view->meta.v2d.cols;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */
KernelStatus kernel_3d_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v3d.height * view->meta.v3d.cols * view->meta.v3d.depth;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

KernelStatus kernel_aos_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.aos.num_structs * view->meta.aos.struct_size;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */
KernelStatus kernel_soa_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.soa.field_length * view->meta.soa.num_fields;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */
KernelStatus kernel_aosoa_native_scan(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    double *restrict pool      = view->buffer->pool;
    const uint64_t vl          = view->meta.aosoa.vector_length;
    if (vl == 0) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const uint64_t total = view->meta.aosoa.num_blocks * view->meta.aosoa.num_fields * vl;
    if (total > (uint64_t)INT64_MAX) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    /* A signed trip count avoids GCC 13's unsigned-inscan diagnostics. */
    const int64_t count = (int64_t)total;
    double sum           = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        default(none) \
        num_threads(num_threads) \
        shared(pool, count) \
        reduction(inscan, +:sum)
    for (int64_t i = 0; i < count; ++i) {
        sum += pool[i];
        #pragma omp scan inclusive(sum)
        pool[i] = sum;
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_NATIVE_SCAN */