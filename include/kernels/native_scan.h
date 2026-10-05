#ifndef KERNELS_CONSTRUCT_NATIVE_SCAN_H
#define KERNELS_CONSTRUCT_NATIVE_SCAN_H

#include "core/TestPlan.h"
#include "data/DataView.h"
#include "profiling/Profiler.h"

void kernel_2d_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_3d_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aos_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_soa_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aosoa_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_csr_native_scan(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

#endif /* KERNELS_CONSTRUCT_NATIVE_SCAN_H */
