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
 * @file task.c
 * @brief OpenMP task-based execution for the task construct.
 *

 */

#include "kernels/tasking/task.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"
#include "kernels/common/Slowdown.h"

#if OPENMP_HAS_3_0

KernelStatus kernel_1d_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t   num_threads = test_case->num_threads;
    const uint64_t   chunk       = test_case->chunk_size;
    double *restrict pool        = view->buffer->pool;
    const uint64_t   length      = view->meta.v1d.length;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel \
        default(none) \
        num_threads(num_threads) \
        shared(pool, length, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < length; begin += chunk) {
                const uint64_t end = (begin + chunk < length) ? begin + chunk : length;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool)
                {
                    for (uint64_t r = begin; r < end; r++) {
                        pool[r] = pool[r] * 2.0 + 1.0;
                        SLOWDOWN_AFTER_UPDATE(r);
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* VIEW_2D */
KernelStatus kernel_2d_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t   num_threads = test_case->num_threads;
    const uint64_t   chunk       = test_case->chunk_size;
    double *restrict pool        = view->buffer->pool;
    const uint64_t   rows        = view->meta.v2d.rows;
    const uint64_t   cols        = view->meta.v2d.cols;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, rows, cols, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < rows; begin += chunk) {
                const uint64_t end = (begin + chunk < rows) ? begin + chunk : rows;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool, cols)
                {
                    for (uint64_t r = begin; r < end; r++) {
                        const uint64_t offset = r * cols;
                        for (uint64_t c = 0; c < cols; c++) {
                            pool[offset + c] = pool[offset + c] * 2.0 + 1.0;
                            SLOWDOWN_AFTER_UPDATE(offset + c);
                        }
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}



KernelStatus kernel_3d_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t   num_threads = test_case->num_threads;
    const uint64_t   chunk       = test_case->chunk_size;
    double *restrict pool        = view->buffer->pool;
    const uint64_t   rows        = view->meta.v3d.height;
    const uint64_t   cols        = view->meta.v3d.cols;
    const uint64_t   depth       = view->meta.v3d.depth;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, rows, cols, depth, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < rows; begin += chunk) {
                const uint64_t end = (begin + chunk < rows) ? begin + chunk : rows;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool, cols, depth)
                {
                    for (uint64_t r = begin; r < end; r++) {
                        for (uint64_t c = 0; c < cols; c++) {
                            const uint64_t offset = (r * cols + c) * depth;
                            for (uint64_t d = 0; d < depth; d++) {
                                pool[offset + d] = pool[offset + d] * 2.0 + 1.0;
                                SLOWDOWN_AFTER_UPDATE(offset + d);
                            }
                        }
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}



KernelStatus kernel_aos_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aos.num_structs;
    const uint64_t struct_size = view->meta.aos.struct_size;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_elems, struct_size, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < num_elems; begin += chunk) {
                const uint64_t end = (begin + chunk < num_elems) ? begin + chunk : num_elems;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool, struct_size)
                {
                    for (uint64_t i = begin; i < end; i++) {
                        const uint64_t base = i * struct_size;
                        for (uint64_t f = 0; f < struct_size; f++) {
                            pool[base + f] = pool[base + f] * 2.0 + 1.0;
                            SLOWDOWN_AFTER_UPDATE(base + f);
                        }
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}



KernelStatus kernel_soa_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.soa.field_length;
    const uint64_t num_fields  = view->meta.soa.num_fields;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_elems, num_fields, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < num_fields; begin += chunk) {
                const uint64_t end = (begin + chunk < num_fields) ? begin + chunk : num_fields;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool, num_elems)
                {
                    for (uint64_t f = begin; f < end; f++) {
                        const uint64_t offset = f * num_elems;
                        for (uint64_t i = 0; i < num_elems; i++) {
                            pool[offset + i] = pool[offset + i] * 2.0 + 1.0;
                            SLOWDOWN_AFTER_UPDATE(offset + i);
                        }
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}



KernelStatus kernel_aosoa_task(DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric) {
    const uint32_t num_threads = test_case->num_threads;
    const uint64_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t num_elems   = view->meta.aosoa.num_blocks;
    const uint64_t num_fields  = view->meta.aosoa.num_fields;
    const uint64_t vl          = view->meta.aosoa.vector_length;

    if (vl == 0) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }

    const uint64_t num_tiles = num_elems;
    const uint64_t tile_size = num_fields * vl;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_tiles, num_fields, vl, tile_size, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0; begin < num_tiles; begin += chunk) {
                const uint64_t end = (begin + chunk < num_tiles) ? begin + chunk : num_tiles;
                #pragma omp task \
                    default(none) \
                    firstprivate(begin, end) \
                    shared(pool, num_fields, vl, tile_size)
                {
                    for (uint64_t t = begin; t < end; t++) {
                        const uint64_t tile_base = t * tile_size;
                        for (uint64_t f = 0; f < num_fields; f++) {
                            const uint64_t field_base = tile_base + f * vl;
                            for (uint64_t v = 0; v < vl; v++) {
                                pool[field_base + v] = pool[field_base + v] * 2.0 + 1.0;
                                SLOWDOWN_AFTER_UPDATE(field_base + v);
                            }
                        }
                    }
                }
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_3_0 */