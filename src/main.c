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

#include "core/Analyzer.h"
#include "core/Configuration.h"
#include "core/Dispatcher.h"
#include "core/TestPlan.h"
#include "data/DataBuffer.h"
#include "data/Generator.h"
#include "profiling/Profiler.h"


int main(void)
{
    Configuration config = load_configuration();
    DataBuffer *buffer = NULL;
    TestPlan *plan = NULL;
    Profiler profiler;
    RawSampleSet samples = {0};
    size_t aggregated_count = 0U;
    AggregatedSample *aggregated = NULL;
    int exit_code = EXIT_FAILURE;
    int profiler_initialized = 0;

    printf("[Main] Configuration loaded successfully\n");

    printf(
        "[Main] Allocating data buffer (%llu elements)\n",
        (unsigned long long) config.derived.problem_size_elements
    );

    buffer = create_data_buffer(
        (size_t) config.derived.problem_size_elements,
        config.memory.alignment
    );

    if (buffer == NULL) {
        fprintf(stderr, "error: unable to allocate benchmark data buffer\n");
    } else {
        printf("[Main] Filling data buffer\n");

        generator_fill_pool(
            config.benchmark.generation_seed,
            buffer->pool,
            buffer->element_count
        );

        printf("[Main] Generating test plan\n");

        plan = generate_full_scale_test_plan(buffer, &config);

        if (plan == NULL || !validate_test_plan(plan)) {
            fprintf(stderr, "error: unable to create a valid test plan\n");
        } else {
            printf(
                "[Main] Test plan ready: %zu test sets\n",
                plan->count
            );

            printf("[Main] Initializing profiler\n");

            if (profiler_init(&profiler) != 0) {
                fprintf(stderr, "error: unable to initialize profiler\n");
            } else {
                profiler_initialized = 1;

                printf("[Main] Starting dispatch\n");

                if (dispatch_test_plan(
                        plan,
                        &profiler,
                        config.benchmark.warmup_reps,
                        config.benchmark.work_reps,
                        &samples) != DISPATCH_OK) {
                    fprintf(stderr, "error: benchmark dispatch failed\n");
                } else {
                    printf("[Main] Dispatch completed\n");
                    printf("[Main] Aggregating results\n");

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
                            "[Main] Completed: %zu raw samples, "
                            "%zu aggregated experiments\n",
                            samples.count,
                            aggregated_count
                        );

                        exit_code = EXIT_SUCCESS;
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

    destroy_test_plan(plan);
    destroy_data_buffer(buffer);
    destroy_configuration(&config);

    return exit_code;
}