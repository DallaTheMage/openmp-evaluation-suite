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
#include "core/internal/TestPlanLifecycle.h"

/**
 * @brief Destroy the contents of one kernel test group.
 *
 * @param group Kernel test group to destroy.
 */
static void destroy_kernel_group(KernelTestGroup *group) {
    if (group == NULL) {
        return;
    }
    free(group->cases.data);
    group->cases.data = NULL;
    group->cases.count = 0U;
}


/**
 * @brief Destroy all kernel groups owned by an array.
 *
 * @param groups Kernel group array to destroy.
 */
void test_plan_destroy_kernel_groups(KernelTestGroupArray *groups) {
    if (groups == NULL) {
        return;
    }
    for (size_t i = 0U; i < groups->count; i++) {
        destroy_kernel_group(&groups->data[i]);
    }
    free(groups->data);
    groups->data = NULL;
    groups->count = 0U;
}


/**
 * @brief Destroy one test set.
 *
 * DataView is non-owning, so only its kernel groups require destruction.
 *
 * @param test_set Test set to destroy.
 */
static void destroy_test_set(TestSet *test_set) {
    if (test_set == NULL) {
        return;
    }
    test_plan_destroy_kernel_groups(&test_set->kernels);
    test_set->view = (DataView){0};
}


/* ========================================================================= */
/* Public API                                                                */
/* ========================================================================= */

bool test_plan_max_workload(
    const TestPlan *plan,
    size_t *out_elements) {
    size_t maximum = 0U;

    if (plan == NULL
        || out_elements == NULL
        || (plan->count > 0U && plan->data == NULL)) {
        return false;
    }

    for (size_t i = 0U; i < plan->count; i++) {
        const uint64_t size = plan->data[i].view.window.size;
        if (size > (uint64_t)SIZE_MAX) {
            return false;
        }
        if ((size_t)size > maximum) {
            maximum = (size_t)size;
        }
    }
    *out_elements = maximum;
    return true;
}


bool test_plan_rebind_buffer(
    TestPlan *plan,
    DataBuffer *buffer) {
    if (plan == NULL
        || buffer == NULL
        || (plan->count > 0U && plan->data == NULL)) {
        return false;
    }

    for (size_t i = 0U; i < plan->count; i++) {
        const DataView *view = &plan->data[i].view;
        const uint64_t size  = view->window.size;

        if (size > (uint64_t)SIZE_MAX ||
            (size_t)size > buffer->element_count) {
            return false;
        }
    }

    for (size_t i = 0U; i < plan->count; i++) {
        plan->data[i].view.buffer = buffer;
    }

    return true;
}
/**
 * @brief Destroy a test plan and all resources owned by it.
 *
 * DataViews are non-owning references and are not destroyed here.
 *
 * @param plan Test plan to destroy.
 */
void destroy_test_plan(TestPlan *plan) {
    if (plan != NULL) {
        for (size_t i = 0U; i < plan->count; i++) {
            destroy_test_set(&plan->data[i]);
        }

        free(plan->data);
        free(plan);
    }
    return;
}