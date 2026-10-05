#ifndef KERNELS_SECTIONS_H
#define KERNELS_SECTIONS_H

#include "kernels/registry.h"

void kernel_2d_sections(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric);

#endif
