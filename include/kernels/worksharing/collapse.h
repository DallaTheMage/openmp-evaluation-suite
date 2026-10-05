#ifndef KERNELS_COLLAPSE_H
#define KERNELS_COLLAPSE_H

#include "kernels/registry.h"

void kernel_2d_collapse(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

void kernel_3d_collapse(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

#endif
