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


int main(void)
{
    Configuration config = load_configuration();

    DataBuffer *buffer = NULL;
    TestPlan *plan = NULL;
    FileManager *file_manager = NULL;

    Profiler profiler = {0};
    RawSampleSet samples = {0};

    size_t aggregated_count = 0U;
    AggregatedSample *aggregated = NULL;

    int profiler_initialized = 0;
    int exit_code = EXIT_FAILURE;

    printf("[benchmark] configuration loaded\n");

    printf(
        "[benchmark] allocating data buffer (%llu elements)\n",
        (unsigned long long) config.derived.problem_size_elements
    );

    buffer = create_data_buffer(
        (size_t) config.derived.problem_size_elements,
        config.memory.alignment
    );

    if (buffer != NULL) {
        printf("[benchmark] filling data buffer\n");

        generator_fill_pool(
            config.benchmark.generation_seed,
            buffer->pool,
            buffer->element_count
        );

        printf("[benchmark] generating test plan\n");

        plan = generate_full_scale_test_plan(
            buffer,
            &config
        );
    }

    if (buffer == NULL ||
        plan == NULL ||
        !validate_test_plan(plan)) {
        fprintf(
            stderr,
            "error: unable to create benchmark input or test plan\n"
        );
    } else {
        printf(
            "[benchmark] test plan ready: %zu test sets\n",
            plan->count
        );

        file_manager = create_file_manager(
            "./output",
            config.build.compiler_name,
            CHOSEN_SCHEDULE_NAME
        );

        if (file_manager == NULL ||
            set_current_test(
                file_manager,
                "full_scale"
            ) != 0) {
            fprintf(
                stderr,
                "error: unable to initialize output directory\n"
            );
        } else {
            printf(
                "[benchmark] output directory: "
                "./output/%s/%s/full_scale\n",
                config.build.compiler_name,
                CHOSEN_SCHEDULE_NAME
            );

            printf("[benchmark] initializing profiler\n");

            if (profiler_init(&profiler) != 0) {
                fprintf(
                    stderr,
                    "error: unable to initialize profiler\n"
                );
            } else {
                profiler_initialized = 1;

                printf("[benchmark] starting dispatch\n");

                if (dispatch_test_plan(
                        plan,
                        &profiler,
                        config.benchmark.warmup_reps,
                        config.benchmark.work_reps,
                        &samples) != DISPATCH_OK) {
                    fprintf(
                        stderr,
                        "error: benchmark dispatch failed\n"
                    );
                } else {
                    printf("[benchmark] dispatch completed\n");
                    printf("[benchmark] aggregating results\n");

                    aggregated = analyzer_aggregate_samples(
                        samples.samples,
                        samples.count,
                        &aggregated_count
                    );

                    if (aggregated == NULL) {
                        fprintf(
                            stderr,
                            "error: benchmark analysis failed\n"
                        );
                    } else {
                        printf(
                            "[benchmark] completed: "
                            "%zu raw samples, "
                            "%zu aggregated experiments\n",
                            samples.count,
                            aggregated_count
                        );

                        if (write_benchmark_results(
                                file_manager,
                                plan,
                                &samples,
                                aggregated,
                                aggregated_count) != 0) {
                            fprintf(
                                stderr,
                                "error: unable to write benchmark "
                                "output files\n"
                            );
                        } else {
                            printf(
                                "[benchmark] results written successfully\n"
                            );

                            exit_code = EXIT_SUCCESS;
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
    destroy_data_buffer(buffer);
    destroy_configuration(&config);

    return exit_code;
}