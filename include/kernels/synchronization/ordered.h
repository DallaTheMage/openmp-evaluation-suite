#ifndef KERNELS_ORDERED_H
#define KERNELS_ORDERED_H

#include "kernels/registry.h"

void kernel_2d_ordered(DataView *view,
                       const TestCase *test_case,
                       Profiler *profiler,
                       PerformanceMetric *metric);

#endif
