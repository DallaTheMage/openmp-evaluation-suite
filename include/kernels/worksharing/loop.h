#ifndef KERNELS_LOOP_H
#define KERNELS_LOOP_H

#include "kernels/registry.h"

void kernel_2d_loop(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_3d_loop(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_aos_loop(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_soa_loop(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_aosoa_loop(DataView *view,
                       const TestCase *test_case,
                       Profiler *profiler,
                       PerformanceMetric *metric);

void kernel_csr_loop(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

#endif
