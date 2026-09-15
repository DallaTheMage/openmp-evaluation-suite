#include <stdio.h>
#include <stdlib.h>

#include "config/sizes.h"
#include "config/iterations.h"
#include "config/schedule.h"
#include "config/thread.h"
#include "config/chunksize.h"

#include "core/context.h"
#include "data/collection.h"
#include "data/generator.h"
#include "data/writers/writer.h"
#include "micro/microroutines.h"
#include "test/test.h"
#include "test/tests/stress.h"

int stressTest(ResultWriter *writer, WorkContext *ctx, Logger *logger) {
    if (!ctx || !writer) return 1;

    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;
    uint32_t sizes[] = STRESS_PROBLEM_SIZES;

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numSizes = ARRAY_SIZE(sizes);
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    char log_buffer[256];

    for (size_t i = 0; i < numRoutines; ++i) {
        for (size_t j = 0; j < numSizes; ++j) {
            uint64_t real_size = (uint64_t)1 << sizes[j];
            ctx->input = collection_create((size_t)sizes[j]);

            if (!ctx->input) {
                logger->error("Collection creation problem.");
                return 1;
            }

            DataGenerator *generator = generator_random_create(0.0, (double)real_size);
            if (!generator || !generator_fill(generator, ctx->input)) {
                logger->error("Data generator problem.");
                if (generator) generator_destroy(generator);
                destroy_collections(ctx);
                return 1;
            }
            generator_destroy(generator);

            for (size_t l = 0; l < numChunks; ++l) {
                for (size_t k = 0; k < numThreads; ++k) {
                    RawSample sample = {0};
                    sample.meta.id = (int)i;
                    sample.meta.type = "Stress Test";
                    sample.meta.benchname = microroutines[i].name;

                    sample.config.log2n = (long)sizes[j];
                    sample.config.thread_number = threadnumber[k];
                    sample.config.chunksize = chunksize[l];

                    ctx->threadnumber = threadnumber[k];
                    ctx->chunksize = chunksize[l];
                    ctx->warmup_iterations = WARMUP_REPS;
                    ctx->work_iterations = WORK_REPS;

                    // Formattazione dettagliata della combinazione corrente
                    snprintf(log_buffer, sizeof(log_buffer),
                             "[%s] Routine: %s | log2N: %ld | Threads: %u | Chunk: %u",
                             sample.meta.type,
                             sample.meta.benchname,
                             sample.config.log2n,
                             sample.config.thread_number,
                             sample.config.chunksize);

                    if (logger && logger->log) {
                        logger->log(log_buffer);
                    }
                    benchmark_routine(ctx, microroutines[i].run, writer, &sample);
                }
            }
            destroy_collections(ctx);
        }
        writer->operations.flush(writer);
    }
    return 0;
}
