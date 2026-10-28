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

/**
 * @file registry.c
 * @brief Static registration and lookup of benchmark kernels.
 */

#include "kernels/KernelRegistry.h"
#include "kernels/algorithms/gemm.h"

#include "config/openmp.h"
#include "kernels/reduction/reduction.h"
#include "kernels/scan/native.h"
#include "kernels/scan/two_pass.h"
#include "kernels/synchronization/atomic.h"
#include "kernels/synchronization/critical.h"
#include "kernels/synchronization/master_masked.h"
#include "kernels/synchronization/ordered.h"
#include "kernels/synchronization/sync.h"
#include "kernels/tasking/task.h"
#include "kernels/tasking/taskloop.h"
#include "kernels/vectorization/simd.h"
#include "kernels/worksharing/collapse.h"
#include "kernels/worksharing/loop.h"
#include "kernels/worksharing/sections.h"
#include "kernels/algorithms/jacobi.h"
#include "kernels/algorithms/cg.h"

#define KERNEL_WITH_CHUNK(fn) { .function = (fn), .is_chunk_supported = true }
#define KERNEL_WITHOUT_CHUNK(fn) { .function = (fn), .is_chunk_supported = false }

/*
 * The registry is indexed by ViewType first and KernelType second.
 * Unspecified entries are initialized to zero, which means that their
 * function member is NULL and the combination is unavailable.
 */
const KernelDescriptor
    KERNEL_REGISTRY[VIEW_TYPE_COUNT][KERNEL_COUNT] = {
    [VIEW_1D] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_1d_loop),
                [KERNEL_SECTIONS]  = KERNEL_WITHOUT_CHUNK(kernel_1d_sections),
                [KERNEL_MASTER]    = KERNEL_WITHOUT_CHUNK(kernel_1d_master),
                [KERNEL_ORDERED]   = KERNEL_WITH_CHUNK(kernel_1d_ordered),
                [KERNEL_SYNC]      = KERNEL_WITH_CHUNK(kernel_1d_sync),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_1d_reduction),
                [KERNEL_CRITICAL]  = KERNEL_WITH_CHUNK(kernel_1d_critical),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_TASK]      = KERNEL_WITH_CHUNK(kernel_1d_task),
        #endif
        #if OPENMP_HAS_3_1
                [KERNEL_ATOMIC]    = KERNEL_WITH_CHUNK(kernel_1d_atomic),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_1d_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_1d_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_1d_taskloop),
        #endif
        #if OPENMP_HAS_MASKED
                [KERNEL_MASKED] = KERNEL_WITHOUT_CHUNK(kernel_1d_masked),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_1d_native_scan),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_1d_two_pass_scan),
        #endif
    },

    [VIEW_2D] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_2d_loop),
                [KERNEL_GEMM]      = KERNEL_WITH_CHUNK(kernel_2d_gemm),
                [KERNEL_JACOBI]    = KERNEL_WITH_CHUNK(kernel_2d_jacobi),
                [KERNEL_CG]        = KERNEL_WITH_CHUNK(kernel_2d_cg),
                [KERNEL_SECTIONS]  = KERNEL_WITHOUT_CHUNK(kernel_2d_sections),
                [KERNEL_MASTER]    = KERNEL_WITHOUT_CHUNK(kernel_2d_master),
                [KERNEL_ORDERED]   = KERNEL_WITH_CHUNK(kernel_2d_ordered),
                [KERNEL_SYNC]      = KERNEL_WITH_CHUNK(kernel_2d_sync),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_2d_reduction),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_COLLAPSE]  = KERNEL_WITH_CHUNK(kernel_2d_collapse),
                [KERNEL_TASK]      = KERNEL_WITH_CHUNK(kernel_2d_task),
        #endif
        #if OPENMP_HAS_3_1
                [KERNEL_ATOMIC]    = KERNEL_WITH_CHUNK(kernel_2d_atomic),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_2d_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_2d_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_2d_taskloop),
        #endif
        #if OPENMP_HAS_MASKED
                [KERNEL_MASKED] = KERNEL_WITHOUT_CHUNK(kernel_2d_masked),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_2d_native_scan),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_2d_two_pass_scan),
        #endif
            },

        [VIEW_3D] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_3d_loop),
                [KERNEL_SYNC]      = KERNEL_WITH_CHUNK(kernel_3d_sync),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_3d_reduction),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_COLLAPSE] = KERNEL_WITH_CHUNK(kernel_3d_collapse),
                [KERNEL_TASK]     = KERNEL_WITH_CHUNK(kernel_3d_task),
        #endif
        #if OPENMP_HAS_3_1
                [KERNEL_ATOMIC]   = KERNEL_WITH_CHUNK(kernel_3d_atomic),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_3d_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_3d_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_3d_taskloop),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_3d_two_pass_scan),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_3d_native_scan),
        #endif
            },

        [VIEW_AOS] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_aos_loop),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_aos_reduction),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_TASK] = KERNEL_WITH_CHUNK(kernel_aos_task),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_aos_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_aos_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_aos_taskloop),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_aos_two_pass_scan),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_aos_native_scan),
        #endif
            },

        [VIEW_SOA] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_soa_loop),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_soa_reduction),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_TASK] = KERNEL_WITH_CHUNK(kernel_soa_task),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_soa_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_soa_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_soa_taskloop),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_soa_two_pass_scan),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_soa_native_scan),
        #endif
            },

        [VIEW_AOSOA] = {
        #if OPENMP_HAS_2_0
                [KERNEL_LOOP]      = KERNEL_WITH_CHUNK(kernel_aosoa_loop),
                [KERNEL_REDUCTION] = KERNEL_WITH_CHUNK(kernel_aosoa_reduction),
        #endif
        #if OPENMP_HAS_3_0
                [KERNEL_TASK] = KERNEL_WITH_CHUNK(kernel_aosoa_task),
        #endif
        #if OPENMP_HAS_SIMD
                [KERNEL_SIMD_MEMORY]  = KERNEL_WITH_CHUNK(kernel_aosoa_simd_memory),
                [KERNEL_SIMD_COMPUTE] = KERNEL_WITH_CHUNK(kernel_aosoa_simd_compute),
        #endif
        #if OPENMP_HAS_TASKLOOP
                [KERNEL_TASKLOOP] = KERNEL_WITH_CHUNK(kernel_aosoa_taskloop),
        #endif
        #if OPENMP_HAS_TWO_PASS_SCAN
                [KERNEL_TWO_PASS_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_aosoa_two_pass_scan),
        #endif
        #if OPENMP_HAS_NATIVE_SCAN
                [KERNEL_NATIVE_SCAN] = KERNEL_WITHOUT_CHUNK(kernel_aosoa_native_scan),
        #endif
            },
        };

#undef KERNEL_WITH_CHUNK
#undef KERNEL_WITHOUT_CHUNK

const KernelDescriptor *kernel_registry_get(ViewType view_type, KernelType kernel_type) {
    if ((unsigned) view_type >= VIEW_TYPE_COUNT ||
        (unsigned) kernel_type >= KERNEL_COUNT) {
        return NULL;
    }
    return &KERNEL_REGISTRY[view_type][kernel_type];
}

KernelFunc kernel_registry_get_function(ViewType view_type, KernelType kernel_type) {
    const KernelDescriptor *descriptor = kernel_registry_get(view_type, kernel_type);
    return descriptor != NULL ? descriptor->function : NULL;
}

bool kernel_registry_is_available(ViewType view_type, KernelType kernel_type) {
    return kernel_registry_get_function(view_type, kernel_type) != NULL;
}
