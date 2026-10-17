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
 * @file Dispatcher.h
 * @brief Execution dispatcher for benchmark test plans.
 *
 * The dispatcher translates a TestPlan into benchmark kernel executions.
 *
 * Its responsibilities include:
 *
 * - iterating over test sets and kernel groups;
 * - resolving kernel implementations through the registry;
 * - restoring working data before each invocation;
 * - executing warm-up and measured repetitions;
 * - collecting performance metrics;
 * - storing measured executions as RawSample objects.
 *
 * The dispatcher does not select OpenMP scheduling policies, perform
 * statistical analysis, or own the profiler and data buffers.
 *
 * Kernel execution failures are propagated through DispatchStatus.
 */

#ifndef CORE_DISPATCHER_H
#define CORE_DISPATCHER_H

#include <stddef.h>
#include <stdint.h>

#include "core/Analyzer.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"
#include "data/MemoryManager.h"
#include "data/CacheEviction.h"

/**
 * @brief Result codes returned by the dispatcher.
 *
 * These values distinguish validation, allocation, kernel execution,
 * memory preparation, and profiling failures.
 */
typedef enum DispatchStatus {
    /** Dispatch completed successfully. */
    DISPATCH_OK = 0,

    /** One or more input arguments are invalid. */
    DISPATCH_INVALID_ARGUMENT = 1,

    /** The output sample array size would overflow size_t. */
    DISPATCH_SIZE_OVERFLOW = 2,

    /** Allocation of the output sample array failed. */
    DISPATCH_ALLOCATION_FAILURE = 3,

    /** A required kernel/view implementation is unavailable. */
    DISPATCH_KERNEL_UNAVAILABLE = 4,

    /** A kernel or profiler measurement failed. */
    DISPATCH_PROFILER_FAILURE = 5,

    /** The memory manager could not prepare the requested working data. */
    DISPATCH_MEMORY_PREPARATION_FAILURE = 6,

    /** A kernel reported an execution failure. */
    DISPATCH_KERNEL_EXECUTION_FAILURE = 7
} DispatchStatus;

/**
 * @brief Execute all test cases in a test plan and collect profiling samples.
 *
 * Each test case is executed for the requested number of warm-up and measured
 * repetitions. Before each invocation, the working data is restored through
 * the memory manager.
 *
 * Data restoration occurs outside the kernel's profiling interval.
 *
 * The dispatcher stops processing the plan when an execution fails.
 * On failure, the caller must inspect the returned status and release any
 * partially populated output sample set with dispatch_destroy_samples().
 *
 * @param plan Test plan containing kernel configurations and data views.
 * @param memory_manager Memory manager used to restore working data.
 * @param profiler Initialized profiler used to measure kernel execution.
 * @param warmup_reps Number of warm-up invocations per test case.
 * @param work_reps Number of measured invocations per test case.
 * @param out_samples Output collection for measured profiling samples.
 *
 * @return DISPATCH_OK on success, or an appropriate error status.
 *
 * @pre plan must not be NULL.
 * @pre memory_manager must not be NULL.
 * @pre profiler must not be NULL.
 * @pre out_samples must not be NULL.
 */
DispatchStatus dispatch_test_plan(
    const TestPlan *plan,
    MemoryManager *memory_manager,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples
);

DispatchStatus dispatch_test_plan_with_eviction(
    const TestPlan *plan,
    MemoryManager *memory_manager,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples,
    CacheEviction *eviction
);

/**
 * @brief Release a raw sample set produced by the dispatcher.
 *
 * This function releases the sample array and resets the collection
 * to an empty state.
 *
 * @param samples Raw sample set to destroy.
 *
 * @note Passing NULL is allowed and has no effect.
 */
void dispatch_destroy_samples(
    RawSampleSet *samples
);

#endif /* CORE_DISPATCHER_H */