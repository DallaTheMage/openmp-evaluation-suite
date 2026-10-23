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
#include "core/internal/ScalingWriter.h"
#include "utils/Names.h"
#include <stdint.h>
#include <stdio.h>
#include "core/Analyzer.h"
#include "core/TestPlan.h"

static const AggregatedSample *find_aggregated_sample(
    const AggregatedSample *samples,
    size_t sample_count,
    TestSetUID test_set_id,
    TestCaseUID test_case_id,
    KernelType kernel) {
    if (samples == NULL) {
        return NULL;
    }
    for (size_t i = 0U; i < sample_count; i++) {
        if (samples[i].test_set_id == test_set_id &&
            samples[i].test_case_id == test_case_id &&
            samples[i].kernel == kernel) {
            return &samples[i];
        }
    }
    return NULL;
}

/**
 * @brief Find the one-thread baseline matching a scaled case's chunk size.
 * @param group Group of benchmark cases to search.
 * @param scaled_case Case whose chunk size determines the matching baseline.
 * @return Borrowed matching case, or NULL if no baseline exists.
 */
static const TestCase *find_baseline_case(const KernelTestGroup *group, const TestCase *scaled_case) {
    if (group == NULL || scaled_case == NULL) {
        return NULL;
    }
    for (size_t i = 0U; i < group->cases.count; i++) {
        const TestCase *candidate = &group->cases.data[i];
        if (candidate->num_threads == 1U &&
            candidate->chunk_size == scaled_case->chunk_size) {
            return candidate;
        }
    }
    return NULL;
}

int write_scaling_strong(
    FILE *file,
    const TestPlan *plan,
    const AggregatedSample *aggregated,
    size_t aggregated_count) {

    if (fprintf(
            file,
            "scaling_type,test_set_id,view_type,window_offset,window_size,"
            "kernel,chunk_size,baseline_threads,scaled_threads,"
            "baseline_time_sec,scaled_time_sec,speedup,efficiency,"
            "overhead_sec\n") < 0) {
        return -1;
    }

    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];
        for (size_t group_index = 0U; group_index < set->kernels.count; ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            for (size_t case_index = 0U; case_index < group->cases.count; ++case_index) {
                const TestCase *scaled_case   = &group->cases.data[case_index];
                const TestCase *baseline_case = find_baseline_case(group, scaled_case);
                if (baseline_case != NULL) {
                    const AggregatedSample *scaled_sample = find_aggregated_sample(
                                                                aggregated,
                                                                aggregated_count,
                                                                set->uid,
                                                                scaled_case->uid,
                                                                group->kernel
                                                            );
                    const AggregatedSample *baseline_sample = find_aggregated_sample(
                                                                aggregated,
                                                                aggregated_count,
                                                                set->uid,
                                                                baseline_case->uid,
                                                                group->kernel
                                                            );

                    ScalingMetrics metrics;

                    if (scaled_sample != NULL
                        && baseline_sample != NULL
                        && analyzer_compute_scaling(
                            baseline_sample,
                            baseline_case,
                            scaled_sample,
                            scaled_case,
                            &metrics)) {
                        if (fprintf(
                                file,
                                "strong,%llu,%s,%llu,%llu,%s,%llu,%u,%u,"
                                "%.17g,%.17g,%.17g,%.17g,%.17g\n",
                                (unsigned long long) set->uid,
                                utils_view_type_name(set->view.type),
                                (unsigned long long) set->view.window.offset,
                                (unsigned long long) set->view.window.size,
                                utils_kernel_type_name(group->kernel),
                                (unsigned long long) scaled_case->chunk_size,
                                baseline_case->num_threads,
                                scaled_case->num_threads,
                                baseline_sample->wall_time_sec.mean,
                                scaled_sample->wall_time_sec.mean,
                                metrics.speedup,
                                metrics.efficiency,
                                metrics.overhead_sec) < 0) {
                            return -1;
                        }
                    }
                }
            }
        }
    }
    return ferror(file) ? -1 : 0;
}

static int find_weak_baseline(
    const TestPlan *plan,
    size_t scaled_set_index,
    const KernelTestGroup *scaled_group,
    const TestCase *scaled_case,
    size_t *baseline_set_index,
    const TestCase **baseline_case) {

    if (plan == NULL
        || scaled_group == NULL
        || scaled_case == NULL
        || baseline_set_index == NULL
        || baseline_case == NULL) {
        return 0;
    }
    if (scaled_set_index >= plan->count || plan->data == NULL) {
        return 0;
    }
    *baseline_case = NULL;
    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];
        if (set_index != scaled_set_index &&
            set->view.type == plan->data[scaled_set_index].view.type) {
            for (size_t group_index = 0U; group_index < set->kernels.count; ++group_index) {
                const KernelTestGroup *group = &set->kernels.data[group_index];
                if (group->kernel == scaled_group->kernel) {
                    for (size_t case_index = 0U; case_index < group->cases.count; ++case_index) {
                        const TestCase *candidate = &group->cases.data[case_index];
                        const uint64_t baseline_work = set->view.window.size;
                        const uint64_t scaled_work =
                            plan->data[scaled_set_index].view.window.size;

                        if (candidate->chunk_size == scaled_case->chunk_size
                            && candidate->num_threads == 1U
                            && scaled_case->num_threads > 1U
                            && baseline_work > UINT64_C(0)
                            && scaled_work > UINT64_C(0)
                            && scaled_work % baseline_work == UINT64_C(0)
                            && scaled_work / baseline_work == scaled_case->num_threads) {

                            *baseline_set_index = set_index;
                            *baseline_case = candidate;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return *baseline_case != NULL;
}

int write_scaling_weak(
    FILE *file,
    const TestPlan *plan,
    const AggregatedSample *aggregated,
    size_t aggregated_count) {

    if (fprintf(
            file,
            "scaling_type,baseline_test_set_id,scaled_test_set_id,"
            "view_type,baseline_work_elements,scaled_work_elements,"
            "kernel,chunk_size,baseline_threads,scaled_threads,"
            "baseline_time_sec,scaled_time_sec,work_scale,speedup,"
            "efficiency,overhead_sec\n") < 0) {
        return -1;
    }

    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *scaled_set = &plan->data[set_index];
        for (size_t group_index = 0U; group_index < scaled_set->kernels.count; ++group_index) {
            const KernelTestGroup *scaled_group = &scaled_set->kernels.data[group_index];
            for (size_t case_index = 0U; case_index < scaled_group->cases.count; ++case_index) {
                const TestCase *scaled_case = &scaled_group->cases.data[case_index];
                const TestCase *baseline_case = NULL;
                size_t baseline_set_index = set_index;
                if (scaled_case->num_threads == 1U) {
                    baseline_case = scaled_case;
                } else {
                    if (!find_weak_baseline(
                            plan,
                            set_index,
                            scaled_group,
                            scaled_case,
                            &baseline_set_index,
                            &baseline_case)) {
                        continue;
                    }
                }
                const TestSet *baseline_set = &plan->data[baseline_set_index];
                const AggregatedSample *scaled_sample   = find_aggregated_sample(
                                                            aggregated,
                                                            aggregated_count,
                                                            scaled_set->uid,
                                                            scaled_case->uid,
                                                            scaled_group->kernel
                                                        );
                const AggregatedSample *baseline_sample = find_aggregated_sample(
                                                            aggregated,
                                                            aggregated_count,
                                                            baseline_set->uid,
                                                            baseline_case->uid,
                                                            scaled_group->kernel
                                                        );
                ScalingMetrics metrics;

                if (scaled_sample != NULL &&
                    baseline_sample != NULL &&
                    analyzer_compute_weak_scaling(
                        baseline_sample,
                        baseline_case,
                        baseline_set->view.window.size,
                        scaled_sample,
                        scaled_case,
                        scaled_set->view.window.size,
                        &metrics)) {

                    if (fprintf(
                            file,
                            "weak,%llu,%llu,%s,%llu,%llu,%s,%llu,%u,%u,"
                            "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
                            (unsigned long long) baseline_set->uid,
                            (unsigned long long) scaled_set->uid,
                            utils_view_type_name(scaled_set->view.type),
                            (unsigned long long) baseline_set->view.window.size,
                            (unsigned long long) scaled_set->view.window.size,
                            utils_kernel_type_name(scaled_group->kernel),
                            (unsigned long long) scaled_case->chunk_size,
                            baseline_case->num_threads,
                            scaled_case->num_threads,
                            baseline_sample->wall_time_sec.mean,
                            scaled_sample->wall_time_sec.mean,
                            (double) scaled_set->view.window.size /
                                (double) baseline_set->view.window.size,
                            metrics.speedup,
                            metrics.efficiency,
                            metrics.overhead_sec) < 0) {
                        return -1;
                    }
                }
            }
        }
    }

    return ferror(file) ? -1 : 0;
}
