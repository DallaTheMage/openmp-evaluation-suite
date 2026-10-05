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

static void welford_init(
WelfordAccumulator *acc
) {
if (!acc) {
return;
}

acc->count = 0;
acc->min = INFINITY;
acc->max = -INFINITY;
acc->mean = 0.0;
acc->M2 = 0.0;


}

static void welford_update(
WelfordAccumulator *acc,
double value
) {
if (!acc) {
return;
}

++acc->count;

if (value < acc->min) {
    acc->min = value;
}

if (value > acc->max) {
    acc->max = value;
}

const double delta = value - acc->mean;

acc->mean += delta / (double)acc->count;

acc->M2 += delta * (value - acc->mean);


}

static Metrics welford_finalize(
const WelfordAccumulator *acc
) {
Metrics result = {0.0, 0.0, 0.0, 0.0};

if (!acc || !acc->count) {
    return result;
}

result.min = acc->min;
result.max = acc->max;
result.mean = acc->mean;

if (acc->count > 1) {
    result.variance =
        acc->M2 / (double)(acc->count - 1);
}

return result;


}

static void group_accumulator_init(
GroupAccumulator *acc
) {
if (!acc) {
return;
}

welford_init(&acc->wall_time_sec);
welford_init(&acc->cycles);
welford_init(&acc->instructions);
welford_init(&acc->l1d_misses);
welford_init(&acc->llc_misses);

welford_init(&acc->ipc);
welford_init(&acc->l1d_miss_ratio);
welford_init(&acc->llc_miss_ratio);

welford_init(&acc->energy_pkg_joules);
welford_init(&acc->energy_dram_joules);


}

static void group_accumulator_update(
GroupAccumulator *acc,
const PerformanceMetric *metric
) {
if (!acc || !metric) {
return;
}

welford_update(
    &acc->wall_time_sec,
    metric->wall_time_sec
);

welford_update(
    &acc->cycles,
    (double)metric->cycles
);

welford_update(
    &acc->instructions,
    (double)metric->instructions
);

welford_update(
    &acc->l1d_misses,
    (double)metric->l1d_misses
);

welford_update(
    &acc->llc_misses,
    (double)metric->llc_misses
);

welford_update(
    &acc->ipc,
    metric->ipc
);

welford_update(
    &acc->l1d_miss_ratio,
    metric->l1d_miss_ratio
);

welford_update(
    &acc->llc_miss_ratio,
    metric->llc_miss_ratio
);

welford_update(
    &acc->energy_pkg_joules,
    metric->energy_pkg_joules
);

welford_update(
    &acc->energy_dram_joules,
    metric->energy_dram_joules
);


}

#endif