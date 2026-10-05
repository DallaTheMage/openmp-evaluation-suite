#ifndef CORE_KERNEL_REGISTRY_H
#define CORE_KERNEL_REGISTRY_H

#include "kernels/common/types.h"
#include "data/DataView.h"
#include "core/TestPlan.h"
#include "profiling/Profiler.h"

typedef void (*KernelFunc)(
    DataView *view,
    const TestCase *test_case,
    Profiler *profiler,
    PerformanceMetric *metric
);

extern const KernelFunc
    KERNEL_REGISTRY[VIEW_TYPE_COUNT][KERNEL_COUNT];

KernelFunc get_kernel(
    ViewType view,
    KernelType kernel
);

bool kernel_is_supported(
    ViewType view,
    KernelType kernel
);

const char *get_kernel_name(
    KernelType kernel
);

#endif /* CORE_KERNEL_REGISTRY_H */
