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
 * @file types.h
 * @brief Common kernel identifiers.
 *
 * This header defines the stable identifiers used to select benchmark
 * kernels throughout the benchmark suite.
 *
 * The identifiers are intentionally independent from the execution,
 * profiling and analysis layers.
 */

#ifndef KERNELS_COMMON_TYPES_H
#define KERNELS_COMMON_TYPES_H


/**
 * @brief Identifies a benchmark kernel.
 *
 * The enumeration values are stable identifiers used by test plans,
 * kernel dispatch and result analysis.
 */
typedef enum KernelType {
    KERNEL_LOOP = 0,
    KERNEL_REDUCTION,
    KERNEL_SYNC,
    KERNEL_TASKING,
    KERNEL_SIMD,
    KERNEL_NATIVE_SCAN,
    KERNEL_TWO_PASS_SCAN,

    /** Number of valid kernel identifiers. */
    KERNEL_COUNT
} KernelType;


#endif /* KERNELS_COMMON_TYPES_H */