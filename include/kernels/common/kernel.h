#ifndef KERNELS_COMMON_KERNEL_H
#define KERNELS_COMMON_KERNEL_H

#include "data/DataView.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"


/*
 * Generic OpenMP kernel interface.
 *
 * Every benchmark kernel receives:
 *
 *   view     - data/layout being processed
 *   test_case - execution configuration
 *   profiler - performance profiler
 *   metric   - output metric
 */
typedef void (*KernelFunc)(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

#endif /* KERNELS_COMMON_KERNEL_H */
