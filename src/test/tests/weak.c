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
#include "test/tests/weak.h"

static uint32_t get_log2_u16(unsigned short v) {
    uint32_t log2_val = 0;
    while (v > 1) {
        v >>= 1;
        log2_val++;
    }
    return log2_val;
}

int weakScalingTest(ResultWriter *writer, WorkContext *ctx, Logger *logger) {
    if (!ctx || !writer) return 1;

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
        for (size_t l = 0; l < numChunks; ++l) {
            for (size_t k = 0; k < numThreads; ++k) {
                uint32_t scaled_log2n = base_log2n + get_log2_u16(threadnumber[k]);
                uint64_t real_size = (uint64_t)1 << scaled_log2n;

                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                ctx->input = collection_create((size_t)scaled_log2n);
                if (!ctx->input) {
                    logger->error("Collection creation problem.");
                    return 1;
                }

                DataGenerator *generator = generator_random_create(0.0, (double)real_size);
                if (!generator || !generator_fill(generator, ctx->input)) {
                    logger->error("Collection generation problem.");
                    if (generator) generator_destroy(generator);
                    destroy_collections(ctx);
                    return 1;
                }
                generator_destroy(generator);

                RawSample sample = {0};
                sample.meta.id = (int)i;
                sample.meta.type = "Weak Scaling";
                sample.meta.benchname = microroutines[i].name;

                sample.config.log2n = (long)scaled_log2n;
                sample.config.thread_number = threadnumber[k];
                sample.config.chunksize = chunksize[l];

                benchmark_routine(ctx, microroutines[i].run, writer, &sample);

                destroy_collections(ctx);
            }
        }
        writer->operations.flush(writer);
    }
    return 0;
}
