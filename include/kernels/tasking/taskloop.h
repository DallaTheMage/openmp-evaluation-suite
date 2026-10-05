#ifndef KERNELS_TASKLOOP_H
#define KERNELS_TASKLOOP_H

#include "kernels/registry.h"

void kernel_2d_taskloop(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

void kernel_3d_taskloop(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

void kernel_aos_taskloop(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric);

void kernel_soa_taskloop(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric);

void kernel_aosoa_taskloop(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric);

void kernel_csr_taskloop(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric);

#endif
