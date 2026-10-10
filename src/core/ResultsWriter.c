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
 * @file ResultsWriter.c
 * @brief CSV serialization of benchmark plans and measurements.
 */

#include "core/ResultsWriter.h"

#include <stdint.h>
#include <stdio.h>

#include "data/views/ViewType.h"


static const char *view_type_to_string(ViewType type)
{
    switch (type) {
        case VIEW_1D:
            return "1D";
        case VIEW_2D:
            return "2D";
        case VIEW_3D:
            return "3D";
        case VIEW_AOS:
            return "AoS";
        case VIEW_SOA:
            return "SoA";
        case VIEW_AOSOA:
            return "AoSoA";
        default:
            return "Unknown";
    }
}


static const char *kernel_type_to_string(KernelType type)
{
    switch (type) {
        case KERNEL_LOOP:
            return "Loop";
        case KERNEL_COLLAPSE:
            return "Collapse";
        case KERNEL_SECTIONS:
            return "Sections";
        case KERNEL_SIMD_MEMORY:
            return "SIMD Memory";
        case KERNEL_SIMD_COMPUTE:
            return "SIMD Compute";
        case KERNEL_MASTER:
            return "Master";
        case KERNEL_MASKED:
            return "Masked";
        case KERNEL_ATOMIC:
            return "Atomic";
        case KERNEL_CRITICAL:
            return "Critical";
        case KERNEL_ORDERED:
            return "Ordered";
        case KERNEL_SYNC:
            return "Sync";
        case KERNEL_REDUCTION:
            return "Reduction";
        case KERNEL_TASK:
            return "Task";
        case KERNEL_TASKLOOP:
            return "Taskloop";
        case KERNEL_NATIVE_SCAN:
            return "Native Scan";
        case KERNEL_TWO_PASS_SCAN:
            return "Two-Pass Scan";
        default:
            return "Unknown";
    }
}


static int write_metric_values(FILE *file, const Metrics *metric)
{
    if (file == NULL || metric == NULL) {
        return -1;
    }

    if (fprintf(
            file,
            "%.17g,%.17g,%.17g,%.17g",
            metric->min,
            metric->max,
            metric->mean,
            metric->variance) < 0) {
        return -1;
    }

    return 0;
}


static int write_test_plan(FILE *file, const TestPlan *plan)
{
    if (fprintf(
            file,
            "test_set_id,view_type,window_offset,window_size,"
            "kernel,test_case_id,num_threads,chunk_size\n") < 0) {
        return -1;
    }

    for (size_t i = 0U; i < plan->count; ++i) {
        const TestSet *set = &plan->data[i];

        for (size_t k = 0U; k < set->kernels.count; ++k) {
            const KernelTestGroup *group = &set->kernels.data[k];

            for (size_t c = 0U; c < group->cases.count; ++c) {
                const TestCase *test_case = &group->cases.data[c];

                if (fprintf(
                        file,
                        "%llu,%s,%llu,%llu,%s,%llu,%u,%llu\n",
                        (unsigned long long) set->uid,
                        view_type_to_string(set->view.type),
                        (unsigned long long) set->view.window.offset,
                        (unsigned long long) set->view.window.size,
                        kernel_type_to_string(group->kernel),
                        (unsigned long long) test_case->uid,
                        test_case->num_threads,
                        (unsigned long long) test_case->chunk_size) < 0) {
                    return -1;
                }
            }
        }
    }

    return ferror(file) ? -1 : 0;
}


static int write_raw_samples(FILE *file, const RawSampleSet *samples)
{
    if (fprintf(
            file,
            "test_set_id,test_case_id,kernel,run_id,wall_time_sec,"
            "cycles,instructions,l1d_misses,llc_misses,ipc,"
            "l1d_miss_ratio,llc_miss_ratio,energy_pkg_joules,"
            "energy_dram_joules\n") < 0) {
        return -1;
    }

    for (size_t i = 0U; i < samples->count; ++i) {
        const RawSample *sample = &samples->samples[i];
        const PerformanceMetric *metric = &sample->metric;

        if (fprintf(
                file,
                "%llu,%llu,%s,%u,%.17g,%llu,%llu,%llu,%llu,%.17g,"
                "%.17g,%.17g,%.17g,%.17g\n",
                (unsigned long long) sample->test_set_id,
                (unsigned long long) sample->test_case_id,
                kernel_type_to_string(sample->kernel),
                sample->run_id,
                metric->wall_time_sec,
                (unsigned long long) metric->cycles,
                (unsigned long long) metric->instructions,
                (unsigned long long) metric->l1d_misses,
                (unsigned long long) metric->llc_misses,
                metric->ipc,
                metric->l1d_miss_ratio,
                metric->llc_miss_ratio,
                metric->energy_pkg_joules,
                metric->energy_dram_joules) < 0) {
            return -1;
        }
    }

    return ferror(file) ? -1 : 0;
}


static int write_aggregated_samples(
    FILE *file,
    const AggregatedSample *samples,
    size_t sample_count
)
{
    if (fprintf(
            file,
            "test_set_id,test_case_id,kernel,total_runs,"
            "wall_time_sec_min,wall_time_sec_max,wall_time_sec_mean,"
            "wall_time_sec_variance,cycles_min,cycles_max,cycles_mean,"
            "cycles_variance,instructions_min,instructions_max,"
            "instructions_mean,instructions_variance,l1d_misses_min,"
            "l1d_misses_max,l1d_misses_mean,l1d_misses_variance,"
            "llc_misses_min,llc_misses_max,llc_misses_mean,"
            "llc_misses_variance,ipc_min,ipc_max,ipc_mean,ipc_variance,"
            "l1d_miss_ratio_min,l1d_miss_ratio_max,l1d_miss_ratio_mean,"
            "l1d_miss_ratio_variance,llc_miss_ratio_min,"
            "llc_miss_ratio_max,llc_miss_ratio_mean,llc_miss_ratio_variance,"
            "energy_pkg_joules_min,energy_pkg_joules_max,"
            "energy_pkg_joules_mean,energy_pkg_joules_variance,"
            "energy_dram_joules_min,energy_dram_joules_max,"
            "energy_dram_joules_mean,energy_dram_joules_variance\n") < 0) {
        return -1;
    }

    for (size_t i = 0U; i < sample_count; ++i) {
        const AggregatedSample *sample = &samples[i];

        if (fprintf(
                file,
                "%llu,%llu,%s,%zu,",
                (unsigned long long) sample->test_set_id,
                (unsigned long long) sample->test_case_id,
                kernel_type_to_string(sample->kernel),
                sample->total_runs) < 0) {
            return -1;
        }

        if (write_metric_values(file, &sample->wall_time_sec) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->cycles) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->instructions) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->l1d_misses) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->llc_misses) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->ipc) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->l1d_miss_ratio) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->llc_miss_ratio) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->energy_pkg_joules) != 0 ||
            fputc(',', file) == EOF ||
            write_metric_values(file, &sample->energy_dram_joules) != 0 ||
            fputc('\n', file) == EOF) {
            return -1;
        }
    }

    return ferror(file) ? -1 : 0;
}


static const AggregatedSample *find_aggregated_sample(
    const AggregatedSample *samples,
    size_t sample_count,
    TestSetUID test_set_id,
    TestCaseUID test_case_id,
    KernelType kernel
)
{
    if (samples == NULL) {
        return NULL;
    }

    for (size_t i = 0U; i < sample_count; ++i) {
        if (samples[i].test_set_id == test_set_id &&
            samples[i].test_case_id == test_case_id &&
            samples[i].kernel == kernel) {
            return &samples[i];
        }
    }

    return NULL;
}


static const TestCase *find_baseline_case(
    const KernelTestGroup *group,
    const TestCase *scaled_case
)
{
    if (group == NULL || scaled_case == NULL) {
        return NULL;
    }

    for (size_t i = 0U; i < group->cases.count; ++i) {
        const TestCase *candidate = &group->cases.data[i];

        if (candidate->num_threads == 1U &&
            candidate->chunk_size == scaled_case->chunk_size) {
            return candidate;
        }
    }

    return NULL;
}


static int write_scaling_strong(
    FILE *file,
    const TestPlan *plan,
    const AggregatedSample *aggregated,
    size_t aggregated_count
)
{
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

        for (size_t group_index = 0U;
             group_index < set->kernels.count;
             ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];

            for (size_t case_index = 0U;
                 case_index < group->cases.count;
                 ++case_index) {
                const TestCase *scaled_case = &group->cases.data[case_index];
                const TestCase *baseline_case;
                const AggregatedSample *baseline_sample;
                const AggregatedSample *scaled_sample;
                ScalingMetrics metrics;

                baseline_case = find_baseline_case(group, scaled_case);
                if (baseline_case == NULL ||
                    scaled_case->num_threads == baseline_case->num_threads) {
                    continue;
                }

                scaled_sample = find_aggregated_sample(
                    aggregated,
                    aggregated_count,
                    set->uid,
                    scaled_case->uid,
                    group->kernel
                );
                baseline_sample = find_aggregated_sample(
                    aggregated,
                    aggregated_count,
                    set->uid,
                    baseline_case->uid,
                    group->kernel
                );

                if (scaled_sample == NULL || baseline_sample == NULL ||
                    !analyzer_compute_scaling(
                        baseline_sample,
                        baseline_case,
                        scaled_sample,
                        scaled_case,
                        &metrics)) {
                    continue;
                }

                if (fprintf(
                        file,
                        "strong,%llu,%s,%llu,%llu,%s,%llu,%u,%u,"
                        "%.17g,%.17g,%.17g,%.17g,%.17g\n",
                        (unsigned long long) set->uid,
                        view_type_to_string(set->view.type),
                        (unsigned long long) set->view.window.offset,
                        (unsigned long long) set->view.window.size,
                        kernel_type_to_string(group->kernel),
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

    return ferror(file) ? -1 : 0;
}


static int find_weak_baseline(
    const TestPlan *plan,
    size_t scaled_set_index,
    const KernelTestGroup *scaled_group,
    const TestCase *scaled_case,
    size_t *baseline_set_index,
    const TestCase **baseline_case
)
{
    uint32_t best_threads = UINT32_MAX;

    if (plan == NULL ||
        scaled_group == NULL ||
        scaled_case == NULL ||
        baseline_set_index == NULL ||
        baseline_case == NULL) {
        return 0;
    }

    *baseline_case = NULL;

    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];

        if (set_index == scaled_set_index ||
            set->view.type != plan->data[scaled_set_index].view.type) {
            continue;
        }

        for (size_t group_index = 0U;
             group_index < set->kernels.count;
             ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];

            if (group->kernel != scaled_group->kernel) {
                continue;
            }

            for (size_t case_index = 0U;
                 case_index < group->cases.count;
                 ++case_index) {
                const TestCase *candidate = &group->cases.data[case_index];

                if (candidate->chunk_size == scaled_case->chunk_size &&
                    candidate->num_threads < scaled_case->num_threads &&
                    candidate->num_threads < best_threads) {
                    best_threads = candidate->num_threads;
                    *baseline_set_index = set_index;
                    *baseline_case = candidate;
                }
            }
        }
    }

    return *baseline_case != NULL;
}


static int write_scaling_weak(
    FILE *file,
    const TestPlan *plan,
    const AggregatedSample *aggregated,
    size_t aggregated_count
)
{
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

        for (size_t group_index = 0U;
             group_index < scaled_set->kernels.count;
             ++group_index) {
            const KernelTestGroup *scaled_group =
                &scaled_set->kernels.data[group_index];

            for (size_t case_index = 0U;
                 case_index < scaled_group->cases.count;
                 ++case_index) {
                const TestCase *scaled_case =
                    &scaled_group->cases.data[case_index];
                const TestCase *baseline_case;
                const AggregatedSample *baseline_sample;
                const AggregatedSample *scaled_sample;
                size_t baseline_set_index = 0U;
                const TestSet *baseline_set;
                ScalingMetrics metrics;

                if (!find_weak_baseline(
                        plan,
                        set_index,
                        scaled_group,
                        scaled_case,
                        &baseline_set_index,
                        &baseline_case)) {
                    continue;
                }

                baseline_set = &plan->data[baseline_set_index];
                scaled_sample = find_aggregated_sample(
                    aggregated,
                    aggregated_count,
                    scaled_set->uid,
                    scaled_case->uid,
                    scaled_group->kernel
                );
                baseline_sample = find_aggregated_sample(
                    aggregated,
                    aggregated_count,
                    baseline_set->uid,
                    baseline_case->uid,
                    scaled_group->kernel
                );

                if (scaled_sample == NULL || baseline_sample == NULL ||
                    !analyzer_compute_weak_scaling(
                        baseline_sample,
                        baseline_case,
                        baseline_set->view.window.size,
                        scaled_sample,
                        scaled_case,
                        scaled_set->view.window.size,
                        &metrics)) {
                    continue;
                }

                if (fprintf(
                        file,
                        "weak,%llu,%llu,%s,%llu,%llu,%s,%llu,%u,%u,"
                        "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
                        (unsigned long long) baseline_set->uid,
                        (unsigned long long) scaled_set->uid,
                        view_type_to_string(scaled_set->view.type),
                        (unsigned long long) baseline_set->view.window.size,
                        (unsigned long long) scaled_set->view.window.size,
                        kernel_type_to_string(scaled_group->kernel),
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

    return ferror(file) ? -1 : 0;
}


int write_benchmark_results(
    FileManager *manager,
    const TestPlan *plan,
    const RawSampleSet *samples,
    const AggregatedSample *aggregated,
    size_t aggregated_count,
    ScalingMode scaling_mode
) {
    FILE *plan_file = NULL;
    FILE *raw_file = NULL;
    FILE *aggregated_file = NULL;
    FILE *scaling_file = NULL;
    int result = -1;

    if (manager == NULL ||
        plan == NULL ||
        samples == NULL ||
        (aggregated_count > 0U && aggregated == NULL)) {
        return -1;
    }

    plan_file = open_csv_file(manager, "test_plan");
    raw_file = open_csv_file(manager, "raw_samples");
    aggregated_file = open_csv_file(manager, "aggregated_samples");
    scaling_file = open_csv_file(manager, "scaling");

    if (plan_file != NULL &&
        raw_file != NULL &&
        aggregated_file != NULL &&
        scaling_file != NULL &&
        write_test_plan(plan_file, plan) == 0 &&
        write_raw_samples(raw_file, samples) == 0 &&
        write_aggregated_samples(
            aggregated_file,
            aggregated,
            aggregated_count) == 0 &&
        ((scaling_mode == SCALING_STRONG &&
          write_scaling_strong(
              scaling_file, plan, aggregated, aggregated_count) == 0) ||
         (scaling_mode == SCALING_WEAK &&
          write_scaling_weak(
              scaling_file, plan, aggregated, aggregated_count) == 0))) {
        result = 0;
    }

    if (plan_file != NULL && fclose(plan_file) != 0) {
        result = -1;
    }
    if (raw_file != NULL && fclose(raw_file) != 0) {
        result = -1;
    }
    if (aggregated_file != NULL && fclose(aggregated_file) != 0) {
        result = -1;
    }
    if (scaling_file != NULL && fclose(scaling_file) != 0) {
        result = -1;
    }

    return result;
}