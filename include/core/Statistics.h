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
 * @file Statistics.h
 * @brief Incremental statistical accumulation using Welford's algorithm.
 *
 * This header provides a small, dependency-free statistical accumulator
 * for benchmark measurements.
 *
 * The implementation uses Welford's online algorithm to compute:
 *
 * - minimum;
 * - maximum;
 * - arithmetic mean;
 * - sample variance.
 *
 * The accumulator can be updated one observation at a time without
 * storing the complete input sequence.
 *
 * This is useful for benchmark analysis because the number of executions
 * may be large and there is no need to retain intermediate statistical
 * state after a sample has been processed.
 *
 * The statistics subsystem is intentionally independent from the
 * benchmark-specific data structures defined by Analyzer.h.
 */

#ifndef CORE_STATISTICS_H
#define CORE_STATISTICS_H

#include <stddef.h>


/**
 * @brief Incremental accumulator for one measured quantity.
 *
 * The accumulator stores the state required by Welford's online
 * algorithm.
 *
 * The variance produced by statistics_finalize() is the sample variance:
 *
 *     variance = M2 / (N - 1)
 *
 * when N is greater than one.
 *
 * For zero observations or one observation, the finalized variance
 * is zero.
 */
typedef struct StatisticsAccumulator {
    size_t count;

    double min;
    double max;

    double mean;
    double m2;
} StatisticsAccumulator;


/**
 * @brief Initialize a statistics accumulator.
 *
 * @param accumulator
 *     Accumulator to initialize.
 *
 * @pre accumulator != NULL.
 *
 * @post count is zero.
 * @post mean and m2 are zero.
 *
 * @note The function does not allocate memory.
 */
void statistics_init(
    StatisticsAccumulator *accumulator
);


/**
 * @brief Add one observation to an accumulator.
 *
 * The update is performed incrementally using Welford's algorithm.
 * No previous observations need to be retained.
 *
 * @param accumulator
 *     Accumulator receiving the observation.
 *
 * @param value
 *     Observation to add.
 *
 * @pre accumulator != NULL.
 *
 * @post count is incremented by one.
 * @post min and max include value.
 * @post mean and m2 represent all observations processed so far.
 */
void statistics_update(
    StatisticsAccumulator *accumulator,
    double                 value
);


/**
 * @brief Finalized statistical results.
 *
 * This structure is intentionally independent from Analyzer.h.
 *
 * It represents the statistical summary of one scalar quantity.
 */
typedef struct StatisticsResult {
    double min;
    double max;
    double mean;
    double variance;
} StatisticsResult;


/**
 * @brief Finalize an accumulated statistical result.
 *
 * The returned variance is the sample variance using N - 1 as the
 * denominator when at least two observations are available.
 *
 * For zero observations, all result fields are zero.
 *
 * For one observation, min, max and mean contain the observation and
 * variance is zero.
 *
 * @param accumulator
 *     Accumulator to finalize.
 *
 * @return
 *     Statistical summary of the observations currently stored in
 *     the accumulator.
 *
 * @pre accumulator != NULL.
 */
StatisticsResult statistics_finalize(
    const StatisticsAccumulator *accumulator
);


#endif /* CORE_STATISTICS_H */