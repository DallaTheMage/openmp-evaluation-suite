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
 * @file TestPlan.h
 * @brief Benchmark test-plan data structures and construction API.
 *
 * A TestPlan describes the complete set of benchmark executions that must
 * be performed for a configured benchmark run.
 *
 * The plan is organized hierarchically:
 *
 *     TestPlan
 *         |
 *         +---TestSet
 *               |
 *               +---DataView
 *               |
 *               +---KernelTestGroup
 *                       |
 *                       +---TestCase
 *
 * Kernel availability is determined by the static kernel registry. The
 * test-plan generator must not duplicate OpenMP capability checks or
 * compiler-specific feature detection.
 *
 * A TestCase describes the execution parameters that vary during a
 * benchmark run. The OpenMP scheduling policy itself is not stored here:
 * it is a compile-time property of the benchmark build and is defined by
 * config/params.h.
 *
 * TestPlan owns all dynamically allocated TestSet, KernelTestGroup and
 * TestCase arrays it creates. DataView instances stored in a TestSet do not
 * own their underlying DataBuffer.
 */

#ifndef CORE_TEST_PLAN_H
#define CORE_TEST_PLAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/Configuration.h"
#include "data/DataView.h"
#include "kernels/common/types.h"

/**
 * @brief Opaque identifier for a complete test set.
 */
typedef uint64_t TestSetUID;

/**
 * @brief Opaque identifier for an individual test case.
 */
typedef uint64_t TestCaseUID;

/**
 * @brief One concrete benchmark execution configuration.
 *
 * The thread count determines the number of OpenMP threads used for the
 * execution.
 *
 * The chunk size is relevant to compile-time schedules such as dynamic and
 * guided. For schedules where the chunk size has no effect, it may still be
 * present in the plan as part of the common test-case representation.
 */
typedef struct TestCase {
    TestCaseUID uid;
    uint32_t num_threads;
    uint64_t chunk_size;
} TestCase;

/**
 * @brief Dynamically sized array of test cases.
 */
typedef struct TestCaseArray {
    TestCase *data;
    size_t count;
} TestCaseArray;

/**
 * @brief All test cases associated with one kernel.
 *
 * A kernel test group exists only when the kernel is available for the
 * corresponding DataView type.
 */
typedef struct KernelTestGroup {
    KernelType kernel;
    TestCaseArray cases;
} KernelTestGroup;

/**
 * @brief Dynamically sized array of kernel test groups.
 */
typedef struct KernelTestGroupArray {
    KernelTestGroup *data;
    size_t count;
} KernelTestGroupArray;

/**
 * @brief Benchmark executions associated with one DataView.
 *
 * The DataView is stored by value and remains non-owning with respect to
 * its DataBuffer and any external topology storage.
 */
typedef struct TestSet {
    TestSetUID uid;
    DataView view;
    KernelTestGroupArray kernels;
} TestSet;

/**
 * @brief Complete benchmark execution plan.
 *
 * A plan contains every configured DataView together with every available
 * kernel and its corresponding execution cases.
 */
typedef struct TestPlan {
    TestSet *data;
    size_t count;
} TestPlan;

/**
 * @brief Generate the proportional-scale benchmark plan.
 *
 * The generated plan follows the configured proportional workload strategy
 * and includes only kernel/view combinations reported as available by the
 * kernel registry.
 *
 * @param buffer DataBuffer from which the DataViews are built.
 * @param config Immutable benchmark configuration.
 *
 * @return A newly allocated TestPlan on success, or NULL on failure.
 *
 * @pre buffer != NULL
 * @pre config != NULL
 *
 * @post The returned plan owns all memory required by its internal arrays.
 * @post No unavailable kernel/view combination is inserted into the plan.
 */
TestPlan *generate_proportional_test_plan(
    DataBuffer *buffer,
    const Configuration *config
);

/**
 * @brief Generate the full-scale benchmark plan.
 *
 * The generated plan follows the configured full-scale workload strategy
 * and includes only kernel/view combinations reported as available by the
 * kernel registry.
 *
 * @param buffer DataBuffer from which the DataViews are built.
 * @param config Immutable benchmark configuration.
 *
 * @return A newly allocated TestPlan on success, or NULL on failure.
 *
 * @pre buffer != NULL
 * @pre config != NULL
 *
 * @post The returned plan owns all memory required by its internal arrays.
 * @post No unavailable kernel/view combination is inserted into the plan.
 */
TestPlan *generate_full_scale_test_plan(
    DataBuffer *buffer,
    const Configuration *config
);

/**
 * @brief Validate the structural integrity of a test plan.
 *
 * This function checks ownership-related structure and basic consistency
 * of the plan, including array bounds and kernel/view relationships.
 *
 * It is intended for setup-time validation and debugging only and must not
 * be called from the benchmark hot path.
 *
 * @param plan Test plan to validate.
 *
 * @return true if the plan is structurally valid, false otherwise.
 *
 * @note Validation does not execute kernels and does not measure
 *       performance.
 */
bool validate_test_plan(const TestPlan *plan);

/**
 * @brief Destroy a test plan and all memory owned by it.
 *
 * Passing NULL is allowed and has no effect.
 *
 * @param plan Test plan to destroy.
 */
void destroy_test_plan(TestPlan *plan);

#endif /* CORE_TEST_PLAN_H */