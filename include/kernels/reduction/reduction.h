#ifndef KERNELS_REDUCTION_H
#define KERNELS_REDUCTION_H

#include "kernels/registry.h"

void kernel_2d_reduction(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric);

void kernel_3d_reduction(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric);

void kernel_aos_reduction(DataView *view,
                          const TestCase *test_case,
                          Profiler *profiler,
                          PerformanceMetric *metric);

void kernel_soa_reduction(DataView *view,
                          const TestCase *test_case,
                          Profiler *profiler,
                          PerformanceMetric *metric);

void kernel_aosoa_reduction(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric);

void kernel_csr_reduction(DataView *view,
                          const TestCase *test_case,
                          Profiler *profiler,
                          PerformanceMetric *metric);

#endif
