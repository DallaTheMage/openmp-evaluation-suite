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
#include "test/tests/strong.h"

int strongScalingTest(ResultWriter *writer, WorkContext *ctx) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    size_t i, k, l;

    /* Fixed problem size for Strong Scaling */
    uint32_t log2n = STRONG_LOG2_N_DEFAULT;
    uint64_t real_size = (uint64_t)1 << log2n;

    double time;
    double speedup;
    double efficiency;
    double overhead;

    DataGenerator *generator;
    /* Inizializzazione completa a zero per prevenire warning di Valgrind sullo stack */
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
        return 1;
    }

    for (i = 0; i < numRoutines; ++i) {
        run = microroutines[i].run;
        name = microroutines[i].name;

        ctx->input = collection_create((size_t)log2n);
        ctx->output = collection_create((size_t)log2n);

        if (ctx->input == NULL || ctx->output == NULL) {
            printf("Collection creation problem.\n");
            destroy_collections(ctx);
            return 1;
        }

        generator = generator_random_create((double)0, (double)real_size);

        if (generator == NULL) {
            printf("Generator creation problem.\n");
            destroy_collections(ctx);
            return 1;
        }

        if (!generator_fill(generator, ctx->input)) {
            printf("Collection generation problem.\n");
            generator_destroy(generator);
            destroy_collections(ctx);
            return 1;
        }

        generator_destroy(generator);
        generator = NULL;

        /* Invert loops: loop over chunksize first so each chunk size gets its own baseline */
        for (l = 0; l < numChunks; ++l) {
            double baseline = 0.0;

            for (k = 0; k < numThreads; ++k) {
                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                printf(
                    "Strong Scaling %s with log2N=%u, "
                    "threadnumber=%hu, chunksize=%hu\n",
                    name,
                    (unsigned int)log2n,
                    threadnumber[k],
                    chunksize[l]
                );

                time = benchmark_routine(ctx, run);

                /* Initialize baseline on single-thread or first thread setting */
                if (k == 0 || threadnumber[k] == 1) {
                    baseline = time;
                    speedup = 1.0;
                    efficiency = 1.0;
                    overhead = 0.0;
                } else {
                    speedup = (baseline > 0.0 && time > 0.0) ? (baseline / time) : 0.0;
                    efficiency = (threadnumber[k] > 0) ? (speedup / (double)threadnumber[k]) : 0.0;
                    overhead = time - (baseline / (double)threadnumber[k]);
                }

                result.test_id = (int)i;
                result.test_type = "Strong Scaling";
                result.benchname = name;
                result.log2n = (long)log2n;
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
            }
        }

        destroy_collections(ctx);
        writer->operations.flush(writer);
    }
    return 0;
}