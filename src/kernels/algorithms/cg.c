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

/** @file cg.c
 *  @brief Matrix-free Conjugate Gradient on a five-point Dirichlet Laplacian.
 */

#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "config/openmp.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"
#include "kernels/algorithms/cg.h"
#include "kernels/algorithms/ViewValidation.h"

#if OPENMP_HAS_2_0


/** @brief Apply the SPD five-point Dirichlet Poisson matrix without CSR storage. */
static void poisson_apply(const double *x, double *y, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            spd_five_point_dirichlet(i, j, rows, cols, x, y);
        }
    }
}
/** @brief Parallelize a matrix-vector product across independent grid rows. */
static void poisson_apply_parallel(
    const double *x,
    double *y,
    size_t rows,
    size_t cols,
    const TestCase *tc) {
    #pragma omp parallel for \
        default(none) \
        num_threads(tc->num_threads) \
        schedule(CHOSEN_SCHEDULE, tc->chunk_size) \
        shared(cols, rows, tc, x, y)
    for(size_t i = 0; i < rows; ++i) {
        for(size_t j = 0; j < cols; ++j) {
            spd_five_point_dirichlet(i, j, rows, cols, x, y);
        }
    }
}

KernelStatus kernel_2d_cg(DataView *view,const TestCase *tc,
                          Profiler *profiler,PerformanceMetric *metric) {
    if(!kernel_valid_2d_view(view,tc, profiler,metric, 2)) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    size_t rows  = (size_t)view->meta.v2d.rows;
    size_t cols  = (size_t)view->meta.v2d.cols;
    size_t count = rows * cols;
    size_t bytes = count * sizeof(double);

    double *x     = calloc(count, sizeof(double));
    double *r     = malloc(bytes);
    double *p     = malloc(bytes);
    double *ap    = malloc(bytes);
    double *check = malloc(bytes);

    if(!x || !r || !p || !ap || !check) {
        free_cg_components(x, r, p, ap, check);
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }

    /* Bounded RHS avoids overflow and makes the workload deterministic. */
    const double *in = view->buffer->pool + view->window.offset;
    for(size_t z = 0; z < count; ++z) {
        r[z] = 0.25 + (double)(((z * 17U) + 3U) % 97U) / 97.0;
        if(isfinite(in[z])) {
            r[z] += 0.01 * (in[z] / (1.0 + fabs(in[z])));
        }
        p[z] = r[z];
    }
    double rr = 0.0;
    for(size_t z = 0; z < count; ++z) {
        rr += r[z] * r[z];
    }
    if(!isfinite(rr) || rr <= 0.0) {
        free_cg_components(x, r, p, ap, check);
        return KERNEL_STATUS_EXECUTION_FAILURE;
    }
    const double initial_rr = rr;
    int valid = 1;
    if(profiler_start(profiler) != 0) {
        free_cg_components(x, r, p, ap, check);
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    for(unsigned it = 0; it < CG_STEPS; ++it) {
        poisson_apply_parallel(p, ap, rows, cols, tc);
        double pap = 0.0;
        #pragma omp parallel for \
            default(none) \
            reduction(+:pap) \
            num_threads(tc->num_threads) \
            schedule(CHOSEN_SCHEDULE, tc->chunk_size) \
            shared(ap, count, p, tc)
        for(size_t z = 0; z < count; ++z) {
            pap += p[z] * ap[z];
        }
        if(!isfinite(pap) || pap <= 0.0 || !isfinite(rr)) {
            valid = 0;
            break;
        }
        double alpha = rr / pap;
        double new_rr = 0.0;
        #pragma omp parallel for \
            default(none) \
            reduction(+:new_rr) \
            num_threads(tc->num_threads) \
            schedule(CHOSEN_SCHEDULE, tc->chunk_size) \
            shared(alpha, ap, count, p, r, tc, x)
        for(size_t z = 0; z < count; ++z) {
            x[z]   += alpha * p[z];
            r[z]   -= alpha * ap[z];
            new_rr += r[z] * r[z];
        }
        if(!isfinite(new_rr)) {
            valid = 0;
            break;
        }
        /* Numerical termination only avoids division by zero on tiny systems. */
        if(new_rr <= (1e-26 * initial_rr)) {
            rr = new_rr;
            break;
        }
        /* The regular benchmark uses a fixed iteration count. */
        double beta = new_rr / rr;
        #pragma omp parallel for \
            default(none) \
            num_threads(tc->num_threads) \
            schedule(CHOSEN_SCHEDULE, tc->chunk_size) \
            shared(beta, count, p, r, tc)
        for(size_t z = 0; z < count; ++z) {
            p[z] = r[z] + (beta * p[z]);
        }
        rr = new_rr;
    }
    int stop = profiler_stop(profiler, metric);
    if(valid) {
        poisson_apply(x, check, rows, cols);
        /* Check the true residual independently of the updated r vector. */
        double true_rr = 0.0;
        for(size_t z = 0; z < count; ++z) {
            double rhs = 0.25 + (double)(((z * 17U) + 3U) % 97U) / 97.0;
            if(isfinite(in[z])) {
                rhs += 0.01 * (in[z] / (1.0 + fabs(in[z])));
            }
            double d = rhs - check[z];
            true_rr += d * d;
        }
        if(!isfinite(true_rr) || !isfinite(rr)
            || fabs(sqrt(true_rr) - sqrt(rr)) > (1e-7 * fmax(1.0, sqrt(initial_rr)))
        ) {
            valid = 0;
        }
    }
    free_cg_components(x, r, p, ap, check);
    if(stop != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return valid ? KERNEL_STATUS_OK : KERNEL_STATUS_EXECUTION_FAILURE;
}
#endif
