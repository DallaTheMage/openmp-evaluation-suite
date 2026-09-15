#include <stdlib.h>
#include <omp.h>

#include "core/context.h"
#include "data/collection.h"
#include "core/logger.h"
#include "test/test.h"
#include "test/tests/stress.h"
#include "test/tests/strong.h"
#include "test/tests/weak.h"

void destroy_collections(WorkContext *ctx) {
    if (ctx == NULL) return;
    if (ctx->input != NULL) {
        collection_destroy(ctx->input);
        ctx->input = NULL;
    }
}

void benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *),
                       ResultWriter *writer, RawSample *base_sample) {
    if (ctx == NULL || writer == NULL || base_sample == NULL || ctx->work_iterations == 0) {
        return;
    }

    // Warmup
    for (size_t i = 0; i < ctx->warmup_iterations; ++i) {
        run(ctx);
    }

    // Esecuzione e campionamento dei dati grezzi
    for (size_t i = 0; i < ctx->work_iterations; ++i) {
        double start = omp_get_wtime();
        run(ctx);
        double end = omp_get_wtime();

        base_sample->run_id = (int)i;
        base_sample->elapsed_time = end - start;

        // Scrittura immediata del campione grezzo nel CSV
        writer->operations.write_raw(writer, base_sample);
    }
}

static const Test tests[] = {
    { "memory_stress", stressTest },
    { "weak_scaling", weakScalingTest },
    { "strong_scaling", strongScalingTest }
};

const Test *get_test_set(void) {
    return tests;
}

size_t get_test_count(void) {
    return sizeof(tests) / sizeof(tests[0]);
}
