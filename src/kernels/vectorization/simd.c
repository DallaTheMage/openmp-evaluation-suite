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
 * @file simd.c
 * @brief SIMD memory-bound and compute-bound benchmark kernels.
 *
 * The memory-bound kernels perform a reduction over the selected data layout.
 * The compute-bound kernels apply additional arithmetic to each loaded value
 * before reducing it. OpenMP SIMD directives expose vectorization opportunities
 * while the outer parallel regions distribute work across threads.
 */

#include "kernels/vectorization/simd.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_SIMD

/* ============================================================================
 * SIMD MEMORY-BOUND BENCHMARK
 *
 * Objective:
 *   Highlight memory behavior and access patterns.
 *
 * The computational workload per element is intentionally minimal:
 *
 *     load -> reduction
 *
 * This makes differences among 2D, 3D, AoS, SoA, and AoSoA layouts more
 * strongly influenced by data layout and memory-access locality.
 * ========================================================================== */

/* --------------------------------------------------------------------------
 * VIEW_2D
 * -------------------------------------------------------------------------- */
KernelStatus kernel_2d_simd_memory(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {
            const uint64_t offset = r * cols;
            #pragma omp simd reduction(+:sink)
            for (uint64_t c = 0; c < cols; ++c) {
                sink += pool[offset + c];
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_3D
 * -------------------------------------------------------------------------- */
KernelStatus kernel_3d_simd_memory(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v3d.height;
    const uint64_t cols        = view->meta.v3d.cols;
    const uint64_t depth       = view->meta.v3d.depth;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, depth, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {
            for (uint64_t c = 0; c < cols; ++c) {
                const uint64_t offset = (r * cols + c) * depth;
                #pragma omp simd reduction(+:sink)
                for (uint64_t d = 0; d < depth; ++d) {
                    sink += pool[offset + d];
                }
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_AOS
 * -------------------------------------------------------------------------- */
KernelStatus kernel_aos_simd_memory(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aos.num_structs;
    const uint64_t struct_size = view->meta.aos.struct_size;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, struct_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t i = 0; i < num_elems; ++i) {
            const uint64_t base = i * struct_size;
            #pragma omp simd reduction(+:sink)
            for (uint64_t f = 0; f < struct_size; ++f) {
                sink += pool[base + f];
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_SOA
 * -------------------------------------------------------------------------- */
KernelStatus kernel_soa_simd_memory(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.soa.field_length;
    const uint64_t num_fields  = view->meta.soa.num_fields;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, num_fields, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t f = 0; f < num_fields; ++f) {
            const uint64_t field_offset = f * num_elems;
            #pragma omp simd reduction(+:sink)
            for (uint64_t i = 0; i < num_elems; ++i) {
                sink += pool[field_offset + i];
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_AOSOA
 * -------------------------------------------------------------------------- */
KernelStatus kernel_aosoa_simd_memory(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aosoa.num_blocks;
    const uint64_t num_fields  = view->meta.aosoa.num_fields;
    const uint64_t vl          = view->meta.aosoa.vector_length;
    const uint64_t num_tiles   = num_elems;
    const uint64_t tile_size   = num_fields * vl;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_tiles, num_fields, vl, tile_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t t = 0; t < num_tiles; ++t) {
            const uint64_t tile_base = t * tile_size;
            for (uint64_t f = 0; f < num_fields; ++f) {
                const uint64_t field_base = tile_base + f * vl;
                #pragma omp simd reduction(+:sink)
                for (uint64_t v = 0; v < vl; ++v) {
                    sink += pool[field_base + v];
                }
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * SIMD COMPUTE-BOUND BENCHMARK
 *
 * Objective:
 *   Increase the arithmetic workload for each loaded element.
 *
 * The access pattern remains the same as in the memory-bound version; only
 * the computational intensity changes. This helps show how layout and access
 * pattern differences change when the bottleneck shifts from memory bandwidth
 * toward SIMD execution units.
 * ========================================================================== */

/* --------------------------------------------------------------------------
 * VIEW_2D
 * -------------------------------------------------------------------------- */
KernelStatus kernel_2d_simd_compute(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {
            const uint64_t offset = r * cols;
            #pragma omp simd reduction(+:sink)
            for (uint64_t c = 0; c < cols; ++c) {
                double x = pool[offset + c];
                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;
                sink += x;
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_3D
 * -------------------------------------------------------------------------- */
KernelStatus kernel_3d_simd_compute(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v3d.height;
    const uint64_t cols        = view->meta.v3d.cols;
    const uint64_t depth       = view->meta.v3d.depth;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, depth, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {
            for (uint64_t c = 0; c < cols; ++c) {
                const uint64_t offset = (r * cols + c) * depth;
                #pragma omp simd reduction(+:sink)
                for (uint64_t d = 0; d < depth; ++d) {
                    double x = pool[offset + d];
                    x = x * 1.000001 + 0.999999;
                    x = x * 0.999999 + 1.000001;
                    x = x * 1.000003 + 0.999997;
                    x = x * 0.999997 + 1.000003;
                    x = x * 1.000007 + 0.999993;
                    x = x * 0.999993 + 1.000007;
                    sink += x;
                }
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_AOS
 * -------------------------------------------------------------------------- */
KernelStatus kernel_aos_simd_compute(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aos.num_structs;
    const uint64_t struct_size = view->meta.aos.struct_size;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, struct_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t i = 0; i < num_elems; ++i) {
            const uint64_t base = i * struct_size;
            #pragma omp simd reduction(+:sink)
            for (uint64_t f = 0; f < struct_size; ++f) {
                double x = pool[base + f];
                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;
                sink += x;
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_SOA
 * -------------------------------------------------------------------------- */
KernelStatus kernel_soa_simd_compute(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.soa.field_length;
    const uint64_t num_fields  = view->meta.soa.num_fields;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, num_fields, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t f = 0; f < num_fields; ++f) {
            const uint64_t field_offset = f * num_elems;
            #pragma omp simd reduction(+:sink)
            for (uint64_t i = 0; i < num_elems; ++i) {
                double x = pool[field_offset + i];
                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;
                sink += x;
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* --------------------------------------------------------------------------
 * VIEW_AOSOA
 * -------------------------------------------------------------------------- */
KernelStatus kernel_aosoa_simd_compute(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aosoa.num_blocks;
    const uint64_t num_fields  = view->meta.aosoa.num_fields;
    const uint64_t vl          = view->meta.aosoa.vector_length;
    const uint64_t num_tiles   = num_elems;
    const uint64_t tile_size   = num_fields * vl;
    double sink                = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_tiles, num_fields, vl, tile_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t t = 0; t < num_tiles; ++t) {
            const uint64_t tile_base = t * tile_size;
            for (uint64_t f = 0; f < num_fields; ++f) {
                const uint64_t field_base = tile_base + f * vl;
                #pragma omp simd reduction(+:sink)
                for (uint64_t v = 0; v < vl; ++v) {
                    double x = pool[field_base + v];
                    x = x * 1.000001 + 0.999999;
                    x = x * 0.999999 + 1.000001;
                    x = x * 1.000003 + 0.999997;
                    x = x * 0.999997 + 1.000003;
                    x = x * 1.000007 + 0.999993;
                    x = x * 0.999993 + 1.000007;
                    sink += x;
                }
            }
        }
    }

    /* Keep the reduction observable so the workload cannot be optimized away. */
    volatile double result = sink;
    (void)result;

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_SIMD */
