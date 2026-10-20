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
 * @file DispatchValidation.h
 * @brief Internal dispatch-plan and optional result validation.
 */

#ifndef CORE_INTERNAL_DISPATCH_VALIDATION_H
#define CORE_INTERNAL_DISPATCH_VALIDATION_H

#include "core/Dispatcher.h"
#include "kernels/common/BenchmarkValidation.h"

/**
 * @brief Check whether a plan can be dispatched safely.
 * @param plan Test plan to check.
 * @param work_reps Requested repetitions of the work unit.
 * @return Validation status.
 */
DispatchStatus dispatch_validate_plan_for_dispatch(const TestPlan *plan,
    uint32_t work_reps);

#if BENCH_VALIDATE_RESULTS
/**
 * @brief Check results for supported loop and reduction kernels.
 * @param kernel Executed kernel type.
 * @param view View used for the execution.
 * @param memory_manager Manager containing the resulting data.
 * @return Correctness validation status.
 */
DispatchStatus dispatch_validate_loop_or_reduction(KernelType kernel,
    const DataView *view, MemoryManager *memory_manager);
#endif

#endif
