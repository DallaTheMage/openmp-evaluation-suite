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
#include "test/tests/strong.h"

int strongScalingTest(ResultWriter *writer, WorkContext *ctx, Logger *logger) {
    if (!ctx || !writer) return 1;

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

    uint32_t log2n = STRONG_LOG2_N_DEFAULT;
    uint64_t real_size = (uint64_t)1 << log2n;

    for (size_t i = 0; i < numRoutines; ++i) {
        ctx->input = collection_create((size_t)log2n);
        if (!ctx->input) {
            logger->error("Collection creation problem.");
            return 1;
        }

        DataGenerator *generator = generator_random_create(0.0, (double)real_size);
        if (!generator || !generator_fill(generator, ctx->input)) {
            logger->error("Generator creation or filling problem.");
            if (generator) generator_destroy(generator);
            destroy_collections(ctx);
            return 1;
        }
        generator_destroy(generator);

        for (size_t l = 0; l < numChunks; ++l) {
            for (size_t k = 0; k < numThreads; ++k) {
                RawSample sample = {0};
                sample.meta.id = (int)i;
                sample.meta.type = "Strong Scaling";
                sample.meta.benchname = microroutines[i].name;

                sample.config.log2n = (long)log2n;
                sample.config.thread_number = threadnumber[k];
                sample.config.chunksize = chunksize[l];

                ctx->threadnumber = threadnumber[k];
                ctx->chunksize = chunksize[l];
                ctx->warmup_iterations = WARMUP_REPS;
                ctx->work_iterations = WORK_REPS;

                benchmark_routine(ctx, microroutines[i].run, writer, &sample);
            }
        }

        destroy_collections(ctx);
        writer->operations.flush(writer);
    }
    return 0;
}
