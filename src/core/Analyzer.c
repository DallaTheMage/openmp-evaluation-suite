
#include <stdlib.h>
#include "core/Analyzer.h"
#include "utils/welford.h"

bool analyzer_same_test_case(
const RawSample *a,
const RawSample *b
) {
return a &&
b &&
a->test_set_id == b->test_set_id &&
a->test_case_id == b->test_case_id;
}

bool analyzer_same_experiment(
const RawSample *a,
const RawSample *b
) {
return analyzer_same_test_case(a, b) &&
a->kernel == b->kernel;
}

bool analyzer_same_experiment_aggregate(
const AggregatedSample *a,
const AggregatedSample *b
) {
return a &&
b &&
a->test_set_id == b->test_set_id &&
a->test_case_id == b->test_case_id &&
a->kernel == b->kernel;
}

static void finalize_group(
AggregatedSample *dst,
TestSetUID set_id,
TestCaseUID case_id,
KernelType kernel,
const GroupAccumulator *acc
) {
dst->test_set_id = set_id;
dst->test_case_id = case_id;
dst->kernel = kernel;
dst->total_runs = acc->wall_time_sec.count;

dst->wall_time_sec = welford_finalize(&acc->wall_time_sec);
dst->cycles = welford_finalize(&acc->cycles);
dst->instructions = welford_finalize(&acc->instructions);
dst->l1d_misses = welford_finalize(&acc->l1d_misses);
dst->llc_misses = welford_finalize(&acc->llc_misses);

dst->ipc = welford_finalize(&acc->ipc);
dst->l1d_miss_ratio = welford_finalize(&acc->l1d_miss_ratio);
dst->llc_miss_ratio = welford_finalize(&acc->llc_miss_ratio);

dst->energy_pkg_joules =
    welford_finalize(&acc->energy_pkg_joules);

dst->energy_dram_joules =
    welford_finalize(&acc->energy_dram_joules);


}

AggregatedSample *analyzer_aggregate_samples(
const RawSample *samples,
size_t sample_count,
size_t *out_count
) {
if (out_count) {
*out_count = 0;
}

if (!samples || !sample_count || !out_count) {
    return NULL;
}

size_t group_count = 1;

for (size_t i = 1; i < sample_count; ++i) {
    if (!analyzer_same_experiment(
            &samples[i - 1],
            &samples[i])) {
        ++group_count;
    }
}

AggregatedSample *result =
    calloc(group_count, sizeof(*result));

if (!result) {
    return NULL;
}

GroupAccumulator acc;
group_accumulator_init(&acc);

size_t group_index = 0;

TestSetUID set_id = samples[0].test_set_id;
TestCaseUID case_id = samples[0].test_case_id;
KernelType kernel = samples[0].kernel;

for (size_t i = 0; i < sample_count; ++i) {
    const RawSample *sample = &samples[i];

    if (sample->test_set_id != set_id ||
        sample->test_case_id != case_id ||
        sample->kernel != kernel) {

        finalize_group(
            &result[group_index],
            set_id,
            case_id,
            kernel,
            &acc
        );

        ++group_index;

        set_id = sample->test_set_id;
        case_id = sample->test_case_id;
        kernel = sample->kernel;

        group_accumulator_init(&acc);
    }

    group_accumulator_update(&acc, &sample->metric);
}

finalize_group(
    &result[group_index],
    set_id,
    case_id,
    kernel,
    &acc
);

*out_count = group_count;
return result;


}

bool analyzer_compute_scaling(
const AggregatedSample *baseline,
const TestPoint *baseline_point,
const AggregatedSample *scaled,
const TestPoint *scaled_point,
ScalingMetrics *out_metrics
) {
if (!baseline ||
!baseline_point ||
!scaled ||
!scaled_point ||
!out_metrics) {
return false;
}

if (baseline->test_set_id != scaled->test_set_id) {
    return false;
}

if (baseline->kernel != scaled->kernel) {
    return false;
}

if (baseline->test_case_id != baseline_point->id ||
    scaled->test_case_id != scaled_point->id) {
    return false;
}

const double baseline_time =
    baseline->wall_time_sec.mean;

const double scaled_time =
    scaled->wall_time_sec.mean;

const uint32_t threads =
    scaled_point->num_threads;

if (baseline_time <= 0.0 ||
    scaled_time <= 0.0 ||
    threads == 0) {
    return false;
}

out_metrics->speedup =
    baseline_time / scaled_time;

out_metrics->efficiency =
    out_metrics->speedup / (double)threads;

out_metrics->overhead_sec =
    (double)threads * scaled_time - baseline_time;

return true;


}
