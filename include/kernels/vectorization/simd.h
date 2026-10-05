#ifndef KERNELS_SIMD_H
#define KERNELS_SIMD_H

#include "kernels/registry.h"

void kernel_2d_simd(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_3d_simd(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric);

void kernel_aos_simd(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_soa_simd(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

void kernel_aosoa_simd(DataView *view,
                       const TestCase *test_case,
                       Profiler *profiler,
                       PerformanceMetric *metric);

void kernel_csr_simd(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric);

#endif
