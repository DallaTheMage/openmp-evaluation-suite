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
#include "test/weak.h"

/* Calcola log2 per potenze di 2 senza math.h */
static uint32 get_log2_u16(unsigned short v) {
    uint32 log2_val = 0;
    while (v > 1) {
        v >>= 1;
        log2_val++;
    }
    return log2_val;
}

int weakScalingTest(const char *output_file) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;

    const char *filename = output_file;
    const char *writing_mode = "a";
    const char *header = "Test ID, Test name, Base log2n, Scaled log2n, Thread number, Chunk size, AVG time, Efficiency, Overhead";

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines;
    size_t numThreads;
    size_t numChunks;

    size_t i, k, l;

    uint32 base_log2n = WEAK_LOG2_N_PER_THREAD;
    uint32 scaled_log2n;
    uint64 real_size;

    double time;
    double efficiency;
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

        for (k = 0; k < numThreads; ++k) {
            ctx->threadnumber = threadnumber[k];

            /*
             * Weak Scaling: N_totale = N_per_thread * P
             * In scala logaritmica: log2(N_totale) = base_log2n + log2(P)
             */
            scaled_log2n = base_log2n + get_log2_u16(threadnumber[k]);
            real_size = (uint64)1 << scaled_log2n;

            ctx->input = collection_create((size_t)scaled_log2n);
            ctx->output = collection_create((size_t)scaled_log2n);

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

            for (l = 0; l < numChunks; ++l) {
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

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

                /*
                 * In Weak Scaling l'Efficienza ideale E = T(1) / T(P) e vale 1.0
                 */
                if (threadnumber[k] == 1) {
                    baseline = time;
                    efficiency = 1.0;
                    overhead = 0.0;
                } else { // TODO : CHECK CORRECTNESS OF THIS FORMULA
                    efficiency = baseline / time;
                    overhead = time - baseline;
                }

                result.test_id = (int)i;
                result.benchname = name;
                result.log2n = (long)scaled_log2n;
                result.threadnumber = threadnumber[k];
                result.chunksize = chunksize[l];
                result.time = time;
                result.efficiency = efficiency; /* Salvato in speedup per compatibilità con ResultStruct */
                result.overhead = overhead;

                if (!writer->operations.write(writer, &result)) {
                    printf("Result writing problem.\n");
                    cleanup_test_context(ctx, writer);
                    return 1;
                }
            }

            /* Dealloca prima del prossimo livello di thread poiché cambia la dimensione scalata */
            destroy_collections(ctx);
        }
        writer->operations.flush(writer);
    }

    writer->operations.close(writer);
    free(writer);
    free(ctx);
    return 0;
}
