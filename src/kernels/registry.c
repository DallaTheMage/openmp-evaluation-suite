#include "kernels/registry.h"
#include "config/openmp.h"

#include "kernels/common/kernel.h"
#include "kernels/worksharing/loop.h"
#include "kernels/worksharing/collapse.h"
#include "kernels/worksharing/sections.h"
#include "kernels/vectorization/simd.h"
#include "kernels/synchronization/master_masked.h"
#include "kernels/synchronization/atomic.h"
#include "kernels/synchronization/critical.h"
#include "kernels/synchronization/ordered.h"
#include "kernels/tasking/taskloop.h"
#include "kernels/tasking/task.h"
#include "kernels/reduction/reduction.h"
#include "kernels/algorithms/native_scan.h"
#include "kernels/algorithms/two_pass_scan.h"


const KernelFunc KERNEL_REGISTRY[VIEW_TYPE_COUNT][KERNEL_COUNT] = {
    [VIEW_2D] = {
        [KERNEL_LOOP] = kernel_2d_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_2d_reduction,
        [KERNEL_SYNC]      = kernel_2d_sync,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_2d_taskloop,
    #endif

    #if OPENMP_HAS_SIMD
        [KERNEL_SIMD] = kernel_2d_simd,
    #endif

    #if OPENMP_HAS_NATIVE_SCAN
        [KERNEL_NATIVE_SCAN] = kernel_2d_native_scan,
    #endif

    #if OPENMP_HAS_TWO_PASS_SCAN
        [KERNEL_TWO_PASS_SCAN] = kernel_2d_two_pass_scan,
    #endif
        },

    [VIEW_3D] = {
        [KERNEL_LOOP] = kernel_3d_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_3d_reduction,
        [KERNEL_SYNC]      = kernel_3d_sync,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_3d_taskloop,
    #endif

    #if OPENMP_HAS_SIMD
        [KERNEL_SIMD] = kernel_3d_simd,
    #endif
        },

    [VIEW_AOS] = {
        [KERNEL_LOOP] = kernel_aos_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_aos_reduction,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_aos_taskloop,
    #endif
        },

    [VIEW_SOA] = {
        [KERNEL_LOOP] = kernel_soa_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_soa_reduction,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_soa_taskloop,
    #endif
    },

    [VIEW_AOSOA] = {
        [KERNEL_LOOP] = kernel_aosoa_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_aosoa_reduction,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_aosoa_taskloop,
    #endif

    #if OPENMP_HAS_SIMD
        [KERNEL_SIMD] = kernel_aosoa_simd,
    #endif
    },

    [VIEW_CSR] = {
        [KERNEL_LOOP] = kernel_csr_loop,

    #if OPENMP_HAS_2_0
        [KERNEL_REDUCTION] = kernel_csr_reduction,
    #endif

    #if OPENMP_HAS_TASKLOOP
        [KERNEL_TASKING] = kernel_csr_taskloop,
    #endif

    #if OPENMP_HAS_NATIVE_SCAN
        [KERNEL_NATIVE_SCAN] = kernel_csr_native_scan,
    #endif

    #if OPENMP_HAS_TWO_PASS_SCAN
        [KERNEL_TWO_PASS_SCAN] = kernel_csr_two_pass_scan,
    #endif
    }
};
