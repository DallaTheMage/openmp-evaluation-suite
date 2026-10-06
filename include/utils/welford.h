#ifndef WELFORD_H
#define WELFORD_H

    #include <stddef.h>
    #include <math.h>

    #include "core/Analyzer.h"

    typedef struct WelfordAccumulator {
        size_t count;
        double min;
        double max;
        double mean;
        double M2;
    } WelfordAccumulator;

    typedef struct GroupAccumulator {
        WelfordAccumulator wall_time_sec;
        WelfordAccumulator cycles;
        WelfordAccumulator instructions;
        WelfordAccumulator l1d_misses;
        WelfordAccumulator llc_misses;

        WelfordAccumulator ipc;
        WelfordAccumulator l1d_miss_ratio;
        WelfordAccumulator llc_miss_ratio;

        WelfordAccumulator energy_pkg_joules;
        WelfordAccumulator energy_dram_joules;
    } GroupAccumulator;

    static void welford_init(WelfordAccumulator *accumulator) {
        if (accumulator) {
            accumulator->count = 0;
            accumulator->min = INFINITY;
            accumulator->max = -INFINITY;
            accumulator->mean = 0.0;
            accumulator->M2 = 0.0;
        }
        return;
    }

    static void welford_update(WelfordAccumulator *accumulator, double value) {
        if (accumulator) {
            ++accumulator->count;
            accumulator->min = value < accumulator->min ? value : accumulator->min;
            accumulator->max = value > accumulator->max ? value : accumulator->max;
            const double delta = value - accumulator->mean;
            accumulator->mean += delta / (double)accumulator->count;
            accumulator->M2 += delta * (value - accumulator->mean);
        }
        return;
    }

    static Metrics welford_finalize(const WelfordAccumulator *accumulator) {
        Metrics result = {0.0, 0.0, 0.0, 0.0};
        if (!accumulator || !accumulator->count) {
            return result;
        }
        result.min = accumulator->min;
        result.max = accumulator->max;
        result.mean = accumulator->mean;
        result.variance = accumulator->count > 1
            ? (accumulator->M2 / (double)(accumulator->count - 1))
            : result.variance;
        return result;
    }

    static void group_accumulator_init(GroupAccumulator *accumulator) {
        if (accumulator) {
            welford_init(&accumulator->wall_time_sec);
            welford_init(&accumulator->cycles);
            welford_init(&accumulator->instructions);
            welford_init(&accumulator->l1d_misses);
            welford_init(&accumulator->llc_misses);
            welford_init(&accumulator->ipc);
            welford_init(&accumulator->l1d_miss_ratio);
            welford_init(&accumulator->llc_miss_ratio);
            welford_init(&accumulator->energy_pkg_joules);
            welford_init(&accumulator->energy_dram_joules);
        }
        return;
    }

    static void group_accumulator_update(GroupAccumulator *accumulator,const PerformanceMetric *metric) {
        if (accumulator && metric) {
            welford_update(&accumulator->wall_time_sec,metric->wall_time_sec);
            welford_update(&accumulator->cycles,(double)metric->cycles);
            welford_update(&accumulator->instructions,(double)metric->instructions);
            welford_update(&accumulator->l1d_misses,(double)metric->l1d_misses);
            welford_update(&accumulator->llc_misses,(double)metric->llc_misses);
            welford_update(&accumulator->ipc,metric->ipc);
            welford_update(&accumulator->l1d_miss_ratio,metric->l1d_miss_ratio);
            welford_update(&accumulator->llc_miss_ratio,metric->llc_miss_ratio);
            welford_update(&accumulator->energy_pkg_joules,metric->energy_pkg_joules);
            welford_update(&accumulator->energy_dram_joules,metric->energy_dram_joules);
        }
        return;
    }
#endif