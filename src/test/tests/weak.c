#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* 1. Tipi base e configurazioni del progetto */
#include "config/sizes.h"
#include "config/iterations.h"
#include "config/schedule.h"
#include "config/thread.h"
#include "config/chunksize.h"

/* 2. Strutture dati e generatori */
#include "core/context.h"
#include "data/collection.h"
#include "data/generator.h"
#include "data/writers/writer.h"

/* 3. Micro-routine e utility di test condivise */
#include "micro/microroutines.h"
#include "test/test.h"

/* 4. Header specifico del test corrente (es. strong.h, weak.h o stress.h) */
#include "test/tests/weak.h"

/* Calcola log2 per potenze di 2 senza math.h */
static uint32_t get_log2_u16(unsigned short v) {
    uint32_t log2_val = 0;
    while (v > 1) {
        v >>= 1;
        log2_val++;
    }
    return log2_val;
}

int weakScalingTest(ResultWriter *writer, WorkContext *ctx) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    size_t i, k, l;

    uint32_t base_log2n = WEAK_LOG2_N_PER_THREAD;
    uint32_t scaled_log2n;
    uint64_t real_size;

    double time;
    double speedup;
    double efficiency;
    double overhead;

    DataGenerator *generator;
    // Inizializza la struct a zero per evitare qualsiasi campo rimasto indefinito
    Result result = {0};

    void (*run)(WorkContext *);
    const char *name;

    if (ctx == NULL) {
        printf("Context problem.\n");
        return 1;
    }

    ctx->input = NULL;
    ctx->output = NULL;

    if (writer == NULL) {
        printf("ResultWriter creation problem.\n");
        free(ctx);
        return 1;
    }

    for (i = 0; i < numRoutines; ++i) {
        run = microroutines[i].run;
        name = microroutines[i].name;

        for (l = 0; l < numChunks; ++l) {
            /* Reset baseline per chunksize configuration */
            double baseline = 0.0;

            for (k = 0; k < numThreads; ++k) {
                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                scaled_log2n = base_log2n + get_log2_u16(threadnumber[k]);
                real_size = (uint64_t)1 << scaled_log2n;

                ctx->input = collection_create((size_t)scaled_log2n);
                ctx->output = collection_create((size_t)scaled_log2n);

                if (ctx->input == NULL || ctx->output == NULL) {
                    printf("Collection creation problem.\n");
                    cleanup_test_context(ctx, writer);
                    return 1;
                }

                generator = generator_random_create((double)0, (double)real_size);

                if (generator == NULL) {
                    printf("Generator creation problem.\n");
                    cleanup_test_context(ctx, writer);
                    return 1;
                }

                if (!generator_fill(generator, ctx->input)) {
                    printf("Collection generation problem.\n");
                    generator_destroy(generator);
                    cleanup_test_context(ctx, writer);
                    return 1;
                }

                generator_destroy(generator);
                generator = NULL;

                printf(
                    "Weak Scaling %s with base_log2N=%u, scaled_log2N=%u, "
                    "threadnumber=%hu, chunksize=%hu\n",
                    name,
                    (unsigned int)base_log2n,
                    (unsigned int)scaled_log2n,
                    threadnumber[k],
                    chunksize[l]
                );

                time = benchmark_routine(ctx, run);

                /* Calcolo di baseline, efficiency, speedup e overhead */
                if (k == 0 || threadnumber[k] == 1) {
                    baseline = time;
                    efficiency = 1.0;
                    speedup = (double)threadnumber[k];
                    overhead = 0.0;
                } else {
                    efficiency = (baseline > 0.0) ? (baseline / time) : 0.0;
                    speedup = efficiency * (double)threadnumber[k];
                    overhead = time - baseline;
                }

                result.test_id = (int)i;
                result.test_type = "Weak Scaling";
                result.benchname = name;
                result.log2n = (long)scaled_log2n;
                result.threadnumber = threadnumber[k];
                result.chunksize = chunksize[l];
                result.time = time;
                result.speedup = speedup;
                result.efficiency = efficiency;
                result.overhead = overhead;

                if (!writer->operations.write(writer, &result)) {
                    printf("Result writing problem.\n");
                    destroy_collections(ctx);
                    return 1;
                }

                destroy_collections(ctx);
            }
        }
        writer->operations.flush(writer);
    }

    return 0;
}