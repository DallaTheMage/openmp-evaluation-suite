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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file Statistics.c
 * @brief Incremental statistical accumulation using Welford's algorithm.
 */

#include "core/Statistics.h"

#include <float.h>
#include <math.h>


void statistics_init(StatisticsAccumulator *accumulator)
{
    if (accumulator == NULL) {
        return;
    }

    accumulator->count = 0U;
    accumulator->min = DBL_MAX;
    accumulator->max = -DBL_MAX;
    accumulator->mean = 0.0;
    accumulator->m2 = 0.0;
}


void statistics_update(
    StatisticsAccumulator *accumulator,
    double value
)
{
    double delta;

    if (accumulator == NULL || !isfinite(value)) {
        return;
    }

    if (accumulator->count == 0U) {
        accumulator->min = value;
        accumulator->max = value;
    } else {
        if (value < accumulator->min) {
            accumulator->min = value;
        }
        if (value > accumulator->max) {
            accumulator->max = value;
        }
    }

    ++accumulator->count;

    delta = value - accumulator->mean;
    accumulator->mean += delta / (double) accumulator->count;
    accumulator->m2 += delta * (value - accumulator->mean);
}


StatisticsResult statistics_finalize(
    const StatisticsAccumulator *accumulator
)
{
    StatisticsResult result = {0.0, 0.0, 0.0, 0.0};

    if (accumulator == NULL || accumulator->count == 0U) {
        return result;
    }

    result.min = accumulator->min;
    result.max = accumulator->max;
    result.mean = accumulator->mean;

    if (accumulator->count > 1U) {
        result.variance = accumulator->m2 /
                          (double) (accumulator->count - 1U);
    }

    return result;
}
