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
 * @brief Command-line entry point and top-level benchmark orchestration.
 */

#include <stdint.h>
#include <stdio.h>
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
 * This function owns all resources whose lifetime is limited to one suite.
 * The shared MemoryManager is borrowed from the caller and is never destroyed
 * here. All resources acquired by this function are released at @c cleanup,
 * regardless of which operation fails.
 *
 * @param memory_manager Shared manager containing original and working data.
 * @param config Validated benchmark configuration.
 * @param test_name Logical suite name and output-directory component.
 * @param scaling_mode Scaling mode recorded in the output.
 * @param proportional Non-zero selects the proportional test plan.
 *
 * @return 0 on success; non-zero if setup, execution, analysis, or output
 *         generation fails.
 */
static int run_benchmark_suite(
    MemoryManager *memory_manager,
    const Configuration *config,
    const char *test_name,
    ScalingMode scaling_mode,
    int proportional)
{
    DataBuffer *original_buffer = NULL;
    DataBuffer *work_buffer = NULL;
    TestPlan *plan = NULL;
    FileManager *file_manager = NULL;

    Profiler profiler = {0};
    RawSampleSet samples = {0};
    AggregatedSample *aggregated = NULL;

    size_t aggregated_count = 0U;
    size_t max_workload = 0U;

    int profiler_initialized = 0;
    int result = EXIT_FAILURE;

    if (memory_manager == NULL ||
        config == NULL ||
        test_name == NULL) {
        fprintf(
            stderr,
            "[Error] Invalid arguments for benchmark suite\n"
        );
        goto cleanup;
    }

    printf("[Main] Starting suite: %s\n", test_name);

    original_buffer =
        memory_manager_original_buffer(memory_manager);

    if (original_buffer == NULL) {
        fprintf(
            stderr,
            "[Error] Original data buffer is unavailable\n"
        );
        goto cleanup;
    }

    /*
     * Build the test plan against the immutable original data buffer.
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
     * Reserve enough working memory once, before dispatching any kernel.
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

    /*
     * Create and select the output directory for this suite.
     */
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

    /*
     * Initialize the profiler before dispatch.
     * Kernels borrow this instance and do not own it.
     */
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

    /*
     * Execute warm-ups and measured repetitions.
     */
    printf("[Main] Dispatching %s suite\n", test_name);

    {
        const DispatchStatus dispatch_status =
            dispatch_test_plan(
                plan,
                memory_manager,
                &profiler,
                config->benchmark.warmup_reps,
                config->benchmark.work_reps,
                &samples
            );

        if (dispatch_status != DISPATCH_OK) {
            fprintf(
                stderr,
                "[Error] Benchmark dispatch failed for %s "
                "(status=%d)\n",
                test_name,
                (int)dispatch_status
            );
            goto cleanup;
        }
    }

    printf("[Main] Dispatch completed for %s\n", test_name);

    /*
     * Aggregate only the complete sample set returned by a
     * successful dispatch.
     */
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

    /*
     * Persist the raw and aggregated benchmark results.
     */
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

    result = EXIT_SUCCESS;

cleanup:
    /*
     * Release resources in reverse acquisition order.
     */
    free(aggregated);
    dispatch_destroy_samples(&samples);

    if (profiler_initialized) {
        profiler_cleanup(&profiler);
    }

    destroy_file_manager(file_manager);
    destroy_test_plan(plan);

    return result;
}

/**
 * @brief Run the benchmark suites enabled by the build configuration.
 *
 * The MemoryManager is created once and shared by all enabled suites.
 * This permits reuse of its original buffer and working-buffer capacity.
 * A failure in one suite does not prevent the other enabled suite from
 * being attempted.
 *
 * @param config Loaded benchmark configuration.
 *
 * @return EXIT_SUCCESS if every enabled suite succeeds;
 *         EXIT_FAILURE otherwise.
 */
static int run_application(const Configuration *config)
{
    MemoryManager *memory_manager = NULL;

    int full_scale_failed = 0;
    int proportional_failed = 0;
    int exit_code = EXIT_FAILURE;

    if (config == NULL) {
        fprintf(
            stderr,
            "[Error] Invalid configuration\n"
        );
        goto cleanup;
    }

    /*
     * Ensure the configured dataset fits the address space.
     */
    if (config->derived.problem_size_elements == 0U ||
        config->derived.problem_size_elements > (uint64_t)SIZE_MAX) {
        fprintf(
            stderr,
            "[Error] Configured dataset size is invalid "
            "or not addressable\n"
        );
        goto cleanup;
    }

    printf(
        "[Main] Allocating and initializing original data buffer "
        "(%llu elements)\n",
        (unsigned long long)config->derived.problem_size_elements
    );

    /*
     * The manager owns the original buffer and the reusable working buffer.
     */
    memory_manager = memory_manager_create(
        (size_t)config->derived.problem_size_elements,
        config->memory.alignment,
        config->benchmark.generation_seed
    );

    if (memory_manager == NULL) {
        fprintf(
            stderr,
            "[Error] Unable to initialize memory manager\n"
        );
        goto cleanup;
    }

    /*
     * Suites are independent: try every enabled suite even if
     * an earlier suite fails.
     */
#if RUN_FULL_SCALE
    full_scale_failed = (
        run_benchmark_suite(
            memory_manager,
            config,
            "full_scale",
            SCALING_STRONG,
            0
        ) != 0
    );
#endif

#if RUN_PROPORTIONAL
    proportional_failed = (
        run_benchmark_suite(
            memory_manager,
            config,
            "proportional",
            SCALING_WEAK,
            1
        ) != 0
    );
#endif

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

    /*
     * Success requires every enabled suite to have completed successfully.
     */
    if (!full_scale_failed && !proportional_failed) {
        exit_code = EXIT_SUCCESS;
    }

cleanup:
    /*
     * The manager is application-owned and is destroyed exactly once.
     * memory_manager_destroy() must accept NULL.
     */
    memory_manager_destroy(memory_manager);

    return exit_code;
}

/**
 * @brief Application entry point.
 *
 * @return EXIT_SUCCESS on success; EXIT_FAILURE on any fatal suite error.
 */
int main(void)
{
    Configuration config = load_configuration();
    int exit_code;

    printf("[Main] Configuration loaded\n");

    exit_code = run_application(&config);

    /*
     * Configuration owns dynamically allocated lists and is released here.
     */
    destroy_configuration(&config);

    return exit_code;
}