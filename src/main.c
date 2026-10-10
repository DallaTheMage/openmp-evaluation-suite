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
 * @file main.c
 * @brief Command-line entry point for the benchmark suite.
 */

#include <stdio.h>
#include <stdlib.h>

#include "config/params.h"
#include "core/Analyzer.h"
#include "core/Configuration.h"
#include "core/Dispatcher.h"
#include "core/FileManager.h"
#include "core/ResultsWriter.h"
#include "core/TestPlan.h"
#include "data/DataBuffer.h"
#include "data/Generator.h"
#include "profiling/Profiler.h"


/**
 * @brief Execute one benchmark suite and write its results.
 *
 * The function owns all per-suite execution state. This is important when
 * more than one suite is enabled: the profiler, samples, aggregated results
 * and output directory are reset between full-scale and proportional runs.
 *
 * @param buffer Shared initialized benchmark data buffer.
 * @param config Runtime benchmark configuration.
 * @param test_name Output-directory name and logical suite name.
 * @param scaling_mode Scaling metric type written to scaling.csv.
 * @param proportional Non-zero to generate the proportional test plan.
 *
 * @return 0 on success, non-zero on failure.
 */
static int run_benchmark_suite( DataBuffer *buffer, const Configuration *config, const char *test_name, ScalingMode scaling_mode, int proportional) {
    TestPlan *plan = NULL;
    FileManager *file_manager = NULL;
    Profiler profiler = {0};
    RawSampleSet samples = {0};
    AggregatedSample *aggregated = NULL;
    size_t aggregated_count = 0U;
    int profiler_initialized = 0;
    int result = -1;

    printf("[Main] Starting suite: %s\n", test_name);
    if (proportional) {
        plan = generate_proportional_test_plan(buffer, config);
    } else {
        plan = generate_full_scale_test_plan(buffer, config);
    }
    if (plan == NULL || !validate_test_plan(plan)) {
        fprintf(stderr, "[Error] Unable to create valid %s test plan\n", test_name);
    } else {

        printf("[Main] %s test plan ready: %zu test sets\n", test_name, plan->count);
        file_manager = create_file_manager("./output", config->build.compiler_name, CHOSEN_SCHEDULE_NAME);
        if (file_manager == NULL || set_current_test(file_manager, test_name) != 0) {
            fprintf(stderr,"[Error] Unable to initialize output directory for %s\n",test_name);
        } else {

            printf("[Main] Output directory: ./output/%s/%s/%s\n", config->build.compiler_name, CHOSEN_SCHEDULE_NAME, test_name);
            printf("[Main] Initializing profiler for %s\n", test_name);
            if (profiler_init(&profiler) != 0) {
                fprintf(stderr,"[Error] Unable to initialize profiler for %s\n",test_name);
            } else {
                profiler_initialized = 1;

                printf("[Main] Dispatching %s suite\n", test_name);
                if (dispatch_test_plan(plan, &profiler, config->benchmark.warmup_reps, config->benchmark.work_reps, &samples) != DISPATCH_OK) {
                    fprintf(stderr,"[Error] Benchmark dispatch failed for %s\n",test_name);
                } else {

                    printf("[Main] Dispatch completed for %s\n",test_name);
                    aggregated = analyzer_aggregate_samples(samples.samples, samples.count, &aggregated_count);
                    if (aggregated == NULL) {
                        fprintf(stderr, "[Error] Benchmark analysis failed for %s\n", test_name);
                    } else {

                        printf("[Main] %s: %zu raw samples, " "%zu aggregated experiments\n", test_name, samples.count, aggregated_count);
                        if (write_benchmark_results(file_manager, plan, &samples, aggregated, aggregated_count, scaling_mode) != 0) {
                            fprintf(stderr, "[Error] Unable to write %s output files\n", test_name);
                        } else {

                            printf("[Main] %s results written successfully\n", test_name);
                            result = 0;
                        }
                    }
                }
            }
        }
    }
    free(aggregated);
    dispatch_destroy_samples(&samples);
    if (profiler_initialized) {
        profiler_cleanup(&profiler);
    }
    destroy_file_manager(file_manager);
    destroy_test_plan(plan);
    return result;
}

int main(void) {
    Configuration config = load_configuration();
    DataBuffer *buffer = NULL;
    int exit_code = EXIT_FAILURE;
    int full_scale_failed = 0;
    int proportional_failed = 0;
    printf("[Main] Configuration loaded\n");
    printf("[Main] Allocating data buffer (%llu elements)\n", (unsigned long long)config.derived.problem_size_elements);
    buffer = create_data_buffer((size_t)config.derived.problem_size_elements, config.memory.alignment);
    if (buffer == NULL) {
        fprintf(stderr, "[Error] Unable to allocate benchmark data buffer\n");
    } else {
        printf("[Main] Filling data buffer\n");
        generator_fill_pool(config.benchmark.generation_seed, buffer->pool, buffer->element_count);

    #if RUN_FULL_SCALE
        full_scale_failed = run_benchmark_suite(buffer, &config, "full_scale", SCALING_STRONG, 0) != 0;
    #endif

    #if RUN_PROPORTIONAL
        proportional_failed = run_benchmark_suite(buffer, &config, "proportional", SCALING_WEAK, 1) != 0;
    #endif
        if (!full_scale_failed && !proportional_failed) {
            exit_code = EXIT_SUCCESS;
        }
    }
    if (full_scale_failed) {
        fprintf(stderr, "[Error] Full-scale (Strong Scaling) suite failed\n");
    }
    if (proportional_failed) {
        fprintf(stderr, "[Error] Proportional (Weak Scaling) suite failed\n");
    }
    destroy_data_buffer(buffer);
    destroy_configuration(&config);
    return exit_code;
}