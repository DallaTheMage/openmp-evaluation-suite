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
#include <stdint.h>
#include <stdlib.h>

#include "config/params.h"
#include "core/Analyzer.h"
#include "core/Configuration.h"
#include "core/Dispatcher.h"
#include "core/FileManager.h"
#include "core/ResultsWriter.h"
#include "core/TestPlan.h"
#include "data/MemoryManager.h"
#include "profiling/Profiler.h"

/**
 * @brief Execute one benchmark suite and write its results.
 *
 * The function owns all per-suite execution state. This is important when
 * more than one suite is enabled: the profiler, samples, aggregated results
 * and output directory are reset between full-scale and proportional runs.
 *
 * The test plan is generated using the original data buffer. After the
 * maximum workload is determined, its views are rebound to the reusable
 * working buffer owned by the memory manager.
 *
 * @param memory_manager Owner of the original and working data buffers.
 * @param config Runtime benchmark configuration.
 * @param test_name Output-directory name and logical suite name.
 * @param scaling_mode Scaling metric type written to scaling.csv.
 * @param proportional Non-zero to generate the proportional test plan.
 *
 * @return 0 on success, non-zero on failure.
 */
static int run_benchmark_suite(
    MemoryManager *memory_manager,
    const Configuration *config,
    const char *test_name,
    ScalingMode scaling_mode,
    int proportional)
{
    DataBuffer *original_buffer;
    DataBuffer *work_buffer;
    TestPlan *plan = NULL;
    FileManager *file_manager = NULL;
    Profiler profiler = {0};
    RawSampleSet samples = {0};
    AggregatedSample *aggregated = NULL;
    size_t aggregated_count = 0U;
    size_t max_workload = 0U;
    int profiler_initialized = 0;
    int result = -1;

    if (memory_manager == NULL ||
        config == NULL ||
        test_name == NULL) {
        fprintf(
            stderr,
            "[Error] Invalid arguments for benchmark suite\n"
        );
        return -1;
    }

    original_buffer =
        memory_manager_original_buffer(memory_manager);

    if (original_buffer == NULL) {
        fprintf(
            stderr,
            "[Error] Original data buffer is unavailable\n"
        );
        return -1;
    }

    printf("[Main] Starting suite: %s\n", test_name);

    /*
     * Generate the test plan using the immutable original buffer.
     * The views will be rebound to the working buffer after allocation.
     */
    if (proportional) {
        plan = generate_proportional_test_plan(
            original_buffer,
            config
        );
    } else {
        plan = generate_full_scale_test_plan(
            original_buffer,
            config
        );
    }

    if (plan == NULL || !validate_test_plan(plan)) {
        fprintf(
            stderr,
            "[Error] Unable to create valid %s test plan\n",
            test_name
        );
        goto cleanup;
    }

    /*
     * Determine the maximum workload before allocating working storage.
     * The memory manager retains its capacity between benchmark suites.
     */
    if (!test_plan_max_workload(plan, &max_workload) ||
        memory_manager_reserve_work(
            memory_manager,
            max_workload
        ) != 0) {
        fprintf(
            stderr,
            "[Error] Unable to reserve working memory for %s\n",
            test_name
        );
        goto cleanup;
    }

    work_buffer = memory_manager_work_buffer(memory_manager);

    /*
     * Rebind every view to the working buffer, then validate the plan again.
     */
    if (work_buffer == NULL ||
        !test_plan_rebind_buffer(plan, work_buffer) ||
        !validate_test_plan(plan)) {
        fprintf(
            stderr,
            "[Error] Unable to bind working memory for %s\n",
            test_name
        );
        goto cleanup;
    }

    printf(
        "[Main] %s test plan ready: %zu test sets; "
        "work capacity: %zu elements\n",
        test_name,
        plan->count,
        work_buffer->element_count
    );

    file_manager = create_file_manager(
        "./output",
        config->build.compiler_name,
        CHOSEN_SCHEDULE_NAME
    );

    if (file_manager == NULL ||
        set_current_test(file_manager, test_name) != 0) {
        fprintf(
            stderr,
            "[Error] Unable to initialize output directory for %s\n",
            test_name
        );
        goto cleanup;
    }

    printf(
        "[Main] Output directory: ./output/%s/%s/%s\n",
        config->build.compiler_name,
        CHOSEN_SCHEDULE_NAME,
        test_name
    );

    printf("[Main] Initializing profiler for %s\n", test_name);

    if (profiler_init(&profiler) != 0) {
        fprintf(
            stderr,
            "[Error] Unable to initialize profiler for %s\n",
            test_name
        );
        goto cleanup;
    }

    profiler_initialized = 1;

    printf("[Main] Dispatching %s suite\n", test_name);

    if (dispatch_test_plan(
            plan,
            memory_manager,
            &profiler,
            config->benchmark.warmup_reps,
            config->benchmark.work_reps,
            &samples) != DISPATCH_OK) {
        fprintf(
            stderr,
            "[Error] Benchmark dispatch failed for %s\n",
            test_name
        );
        goto cleanup;
    }

    printf("[Main] Dispatch completed for %s\n", test_name);

    aggregated = analyzer_aggregate_samples(
        samples.samples,
        samples.count,
        &aggregated_count
    );

    if (aggregated == NULL) {
        fprintf(
            stderr,
            "[Error] Benchmark analysis failed for %s\n",
            test_name
        );
        goto cleanup;
    }

    printf(
        "[Main] %s: %zu raw samples, "
        "%zu aggregated experiments\n",
        test_name,
        samples.count,
        aggregated_count
    );

    if (write_benchmark_results(
            file_manager,
            plan,
            &samples,
            aggregated,
            aggregated_count,
            scaling_mode) != 0) {
        fprintf(
            stderr,
            "[Error] Unable to write %s output files\n",
            test_name
        );
        goto cleanup;
    }

    printf(
        "[Main] %s results written successfully\n",
        test_name
    );

    result = 0;

cleanup:
    free(aggregated);
    dispatch_destroy_samples(&samples);

    if (profiler_initialized) {
        profiler_cleanup(&profiler);
    }

    destroy_file_manager(file_manager);
    destroy_test_plan(plan);

    return result;
}


int main(void)
{
    Configuration config = load_configuration();
    MemoryManager *memory_manager = NULL;

    int exit_code = EXIT_FAILURE;
    int full_scale_failed = 0;
    int proportional_failed = 0;

    printf("[Main] Configuration loaded\n");

    printf(
        "[Main] Allocating and initializing original data buffer "
        "(%llu elements)\n",
        (unsigned long long)config.derived.problem_size_elements
    );

    /*
     * Ensure that the configured dataset size is representable by size_t
     * before passing it to the memory manager.
     */
    if (config.derived.problem_size_elements > (uint64_t)SIZE_MAX) {
        fprintf(
            stderr,
            "[Error] Configured dataset exceeds addressable memory\n"
        );
        goto cleanup;
    }

    /*
     * The manager allocates and initializes the original buffer once.
     * The working buffer is allocated later, after each test plan has
     * determined its maximum required workload.
     */
    memory_manager = memory_manager_create(
        (size_t)config.derived.problem_size_elements,
        config.memory.alignment,
        config.benchmark.generation_seed
    );

    if (memory_manager == NULL) {
        fprintf(
            stderr,
            "[Error] Unable to initialize memory manager\n"
        );
        goto cleanup;
    }

#if RUN_FULL_SCALE
    full_scale_failed = run_benchmark_suite(
        memory_manager,
        &config,
        "full_scale",
        SCALING_STRONG,
        0
    ) != 0;
#endif

#if RUN_PROPORTIONAL
    proportional_failed = run_benchmark_suite(
        memory_manager,
        &config,
        "proportional",
        SCALING_WEAK,
        1
    ) != 0;
#endif

    if (!full_scale_failed && !proportional_failed) {
        exit_code = EXIT_SUCCESS;
    }

cleanup:
    if (full_scale_failed) {
        fprintf(
            stderr,
            "[Error] Full-scale (Strong Scaling) suite failed\n"
        );
    }

    if (proportional_failed) {
        fprintf(
            stderr,
            "[Error] Proportional (Weak Scaling) suite failed\n"
        );
    }

    memory_manager_destroy(memory_manager);
    destroy_configuration(&config);

    return exit_code;
}