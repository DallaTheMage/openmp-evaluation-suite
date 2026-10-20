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
#include "data/CacheEviction.h"
#include "core/internal/BenchmarkRunner.h"

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
int run_benchmark_suite(
    MemoryManager *memory_manager,
    const Configuration *config,
    const char *test_name,
    ScalingMode scaling_mode,
    int proportional) {
    DataBuffer *original_buffer = NULL;
    DataBuffer *work_buffer = NULL;
    TestPlan *plan = NULL;
    FileManager *file_manager = NULL;

    Profiler profiler = {0};
    RawSampleSet samples = {0};
    AggregatedSample *aggregated = NULL;

    /*
     * Cache eviction resources.
     *
     * The eviction buffer is owned by this benchmark suite
     * and released during cleanup.
     */
    CacheEviction  eviction        = {0};
    CacheEviction *active_eviction = NULL;

    size_t aggregated_count = 0U;
    size_t max_workload     = 0U;

    int profiler_initialized = 0;
    int result = EXIT_FAILURE;

    if (memory_manager == NULL
        || config == NULL
        || test_name == NULL) {
        fprintf(stderr, "[Error] Invalid arguments for benchmark suite\n");
        goto cleanup;
    }

    printf("[Main] Starting suite: %s\n", test_name);
    original_buffer = memory_manager_original_buffer(memory_manager);

    if (original_buffer == NULL) {
        fprintf(stderr, "[Error] Original data buffer is unavailable\n");
        goto cleanup;
    }

    /* Build the test plan against the immutable original data buffer. */
    if (proportional) {
        plan = generate_proportional_test_plan(original_buffer, config);
    } else {
        plan = generate_full_scale_test_plan(original_buffer, config);
    }

    if (plan == NULL || !validate_test_plan(plan)) {
        fprintf(stderr, "[Error] Unable to create valid %s test plan\n", test_name);
        goto cleanup;
    }

    /* Reserve enough working memory once, before dispatching any kernel. */
    if (!test_plan_max_workload(plan, &max_workload) ||
        memory_manager_reserve_work(memory_manager, max_workload) != 0) {
        fprintf(stderr, "[Error] Unable to reserve working memory for %s\n", test_name);
        goto cleanup;
    }
    work_buffer = memory_manager_work_buffer(memory_manager);
    if (work_buffer == NULL ||
        !test_plan_rebind_buffer(plan, work_buffer) ||
        !validate_test_plan(plan)) {
        fprintf(stderr, "[Error] Unable to bind working memory for %s\n", test_name);
        goto cleanup;
    }

    printf("[Main] %s test plan ready: %zu test sets; work capacity: %zu elements\n",
        test_name,
        plan->count,
        work_buffer->element_count
    );

    /* Create and select the output directory for this suite. */
    file_manager = create_file_manager("./output", config->build.compiler_name, CHOSEN_SCHEDULE_NAME);

    if (file_manager == NULL ||
        set_current_test(file_manager, test_name) != 0) {
        fprintf(stderr, "[Error] Unable to initialize output directory for %s\n", test_name);
        goto cleanup;
    }

    printf("[Main] Output directory: ./output/%s/%s/%s\n",
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
        fprintf(stderr, "[Error] Unable to initialize profiler for %s\n", test_name);
        goto cleanup;
    }

    profiler_initialized = 1;

    /*
     * Initialize cache eviction when requested by Configuration.
     *
     * Allocation and initialization happen outside the
     * measured benchmark interval.
     */
    if (config->memory.cache_eviction.enabled != 0U) {
        const CacheEvictionConfig *eviction_config = &config->memory.cache_eviction;
        if (cache_eviction_init(
                &eviction,
                (size_t)config->memory.detected_llc_size_bytes,
                (size_t)eviction_config->multiplier,
                eviction_config->stride_bytes
            ) != 0) {
            fprintf(stderr, "[Error] Unable to initialize cache eviction for %s\n", test_name);
            goto cleanup;
        }
        active_eviction = &eviction;
        printf(
            "[Main] Cache eviction enabled for %s: %zu bytes, stride=%zu bytes\n",
            test_name,
            eviction.size_bytes,
            eviction.stride_bytes
        );

    } else {
        printf("[Main] Cache eviction disabled for %s\n", test_name);
    }


    /* Execute warm-ups and measured repetitions. */
    printf("[Main] Dispatching %s suite\n", test_name);

    const DispatchStatus dispatch_status =
        dispatch_test_plan_with_eviction(
            plan,
            memory_manager,
            &profiler,
            config->benchmark.warmup_reps,
            config->benchmark.work_reps,
            &samples,
            active_eviction
        );

    if (dispatch_status != DISPATCH_OK) {
        fprintf(stderr, "[Error] Benchmark dispatch failed for %s (status=%d)\n",
            test_name,
            (int)dispatch_status
        );
        goto cleanup;
    }

    printf("[Main] Dispatch completed for %s\n", test_name);

    /*
     * Aggregate only the complete sample set returned by a
     * successful dispatch.
     */
    aggregated = analyzer_aggregate_samples(samples.samples, samples.count, &aggregated_count);

    if (aggregated == NULL) {
        fprintf(stderr, "[Error] Benchmark analysis failed for %s\n", test_name);
        goto cleanup;
    }

    printf("[Main] %s: %zu raw samples, %zu aggregated experiments\n",
        test_name,
        samples.count,
        aggregated_count
    );

    /* Persist the raw and aggregated benchmark results. */
    if (write_benchmark_results(
            file_manager,
            plan,
            &samples,
            aggregated,
            aggregated_count,
            config,
            scaling_mode) != 0) {
        fprintf(stderr, "[Error] Unable to write %s output files\n", test_name);
        goto cleanup;
    }
    printf("[Main] %s results written successfully\n", test_name);
    result = EXIT_SUCCESS;

cleanup:
    /*
     * Release resources in reverse acquisition order.
     */
    free(aggregated);
    dispatch_destroy_samples(&samples);
    /*
     * Release the cache eviction buffer.
     * Safe even when eviction was disabled,
     * because the context was zero-initialized.
     */
    cache_eviction_destroy(&eviction);
    if (profiler_initialized) {
        profiler_cleanup(&profiler);
    }
    destroy_file_manager(file_manager);
    destroy_test_plan(plan);
    return result;
}
