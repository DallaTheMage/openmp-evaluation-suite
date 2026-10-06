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
 * @file Analyzer.h
 * @brief Analysis and aggregation interface for benchmark measurements.
 *
 * This header defines the data structures and public operations used to
 * transform raw benchmark measurements into aggregated statistical results.
 *
 * The analyzer operates after benchmark execution has completed. It does
 * not participate in the benchmark hot path and must not be required by
 * benchmark kernels.
 *
 * The analysis pipeline is:
 *
 *         RawSample
 *             |
 *             v
 *         RawSampleSet
 *             |
 *             v
 *  analyzer_aggregate_samples()
 *             |
 *             v
 *       AggregatedSample
 *
 * Statistical accumulation is delegated to the statistics subsystem.
 * The analyzer is responsible for identifying samples belonging to the
 * same experiment and for computing experiment-level results.
 */

#ifndef CORE_ANALYZER_H
#define CORE_ANALYZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/TestPlan.h"
#include "kernels/registry.h"
#include "profiling/Profiler.h"


/**
 * @brief One raw measurement produced by a benchmark execution.
 *
 * A RawSample contains the complete measurement associated with one
 * execution of one kernel for one test case.
 *
 * The PerformanceMetric structure contains the values measured by the
 * profiler. No statistical aggregation is performed at this level.
 */
typedef struct RawSample {
    TestSetUID       test_set_id;
    TestCaseUID      test_case_id;
    KernelType       kernel;
    uint32_t         run_id;
    PerformanceMetric metric;
} RawSample;


/**
 * @brief Dynamically allocated collection of raw benchmark samples.
 *
 * The collection owns the memory referenced by @c samples.
 */
typedef struct RawSampleSet {
    RawSample *samples;
    size_t     count;
} RawSampleSet;


/**
 * @brief Statistical summary of one measured quantity.
 *
 * Variance is the sample variance, i.e. the unbiased estimator using
 * N - 1 as denominator when at least two observations are available.
 *
 * For a single observation, variance is zero.
 */
typedef struct Metrics {
    double min;
    double max;
    double mean;
    double variance;
} Metrics;


/**
 * @brief Aggregated measurements belonging to one experiment.
 *
 * Samples are grouped by test set, test case and kernel.
 *
 * Each Metrics member describes the distribution of the corresponding
 * quantity across all executions belonging to the experiment.
 */
typedef struct AggregatedSample {
    TestSetUID  test_set_id;
    TestCaseUID test_case_id;
    KernelType  kernel;

    size_t total_runs;

    Metrics wall_time_sec;

    Metrics cycles;
    Metrics instructions;
    Metrics l1d_misses;
    Metrics llc_misses;

    Metrics ipc;
    Metrics l1d_miss_ratio;
    Metrics llc_miss_ratio;

    Metrics energy_pkg_joules;
    Metrics energy_dram_joules;
} AggregatedSample;


/**
 * @brief Scaling metrics computed between two benchmark configurations.
 *
 * Speedup is defined as:
 *
 *     baseline_time / scaled_time
 *
 * Efficiency is defined relative to the number of threads of the two
 * corresponding test cases.
 *
 * Overhead is expressed in seconds.
 */
typedef struct ScalingMetrics {
    double speedup;
    double efficiency;
    double overhead_sec;
} ScalingMetrics;


/**
 * @brief Aggregate raw benchmark samples by experiment identity.
 *
 * Samples belonging to the same test set, test case and kernel are
 * aggregated into one AggregatedSample.
 *
 * Statistical accumulation is performed using the statistics subsystem.
 *
 * @param samples
 *     Array of raw samples.
 *
 * @param sample_count
 *     Number of elements in @p samples.
 *
 * @param out_count
 *     Output location receiving the number of aggregated samples.
 *
 * @return
 *     Dynamically allocated array of AggregatedSample objects on success.
 *     NULL on invalid input or allocation failure.
 *
 * @pre If @p sample_count is greater than zero, @p samples must not be NULL.
 * @pre @p out_count must not be NULL.
 *
 * @post On success, @p out_count contains the number of aggregated
 *       experiments.
 *
 * @note The caller owns the returned array and must release it with free().
 */
AggregatedSample *analyzer_aggregate_samples(
    const RawSample *samples,
    size_t           sample_count,
    size_t          *out_count
);


/**
 * @brief Compute scaling metrics between two aggregated experiments.
 *
 * The baseline and scaled measurements must describe compatible
 * experiments. The corresponding TestCase objects provide execution
 * parameters such as the number of threads required to compute scaling
 * efficiency.
 *
 * @param baseline
 *     Aggregated baseline measurement.
 *
 * @param baseline_point
 *     Test case describing the baseline execution.
 *
 * @param scaled
 *     Aggregated scaled measurement.
 *
 * @param scaled_point
 *     Test case describing the scaled execution.
 *
 * @param out_metrics
 *     Output structure receiving the computed scaling metrics.
 *
 * @return
 *     true when the metrics can be computed, false otherwise.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The two measurements must represent compatible experiments.
 * @pre Mean wall-clock times must be valid and non-zero.
 */
bool analyzer_compute_scaling(
    const AggregatedSample *baseline,
    const TestCase         *baseline_point,
    const AggregatedSample *scaled,
    const TestCase         *scaled_point,
    ScalingMetrics         *out_metrics
);


/**
 * @brief Check whether two raw samples belong to the same test case.
 *
 * This comparison checks the identity of the experiment configuration
 * represented by the raw samples.
 *
 * @param a First sample.
 * @param b Second sample.
 *
 * @return true if the samples represent the same test case, false otherwise.
 *
 * @pre Both pointers must be non-NULL.
 */
bool analyzer_same_test_case(
    const RawSample *a,
    const RawSample *b
);


/**
 * @brief Check whether two raw samples belong to the same experiment.
 *
 * This comparison includes the kernel identity in addition to the test
 * case identity.
 *
 * @param a First sample.
 * @param b Second sample.
 *
 * @return true if both samples belong to the same experiment, false otherwise.
 *
 * @pre Both pointers must be non-NULL.
 */
bool analyzer_same_experiment(
    const RawSample *a,
    const RawSample *b
);


/**
 * @brief Check whether two aggregated samples represent the same experiment.
 *
 * @param a First aggregated sample.
 * @param b Second aggregated sample.
 *
 * @return true if both aggregated samples have the same experiment identity,
 *         false otherwise.
 *
 * @pre Both pointers must be non-NULL.
 */
bool analyzer_same_experiment_aggregate(
    const AggregatedSample *a,
    const AggregatedSample *b
);


#endif /* CORE_ANALYZER_H */