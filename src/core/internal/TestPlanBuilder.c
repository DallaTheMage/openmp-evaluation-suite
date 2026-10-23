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
#include "kernels/KernelRegistry.h"

typedef struct Profiler Profiler;
typedef struct PerformanceMetric PerformanceMetric;

/**
 * @brief Convert a configured shape to a DataView creation descriptor.
 *
 * @param destination Destination descriptor.
 * @param source Configuration shape.
 * @param type Selected view type.
 * @param size Logical view size in elements.
 *
 * @return true on success, false for an invalid view type or argument.
 */
static bool make_view_create_config(
    ViewCreateConfig *destination,
    const ViewShapeConfig *source,
    ViewType type,
    uint64_t size) {

    if (destination == NULL
        || source == NULL
        || size == UINT64_C(0)) {
        return false;
    }

    *destination = (ViewCreateConfig){0};

    switch (type) {
        case VIEW_1D:
            destination->v1d.length = size;
            break;

        case VIEW_2D:
            destination->v2d.cols = source->v2d.cols;
            break;

        case VIEW_3D:
            destination->v3d.depth = source->v3d.depth;
            destination->v3d.cols = source->v3d.cols;
            break;

        case VIEW_AOS:
            destination->aos.struct_size = source->aos.struct_size;
            break;

        case VIEW_SOA:
            destination->soa.num_fields = source->soa.num_fields;
            break;

        case VIEW_AOSOA:
            destination->aosoa.vector_length = source->aosoa.vector_length;
            destination->aosoa.num_fields = source->aosoa.num_fields;
            break;

        case VIEW_TYPE_COUNT:
        default:
            return false;
    }

    return true;
}


/**
 * @brief Create one DataView for a test set.
 *
 * @param test_set Destination test set.
 * @param buffer Referenced data buffer.
 * @param config Benchmark configuration.
 * @param type View type.
 * @param size Logical view size in elements.
 *
 * @return true on success, false otherwise.
 */
static bool initialize_test_set_view(
    TestSet *test_set,
    DataBuffer *buffer,
    const Configuration *config,
    ViewType type,
    uint64_t size) {
    ViewCreateConfig view_config;

    if (test_set == NULL ||
        buffer == NULL ||
        config == NULL) {
        return false;
    }

    if (!make_view_create_config(
            &view_config,
            &config->shapes[type],
            type,
            size)) {
        return false;
    }

    test_set->view = create_data_view(
        buffer,
        type,
        &view_config,
        UINT64_C(0),
        size
    );

    if (test_set->view.buffer == NULL) {
        if (type == VIEW_2D) {
            fprintf(stderr,
                    "[TestPlan] Invalid 2D view: window=%llu, cols=%llu. "
                    "Require window >= cols and window %% cols == 0.\n",
                    (unsigned long long)size,
                    (unsigned long long)view_config.v2d.cols);
        } else if (type == VIEW_3D) {
            fprintf(stderr,
                    "[TestPlan] Invalid 3D view: window=%llu, depth=%llu, cols=%llu. "
                    "Require window to be a positive multiple of depth*cols "
                    "(without overflow).\n",
                    (unsigned long long)size,
                    (unsigned long long)view_config.v3d.depth,
                    (unsigned long long)view_config.v3d.cols);
        }
        return false;
    }
    return true;
}


/**
 * @brief Populate one kernel group for a test set.
 *
 * @param test_set Destination test set.
 * @param config Benchmark configuration.
 * @param kernel_type Kernel type to populate.
 * @param fixed_thread_count Fixed thread count, or zero for all counts.
 *
 * @return true on success, false on allocation failure or invalid input.
 */
static bool populate_kernel_group(
    TestSet *test_set,
    const Configuration *config,
    KernelType kernel_type,
    uint32_t fixed_thread_count) {

    KernelTestGroup *group;
    size_t case_count = 0U;
    size_t case_index = 0U;

    if (test_set == NULL || config == NULL) {
        return false;
    }

    group = &test_set->kernels.data[test_set->kernels.count];
    group->kernel = kernel_type;

    if (fixed_thread_count != 0U) {
        for (size_t i = 0U; i < config->execution.threads.count; i++) {
            if (config->execution.threads.data[i] == fixed_thread_count) {
                case_count = config->execution.chunk_sizes.count;
                break;
            }
        }
    } else if (config->execution.chunk_sizes.count == 0U) {
        case_count = 0U;
    } else {
        if (config->execution.threads.count >
            SIZE_MAX / config->execution.chunk_sizes.count) {
            return false;
        }

        case_count = config->execution.threads.count *
                     config->execution.chunk_sizes.count;
    }

    if (case_count == 0U) {
        test_set->kernels.count++;
    } else {
        if (case_count > (SIZE_MAX / sizeof(*group->cases.data))) {
            return false;
        }
        group->cases.data = calloc(case_count, sizeof(*group->cases.data));
        if (group->cases.data == NULL) {
            return false;
        }
        for (size_t i = 0U; i < config->execution.threads.count; i++) {
            const uint32_t num_threads = config->execution.threads.data[i];
            if (fixed_thread_count == 0U || num_threads == fixed_thread_count) {
                for (size_t c = 0U; c < config->execution.chunk_sizes.count; c++) {
                    TestCase *test_case    = &group->cases.data[case_index];
                    test_case->uid         = (TestCaseUID)case_index;
                    test_case->num_threads = num_threads;
                    test_case->chunk_size  = config->execution.chunk_sizes.data[c];
                    case_index++;
                }
            }
        }
        group->cases.count = case_index;
        test_set->kernels.count++;
    }

    return true;
}


/**
 * @brief Populate all available kernel groups for one test set.
 *
 * @param test_set Test set to populate.
 * @param config Benchmark configuration.
 * @param fixed_thread_count Fixed thread count, or zero for all counts.
 *
 * @return true on success, false on allocation failure or invalid input.
 */
static bool populate_test_set_kernels(
    TestSet *test_set,
    const Configuration *config,
    uint32_t fixed_thread_count) {

    size_t group_count = 0U;

    if (test_set == NULL || config == NULL) {
        return false;
    }

    for (size_t kernel = 0U; kernel < (size_t)KERNEL_COUNT; ++kernel) {
        if (kernel_registry_is_available(test_set->view.type, (KernelType)kernel)) {
            ++group_count;
        }
    }

    if (group_count == 0U) {
        return true;
    }

    test_set->kernels.data = calloc(
        group_count,
        sizeof(*test_set->kernels.data)
    );

    if (test_set->kernels.data == NULL) {
        return false;
    }

    for (size_t kernel = 0U; kernel < (size_t)KERNEL_COUNT; ++kernel) {
        const KernelType kernel_type = (KernelType)kernel;
        if (kernel_registry_is_available(test_set->view.type, kernel_type)) {
            if (!populate_kernel_group(test_set, config, kernel_type, fixed_thread_count)) {
                test_plan_destroy_kernel_groups(&test_set->kernels);
                return false;
            }
        }
    }

    return true;
}


/**
 * @brief Allocate the top-level TestSet array.
 *
 * @param count Number of test sets.
 *
 * @return Allocated array, or NULL on failure.
 */
static TestSet *allocate_test_sets(size_t count) {
    if (count == 0U ||
        count > SIZE_MAX / sizeof(TestSet)) {
        return NULL;
    }

    return calloc(count, sizeof(TestSet));
}


/**
 * @brief Build a test plan from a sequence of logical workload sizes.
 *
 * @param buffer Referenced DataBuffer.
 * @param config Benchmark configuration.
 * @param sizes Workload sizes.
 * @param size_count Number of workload sizes.
 * @param proportional True when each size corresponds to one thread count.
 *
 * @return Newly allocated TestPlan, or NULL on failure.
 */
static TestPlan *build_test_plan(
    DataBuffer *buffer,
    const Configuration *config,
    const uint64_t *sizes,
    size_t size_count,
    bool proportional) {

    TestPlan *plan;
    size_t set_count;
    size_t set_index = 0U;

    if (buffer == NULL ||
        config == NULL ||
        sizes == NULL ||
        size_count == 0U) {
        return NULL;
    }
    if (proportional &&
        size_count != config->execution.threads.count) {
        return NULL;
    }
    if (size_count > SIZE_MAX / (size_t)VIEW_TYPE_COUNT) {
        return NULL;
    }
    set_count = size_count * (size_t)VIEW_TYPE_COUNT;
    plan = calloc(1U, sizeof(*plan));

    if (plan == NULL) {
        return NULL;
    }

    plan->data = allocate_test_sets(set_count);

    if (plan->data == NULL) {
        free(plan);
        return NULL;
    }
    plan->count = set_count;
    for (size_t i = 0U; i < size_count; ++i) {
        const uint32_t fixed_thread_count = proportional
            ? config->execution.threads.data[i]
            : 0U;

        if (sizes[i] == UINT64_C(0) ||
            sizes[i] > (uint64_t)buffer->element_count) {
            destroy_test_plan(plan);
            return NULL;
        }

        for (size_t type_index = 0U; type_index < (size_t)VIEW_TYPE_COUNT; ++type_index) {
            TestSet *test_set   = &plan->data[set_index];
            const ViewType type = (ViewType)type_index;
            test_set->uid       = (TestSetUID)set_index;

            if (!initialize_test_set_view(test_set, buffer, config, type, sizes[i])) {
                destroy_test_plan(plan);
                return NULL;
            }
            if (!populate_test_set_kernels(test_set,config,fixed_thread_count)) {
                destroy_test_plan(plan);
                return NULL;
            }
            ++set_index;
        }
    }
    return plan;
}


/**
 * @brief Generate a proportional test plan.
 *
 * @param buffer Data buffer referenced by the plan.
 * @param config Benchmark configuration.
 *
 * @return Newly allocated test plan, or NULL on failure.
 */
TestPlan *generate_proportional_test_plan(DataBuffer *buffer, const Configuration *config) {
    uint64_t *sizes;
    TestPlan *plan;

    if (buffer == NULL
        || config == NULL
        || config->execution.threads.count == 0U
        || config->derived.size_per_thread == UINT64_C(0)) {
        return NULL;
    }

    if (config->execution.threads.count > SIZE_MAX / sizeof(*sizes)) {
        return NULL;
    }
    sizes = malloc(config->execution.threads.count * sizeof(*sizes));
    if (sizes == NULL) {
        return NULL;
    }
    for (size_t i = 0U; i < config->execution.threads.count; ++i) {
        const uint64_t threads = (uint64_t)config->execution.threads.data[i];
        if (threads == UINT64_C(0)
            || config->derived.size_per_thread > UINT64_MAX / threads) {
            free(sizes);
            return NULL;
        }
        sizes[i] = config->derived.size_per_thread * threads;
        if (sizes[i] > (uint64_t)buffer->element_count) {
            free(sizes);
            return NULL;
        }
    }
    plan = build_test_plan(buffer, config, sizes, config->execution.threads.count, true);
    free(sizes);
    return plan;
}


/**
 * @brief Generate a full-scale test plan.
 *
 * @param buffer Data buffer referenced by the plan.
 * @param config Benchmark configuration.
 *
 * @return Newly allocated test plan, or NULL on failure.
 */
TestPlan *generate_full_scale_test_plan(DataBuffer *buffer, const Configuration *config) {
    if (buffer == NULL
        || config == NULL
        || config->execution.window_sizes.count == 0U) {
        return NULL;
    }
    for (size_t i = 0U; i < config->execution.window_sizes.count; ++i) {
        const uint64_t size = config->execution.window_sizes.data[i];
        if (size == UINT64_C(0) || size > (uint64_t)buffer->element_count) {
            return NULL;
        }
    }
    return build_test_plan(
        buffer,
        config,
        config->execution.window_sizes.data,
        config->execution.window_sizes.count,
        false
    );
}
