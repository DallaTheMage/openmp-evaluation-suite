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
#include "kernels/algorithms/ViewValidation.h"

#if OPENMP_HAS_2_0
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
    if (!kernel_valid_2d_view(view, tc, profiler, metric, 3)) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    size_t rows         = (size_t)view->meta.v2d.rows;
    size_t cols         = (size_t)view->meta.v2d.cols;
    size_t count        = rows * cols;
    size_t bytes        = count * sizeof(double);
    const double *input = view->buffer->pool + view->window.offset;
    double *a           = malloc(bytes);
    double *b           = malloc(bytes);
    double *ref         = malloc(bytes);
    double *tmp         = malloc(bytes);

    if (!a || !b || !ref || !tmp) {
        free_jacobi_components(a, b, ref, tmp);
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }
    memcpy(a, input, bytes);
    memcpy(ref, input, bytes);
    /* Boundary values never change; prefill both workspaces before timing. */
    memcpy(b, input, bytes);
    if (profiler_start(profiler) != 0) {
        free_jacobi_components(a, b, ref, tmp);
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
        a            = b;
        b            = swap;
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
    free_jacobi_components(a, b, ref, tmp);
    if(stop != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return valid ? KERNEL_STATUS_OK : KERNEL_STATUS_EXECUTION_FAILURE;
}
#endif
