#ifndef KERNELS_CRITICAL_H
#define KERNELS_CRITICAL_H

#include "kernels/registry.h"

void kernel_2d_critical(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

void kernel_3d_critical(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

#endif
