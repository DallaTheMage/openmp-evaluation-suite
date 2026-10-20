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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/TestPlan.h"
typedef struct Profiler Profiler;
typedef struct PerformanceMetric PerformanceMetric;
#include "kernels/KernelRegistry.h"

/**
 * @brief Validate the structure and contents of a test plan.
 *
 * @param plan Test plan to validate.
 *
 * @return true if the plan is valid, false otherwise.
 */
bool validate_test_plan(const TestPlan *plan) {
    if (plan == NULL) {
        return false;
    }

    if (plan->count > 0U
        && plan->data == NULL) {
        return false;
    }

    for (size_t set_index = 0U; set_index < plan->count; ++set_index) {
        const TestSet *test_set = &plan->data[set_index];
        if (test_set->uid != (TestSetUID)set_index
            || test_set->view.buffer == NULL
            || test_set->view.type >= VIEW_TYPE_COUNT
            || test_set->view.window.size == UINT64_C(0)) {
            return false;
        }

        if (test_set->kernels.count > 0U
            && test_set->kernels.data == NULL) {
            return false;
        }

        for (size_t group_index = 0U; group_index < test_set->kernels.count; ++group_index) {
            const KernelTestGroup *group = &test_set->kernels.data[group_index];
            if (group->kernel >= KERNEL_COUNT ||
                !kernel_registry_is_available(
                    test_set->view.type,
                    group->kernel)) {
                return false;
            }

            if (group->cases.count > 0U && group->cases.data == NULL) {
                return false;
            }

            for (size_t case_index = 0U; case_index < group->cases.count; ++case_index) {
                const TestCase *test_case = &group->cases.data[case_index];
                if (test_case->uid != (TestCaseUID)case_index
                    || test_case->num_threads == 0U
                    || test_case->chunk_size == UINT64_C(0)) {
                    return false;
                }
            }
        }
    }

    return true;
}
