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

/**
 * @file main.c
 * @brief Command-line entry point and top-level benchmark orchestration.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config/params.h"
#include "core/internal/BenchmarkRunner.h"
#include "core/Configuration.h"
#include "core/Dispatcher.h"
#include "core/FileManager.h"
#include "core/ResultsWriter.h"
#include "core/TestPlan.h"
#include "data/MemoryManager.h"
#include "profiling/Profiler.h"
#include "data/CacheEviction.h"

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
        fprintf(stderr, "[Error] Invalid configuration\n");
        goto cleanup;
    }

     /* Ensure the configured dataset fits the address space. */
    if (config->derived.problem_size_elements == 0U ||
        config->derived.problem_size_elements > (uint64_t)SIZE_MAX) {
        fprintf(stderr, "[Error] Configured dataset size is invalid or not addressable\n");
        goto cleanup;
    }

    printf(
        "[Main] Allocating and initializing original data buffer (%llu elements)\n",
        (unsigned long long)config->derived.problem_size_elements
    );

     /* The manager owns the original buffer and the reusable working buffer. */
    memory_manager = memory_manager_create(
        (size_t)config->derived.problem_size_elements,
        config->memory.alignment,
        config->benchmark.generation_seed
    );

    if (memory_manager == NULL) {
        fprintf(stderr, "[Error] Unable to initialize memory manager\n");
        goto cleanup;
    }

    /*
     * Suites are independent: try every enabled suite even if
     * an earlier suite fails.
     */
#if RUN_FULL_SCALE
    full_scale_failed = (
        run_benchmark_suite(memory_manager, config, "Full_scale", SCALING_STRONG, 0) != 0
    );
#endif

#if RUN_PROPORTIONAL
    proportional_failed = (
        run_benchmark_suite(memory_manager, config, "Proportional", SCALING_WEAK, 1) != 0
    );
#endif

    if (full_scale_failed) {
        fprintf(stderr, "[Error] Full-scale (Strong Scaling) suite failed\n");
    }
    if (proportional_failed) {
        fprintf(stderr, "[Error] Proportional (Weak Scaling) suite failed\n");
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
int main(void) {
    Configuration config = load_configuration();
    int exit_code;
    printf("[Main] Configuration loaded\n");
    exit_code = run_application(&config);
    /* Configuration owns dynamically allocated lists and is released here. */
    destroy_configuration(&config);
    return exit_code;
}