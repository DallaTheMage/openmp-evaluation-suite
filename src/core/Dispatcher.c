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
 * @file Dispatcher.c
 * @brief Execution orchestration for benchmark test plans.
 *
 * The dispatcher resolves registered kernels, executes warm-up and measured
 * repetitions, and stores the resulting performance metrics.
 *
 * Before each invocation, the dispatcher restores the requested logical
 * window into the working buffer. Data restoration is performed outside
 * the kernel profiling interval.
 *
 * Logical windows may have arbitrary offsets in the original dataset.
 * Kernels receive a compact working view whose window starts at offset zero.
 *
 * Kernel failures are propagated through DispatchStatus.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "core/Dispatcher.h"
#include "utils/Names.h"
#include "core/internal/DispatchExecution.h"
#include "core/internal/DispatchValidation.h"
#include "kernels/KernelRegistry.h"

/**
 * @brief Check whether a size_t multiplication would overflow.
 *
 * @param a First operand.
 * @param b Second operand.
 * @param result Destination for the multiplication result.
 * @return 1 if an overflow would occur, 0 otherwise.
 */
static int size_mul_overflow(
    size_t a,
    size_t b,
    size_t *result) {
    if (result == NULL) {
        return 1;
    }
    if (a != 0U && b > SIZE_MAX / a) {
        return 1;
    }
    *result = a * b;
    return 0;
}

/**
 * @brief Check whether a size_t addition would overflow.
 *
 * @param a First operand.
 * @param b Second operand.
 * @param result Destination for the addition result.
 * @return 1 if an overflow would occur, 0 otherwise.
 */
static int size_add_overflow(
    size_t a,
    size_t b,
    size_t *result) {
    if (result == NULL) {
        return 1;
    }
    if (b > SIZE_MAX - a) {
        return 1;
    }
    *result = a + b;
    return 0;
}

/**
 * @brief Count all measured samples required by a test plan.
 *
 * @param plan Test plan.
 * @param work_reps Number of measured repetitions.
 * @param out_count Destination for the total sample count.
 * @return Dispatch status.
 */
static DispatchStatus count_samples(
    const TestPlan *plan,
    uint32_t work_reps,
    size_t *out_count) {
    size_t total = 0U;
    if (plan == NULL || out_count == NULL || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }
    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];
        for (size_t group_index = 0U; group_index < set->kernels.count; ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            size_t group_samples;
            if (size_mul_overflow(
                    group->cases.count,
                    (size_t)work_reps,
                    &group_samples)) {
                return DISPATCH_SIZE_OVERFLOW;
            }
            if (size_add_overflow(
                    total,
                    group_samples,
                    &total)) {
                return DISPATCH_SIZE_OVERFLOW;
            }
        }
    }
    *out_count = total;
    return DISPATCH_OK;
}

static DispatchStatus dispatch_execute_test_case(
    const TestSet *set,
    const KernelTestGroup *group,
    const TestCase *test_case,
    KernelFunc function,
    MemoryManager *memory_manager,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples,
    CacheEviction *eviction,
    size_t *sample_index) {
    const DataView view = set->view;
    DispatchStatus status;
    const size_t sample_count = out_samples->count;
    double total_wall_time = 0.0;
    const size_t experiment_index = ((*sample_index) / (size_t)work_reps) + 1U;
    const size_t experiment_count = sample_count / (size_t)work_reps;
    const double window_gib =
        (double)view.window.size *
        ((double)sizeof(double) / 1073741824.0);
    printf(
        "[Dispatcher] Experiment %zu/%zu\n"
        "    View       : %s\n"
        "    Kernel     : %s\n"
        "    Threads    : %u\n"
        "    Chunk size : %llu\n"
        "    Offset     : %llu\n"
        "    Window     : %llu elements (%.2f GiB)\n",
        experiment_index,
        experiment_count,
        utils_view_type_name(view.type),
        utils_kernel_type_name(group->kernel),
        test_case->num_threads,
        (unsigned long long)test_case->chunk_size,
        (unsigned long long)view.window.offset,
        (unsigned long long)view.window.size,
        window_gib
    );
    fflush(stdout);

    #if BENCH_VALIDATE_RESULTS
    if (group->kernel == KERNEL_LOOP ||
        group->kernel == KERNEL_REDUCTION ||
        group->kernel == KERNEL_LOOP) {
        PerformanceMetric validation_metric;
        status = dispatch_execute_kernel(
            function, &view, test_case, memory_manager,
            profiler, &validation_metric, eviction);
        if (status == DISPATCH_OK) {
            status = dispatch_validate_loop_or_reduction(
                group->kernel, &view, memory_manager);
        }
        if (status != DISPATCH_OK) {
            return status;
        }
    }
    #endif

    for (uint32_t warmup = 0U; warmup < warmup_reps; ++warmup) {
        PerformanceMetric warmup_metric;
        status = dispatch_execute_kernel(
            function,
            &view,
            test_case,
            memory_manager,
            profiler,
            &warmup_metric,
            eviction
        );
        if (status != DISPATCH_OK) {
            return status;
        }
    }

    for (uint32_t run_id = 0U; run_id < work_reps; ++run_id) {
        RawSample *sample = &out_samples->samples[(*sample_index)];
        status = dispatch_execute_kernel(
            function,
            &view,
            test_case,
            memory_manager,
            profiler,
            &sample->metric,
            eviction
        );
        if (status != DISPATCH_OK) {
            return status;
        }
        sample->test_set_id  = set->uid;
        sample->test_case_id = test_case->uid;
        sample->kernel       = group->kernel;
        sample->run_id       = run_id;
        total_wall_time     += sample->metric.wall_time_sec;
        (*sample_index)++;
    }

    printf(
        "[Dispatcher] Experiment %zu/%zu completed "
        "(average wall time: %.6f s)\n",
        experiment_index,
        experiment_count,
        total_wall_time / (double)work_reps
    );
    fflush(stdout);    return DISPATCH_OK;
}

/**
 * @brief Execute a test plan and collect profiling samples.
 *
 * @param plan Test plan containing kernel configurations and data views.
 * @param memory_manager Memory manager used to restore working data.
 * @param profiler Initialized profiler used to measure kernel execution.
 * @param warmup_reps Number of warm-up invocations per test case.
 * @param work_reps Number of measured invocations per test case.
 * @param out_samples Output collection for measured profiling samples.
 * @return DISPATCH_OK on success, or an appropriate error status.
 *
 * @pre plan must not be NULL.
 * @pre memory_manager must not be NULL.
 * @pre profiler must not be NULL.
 * @pre out_samples must not be NULL.
 */
DispatchStatus dispatch_test_plan_with_eviction(
    const TestPlan *plan,
    MemoryManager *memory_manager,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples,
    CacheEviction *eviction) {

    DispatchStatus status;
    size_t sample_count = 0U;
    size_t sample_index = 0U;

    if (plan == NULL ||
        memory_manager == NULL ||
        profiler == NULL ||
        out_samples == NULL ||
        work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    out_samples->samples = NULL;
    out_samples->count = 0U;

    status = dispatch_validate_plan_for_dispatch(plan, work_reps);

    if (status != DISPATCH_OK) {
        return status;
    }

    status = count_samples(plan, work_reps, &sample_count);

    if (status != DISPATCH_OK) {
        return status;
    }

    if (sample_count != 0U) {
        out_samples->samples = (RawSample *)calloc(
            sample_count,
            sizeof(*out_samples->samples)
        );

        if (out_samples->samples == NULL) {
            return DISPATCH_ALLOCATION_FAILURE;
        }
    }

    out_samples->count = sample_count;

    printf(
        "[Dispatcher] Starting %zu measured samples\n",
        sample_count
    );
    fflush(stdout);

    for (size_t set_index = 0U; set_index < plan->count; set_index++) {
        const TestSet *set = &plan->data[set_index];
        const DataView view = set->view;

        for (size_t group_index = 0U; group_index < set->kernels.count; group_index++) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            const KernelFunc function = kernel_registry_get_function(
                                            view.type,
                                            group->kernel
                                        );

            for (size_t case_index = 0U; case_index < group->cases.count; case_index++) {
                const TestCase *test_case = &group->cases.data[case_index];
                status = dispatch_execute_test_case(
                    set, group, test_case, function, memory_manager, profiler,
                    warmup_reps, work_reps, out_samples, eviction, &sample_index);
                if (status != DISPATCH_OK) {
                    dispatch_destroy_samples(out_samples);
                    return status;
                }
            }
        }
    }
    printf("[Dispatcher] All experiments completed\n");
    fflush(stdout);
    return DISPATCH_OK;
}

DispatchStatus dispatch_test_plan(
    const TestPlan *plan,
    MemoryManager *memory_manager,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples
) {
    return dispatch_test_plan_with_eviction(
        plan,
        memory_manager,
        profiler,
        warmup_reps,
        work_reps,
        out_samples,
        NULL
    );
}

/**
 * @brief Release the dynamically allocated sample array.
 *
 * @param samples Sample set to destroy. Passing NULL is allowed.
 */
void dispatch_destroy_samples(RawSampleSet *samples)
{
    if (samples != NULL) {
        free(samples->samples);
        samples->samples = NULL;
        samples->count = 0U;
    }
}