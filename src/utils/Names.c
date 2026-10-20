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
 * @file Names.c
 * @brief Human-readable names for views and kernels.
 */

/* Shared human-readable labels for views and kernels.
 * CSV output deliberately retains its established spellings.
 */
#include "utils/Names.h"

const char *utils_view_type_name(ViewType type)
{
    switch (type) {
        case VIEW_1D:
            return "1D";
        case VIEW_2D:
            return "2D";
        case VIEW_3D:
            return "3D";
        case VIEW_AOS:
            return "AoS";
        case VIEW_SOA:
            return "SoA";
        case VIEW_AOSOA:
            return "AoSoA";
        default:
            return "Unknown";
    }
}

const char *utils_kernel_type_name(KernelType type) {
    switch (type) {
        case KERNEL_LOOP:
            return "Loop";
        case KERNEL_LOOP_SOA_INNER:
            return "Loop SoA Inner";
        case KERNEL_REDUCTION_SOA_INNER:
            return "Reduction SoA Inner";
        case KERNEL_COLLAPSE:
            return "Collapse";
        case KERNEL_SECTIONS:
            return "Sections";
        case KERNEL_SIMD_MEMORY:
            return "SIMD Memory";
        case KERNEL_SIMD_COMPUTE:
            return "SIMD Compute";
        case KERNEL_MASTER:
            return "Master";
        case KERNEL_MASKED:
            return "Masked";
        case KERNEL_ATOMIC:
            return "Atomic";
        case KERNEL_CRITICAL:
            return "Critical";
        case KERNEL_ORDERED:
            return "Ordered";
        case KERNEL_SYNC:
            return "Sync";
        case KERNEL_REDUCTION:
            return "Reduction";
        case KERNEL_TASK:
            return "Task";
        case KERNEL_TASKLOOP:
            return "Taskloop";
        case KERNEL_NATIVE_SCAN:
            return "Native Scan";
        case KERNEL_GEMM:
            return "GEMM";
        case KERNEL_CG:
            return "Conjugate Gradient";
        case KERNEL_JACOBI:
            return "Jacobi 2D";
        case KERNEL_TWO_PASS_SCAN:
            return "Two-Pass Scan";
        default:
            return "Unknown";
    }
}
