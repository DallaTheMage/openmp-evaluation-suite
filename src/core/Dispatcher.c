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
 * @file Dispatcher.c
 * @brief Execution orchestration for benchmark test plans.
 *
 * The dispatcher deliberately contains no benchmark-specific computation.
 * Its hot execution loop only resolves an already registered kernel,
 * invokes it for the requested number of repetitions and stores the metric
 * produced by that kernel.
 */

#include "core/Dispatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels/KernelRegistry.h"


/* ========================================================================== */
/* Internal helpers                                                           */
/* ========================================================================== */

/**
 * @brief Check whether a size_t multiplication would overflow.
 */
static int size_mul_overflow(
    size_t a,
    size_t b,
    size_t *result
) {
    if (!result) {
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
 */
static int size_add_overflow(
    size_t a,
    size_t b,
    size_t *result
) {
    if (!result) {
        return 1;
    }

    if (b > SIZE_MAX - a) {
        return 1;
    }

    *result = a + b;
    return 0;
}


/**
 * @brief Validate the structural fields required by the dispatcher.
 */
static DispatchStatus validate_plan_for_dispatch(
    const TestPlan *plan,
    uint32_t work_reps
) {
    if (!plan || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    if (plan->count != 0U && !plan->data) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    for (size_t set_index = 0; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];

        if (!set->view.buffer) {
            return DISPATCH_INVALID_ARGUMENT;
        }

        if (set->kernels.count != 0U && !set->kernels.data) {
            return DISPATCH_INVALID_ARGUMENT;
        }

        for (size_t group_index = 0;
             group_index < set->kernels.count;
             ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            const KernelFunc function =
                kernel_registry_get_function(set->view.type, group->kernel);

            if (!function) {
                return DISPATCH_KERNEL_UNAVAILABLE;
            }

            if (group->cases.count != 0U && !group->cases.data) {
                return DISPATCH_INVALID_ARGUMENT;
            }

            for (size_t case_index = 0;
                 case_index < group->cases.count;
                 ++case_index) {
                const TestCase *test_case =
                    &group->cases.data[case_index];

                if (test_case->num_threads == 0U) {
                    return DISPATCH_INVALID_ARGUMENT;
                }
            }
        }
    }

    return DISPATCH_OK;
}


/**
 * @brief Count all measured samples required by a test plan.
 */
static DispatchStatus count_samples(
    const TestPlan *plan,
    uint32_t work_reps,
    size_t *out_count
) {
    size_t total = 0U;

    if (!plan || !out_count || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    for (size_t set_index = 0; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];

        for (size_t group_index = 0;
             group_index < set->kernels.count;
             ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            size_t group_samples;

            if (size_mul_overflow(
                    group->cases.count,
                    (size_t) work_reps,
                    &group_samples)) {
                return DISPATCH_SIZE_OVERFLOW;
            }

            if (size_add_overflow(total, group_samples, &total)) {
                return DISPATCH_SIZE_OVERFLOW;
            }
        }
    }

    *out_count = total;
    return DISPATCH_OK;
}


/**
 * @brief Execute one kernel invocation and store its metric.
 */
static DispatchStatus execute_kernel(
    KernelFunc function,
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
) {
    if (!function || !view || !test_case || !profiler || !metric) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

    memset(metric, 0, sizeof(*metric));
    function(view, test_case, profiler, metric);

    /*
     * KernelFunc is intentionally void-returning. The profiler state is the
     * only execution-level failure signal currently exposed to the caller.
     * A correctly implemented kernel must leave the profiler stopped.
     */
    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

    return DISPATCH_OK;
}


/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

DispatchStatus dispatch_test_plan(
    const TestPlan *plan,
    Profiler *profiler,
    uint32_t warmup_reps,
    uint32_t work_reps,
    RawSampleSet *out_samples
) {
    DispatchStatus status;
    size_t sample_count = 0U;
    size_t sample_index = 0U;

    if (!plan || !profiler || !out_samples || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    /* Never leak or silently append to a caller-owned previous result. */
    out_samples->samples = NULL;
    out_samples->count = 0U;

    status = validate_plan_for_dispatch(plan, work_reps);

    if (status != DISPATCH_OK) {
        return status;
    }

    status = count_samples(plan, work_reps, &sample_count);

    if (status != DISPATCH_OK) {
        return status;
    }

    if (sample_count != 0U) {
        out_samples->samples =
            calloc(sample_count, sizeof(*out_samples->samples));

        if (!out_samples->samples) {
            return DISPATCH_ALLOCATION_FAILURE;
        }
    }

    out_samples->count = sample_count;

    printf(
        "[Dispatcher] Starting %zu measured samples\n",
        sample_count
    );
    fflush(stdout);

    for (size_t set_index = 0; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];
        DataView view = set->view;

        for (size_t group_index = 0;
             group_index < set->kernels.count;
             ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            const KernelFunc function =
                kernel_registry_get_function(set->view.type, group->kernel);

            for (size_t case_index = 0;
                 case_index < group->cases.count;
                 ++case_index) {
                const TestCase *test_case =
                    &group->cases.data[case_index];

                printf(
                    "[Dispatcher] Experiment %zu/%zu: set=%llu kernel=%d "
                    "case=%llu threads=%u chunk=%llu\n",
                    sample_index / (size_t) work_reps + 1U,
                    sample_count / (size_t) work_reps,
                    (unsigned long long) set->uid,
                    (int) group->kernel,
                    (unsigned long long) test_case->uid,
                    test_case->num_threads,
                    (unsigned long long) test_case->chunk_size
                );
                fflush(stdout);

                /* Warm-up executions are deliberately discarded. */
                for (uint32_t warmup = 0U;
                     warmup < warmup_reps;
                     ++warmup) {
                    PerformanceMetric warmup_metric;

                    status = execute_kernel(
                        function,
                        &view,
                        test_case,
                        profiler,
                        &warmup_metric
                    );

                    if (status != DISPATCH_OK) {
                        dispatch_destroy_samples(out_samples);
                        return status;
                    }
                }

                for (uint32_t run_id = 0U;
                     run_id < work_reps;
                     ++run_id) {
                    RawSample *sample =
                        &out_samples->samples[sample_index];

                    status = execute_kernel(
                        function,
                        &view,
                        test_case,
                        profiler,
                        &sample->metric
                    );

                    if (status != DISPATCH_OK) {
                        dispatch_destroy_samples(out_samples);
                        return status;
                    }

                    sample->test_set_id = set->uid;
                    sample->test_case_id = test_case->uid;
                    sample->kernel = group->kernel;
                    sample->run_id = run_id;

                    ++sample_index;
                }

                printf(
                    "[Dispatcher] Experiment %zu/%zu completed\n",
                    sample_index / (size_t) work_reps,
                    sample_count / (size_t) work_reps
                );
                fflush(stdout);
            }
        }
    }

    printf("[Dispatcher] All experiments completed\n");
    fflush(stdout);

    return DISPATCH_OK;
}


void dispatch_destroy_samples(RawSampleSet *samples)
{
    if (!samples) {
        return;
    }

    free(samples->samples);
    samples->samples = NULL;
    samples->count = 0U;
}