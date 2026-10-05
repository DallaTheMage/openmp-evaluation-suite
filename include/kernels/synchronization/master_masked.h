#ifndef KERNELS_MASTER_H
#define KERNELS_MASTER_H

#include "kernels/registry.h"

void kernel_2d_master(DataView *view,
                      const TestCase *test_case,
                      Profiler *profiler,
                      PerformanceMetric *metric);

void kernel_2d_masked(DataView *view,
                      const TestCase *test_case,
                      Profiler *profiler,
                      PerformanceMetric *metric);

#endif
