#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "test/common.h"
#include "data/collection.h"

void destroy_collections(WorkContext *ctx) {
    if (ctx == NULL) return;

    if (ctx->input != NULL) {
        collection_destroy(ctx->input);
        ctx->input = NULL;
    }

    if (ctx->output != NULL) {
        collection_destroy(ctx->output);
        ctx->output = NULL;
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

double benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *)) {
    double start;
    double total = 0.0;
    size_t i;

    for (i = 0; i < ctx->warmup_iterations; ++i) {
        run(ctx);
    }

    for (i = 0; i < ctx->work_iterations; ++i) {
        start = omp_get_wtime();
        run(ctx);
        total += omp_get_wtime() - start;
    }

    return total / (double)ctx->work_iterations;
}
