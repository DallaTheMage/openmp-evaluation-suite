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
 * @file ViewValidation.c
 * @brief Two-dimensional kernel view validity checks.
 */

#include <stdint.h>
#include <stddef.h>

#include "kernels/algorithms/ViewValidation.h"

int kernel_valid_2d_view(const DataView *v, const TestCase *tc,
                         const Profiler *profiler, const PerformanceMetric *metric,
                         size_t min_side) {
    if (!v || !tc || !profiler || !metric || !v->buffer || !v->buffer->pool
        || v->type != VIEW_2D || !tc->num_threads || !tc->chunk_size
        || v->meta.v2d.rows < min_side || v->meta.v2d.cols < min_side) {
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
