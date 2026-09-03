#include <stddef.h>

#include "config/openmp.h"
#include "micro/microroutines.h"

#include "micro/basic.h"
#include "micro/matrix.h"
#include "micro/reduction.h"
#include "micro/synchronization.h"
#include "micro/tasks.h"
#include "micro/simd.h"


static const MicroRoutine microroutines[] = {
    {
        "Parallel for Arithmetic scale",
        routine_micro_scale
    },

    {
        "Parallel for best cache",
        routine_matrix_row_best
    },

    {
        "Parallel for worst cache",
        routine_matrix_col_worst
    },

#if OPENMP_HAS_COLLAPSE
    {
        "Parallel for collapse",
        routine_matrix_collapse
    },
#endif

    {
        "Parallel for",
        routine_for
    },

    {
        "Parallel for reduction",
        routine_reduction_sum
    },

#if OPENMP_HAS_SCAN
    {
        "Parallel inclusive scan",
        routine_scan_inclusive
    },
#endif

#if OPENMP_HAS_SECTIONS
    {
        "Sections",
        routine_sections
    },
#endif

#if OPENMP_HAS_CRITICAL
    {
        "Critical reduction",
        routine_critical_reduction
    },
#endif

#if OPENMP_HAS_ATOMIC
    {
        "Atomic reduction",
        routine_atomic_reduction
    },
#endif

#if OPENMP_HAS_BARRIER
    {
        "Barrier",
        routine_barrier
    },
#endif

#if OPENMP_HAS_NOWAIT
    {
        "Nowait",
        routine_nowait
    },
#endif

#if OPENMP_HAS_ORDERED
    {
        "Ordered",
        routine_ordered
    },
#endif

#if OPENMP_HAS_3_0
    {
        "Task",
        routine_task_divide_conquer
    },
#endif

#if OPENMP_HAS_TASKLOOP
    {
        "Taskloop",
        routine_taskloop_scale
    },
#endif

#if OPENMP_HAS_SIMD
    {
        "SIMD",
        routine_simd
    },

    {
        "Parallel for SIMD",
        routine_parallel_for_simd
    },
#endif

#if OPENMP_HAS_MASKED
    {
        "Masked",
        routine_masked
    },
#endif

#if OPENMP_HAS_LOOP
    {
        "Loop",
        routine_loop
    },
#endif
};


const MicroRoutine *get_microroutines(void)
{
    return microroutines;
}


size_t get_microroutines_count(void)
{
    return sizeof(microroutines) / sizeof(microroutines[0]);
}
