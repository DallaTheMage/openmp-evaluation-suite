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

int weakScalingTest(ResultWriter *writer, WorkContext *ctx, Logger *Logger) {
    if (ctx == NULL) {
        printf("Context problem: WorkContext is NULL.\n");
        return 1;
    }
    if (writer == NULL) {
        printf("ResultWriter creation problem: writer is NULL.\n");
        return 1;
    }

#ifdef WEAK_THREADS
    unsigned short threadnumber[] = WEAK_THREADS;
#else
    unsigned short threadnumber[] = STRESS_THREADS;
#endif

#ifdef WEAK_CHUNKS
    unsigned short chunksize[] = WEAK_CHUNKS;
#else
    unsigned short chunksize[] = STRESS_CHUNKS;
#endif

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    uint32_t base_log2n = WEAK_LOG2_N_PER_THREAD;

    for (size_t i = 0; i < numRoutines; ++i) {
        void (*run)(WorkContext *) = microroutines[i].run;
        const char *name = microroutines[i].name;

        for (size_t l = 0; l < numChunks; ++l) {
            double baseline_time = 0.0;

            for (size_t k = 0; k < numThreads; ++k) {
                TestResult result = {0};

                uint32_t scaled_log2n = base_log2n + get_log2_u16(threadnumber[k]);
                uint64_t real_size = (uint64_t)1 << scaled_log2n;

                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                ctx->input = collection_create((size_t)scaled_log2n);

                if (ctx->input == NULL) {
                    printf("Collection creation problem.\n");
                    destroy_collections(ctx);
                    return 1;
                }

                DataGenerator *generator = generator_random_create(0.0, (double)real_size);
                if (generator == NULL || !generator_fill(generator, ctx->input)) {
                    printf("Collection generation problem.\n");
                    if (generator) generator_destroy(generator);
                    destroy_collections(ctx);
                    return 1;
                }
                generator_destroy(generator);

                // Metadati e configurazione
                result.meta.id = (int)i;
                result.meta.type = "Weak Scaling";
                result.meta.benchname = name;

                result.config.log2n = (long)scaled_log2n;
                result.config.thread_number = threadnumber[k];
                result.config.chunksize = chunksize[l];

                printf(
                    "Weak Scaling %s with base_log2N=%u, scaled_log2N=%u, "
                    "threadnumber=%hu, chunksize=%hu\n",
                    name,
                    (unsigned int)base_log2n,
                    (unsigned int)scaled_log2n,
                    threadnumber[k],
                    chunksize[l]
                );

                // Esegue il benchmark e popola result.time (mean, min, max, variance)
                benchmark_routine(ctx, run, &result);

                double current_time = result.time.mean;

                /*
                 * Calcolo delle metriche per il Weak Scaling (Gustafson's Law):
                 * Efficiency = T1 / Tp
                 * Speedup    = Efficiency * P = (T1 / Tp) * P
                 * Overhead   = Tp - T1
                 */
                if (k == 0 || threadnumber[k] == 1) {
                    baseline_time = current_time;
                    result.metrics.efficiency = 1.0;
                    result.metrics.speedup = (double)threadnumber[k];
                    result.metrics.overhead = 0.0;
                } else {
                    double threads_cnt = (double)threadnumber[k];

                    result.metrics.efficiency = (current_time > 0.0)
                                               ? (baseline_time / current_time) : 0.0;
                    result.metrics.speedup = result.metrics.efficiency * threads_cnt;

                    double raw_overhead = current_time - baseline_time;
                    result.metrics.overhead = (raw_overhead > 0.0) ? raw_overhead : 0.0;
                }

                if (!writer->operations.write(writer, &result)) {
                    printf("TestResult writing problem.\n");
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