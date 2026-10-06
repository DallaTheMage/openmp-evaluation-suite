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
 * The dispatcher is responsible for translating a TestPlan into actual
 * kernel executions.
 *
 * Its responsibilities are limited to execution orchestration:
 *
 * - iterate over test sets;
 * - iterate over kernel test groups;
 * - resolve kernel implementations through the kernel registry;
 * - perform warm-up executions;
 * - perform measured executions;
 * - collect the resulting PerformanceMetric values;
 * - store measured executions as RawSample objects.
 *
 * The dispatcher does not:
 *
 * - select OpenMP scheduling policies;
 * - modify OpenMP runtime scheduling state;
 * - perform statistical analysis;
 * - compute aggregated benchmark results;
 * - own the Profiler;
 * - own the DataBuffer referenced by a DataView.
 *
 * OpenMP scheduling is a compile-time property of the benchmark build and
 * is therefore completely outside the dispatcher's runtime control.
 *
 * The execution flow is:
 *
 *     TestPlan
 *         |
 *         v
 *     Dispatcher
 *         |
 *         +----> Kernel Registry
 *         |
 *         +----> Profiler
 *         |
 *         v
 *     RawSampleSet
 *
 * The dispatcher is part of the benchmark control path and is never called
 * from inside a benchmark kernel's hot loop.
 */

#ifndef CORE_DISPATCHER_H
#define CORE_DISPATCHER_H

#include <stddef.h>
#include <stdint.h>

#include "core/Analyzer.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"


/**
 * @brief Result codes returned by the dispatcher.
 *
 * The dispatcher reports setup and orchestration failures explicitly so the
 * caller can distinguish an invalid invocation from a failed execution.
 * Kernel implementations themselves currently expose a void return type;
 * therefore the dispatcher can only detect failures visible through the
 * profiler state and its own validation.
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

    /** A kernel returned while its profiler measurement was still active. */
    DISPATCH_PROFILER_FAILURE = 5
} DispatchStatus;


/**
 * @brief Dispatch and execute all valid experiments in a test plan.
 *
 * For every test set and kernel test group in the plan, the dispatcher
 * resolves the corresponding kernel through the kernel registry.
 *
 * Each valid test case is executed according to the following sequence:
 *
 *     warm-up executions
 *             |
 *             v
 *     measured executions
 *
 * Warm-up executions are not included in the returned RawSampleSet.
 *
 * Each measured execution produces exactly one RawSample containing:
 *
 * - the identity of the test set;
 * - the identity of the test case;
 * - the kernel type;
 * - the execution number;
 * - the PerformanceMetric produced by the profiler.
 *
 * The profiler is owned and initialized by the caller. The dispatcher
 * only uses it for benchmark measurements and does not destroy it.
 *
 * @param plan
 *     Test plan to execute.
 *
 * @param profiler
 *     Initialized profiler used for measured executions.
 *
 * @param warmup_reps
 *     Number of unmeasured warm-up executions for each test case.
 *
 * @param work_reps
 *     Number of measured executions for each test case.
 *
 * @param out_samples
 *     Output location receiving the dynamically allocated raw sample set.
 *     On success, ownership is transferred to the caller.
 *
 * @return
 *     A DispatchStatus value. DISPATCH_OK indicates success.
 *
 * @pre plan != NULL.
 * @pre profiler != NULL.
 * @pre out_samples != NULL.
 * @pre profiler must have been successfully initialized.
 * @pre work_reps > 0.
 *
 * @post On success, @p out_samples contains a valid RawSampleSet.
 * @post Warm-up executions are not present in the returned sample set.
 *
 * @note The function does not modify the TestPlan.
 * @note The function does not modify the OpenMP runtime schedule.
 * @note The caller owns the returned RawSampleSet and must release it with
 *       dispatch_destroy_samples().
 */
DispatchStatus dispatch_test_plan(
    const TestPlan *plan,
    Profiler       *profiler,
    uint32_t        warmup_reps,
    uint32_t        work_reps,
    RawSampleSet   *out_samples
);


/**
 * @brief Release a raw sample set produced by the dispatcher.
 *
 * This function releases the dynamically allocated sample array and
 * resets the RawSampleSet to an empty state.
 *
 * @param samples
 *     Raw sample set to destroy.
 *
 * @pre The object must either be initialized by dispatch_test_plan() or
 *     contain zero/NULL values.
 *
 * @post samples->samples == NULL.
 * @post samples->count == 0.
 *
 * @note Passing NULL is allowed and has no effect.
 */
void dispatch_destroy_samples(
    RawSampleSet *samples
);


#endif /* CORE_DISPATCHER_H */