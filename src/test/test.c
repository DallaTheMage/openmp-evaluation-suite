#include <stdlib.h>
#include <omp.h>
#include "test/test.h"
#include "data/collection.h"
#include "test/tests/weak.h"
#include "test/tests/strong.h"
#include "test/tests/stress.h"

void destroy_collections(WorkContext *ctx) {
    if (ctx == NULL) return;

    if (ctx->input != NULL) {
        collection_destroy(ctx->input);
        ctx->input = NULL;
    }
}

void cleanup_test_context(WorkContext *ctx, ResultWriter *writer) {
    destroy_collections(ctx);

    if (writer != NULL) {
        writer->operations.close(writer);
        free(writer);
    }

    if (ctx != NULL) {
        free(ctx);
    }
}

void benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *), TestResult *result) {
    // Controllo di guardia per evitare divisioni per zero
    if (ctx == NULL || result == NULL || ctx->work_iterations == 0) {
        if (result != NULL) {
            result->time = (SampleStats){ .mean = 0.0, .min = 0.0, .max = 0.0, .variance = 0.0 };
        }
        return;
    }

    // Warmup
    for (size_t i = 0; i < ctx->warmup_iterations; ++i) {
        run(ctx);
    }

    double min_time = 0.0;
    double max_time = 0.0;
    double mean = 0.0;
    double M2 = 0.0; // Usato per l'algoritmo di Welford

    for (size_t i = 0; i < ctx->work_iterations; ++i) {
        double start = omp_get_wtime();
        run(ctx);
        double end = omp_get_wtime();

        double elapsed = end - start;

        // Min/Max
        if (i == 0) {
            min_time = elapsed;
            max_time = elapsed;
        } else {
            if (elapsed < min_time) min_time = elapsed;
            if (elapsed > max_time) max_time = elapsed;
        }

        // Algoritmo di Welford per Media e Varianza numericamente stabili
        double delta = elapsed - mean;
        mean += delta / (double)(i + 1);
        double delta2 = elapsed - mean;
        M2 += delta * delta2;
    }

    double count = (double)ctx->work_iterations;

    result->time.mean = mean;
    result->time.min = min_time;
    result->time.max = max_time;
    // Varianza di popolazione (per la varianza campionaria usa: M2 / (count - 1))
    result->time.variance = M2 / count;
}

static const Test tests[] = {
    { "Stress Test", stressTest },
    { "Weak Scaling Test", weakScalingTest },
    { "Strong Scaling Test", strongScalingTest }
};

const Test *get_test_set(void) {
    return tests;
}

size_t get_test_count(void) {
    return sizeof(tests) / sizeof(tests[0]);
}