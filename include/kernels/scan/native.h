#ifndef KERNELS_SCAN_H
#define KERNELS_SCAN_H

#include "kernels/registry.h"

void kernel_2d_native_scan(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric);

void kernel_3d_native_scan(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric);

void kernel_aos_native_scan(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric);

void kernel_soa_native_scan(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric);

void kernel_aosoa_native_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric);

void kernel_csr_native_scan(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric);


void kernel_2d_two_pass_scan(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric);

void kernel_3d_two_pass_scan(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric);

void kernel_aos_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric);

void kernel_soa_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric);

void kernel_aosoa_two_pass_scan(DataView *view,
                                const TestCase *test_case,
                                Profiler *profiler,
                                PerformanceMetric *metric);

void kernel_csr_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric);

#endif
