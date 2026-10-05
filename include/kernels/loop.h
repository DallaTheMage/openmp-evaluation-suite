#ifndef KERNELS_CONSTRUCT_LOOP_H
#define KERNELS_CONSTRUCT_LOOP_H

#include "core/TestPlan.h"
#include "data/DataView.h"
#include "profiling/Profiler.h"

void kernel_2d_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_3d_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aos_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_soa_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_aosoa_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

void kernel_csr_loop(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

#endif /* KERNELS_CONSTRUCT_LOOP_H */
