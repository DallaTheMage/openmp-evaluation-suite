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
 * @file registry.h
 * @brief Static registry of benchmark kernels.
 *
 * The kernel registry provides the association between a logical
 * ViewType/KernelType pair and the corresponding kernel implementation.
 *
 * The registry is a compile-time/static data structure. Kernel availability
 * may depend on:
 *
 * - the DataView layout;
 * - the OpenMP version supported by the compiler;
 * - compiler-specific OpenMP capabilities;
 * - other compile-time capabilities exposed by the OpenMP configuration.
 *
 * An unavailable ViewType/KernelType combination is represented by a
 * descriptor whose function member is NULL.
 *
 * This design allows the test-plan generator and dispatcher to query kernel
 * availability without duplicating capability logic or maintaining
 * switch-based dispatch code.
 */

#ifndef KERNEL_REGISTRY_H
#define KERNEL_REGISTRY_H

#include <stdbool.h>

#include "data/views/ViewType.h"
#include "kernels/common/kernel.h"
#include "kernels/common/types.h"


/**
 * @brief Describes one registered kernel implementation.
 *
 * The descriptor associates a stable kernel identifier with its executable
 * implementation.
 *
 * A descriptor with a NULL function is considered unavailable.
 */
typedef struct KernelDescriptor {
    KernelType type;
    KernelFunc function;
} KernelDescriptor;


/**
 * @brief Static kernel registry.
 *
 * The first dimension is indexed by ViewType.
 * The second dimension is indexed by KernelType.
 *
 * Every valid entry must have a matching KernelType value.
 * An unavailable combination has function == NULL.
 *
 * The registry is owned by the registry implementation and must not be
 * modified by callers.
 */
extern const KernelDescriptor
    KERNEL_REGISTRY[VIEW_TYPE_COUNT][KERNEL_COUNT];


/**
 * @brief Returns the descriptor for a ViewType/KernelType combination.
 *
 * @param view_type DataView layout.
 * @param kernel_type Kernel identifier.
 *
 * @return Pointer to the corresponding static descriptor, or NULL if the
 *         supplied identifiers are outside their valid ranges.
 *
 * @note The returned descriptor remains owned by the registry and must not
 *       be modified or freed by the caller.
 */
const KernelDescriptor *kernel_registry_get(
    ViewType view_type,
    KernelType kernel_type
);


/**
 * @brief Returns the function implementing a kernel for a given view.
 *
 * @param view_type DataView layout.
 * @param kernel_type Kernel identifier.
 *
 * @return Registered KernelFunc, or NULL when the combination is not
 *         available in the current build.
 */
KernelFunc kernel_registry_get_function(
    ViewType view_type,
    KernelType kernel_type
);


/**
 * @brief Checks whether a kernel is available for a given view.
 *
 * @param view_type DataView layout.
 * @param kernel_type Kernel identifier.
 *
 * @return true if the combination has a registered implementation,
 *         false otherwise.
 */
bool kernel_registry_is_available(
    ViewType view_type,
    KernelType kernel_type
);


#endif /* KERNEL_REGISTRY_H */