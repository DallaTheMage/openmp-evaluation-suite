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
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "core/Dispatcher.h"
#include "kernels/KernelRegistry.h"

/* ========================================================================== */
/* Internal helpers                                                           */
/* ========================================================================== */

/**
 * @brief Convert a view type to a human-readable string.
 *
 * @param type View type.
 * @return Constant string describing the view type.
 */
static const char *view_type_to_string(ViewType type)
{
    switch (type) {
        case VIEW_1D:
            return "1D";

        case VIEW_2D:
            return "2D";

        case VIEW_3D:
            return "3D";

        case VIEW_AOS:
            return "AoS";

        case VIEW_SOA:
            return "SoA";

        case VIEW_AOSOA:
            return "AoSoA";

        default:
            return "Unknown";
    }
}

/**
 * @brief Convert a kernel type to a human-readable string.
 *
 * @param type Kernel type.
 * @return Constant string describing the kernel.
 */
static const char *kernel_type_to_string(KernelType type)
{
    switch (type) {
        case KERNEL_LOOP:
            return "Loop";

        case KERNEL_COLLAPSE:
            return "Collapse";

        case KERNEL_SECTIONS:
            return "Sections";

        case KERNEL_SIMD_MEMORY:
            return "SIMD Memory";

        case KERNEL_SIMD_COMPUTE:
            return "SIMD Compute";

        case KERNEL_MASTER:
            return "Master";

        case KERNEL_MASKED:
            return "Masked";

        case KERNEL_ATOMIC:
            return "Atomic";

        case KERNEL_CRITICAL:
            return "Critical";

        case KERNEL_ORDERED:
            return "Ordered";

        case KERNEL_SYNC:
            return "Sync";

        case KERNEL_REDUCTION:
            return "Reduction";

        case KERNEL_TASK:
            return "Task";

        case KERNEL_TASKLOOP:
            return "Taskloop";

        case KERNEL_NATIVE_SCAN:
            return "Native Scan";

        case KERNEL_GEMM:
            return "GEMM";
        case KERNEL_JACOBI:
            return "Jacobi 2D";
        case KERNEL_CG:
            return "Conjugate Gradient";

        case KERNEL_TWO_PASS_SCAN:
            return "Two-Pass Scan";

        default:
            return "Unknown";
    }
}

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
    size_t *result)
{
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
    size_t *result)
{
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
 * @brief Validate the structural fields required by the dispatcher.
 *
 * This function validates the plan structure and ensures that every
 * requested kernel implementation is available.
 *
 * Logical window bounds are checked when the window is restored by the
 * memory manager.
 *
 * @param plan Test plan to validate.
 * @param work_reps Number of measured repetitions.
 * @return Dispatch status.
 */
static DispatchStatus validate_plan_for_dispatch(
    const TestPlan *plan,
    uint32_t work_reps)
{
    if (plan == NULL || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    if (plan->count != 0U && plan->data == NULL) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    for (size_t set_index = 0U;
         set_index < plan->count;
         ++set_index) {

        const TestSet *set = &plan->data[set_index];

        if (set->view.buffer == NULL) {
            return DISPATCH_INVALID_ARGUMENT;
        }

        if (set->kernels.count != 0U &&
            set->kernels.data == NULL) {
            return DISPATCH_INVALID_ARGUMENT;
        }

        for (size_t group_index = 0U;
             group_index < set->kernels.count;
             ++group_index) {

            const KernelTestGroup *group =
                &set->kernels.data[group_index];

            const KernelFunc function =
                kernel_registry_get_function(
                    set->view.type,
                    group->kernel
                );

            if (function == NULL) {
                return DISPATCH_KERNEL_UNAVAILABLE;
            }

            if (group->cases.count != 0U &&
                group->cases.data == NULL) {
                return DISPATCH_INVALID_ARGUMENT;
            }

            for (size_t case_index = 0U;
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
 *
 * Warm-up repetitions are not included in the sample count.
 *
 * @param plan Test plan.
 * @param work_reps Number of measured repetitions.
 * @param out_count Destination for the total sample count.
 * @return Dispatch status.
 */
static DispatchStatus count_samples(
    const TestPlan *plan,
    uint32_t work_reps,
    size_t *out_count)
{
    size_t total = 0U;

    if (plan == NULL ||
        out_count == NULL ||
        work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    for (size_t set_index = 0U;
         set_index < plan->count;
         ++set_index) {

        const TestSet *set = &plan->data[set_index];

        for (size_t group_index = 0U;
             group_index < set->kernels.count;
             ++group_index) {

            const KernelTestGroup *group =
                &set->kernels.data[group_index];

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

/**
 * @brief Restore a logical window and execute one kernel invocation.
 *
 * The original view identifies a window in the original dataset.
 * The memory manager copies that window into the beginning of its working
 * buffer, outside the kernel profiling interval.
 *
 * The kernel receives a local copy of the original view. Its buffer points
 * to the working buffer, and its window offset is zero.
 *
 * The original view and test plan are not modified.
 *
 * @param function Kernel function.
 * @param view Original logical data view.
 * @param test_case Test case configuration.
 * @param memory_manager Manager used to restore working data.
 * @param profiler Profiler instance.
 * @param metric Destination for the performance metric.
 * @return Dispatch status.
 */
static DispatchStatus execute_kernel(
    KernelFunc function,
    const DataView *view,
    const TestCase *test_case,
    MemoryManager *memory_manager,
    Profiler *profiler,
    PerformanceMetric *metric,
    CacheEviction *eviction) {

    KernelStatus kernel_status;
    DataView execution_view;
    DataBuffer *working_buffer;

    if (function == NULL ||
        view == NULL ||
        test_case == NULL ||
        memory_manager == NULL ||
        profiler == NULL ||
        metric == NULL) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    /*
     * A new measurement must not start while another measurement is active.
     */
    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

    /*
     * Restore the requested logical window into the working buffer.
     * The memory manager validates the offset and size against both buffers.
     */
    if (view->window.size > (uint64_t)SIZE_MAX ||
        memory_manager_restore_window(
            memory_manager,
            view->window.offset,
            (size_t)view->window.size) != 0) {
        return DISPATCH_MEMORY_PREPARATION_FAILURE;
    }

    /*
     * Retrieve the working buffer after restoration.
     */
    working_buffer = memory_manager_work_buffer(memory_manager);

    if (working_buffer == NULL ||
        working_buffer->pool == NULL ||
        working_buffer->element_count < (size_t)view->window.size) {
        return DISPATCH_MEMORY_PREPARATION_FAILURE;
    }

    /*
     * Preserve the original view metadata while redirecting the kernel to
     * the compact working buffer.
     *
     * The logical window begins at zero in this buffer because the requested
     * source window has already been copied to its beginning.
     */
    execution_view = *view;
    execution_view.buffer = working_buffer;
    execution_view.window.offset = UINT64_C(0);

    /*
     * Cache eviction is executed after restoring the
     * working data and before entering the kernel.
     *
     * The kernel's profiling interval does not include
     * this operation.
     */
    if (eviction != NULL) {
        cache_eviction_run(eviction);
    }

    /*
     * Initialize the destination metric before calling the kernel.
     */
    memset(metric, 0, sizeof(*metric));

    kernel_status = function(
        &execution_view,
        test_case,
        profiler,
        metric
    );

    /*
     * A kernel must leave the profiler stopped, regardless of its status.
     */
    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

    /*
     * Translate the kernel result into the dispatcher's public status.
     */
    switch (kernel_status) {
        case KERNEL_STATUS_OK:
            return DISPATCH_OK;

        case KERNEL_STATUS_PROFILER_FAILURE:
            return DISPATCH_PROFILER_FAILURE;

        case KERNEL_STATUS_INVALID_ARGUMENT:
            return DISPATCH_INVALID_ARGUMENT;

        case KERNEL_STATUS_ALLOCATION_FAILURE:
            return DISPATCH_ALLOCATION_FAILURE;

        case KERNEL_STATUS_EXECUTION_FAILURE:
        default:
            return DISPATCH_KERNEL_EXECUTION_FAILURE;
    }
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

/**
 * @brief Execute a test plan and collect profiling samples.
 *
 * Each test case is executed for the requested number of warm-up and measured
 * repetitions.
 *
 * Before every kernel invocation, the dispatcher restores the corresponding
 * logical window into the working buffer. Restoration occurs outside the
 * kernel profiling interval.
 *
 * Warm-up metrics are discarded. Measured metrics are stored in the output
 * sample set.
 *
 * If an execution fails, dispatch stops and releases the partially populated
 * output collection.
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

    /*
     * Initialize the output collection before allocating resources.
     *
     * The caller must release any previously allocated collection before
     * reusing the same RawSampleSet.
     */
    out_samples->samples = NULL;
    out_samples->count = 0U;

    status = validate_plan_for_dispatch(plan, work_reps);

    if (status != DISPATCH_OK) {
        return status;
    }

    status = count_samples(
        plan,
        work_reps,
        &sample_count
    );

    if (status != DISPATCH_OK) {
        return status;
    }

    /*
     * Allocate storage for measured samples only.
     */
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

    for (size_t set_index = 0U;
         set_index < plan->count;
         ++set_index) {

        const TestSet *set = &plan->data[set_index];

        /*
         * Keep the original DataView unchanged.
         * execute_kernel() creates a compact local view for each invocation.
         */
        const DataView view = set->view;

        for (size_t group_index = 0U;
             group_index < set->kernels.count;
             ++group_index) {

            const KernelTestGroup *group =
                &set->kernels.data[group_index];

            const KernelFunc function =
                kernel_registry_get_function(
                    view.type,
                    group->kernel
                );

            for (size_t case_index = 0U;
                 case_index < group->cases.count;
                 ++case_index) {

                const TestCase *test_case =
                    &group->cases.data[case_index];

                double total_wall_time = 0.0;

                const size_t experiment_index =
                    sample_index / (size_t)work_reps + 1U;

                const size_t experiment_count =
                    sample_count / (size_t)work_reps;

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
                    view_type_to_string(view.type),
                    kernel_type_to_string(group->kernel),
                    test_case->num_threads,
                    (unsigned long long)test_case->chunk_size,
                    (unsigned long long)view.window.offset,
                    (unsigned long long)view.window.size,
                    window_gib
                );
                fflush(stdout);

                /*
                 * Execute warm-up repetitions without storing their metrics.
                 */
                for (uint32_t warmup = 0U;
                     warmup < warmup_reps;
                     ++warmup) {

                    PerformanceMetric warmup_metric;

                    status = execute_kernel(
                        function,
                        &view,
                        test_case,
                        memory_manager,
                        profiler,
                        &warmup_metric,
                        eviction
                    );

                    if (status != DISPATCH_OK) {
                        dispatch_destroy_samples(out_samples);
                        return status;
                    }
                }

                /*
                 * Execute measured repetitions and store their metrics.
                 */
                for (uint32_t run_id = 0U;
                     run_id < work_reps;
                     ++run_id) {

                    RawSample *sample =
                        &out_samples->samples[sample_index];

                    status = execute_kernel(
                        function,
                        &view,
                        test_case,
                        memory_manager,
                        profiler,
                        &sample->metric,
                        eviction
                    );

                    if (status != DISPATCH_OK) {
                        dispatch_destroy_samples(out_samples);
                        return status;
                    }

                    sample->test_set_id = set->uid;
                    sample->test_case_id = test_case->uid;
                    sample->kernel = group->kernel;
                    sample->run_id = run_id;

                    total_wall_time +=
                        sample->metric.wall_time_sec;

                    ++sample_index;
                }

                printf(
                    "[Dispatcher] Experiment %zu/%zu completed "
                    "(average wall time: %.6f s)\n",
                    experiment_index,
                    experiment_count,
                    total_wall_time / (double)work_reps
                );
                fflush(stdout);
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
 * This function resets the collection to an empty state.
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