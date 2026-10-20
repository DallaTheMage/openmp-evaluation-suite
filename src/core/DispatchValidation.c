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
 * @file DispatchValidation.c
 * @brief Structural and optional numerical validation for dispatch.
 */

/* Dispatcher-specific structural and optional numerical checks. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "core/internal/DispatchValidation.h"
#include "kernels/KernelRegistry.h"
#include "kernels/common/BenchmarkValidation.h"

#if BENCH_VALIDATE_RESULTS
/* Shared only by the post-timing reduction validation path. */
volatile double benchmark_validation_reduction_sum = 0.0;

/* Validate a single untimed diagnostic invocation per test configuration.
 * Original input remains immutable in MemoryManager. */
DispatchStatus dispatch_validate_loop_or_reduction(
    KernelType kernel,
    const DataView *view,
    MemoryManager *memory_manager)
{
    const DataBuffer *source = memory_manager_original_buffer(memory_manager);
    const DataBuffer *work   = memory_manager_work_buffer(memory_manager);
    const uint64_t offset    = view->window.offset;
    const uint64_t count     = view->window.size;

    if (source == NULL || work == NULL || source->pool == NULL ||
        work->pool == NULL || (offset > (uint64_t)source->element_count) ||
        (count > ((uint64_t)source->element_count - offset)) ||
        (count > (uint64_t)work->element_count)) {
        return DISPATCH_MEMORY_PREPARATION_FAILURE;
    }
    if (kernel == KERNEL_LOOP || kernel == KERNEL_LOOP_SOA_INNER) {
        for (uint64_t i = 0U; i < count; ++i) {
            const double expected = (source->pool[(size_t)(offset + i)] * 2.0) + 1.0;
            const double actual   = work->pool[(size_t)i];
            /* Absolute + relative tolerance accounts for arithmetic rounding. */
            if (!isfinite(actual) || !isfinite(expected) ||
                fabs(actual - expected) > 1e-12 * fmax(1.0, fabs(expected))) {
                fprintf(stderr,
                    "[Validation] Loop mismatch: index=%llu expected=%.17g actual=%.17g\n",
                    (unsigned long long)i, expected, actual);
                return DISPATCH_KERNEL_EXECUTION_FAILURE;
            }
        }
    } else if (kernel == KERNEL_REDUCTION || kernel == KERNEL_REDUCTION_SOA_INNER) {
        /* A long double reference reduces rounding differences, while
         * tolerating reordering from parallel reductions. */
        long double expected = 0.0L;
        for (uint64_t i = 0U; i < count; ++i) {
            expected += (long double)source->pool[(size_t)(offset + i)];
        }
        const double actual    = benchmark_validation_reduction_sum;
        const double reference = (double)expected;
        const double tolerance = 1e-10 * fmax(1.0, fabs(reference));
        if (!isfinite(actual) || !isfinite(reference) ||
            fabs(actual - reference) > tolerance) {
            fprintf(stderr,
                "[Validation] Reduction mismatch: expected=%.17g actual=%.17g tolerance=%.3g\n",
                reference, actual, tolerance);
            return DISPATCH_KERNEL_EXECUTION_FAILURE;
        }
    }
    return DISPATCH_OK;
}
#endif

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
DispatchStatus dispatch_validate_plan_for_dispatch(
    const TestPlan *plan,
    uint32_t work_reps) {
    if (plan == NULL || work_reps == 0U) {
        return DISPATCH_INVALID_ARGUMENT;
    }
    if (plan->count != 0U && plan->data == NULL) {
        return DISPATCH_INVALID_ARGUMENT;
    }
    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *set = &plan->data[set_index];
        if (set->view.buffer == NULL) {
            return DISPATCH_INVALID_ARGUMENT;
        }
        if (set->kernels.count != 0U && set->kernels.data == NULL) {
            return DISPATCH_INVALID_ARGUMENT;
        }
        for (size_t group_index = 0U; group_index < set->kernels.count; ++group_index) {
            const KernelTestGroup *group = &set->kernels.data[group_index];
            const KernelFunc function = kernel_registry_get_function(
                                            set->view.type,
                                            group->kernel
                                        );
            if (function == NULL) {
                return DISPATCH_KERNEL_UNAVAILABLE;
            }

            if (group->cases.count != 0U && group->cases.data == NULL) {
                return DISPATCH_INVALID_ARGUMENT;
            }

            for (size_t case_index = 0U; case_index < group->cases.count; ++case_index) {
                const TestCase *test_case = &group->cases.data[case_index];
                if (test_case->num_threads == 0U || test_case->chunk_size == 0U) {
                    return DISPATCH_INVALID_ARGUMENT;
                }
            }
        }
    }

    return DISPATCH_OK;
}
