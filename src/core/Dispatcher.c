#include "core/Dispatcher.h"

#include <stdlib.h>

static bool kernel_supports_view(
const KernelEntry *kernel,
ViewType type
) {
if (!kernel || type >= VIEW_TYPE_COUNT) {
return false;
}

return
    (kernel->supported_views &
     (UINT64_C(1) << type)) != 0;


}

static bool dispatch_test_case(
const TestSet *set,
const KernelEntry *kernel,
const TestPoint *point,
Profiler *profiler,
uint32_t warmup_reps,
uint32_t work_reps,
RawSample *samples,
size_t *index
) {
if (!set ||
!kernel ||
!point ||
!kernel->run ||
!samples ||
!index) {
return false;
}

for (uint32_t i = 0; i < warmup_reps; ++i) {
    kernel->run(
        &set->view,
        point,
        NULL,
        NULL
    );
}

for (uint32_t run = 0; run < work_reps; ++run) {
    RawSample *sample = &samples[(*index)++];

    sample->test_set_id = set->id;
    sample->test_case_id = point->id;
    sample->kernel = kernel->construct;
    sample->run_id = run;

    kernel->run(
        &set->view,
        point,
        profiler,
        &sample->metric
    );
}

return true;


}

RawSampleSet dispatch_test_plan(
const TestPlan *plan,
const KernelRegistry *registry,
Profiler *profiler,
uint32_t warmup_reps,
uint32_t work_reps
) {
RawSampleSet result = {NULL, 0};

if (!plan ||
    !registry ||
    !registry->entries ||
    !profiler ||
    !work_reps) {
    return result;
}

size_t test_case_count = 0;

for (size_t i = 0; i < plan->count; ++i) {
    test_case_count += plan->data[i].points.count;
}

if (!test_case_count || !registry->count) {
    return result;
}

const size_t max_samples =
    test_case_count *
    registry->count *
    (size_t)work_reps;

RawSample *samples =
    malloc(max_samples * sizeof(*samples));

if (!samples) {
    return result;
}

size_t sample_count = 0;

for (size_t s = 0; s < plan->count; ++s) {
    const TestSet *set = &plan->data[s];

    for (size_t k = 0; k < registry->count; ++k) {
        const KernelEntry *kernel =
            &registry->entries[k];

        if (!kernel_supports_view(
                kernel,
                set->view.type)) {
            continue;
        }

        for (size_t p = 0;
             p < set->points.count;
             ++p) {

            if (!dispatch_test_case(
                    set,
                    kernel,
                    &set->points.data[p],
                    profiler,
                    warmup_reps,
                    work_reps,
                    samples,
                    &sample_count)) {

                free(samples);

                result.samples = NULL;
                result.count = 0;

                return result;
            }
        }
    }
}

if (!sample_count) {
    free(samples);
    return result;
}

RawSample *tmp =
    realloc(
        samples,
        sample_count * sizeof(*samples)
    );

if (tmp) {
    samples = tmp;
}

result.samples = samples;
result.count = sample_count;

return result;


}

void dispatch_destroy_samples(
RawSampleSet *samples
) {
if (!samples) {
return;
}

free(samples->samples);

samples->samples = NULL;
samples->count = 0;


}