#ifndef KERNELS_ATOMIC_H
#define KERNELS_ATOMIC_H

#include "kernels/registry.h"

void kernel_2d_atomic(DataView *view,
                      const TestCase *test_case,
                      Profiler *profiler,
                      PerformanceMetric *metric);

void kernel_3d_atomic(DataView *view,
                      const TestCase *test_case,
                      Profiler *profiler,
                      PerformanceMetric *metric);

#endif
