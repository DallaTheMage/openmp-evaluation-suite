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
 * @file KernelRegistry.h
 * @brief Static registry of available benchmark kernel implementations.
 *
 * The registry is the single source of truth for the relationship between
 * a logical KernelType and a DataView layout. Availability is represented by
 * a NULL function pointer.
 *
 * The registry is immutable after program initialization. Callers receive
 * read-only descriptors and must not modify or free them.
 */

#ifndef KERNELS_REGISTRY_H
#define KERNELS_REGISTRY_H

#include <stdbool.h>

#include "data/views/ViewType.h"
#include "kernels/common/Kernel.h"
#include "kernels/common/KernelType.h"

/**
 * @brief Describes one KernelType/ViewType registration.
 *
 * The KernelType and ViewType are already encoded by the registry indices,
 * so the descriptor only stores the executable implementation.
 */
typedef struct KernelDescriptor {
    /** Concrete implementation, or NULL when unavailable. */
    KernelFunc function;
} KernelDescriptor;

/**
 * @brief Static kernel registry.
 *
 * The first dimension is indexed by ViewType and the second by KernelType.
 * Entries not supported by the current implementation or OpenMP version
 * contain a NULL function pointer.
 */
extern const KernelDescriptor
    KERNEL_REGISTRY[VIEW_TYPE_COUNT][KERNEL_COUNT];

/**
 * @brief Look up a kernel descriptor.
 *
 * @param view_type DataView layout.
 * @param kernel_type Logical kernel identifier.
 * @return Pointer to the static descriptor, or NULL for invalid identifiers.
 */
const KernelDescriptor *kernel_registry_get(
    ViewType view_type,
    KernelType kernel_type
);

/**
 * @brief Return the implementation registered for a kernel/view pair.
 *
 * @param view_type DataView layout.
 * @param kernel_type Logical kernel identifier.
 * @return Kernel function, or NULL when unavailable or invalid.
 */
KernelFunc kernel_registry_get_function(
    ViewType view_type,
    KernelType kernel_type
);

/**
 * @brief Test whether a kernel/view combination is available.
 *
 * @param view_type DataView layout.
 * @param kernel_type Logical kernel identifier.
 * @return true when a concrete implementation is registered.
 */
bool kernel_registry_is_available(
    ViewType view_type,
    KernelType kernel_type
);

#endif /* KERNELS_REGISTRY_H */
