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

/* 4. Header specifico del test corrente */
#include "test/tests/strong.h"

int strongScalingTest(ResultWriter *writer, WorkContext *ctx, Logger *logger) {
    if (ctx == NULL) {
        printf("Context problem: WorkContext is NULL.\n");
        return 1;
    }
    if (writer == NULL) {
        printf("ResultWriter creation problem: writer is NULL.\n");
        return 1;
    }

    // Usiamo le configurazioni dedicate allo Strong Scaling se definite
#ifdef STRONG_THREADS
    unsigned short threadnumber[] = STRONG_THREADS;
#else
    unsigned short threadnumber[] = STRESS_THREADS;
#endif

#ifdef STRONG_CHUNKS
    unsigned short chunksize[] = STRONG_CHUNKS;
#else
    unsigned short chunksize[] = STRESS_CHUNKS;
#endif

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    /* Dimensione fissa del problema per lo Strong Scaling */
    uint32_t log2n = STRONG_LOG2_N_DEFAULT;
    uint64_t real_size = (uint64_t)1 << log2n;

    for (size_t i = 0; i < numRoutines; ++i) {
        void (*run)(WorkContext *) = microroutines[i].run;
        const char *name = microroutines[i].name;

        ctx->input = collection_create((size_t)log2n);

        if (ctx->input == NULL) {
            printf("Collection creation problem.\n");
            destroy_collections(ctx);
            return 1;
        }

        DataGenerator *generator = generator_random_create(0.0, (double)real_size);
        if (generator == NULL || !generator_fill(generator, ctx->input)) {
            printf("Generator creation or filling problem.\n");
            if (generator) generator_destroy(generator);
            destroy_collections(ctx);
            return 1;
        }
        generator_destroy(generator);

        /* Ciclo esterno su chunksize: ogni configurazione di chunk ha la sua baseline */
        for (size_t l = 0; l < numChunks; ++l) {
            double baseline_time = 0.0;

            for (size_t k = 0; k < numThreads; ++k) {
                TestResult result = {0};

                // Inizializzazione metadati e configurazione
                result.meta.id = (int)i;
                result.meta.type = "Strong Scaling";
                result.meta.benchname = name;

                result.config.log2n = (long)log2n;
                result.config.thread_number = threadnumber[k];
                result.config.chunksize = chunksize[l];

                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                printf(
                    "Strong Scaling %s with log2N=%u, threadnumber=%hu, chunksize=%hu\n",
                    name, (unsigned int)log2n, threadnumber[k], chunksize[l]
                );

                // Popola result.time (mean, min, max, variance)
                benchmark_routine(ctx, run, &result);

                double current_time = result.time.mean;

                // Calcolo delle metriche di parallelismo rispetto alla baseline
                if (k == 0 || threadnumber[k] == 1) {
                    baseline_time = current_time;
                    result.metrics.speedup = 1.0;
                    result.metrics.efficiency = 1.0;
                    result.metrics.overhead = 0.0;
                } else {
                    double threads_cnt = (double)threadnumber[k];

                    result.metrics.speedup = (baseline_time > 0.0 && current_time > 0.0)
                                            ? (baseline_time / current_time) : 0.0;
                    result.metrics.efficiency = (threads_cnt > 0.0)
                                               ? (result.metrics.speedup / threads_cnt) : 0.0;

                    double raw_overhead = current_time - (baseline_time / threads_cnt);
                    result.metrics.overhead = (raw_overhead > 0.0) ? raw_overhead : 0.0;
                }

                if (!writer->operations.write(writer, &result)) {
                    printf("TestResult writing problem.\n");
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