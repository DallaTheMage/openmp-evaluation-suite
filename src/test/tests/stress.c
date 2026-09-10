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

/* 4. Utility Profiling */
#include "profiling/statistics.h"

/* 5. Header specifico del test corrente (es. strong.h, weak.h o stress.h) */
#include "test/tests/stress.h"

int stressTest(ResultWriter *writer, WorkContext *ctx, Logger *logger) {
    if (ctx == NULL) {
        logger->error("WorkContext is NULL.");
        return 1;
    }
    if (writer == NULL) {
        logger->error("ResultWriter is NULL.");
        return 1;
    }

    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;
    uint32_t sizes[] = STRESS_PROBLEM_SIZES;

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numSizes = ARRAY_SIZE(sizes);
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    for (size_t i = 0; i < numRoutines; ++i) {
        void (*run)(WorkContext *) = microroutines[i].run;
        const char *name = microroutines[i].name;

        for (size_t j = 0; j < numSizes; ++j) {
            uint64_t real_size = (uint64_t)1 << sizes[j];

            ctx->input = collection_create((size_t)sizes[j]);

            if (ctx->input == NULL) {
                logger->error("Collection creation problem.");
                destroy_collections(ctx);
                return 1;
            }

            DataGenerator *generator = generator_random_create(0.0, (double)real_size);
            if (generator == NULL || !generator_fill(generator, ctx->input)) {
                logger->error("Data generator problem.");
                if (generator) generator_destroy(generator);
                destroy_collections(ctx);
                return 1;
            }
            generator_destroy(generator);

            for (size_t l = 0; l < numChunks; ++l) {
                double baseline_time = 0.0;

                for (size_t k = 0; k < numThreads; ++k) {
                    TestResult result = {0};

                    // Meta & Config
                    result.meta.id = (int)i;
                    result.meta.type = "Stress Test";
                    result.meta.benchname = name;

                    result.config.log2n = (long)sizes[j];
                    result.config.thread_number = threadnumber[k];
                    result.config.chunksize = chunksize[l];

                    ctx->threadnumber = threadnumber[k];
                    ctx->chunksize = chunksize[l];
                    ctx->warmup_iterations = WARMUP_REPS;
                    ctx->work_iterations = WORK_REPS;

                    printf("Testing %s with log2N=%u, threadnumber=%hu, chunksize=%hu\n",
                           name, (unsigned int)sizes[j], threadnumber[k], chunksize[l]);

                    // Popola result.time (mean, min, max, variance)
                    benchmark_routine(ctx, run, &result);

                    double current_time = result.time.mean;

                    // Calcolo Baseline, Speedup, Efficienza e Overhead
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
                        logger->error("TestResult writing problem.");
                        destroy_collections(ctx);
                        return 1;
                    }
                }
            }
            destroy_collections(ctx);
        }
        writer->operations.flush(writer);
    }
    return 0;
}