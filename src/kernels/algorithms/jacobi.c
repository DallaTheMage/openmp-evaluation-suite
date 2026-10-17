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

/** @file jacobi.c
 * @brief Eight-step five-point Jacobi stencil, with an untimed serial reference.
 */
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config/openmp.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"
#include "kernels/algorithms/jacobi.h"

#if OPENMP_HAS_2_0
/** @brief Check VIEW_2D bounds and byte-size arithmetic before allocation. */
static int valid_view(const DataView *v, const TestCase *tc,
                      const Profiler *profiler, const PerformanceMetric *metric,
                      size_t min_side) {
    if (!v || !tc || !profiler || !metric || !v->buffer || !v->buffer->pool ||
        v->type != VIEW_2D || !tc->num_threads || !tc->chunk_size ||
        v->meta.v2d.rows < min_side || v->meta.v2d.cols < min_side) {
        return 0;
    }
    const uint64_t r = v->meta.v2d.rows;
    const uint64_t c = v->meta.v2d.cols;
    if ((r > SIZE_MAX) || (c > SIZE_MAX) || (r > (SIZE_MAX / c)) ||
        ((r * c) > (SIZE_MAX / sizeof(double))) || (v->window.size < (r * c)) ||
        (v->window.offset > v->buffer->element_count) ||
        ((r * c) > (v->buffer->element_count - v->window.offset))) {
        return 0;
    }
    return 1;
}

#define JACOBI_STEPS 8U
/** @brief Apply one Jacobi stencil step, preserving fixed boundary cells. */
static void serial_step(const double *in, double *out, size_t rows, size_t cols) {
    memcpy(out, in, (rows * cols * sizeof(*out)));
    for (size_t i = 1; (i + 1) < rows; ++i)
        for (size_t j = 1; (j + 1) < cols; ++j) {
            const size_t z = (i * cols) + j;
            out[z] = 0.25 * (in[z - cols] + in[z + cols] + in[z - 1] + in[z + 1]);
        }
}
KernelStatus kernel_2d_jacobi(DataView *view, const TestCase *tc,
                              Profiler *profiler, PerformanceMetric *metric) {
    if (!valid_view(view, tc, profiler, metric, 3)) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    size_t rows = (size_t)view->meta.v2d.rows;
    size_t cols = (size_t)view->meta.v2d.cols;
    size_t count = rows * cols;
    size_t bytes = count * sizeof(double);
    const double *input = view->buffer->pool + view->window.offset;
    double *a = malloc(bytes);
    double *b = malloc(bytes);
    double *ref = malloc(bytes);
    double *tmp = malloc(bytes);

    if (!a || !b || !ref || !tmp) {
        free(a);
        free(b);
        free(ref);
        free(tmp);
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }
    memcpy(a, input, bytes);
    memcpy(ref, input, bytes);
    /* Boundary values never change; prefill both workspaces before timing. */
    memcpy(b, input, bytes);
    if (profiler_start(profiler) != 0) {
        free(a);
        free(b);
        free(ref);
        free(tmp);
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    for (unsigned step = 0; step < JACOBI_STEPS; ++step) {
        #pragma omp parallel for num_threads(tc->num_threads) schedule(CHOSEN_SCHEDULE, tc->chunk_size)
        for (size_t i = 1; i < (rows - 1); ++i) {
            for (size_t j = 1;  j < (cols - 1); ++j) {
                size_t z = (i * cols) + j;
                b[z] = 0.25 * (a[z - cols] + a[z + cols] + a[z - 1] + a[z + 1]);
            }
        }
        double *swap = a;
        a = b;
        b = swap;
    }
    int stop = profiler_stop(profiler, metric);
    /* Independent serial reference, excluded from the timed region. */
    for (unsigned step = 0; step < JACOBI_STEPS; ++step) {
        serial_step(ref, tmp, rows, cols);
        double *swap = ref;
        ref = tmp;
        tmp = swap;
    }
    int valid = 1;
    for (size_t z = 0; z < count; ++z) {
        if (!isfinite(a[z]) || !isfinite(ref[z]) ||
            fabs(a[z] - ref[z]) > (1e-11 * fmax(1.0, fabs(ref[z])))) {
                valid = 0;
                break;
        }
    }
    free(a);
    free(b);
    free(ref);
    free(tmp);
    if(stop != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return valid ? KERNEL_STATUS_OK : KERNEL_STATUS_EXECUTION_FAILURE;
}
#endif
