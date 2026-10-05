#ifndef KERNELS_CONSTRUCT_SYNC_H
#define KERNELS_CONSTRUCT_SYNC_H

#include "core/TestPlan.h"
#include "data/DataView.h"
#include "profiling/Profiler.h"

void kernel_2d_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_3d_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aos_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_soa_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aosoa_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_csr_sync(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

#endif /* KERNELS_CONSTRUCT_SYNC_H */
