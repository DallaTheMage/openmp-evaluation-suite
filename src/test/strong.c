#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* 1. Tipi base e configurazioni del progetto */
#include "config/types.h"
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
#include "test/common.h"

/* 4. Header specifico del test corrente (es. strong.h, weak.h o stress.h) */
#include "test/strong.h"

int strongScalingTest(const char *output_file) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;

    const char *filename = output_file;
    const char *writing_mode = "a";
    const char *header = "Test ID, Test name, Problem size (log2n), Thread number, Chunk size, AVG time, Speedup, Overhead";

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines;
    size_t numThreads;
    size_t numChunks;

    size_t i, k, l;

    /* Dimensione fissa per lo Strong Scaling */
    uint32 log2n = STRONG_LOG2_N_DEFAULT;
    uint64 real_size = (uint64)1 << log2n;

    double time;
    double speedup;
    double overhead;
    double baseline = 0.0;

    WorkContext *ctx;
    DataGenerator *generator;
    ResultWriter *writer;
    Result result;

    void (*run)(WorkContext *);
    const char *name;

    numRoutines = get_microroutines_count();
    numThreads = ARRAY_SIZE(threadnumber);
    numChunks = ARRAY_SIZE(chunksize);

    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
        printf("Context problem.\n");
        return 1;
    }

    ctx->input = NULL;
    ctx->output = NULL;

    writer = create_writer();
    if (writer == NULL) {
        printf("ResultWriter creation problem.\n");
        free(ctx);
        return 1;
    }

    if (!writer->operations.clean(writer, filename) ||
        !writer->operations.open(writer, filename, writing_mode, header)) {
        printf("File cleaning and opening problem.\n");
        cleanup_test_context(ctx, writer);
        return 1;
    }

    for (i = 0; i < numRoutines; ++i) {
        run = microroutines[i].run;
        name = microroutines[i].name;

        /*
         * Poiché la dimensione N è fissa, allochiamo e popoliamo la collezione
         * una sola volta per ogni micro-routine prima del ciclo sui thread.
         */
        ctx->input = collection_create((size_t)log2n);
        ctx->output = collection_create((size_t)log2n);

        if (ctx->input == NULL || ctx->output == NULL) {
            printf("Collection creation problem.\n");
            cleanup_test_context(ctx, writer);
            return 1;
        }

        generator = generator_random_create(
            (datatype)0,
            (datatype)real_size
        );

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

        baseline = 0.0;

        for (k = 0; k < numThreads; ++k) {
            ctx->threadnumber = threadnumber[k];

            for (l = 0; l < numChunks; ++l) {
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

                /* Calcolo di Speedup (S = T1 / Tp) e Overhead (T_overhead = Tp - (T1 / p)) */
                if (threadnumber[k] == 1) {
                    baseline = time;
                    speedup = 1.0;
                    overhead = 0.0;
                } else {
                    speedup = baseline / time;
                    overhead = time - (baseline / threadnumber[k]);
                }

                result.test_id = (int)i;
                result.benchname = name;
                result.log2n = (long)log2n;
                result.threadnumber = threadnumber[k];
                result.chunksize = chunksize[l];
                result.time = time;
                result.speedup = speedup;
                result.overhead = overhead;

                if (!writer->operations.write(writer, &result)) {
                    printf("Result writing problem.\n");
                    cleanup_test_context(ctx, writer);
                    return 1;
                }
            }
        }

        destroy_collections(ctx);
        writer->operations.flush(writer);
    }

    writer->operations.close(writer);
    free(writer);
    free(ctx);
    return 0;
}
