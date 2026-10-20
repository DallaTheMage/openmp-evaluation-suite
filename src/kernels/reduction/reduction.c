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
 * @file reduction.c
 * @brief OpenMP reduction execution for the reduction construct.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 */

#include "kernels/reduction/reduction.h"
#include "kernels/common/BenchmarkValidation.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_2_0
/* ============================================================================
 * VIEW_2D
 * ========================================================================== */
/**
 * @brief Run the reduction benchmark using the 2D data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_2d_reduction(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t rows         = view->meta.v2d.rows;
    const uint64_t cols         = view->meta.v2d.cols;
    double sum                  = 0.0;

    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t r = 0; r < rows; ++r) {
        const uint64_t offset = r * cols;
        for (uint64_t c = 0; c < cols; ++c) {
            sum += pool[offset + c];
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
#if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
#endif
    return KERNEL_STATUS_OK;

}

/* ============================================================================
 * VIEW_3D
 * ========================================================================== */
/**
 * @brief Run the reduction benchmark using the 3D data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_3d_reduction(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t rows         = view->meta.v3d.height;
    const uint64_t cols         = view->meta.v3d.cols;
    const uint64_t depth        = view->meta.v3d.depth;
    double sum                  = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t r = 0; r < rows; ++r) {
        for (uint64_t c = 0; c < cols; ++c) {
            const uint64_t offset = (r * cols + c) * depth;
            for (uint64_t d = 0; d < depth; ++d) {
                sum += pool[offset + d];
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
#if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
#endif
    return KERNEL_STATUS_OK;

}

/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */
/**
 * @brief Run the reduction benchmark using the AOS data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_aos_reduction(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t num_elems    = view->meta.aos.num_structs;
    const uint64_t struct_size  = view->meta.aos.struct_size;
    double sum                  = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t i = 0; i < num_elems; ++i) {
        const uint64_t base = i * struct_size;
        for (uint64_t f = 0; f < struct_size; ++f) {
            sum += pool[base + f];
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
#if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
#endif
    return KERNEL_STATUS_OK;

}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */
/**
 * @brief Run the reduction benchmark using the SOA data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_soa_reduction(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t num_elems    = view->meta.soa.field_length;
    const uint64_t num_fields   = view->meta.soa.num_fields;
    double sum                  = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t f = 0; f < num_fields; ++f) {
        const uint64_t field_offset = f * num_elems;
        for (uint64_t i = 0; i < num_elems; ++i) {
            sum += pool[field_offset + i];
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
#if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
#endif
    return KERNEL_STATUS_OK;

}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */
/**
 * @brief Run the reduction benchmark using the AOSOA data layout.
 * @param view Non-owning view of the benchmark input/output buffer.
 * @param test_case Thread count and scheduling/workload parameters.
 * @param profiler Initialized profiler used to bracket the kernel workload.
 * @param metric Output measurement populated by the profiler.
 * @return Kernel execution status; propagates profiler failures.
 */
KernelStatus kernel_aosoa_reduction(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t num_elems    = view->meta.aosoa.num_blocks;
    const uint64_t num_fields   = view->meta.aosoa.num_fields;
    const uint64_t vl           = view->meta.aosoa.vector_length;
    const uint64_t num_tiles    = num_elems;
    const uint64_t tile_size    = num_fields * vl;
    double sum = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t t = 0; t < num_tiles; ++t) {
        const uint64_t tile_base = t * tile_size;
        for (uint64_t f = 0; f < num_fields; ++f) {
            const uint64_t field_base = tile_base + f * vl;
            for (uint64_t v = 0; v < vl; ++v) {
                sum += pool[field_base + v];
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
#if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
#endif
    return KERNEL_STATUS_OK;
}



/* Diagnostic variant: reduce SoA by parallelizing the element dimension. */
KernelStatus kernel_soa_reduction_inner(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads  = test_case->num_threads;
    const uint32_t chunk        = test_case->chunk_size;
    const double *restrict pool = view->buffer->pool;
    const uint64_t num_elems    = view->meta.soa.field_length;
    const uint64_t num_fields   = view->meta.soa.num_fields;
    double sum                  = 0.0;

    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) reduction(+:sum)
    {
        for (uint64_t f = 0; f < num_fields; ++f) {
            const uint64_t field_offset = f * num_elems;
            #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
            for (uint64_t i = 0; i < num_elems; ++i) {
                sum += pool[field_offset + i];
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    volatile double result = sum;
    (void)result;
    #if BENCH_VALIDATE_RESULTS
    benchmark_validation_reduction_sum = sum;
    #endif
    return KERNEL_STATUS_OK;
}

#endif /* OPENMP_HAS_2_0 */
