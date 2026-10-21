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
 * @file DispatchExecution.c
 * @brief Kernel preparation and invocation for the dispatcher.
 */

/* Kernel invocation and work-buffer preparation. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "core/internal/DispatchExecution.h"

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
DispatchStatus dispatch_execute_kernel(
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

    if ((function == NULL)
        || (view == NULL)
        || (test_case == NULL)
        || (memory_manager == NULL)
        || (profiler == NULL)
        || (metric == NULL)) {
        return DISPATCH_INVALID_ARGUMENT;
    }

    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

    if (view->window.size > (uint64_t)SIZE_MAX ||
        memory_manager_restore_window(
            memory_manager,
            view->window.offset,
            (size_t)view->window.size) != 0) {
        return DISPATCH_MEMORY_PREPARATION_FAILURE;
    }

    working_buffer = memory_manager_work_buffer(memory_manager);

    if (working_buffer == NULL ||
        working_buffer->pool == NULL ||
        working_buffer->element_count < (size_t)view->window.size) {
        return DISPATCH_MEMORY_PREPARATION_FAILURE;
    }

    execution_view               = *view;
    execution_view.buffer        = working_buffer;
    execution_view.window.offset = UINT64_C(0);

    if (eviction != NULL) {
        cache_eviction_run(eviction);
    }

    memset(metric, 0, sizeof(*metric));
    kernel_status = function(&execution_view, test_case, profiler, metric);

    if (profiler->is_running) {
        return DISPATCH_PROFILER_FAILURE;
    }

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
