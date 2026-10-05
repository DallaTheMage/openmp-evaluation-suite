#ifndef CORE_ANALYZER_H
#define CORE_ANALYZER_H

    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>

    #include "core/TestPlan.h"
    #include "kernels/KernelRegistry.h"
    #include "profiling/Profiler.h"

    typedef struct RawSample {
        TestSetUID        test_set_id;
        TestCaseUID       test_case_id;
        KernelType        kernel;
        uint32_t          run_id;
        PerformanceMetric metric;
    } RawSample;

    typedef struct RawSampleSet {
        RawSample *samples;
        size_t count;
    } RawSampleSet;

    typedef struct Metrics {
        double min;
        double max;
        double mean;
        double variance;
    } Metrics;

    typedef struct AggregatedSample {
        TestSetUID  test_set_id;
        TestCaseUID test_case_id;
        KernelType  kernel;

        size_t      total_runs;

        Metrics     wall_time_sec;
        Metrics     cycles;
        Metrics     instructions;
        Metrics     l1d_misses;
        Metrics     llc_misses;

        Metrics     ipc;
        Metrics     l1d_miss_ratio;
        Metrics     llc_miss_ratio;

        Metrics     energy_pkg_joules;
        Metrics     energy_dram_joules;

    } AggregatedSample;

    typedef struct ScalingMetrics {
        double speedup;
        double efficiency;
        double overhead_sec;
    } ScalingMetrics;

    AggregatedSample *analyzer_aggregate_samples(
        const RawSample *samples,
        size_t          sample_count,
        size_t          *out_count
    );

    bool analyzer_compute_scaling(
        const AggregatedSample *baseline,
        const TestPoint        *baseline_point,
        const AggregatedSample *scaled,
        const TestPoint        *scaled_point,
        ScalingMetrics         *out_metrics
    );

    bool analyzer_same_test_case(
        const RawSample *a,
        const RawSample *b
    );

    bool analyzer_same_experiment(
        const RawSample *a,
        const RawSample *b
    );

    bool analyzer_same_experiment_aggregate(
        const AggregatedSample *a,
        const AggregatedSample *b
    );

#endif