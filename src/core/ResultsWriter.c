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

/**
 * @file ResultsWriter.c
 * @brief CSV serialization of benchmark plans and measurements.
 */

#include "core/ResultsWriter.h"

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


static int write_metrics(
    FILE *file,
    const Metrics *metrics
)
{
    if (fprintf(
            file,
            "%.17g,%.17g,%.17g,%.17g",
            metrics->min,
            metrics->max,
            metrics->mean,
            metrics->variance) < 0) {
        return -1;
    }

    return 0;
}


static int write_test_plan(
    FILE *file,
    const TestPlan *plan
)
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


static int write_raw_samples(
    FILE *file,
    const RawSampleSet *samples
)
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
                "%llu,%llu,%s,%u,%.17g,%llu,%llu,%llu,%llu,"
                "%.17g,%.17g,%.17g,%.17g,%.17g\n",
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
            "wall_time_sec_variance,"
            "cycles_min,cycles_max,cycles_mean,cycles_variance,"
            "instructions_min,instructions_max,instructions_mean,"
            "instructions_variance,"
            "l1d_misses_min,l1d_misses_max,l1d_misses_mean,"
            "l1d_misses_variance,"
            "llc_misses_min,llc_misses_max,llc_misses_mean,"
            "llc_misses_variance,"
            "ipc_min,ipc_max,ipc_mean,ipc_variance,"
            "l1d_miss_ratio_min,l1d_miss_ratio_max,l1d_miss_ratio_mean,"
            "l1d_miss_ratio_variance,"
            "llc_miss_ratio_min,llc_miss_ratio_max,llc_miss_ratio_mean,"
            "llc_miss_ratio_variance,"
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

        if (write_metrics(file, &sample->wall_time_sec) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->cycles) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->instructions) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->l1d_misses) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->llc_misses) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->ipc) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->l1d_miss_ratio) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->llc_miss_ratio) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->energy_pkg_joules) != 0 ||
            fputc(',', file) == EOF ||
            write_metrics(file, &sample->energy_dram_joules) != 0 ||
            fputc('\n', file) == EOF) {
            return -1;
        }
    }

    return ferror(file) ? -1 : 0;
}


int write_benchmark_results(
    FileManager *manager,
    const TestPlan *plan,
    const RawSampleSet *samples,
    const AggregatedSample *aggregated,
    size_t aggregated_count
)
{
    FILE *plan_file = NULL;
    FILE *raw_file = NULL;
    FILE *aggregated_file = NULL;
    int result = -1;

    if (manager == NULL ||
        plan == NULL ||
        samples == NULL ||
        (aggregated_count > 0U && aggregated == NULL)) {
        return -1;
    }

    plan_file = open_csv_file(manager, "test_plan");

    if (plan_file == NULL) {
        goto cleanup;
    }

    raw_file = open_csv_file(manager, "raw_samples");

    if (raw_file == NULL) {
        goto cleanup;
    }

    aggregated_file = open_csv_file(manager, "aggregated_samples");

    if (aggregated_file == NULL) {
        goto cleanup;
    }

    if (write_test_plan(plan_file, plan) != 0 ||
        write_raw_samples(raw_file, samples) != 0 ||
        write_aggregated_samples(
            aggregated_file,
            aggregated,
            aggregated_count) != 0) {
        goto cleanup;
    }

    result = 0;

cleanup:
    if (plan_file != NULL) {
        if (fclose(plan_file) != 0) {
            result = -1;
        }
    }

    if (raw_file != NULL) {
        if (fclose(raw_file) != 0) {
            result = -1;
        }
    }

    if (aggregated_file != NULL) {
        if (fclose(aggregated_file) != 0) {
            result = -1;
        }
    }

    return result;
}