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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https\://www.gnu.org/licenses/>.
 */
/**
 * @file ResultsWriter.c
 * @brief CSV serialization of benchmark plans and measurements.
 */
#include "core/ResultsWriter.h"
#include "core/internal/ScalingWriter.h"
#include "utils/Names.h"
#include <stdint.h>
#include <stdio.h>
#include "data/views/ViewType.h"
#include "config/params.h"

static int write_metric_values(FILE *file, const Metrics *metric) {
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

static int write_test_plan(FILE *file, const TestPlan *plan) {
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
                        utils_view_type_name(set->view.type),
                        (unsigned long long) set->view.window.offset,
                        (unsigned long long) set->view.window.size,
                        utils_kernel_type_name(group->kernel),
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

static int write_raw_samples(FILE *file, const RawSampleSet *samples) {
    if (fprintf(
            file,
            "test_set_id,test_case_id,kernel,run_id,wall_time_sec,"
            "cycles,instructions,l1d_misses,llc_misses,ipc,"
            "l1d_miss_ratio_per_instruction,llc_miss_ratio_per_instruction,energy_pkg_joules,"
            "energy_dram_joules,hw_counters_valid,energy_pkg_valid,energy_dram_valid\n") < 0) {
        return -1;
    }
    for (size_t i = 0U; i < samples->count; ++i) {
        const RawSample *sample = &samples->samples[i];
        const PerformanceMetric *metric = &sample->metric;
        if (fprintf(
                file,
                "%llu,%llu,%s,%u,%.17g,%llu,%llu,%llu,%llu,%.17g,"
                "%.17g,%.17g,%.17g,%.17g,%d,%d,%d\n",
                (unsigned long long) sample->test_set_id,
                (unsigned long long) sample->test_case_id,
                utils_kernel_type_name(sample->kernel),
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
                metric->energy_dram_joules,
                metric->hw_counters_valid ? 1 : 0,
                metric->energy_pkg_valid ? 1 : 0,
                metric->energy_dram_valid ? 1 : 0) < 0) {
            return -1;
        }
    }
    return ferror(file) ? -1 : 0;
}

static int write_aggregated_samples(FILE *file, const AggregatedSample *samples, size_t sample_count) {
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
                utils_kernel_type_name(sample->kernel),
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

/* RFC 4180 quoting for values containing delimiters, quotes or newlines. */
static int write_metadata_field(FILE *file, const char *value) {
    const unsigned char *cursor = (const unsigned char *)(value != NULL ? value : "");
    if (fputc('"', file) == EOF) return -1;
    while (*cursor != '\0') {
        if (*cursor == '"' && fputc('"', file) == EOF) return -1;
        if (fputc((int)*cursor, file) == EOF) return -1;
        ++cursor;
    }
    return fputc('"', file) == EOF ? -1 : 0;
}

static int write_metadata_pair(FILE *file, const char *key, const char *value) {
    if (write_metadata_field(file, key) != 0 ||
        fputc(',', file) == EOF ||
        write_metadata_field(file, value) != 0 ||
        fputc('\n', file) == EOF) return -1;
    return 0;
}

static int write_metadata_u64(FILE *file, const char *key, uint64_t value) {
    char buffer[32];
    if (snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value) < 0) return -1;
    return write_metadata_pair(file, key, buffer);
}

static int write_metadata(FILE *file, const Configuration *config) {
    if (fprintf(file, "key,value\n") < 0 ||
        write_metadata_pair(file, "compiler", config->build.compiler_name) != 0 ||
        write_metadata_pair(file, "compiler_family", config->build.compiler_family) != 0 ||
        write_metadata_pair(file, "compiler_version", config->build.compiler_version) != 0 ||
        write_metadata_pair(file, "compiler_flags", config->build.compiler_flags) != 0 ||
        write_metadata_pair(file, "openmp_version", config->build.openmp_version) != 0 ||
        write_metadata_pair(file, "schedule", CHOSEN_SCHEDULE_NAME) != 0 ||
        write_metadata_pair(file, "hw_counters_enabled", ENABLE_HW_COUNTERS ? "true" : "false") != 0 ||
        write_metadata_u64(file, "warmup_reps", config->benchmark.warmup_reps) != 0 ||
        write_metadata_u64(file, "work_reps", config->benchmark.work_reps) != 0 ||
        write_metadata_u64(file, "generation_seed", config->benchmark.generation_seed) != 0 ||
        write_metadata_u64(file, "slowdown_factor", config->benchmark.slowdown_factor) != 0 ||
        write_metadata_u64(file, "problem_size_elements", config->derived.problem_size_elements) != 0 ||
        write_metadata_u64(file, "size_per_thread", config->derived.size_per_thread) != 0 ||
        write_metadata_u64(file, "view_2d_cols", config->shapes[VIEW_2D].v2d.cols) != 0) return -1;
    return ferror(file) ? -1 : 0;
}

int write_benchmark_results(
    FileManager *manager,
    const TestPlan *plan,
    const RawSampleSet *samples,
    const AggregatedSample *aggregated,
    size_t aggregated_count,
    const Configuration *config,
    ScalingMode scaling_mode) {
    FILE *plan_file = NULL;
    FILE *raw_file = NULL;
    FILE *aggregated_file = NULL;
    FILE *scaling_file = NULL;
    FILE *metadata_file = NULL;
    int result = -1;
    if (manager == NULL ||
        plan == NULL ||
        samples == NULL ||
        config == NULL ||
        (aggregated_count > 0U && aggregated == NULL)) {
        return -1;
    }
    plan_file = open_csv_file(manager, "test_plan");
    raw_file = open_csv_file(manager, "raw_samples");
    aggregated_file = open_csv_file(manager, "aggregated_samples");
    scaling_file = open_csv_file(manager, "scaling");
    metadata_file = open_schedule_csv_file(manager, "metadata");
    if (plan_file != NULL &&
        raw_file != NULL &&
        aggregated_file != NULL &&
        scaling_file != NULL &&
        metadata_file != NULL &&
        write_metadata(metadata_file, config) == 0 &&
        write_test_plan(plan_file, plan) == 0 &&
        write_raw_samples(raw_file, samples) == 0 &&
        write_aggregated_samples(aggregated_file, aggregated, aggregated_count) == 0 &&
        ((scaling_mode == SCALING_STRONG &&
          write_scaling_strong(scaling_file, plan, aggregated, aggregated_count) == 0) ||
         (scaling_mode == SCALING_WEAK &&
          write_scaling_weak(scaling_file, plan, aggregated, aggregated_count) == 0))) {
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
    if (metadata_file != NULL && fclose(metadata_file) != 0) {
        result = -1;
    }
    return result;
}
