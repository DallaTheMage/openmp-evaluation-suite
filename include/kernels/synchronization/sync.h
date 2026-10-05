#ifndef KERNELS_TASK_H
#define KERNELS_TASK_H

#include "kernels/registry.h"

void kernel_2d_task(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_3d_task(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_aos_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_soa_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_aosoa_task(DataView *view,
                       const TestCase *test_case,
                       Profiler *profiler,
                       PerformanceMetric *metric);

void kernel_csr_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

#endif
