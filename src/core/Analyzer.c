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
 * @file Analyzer.c
 * @brief Analysis and aggregation of benchmark measurements.
 */

#include "core/Analyzer.h"

#include <stdlib.h>

#include "core/Statistics.h"


typedef struct GroupAccumulator {
    StatisticsAccumulator wall_time_sec;
    StatisticsAccumulator cycles;
    StatisticsAccumulator instructions;
    StatisticsAccumulator l1d_misses;
    StatisticsAccumulator llc_misses;
    StatisticsAccumulator ipc;
    StatisticsAccumulator l1d_miss_ratio;
    StatisticsAccumulator llc_miss_ratio;
    StatisticsAccumulator energy_pkg_joules;
    StatisticsAccumulator energy_dram_joules;
} GroupAccumulator;


static void group_accumulator_init(GroupAccumulator *acc)
{
    if (acc == NULL) {
        return;
    }

    statistics_init(&acc->wall_time_sec);
    statistics_init(&acc->cycles);
    statistics_init(&acc->instructions);
    statistics_init(&acc->l1d_misses);
    statistics_init(&acc->llc_misses);
    statistics_init(&acc->ipc);
    statistics_init(&acc->l1d_miss_ratio);
    statistics_init(&acc->llc_miss_ratio);
    statistics_init(&acc->energy_pkg_joules);
    statistics_init(&acc->energy_dram_joules);
}


static void group_accumulator_update(
    GroupAccumulator *acc,
    const PerformanceMetric *metric
)
{
    if (acc == NULL || metric == NULL) {
        return;
    }

    statistics_update(&acc->wall_time_sec, metric->wall_time_sec);
    statistics_update(&acc->cycles, (double) metric->cycles);
    statistics_update(&acc->instructions, (double) metric->instructions);
    statistics_update(&acc->l1d_misses, (double) metric->l1d_misses);
    statistics_update(&acc->llc_misses, (double) metric->llc_misses);
    statistics_update(&acc->ipc, metric->ipc);
    statistics_update(&acc->l1d_miss_ratio, metric->l1d_miss_ratio);
    statistics_update(&acc->llc_miss_ratio, metric->llc_miss_ratio);
    statistics_update(&acc->energy_pkg_joules, metric->energy_pkg_joules);
    statistics_update(&acc->energy_dram_joules, metric->energy_dram_joules);
}


static Metrics statistics_to_metrics(const StatisticsResult *result)
{
    Metrics metrics = {0.0, 0.0, 0.0, 0.0};

    if (result != NULL) {
        metrics.min = result->min;
        metrics.max = result->max;
        metrics.mean = result->mean;
        metrics.variance = result->variance;
    }

    return metrics;
}


static void finalize_group(
    AggregatedSample *dst,
    const RawSample *sample,
    const GroupAccumulator *acc
)
{
    if (dst == NULL || sample == NULL || acc == NULL) {
        return;
    }

    dst->test_set_id = sample->test_set_id;
    dst->test_case_id = sample->test_case_id;
    dst->kernel = sample->kernel;
    dst->total_runs = acc->wall_time_sec.count;

    {
        const StatisticsResult result = statistics_finalize(&acc->wall_time_sec);
        dst->wall_time_sec = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->cycles);
        dst->cycles = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->instructions);
        dst->instructions = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->l1d_misses);
        dst->l1d_misses = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->llc_misses);
        dst->llc_misses = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->ipc);
        dst->ipc = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->l1d_miss_ratio);
        dst->l1d_miss_ratio = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->llc_miss_ratio);
        dst->llc_miss_ratio = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->energy_pkg_joules);
        dst->energy_pkg_joules = statistics_to_metrics(&result);
    }
    {
        const StatisticsResult result = statistics_finalize(&acc->energy_dram_joules);
        dst->energy_dram_joules = statistics_to_metrics(&result);
    }
}


bool analyzer_same_test_case(
    const RawSample *a,
    const RawSample *b
)
{
    return a != NULL &&
           b != NULL &&
           a->test_set_id == b->test_set_id &&
           a->test_case_id == b->test_case_id;
}


bool analyzer_same_experiment(
    const RawSample *a,
    const RawSample *b
)
{
    return analyzer_same_test_case(a, b) &&
           a->kernel == b->kernel;
}


bool analyzer_same_experiment_aggregate(
    const AggregatedSample *a,
    const AggregatedSample *b
)
{
    return a != NULL &&
           b != NULL &&
           a->test_set_id == b->test_set_id &&
           a->test_case_id == b->test_case_id &&
           a->kernel == b->kernel;
}


AggregatedSample *analyzer_aggregate_samples(
    const RawSample *samples,
    size_t sample_count,
    size_t *out_count
)
{
    AggregatedSample *result;
    size_t group_count;
    size_t group_index;
    GroupAccumulator accumulator;

    if (out_count == NULL) {
        return NULL;
    }

    *out_count = 0U;

    if (samples == NULL || sample_count == 0U) {
        return NULL;
    }

    group_count = 1U;
    for (size_t i = 1U; i < sample_count; ++i) {
        if (!analyzer_same_experiment(&samples[i - 1U], &samples[i])) {
            ++group_count;
        }
    }

    result = calloc(group_count, sizeof(*result));
    if (result == NULL) {
        return NULL;
    }

    group_index = 0U;
    group_accumulator_init(&accumulator);

    for (size_t i = 0U; i < sample_count; ++i) {
        const RawSample *sample = &samples[i];

        if (i != 0U &&
            !analyzer_same_experiment(&samples[i - 1U], sample)) {
            finalize_group(
                &result[group_index],
                &samples[i - 1U],
                &accumulator
            );
            ++group_index;
            group_accumulator_init(&accumulator);
        }

        group_accumulator_update(&accumulator, &sample->metric);
    }

    finalize_group(
        &result[group_index],
        &samples[sample_count - 1U],
        &accumulator
    );

    *out_count = group_count;
    return result;
}


bool analyzer_compute_scaling(
    const AggregatedSample *baseline,
    const TestCase *baseline_point,
    const AggregatedSample *scaled,
    const TestCase *scaled_point,
    ScalingMetrics *out_metrics
)
{
    double baseline_time;
    double scaled_time;
    double thread_ratio;

    if (baseline == NULL ||
        baseline_point == NULL ||
        scaled == NULL ||
        scaled_point == NULL ||
        out_metrics == NULL) {
        return false;
    }

    if (baseline->test_set_id != scaled->test_set_id ||
        baseline->kernel != scaled->kernel ||
        baseline->test_case_id != baseline_point->uid ||
        scaled->test_case_id != scaled_point->uid ||
        baseline_point->num_threads == 0U ||
        scaled_point->num_threads == 0U) {
        return false;
    }

    baseline_time = baseline->wall_time_sec.mean;
    scaled_time = scaled->wall_time_sec.mean;
    if (baseline_time <= 0.0 || scaled_time <= 0.0) {
        return false;
    }

    thread_ratio = (double) scaled_point->num_threads /
                   (double) baseline_point->num_threads;
    if (thread_ratio <= 0.0) {
        return false;
    }

    out_metrics->speedup = baseline_time / scaled_time;
    out_metrics->efficiency = out_metrics->speedup / thread_ratio;
    out_metrics->overhead_sec =
        scaled_time - (baseline_time / thread_ratio);

    return true;
}


bool analyzer_compute_weak_scaling(
    const AggregatedSample *baseline,
    const TestCase *baseline_point,
    uint64_t baseline_work,
    const AggregatedSample *scaled,
    const TestCase *scaled_point,
    uint64_t scaled_work,
    ScalingMetrics *out_metrics
)
{
    const double baseline_time =
        baseline != NULL ? baseline->wall_time_sec.mean : 0.0;
    const double scaled_time =
        scaled != NULL ? scaled->wall_time_sec.mean : 0.0;
    double work_ratio;

    if (baseline == NULL ||
        baseline_point == NULL ||
        scaled == NULL ||
        scaled_point == NULL ||
        out_metrics == NULL ||
        baseline_work == UINT64_C(0) ||
        scaled_work == UINT64_C(0) ||
        baseline_point->num_threads == 0U ||
        scaled_point->num_threads == 0U ||
        scaled_point->num_threads <= baseline_point->num_threads ||
        baseline->kernel != scaled->kernel ||
        baseline_time <= 0.0 ||
        scaled_time <= 0.0) {
        return false;
    }

    work_ratio = (double)scaled_work / (double)baseline_work;
    if (work_ratio <= 0.0) {
        return false;
    }

    out_metrics->efficiency = baseline_time / scaled_time;
    out_metrics->speedup = work_ratio * out_metrics->efficiency;
    out_metrics->overhead_sec = scaled_time - baseline_time;

    return true;
}