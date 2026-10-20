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
 * @file DispatchExecution.h
 * @brief Internal execution helper for the dispatcher.
 */

#ifndef CORE_INTERNAL_DISPATCH_EXECUTION_H
#define CORE_INTERNAL_DISPATCH_EXECUTION_H

#include "core/Dispatcher.h"
#include "kernels/KernelRegistry.h"

/**
 * @brief Prepare the work buffer and execute one kernel invocation.
 * @param function Kernel function to invoke.
 * @param view Original view describing the input window.
 * @param test_case Case containing thread and scheduling settings.
 * @param memory_manager Manager used to restore working data.
 * @param profiler Profiler collecting this invocation's measurements.
 * @param metric Output measurements for the invocation.
 * @param eviction Optional cache eviction context.
 * @return Status of kernel setup and execution.
 */
DispatchStatus dispatch_execute_kernel(KernelFunc function, const DataView *view,
    const TestCase *test_case, MemoryManager *memory_manager, Profiler *profiler,
    PerformanceMetric *metric, CacheEviction *eviction);

#endif
