#ifndef CORE_DISPATCHER_H
#define CORE_DISPATCHER_H

    #include "core/Analyzer.h"
    #include "core/TestPlan.h"
    #include "kernels/KernelRegistry.h"
    #include "profiling/Profiler.h"

    RawSampleSet dispatch_test_plan(
        const TestPlan *plan,
        Profiler       *profiler,
        uint32_t       warmup_reps,
        uint32_t       work_reps
    );

    void dispatch_destroy_samples(
        RawSampleSet *samples
    );

#endif