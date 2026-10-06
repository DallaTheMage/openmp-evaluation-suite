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
 * @file KernelType.h
 * @brief Stable identifiers for benchmark kernels.
 *
 * KernelType identifies the logical benchmark performed by the suite.
 * The identifier is independent of the DataView layout: the same kernel
 * type may have implementations for several view types.
 *
 * The registry maps a KernelType/ViewType pair to the concrete function
 * implementing that combination. Unsupported combinations are simply
 * left unavailable in the registry.
 */

#ifndef KERNELS_COMMON_KERNEL_TYPE_H
#define KERNELS_COMMON_KERNEL_TYPE_H


/**
 * @brief Identifies one logical benchmark kernel.
 *
 * The enumeration covers the OpenMP constructs and algorithmic kernels
 * currently implemented by the suite. Values are used as indices in the
 * static kernel registry and therefore must remain unique.
 */
typedef enum KernelType {
    KERNEL_LOOP = 0,
    KERNEL_COLLAPSE,
    KERNEL_SECTIONS,
    KERNEL_SIMD_MEMORY,
    KERNEL_SIMD_COMPUTE,
    KERNEL_MASTER,
    KERNEL_MASKED,
    KERNEL_ATOMIC,
    KERNEL_CRITICAL,
    KERNEL_ORDERED,
    KERNEL_SYNC,
    KERNEL_REDUCTION,
    KERNEL_TASK,
    KERNEL_TASKLOOP,
    KERNEL_NATIVE_SCAN,
    KERNEL_TWO_PASS_SCAN,

    /** Number of valid kernel identifiers. */
    KERNEL_COUNT
} KernelType;


/*
 * Compatibility aliases for the previous generic names. They intentionally
 * do not create additional registry entries.
 */
#define KERNEL_TASKING KERNEL_TASKLOOP
#define KERNEL_SIMD    KERNEL_SIMD_MEMORY


#endif /* KERNELS_COMMON_KERNEL_TYPE_H */
