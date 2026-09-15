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

int strongScalingTest(GeneralContext *gen_ctx) {
    if (!gen_ctx || !gen_ctx->work_ctx || !gen_ctx->file_ctx || !gen_ctx->file_ctx->writer) {
        return 1;
    }

    WorkContext *w_ctx = gen_ctx->work_ctx;
    ResultWriter *writer = gen_ctx->file_ctx->writer;
    Logger *logger = gen_ctx->logger;

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

    uint32_t log2n = STRONG_SCALING_SIZE;
    uint64_t real_size = (uint64_t)1 << log2n;

    char log_buffer[256];

    for (size_t i = 0; i < numRoutines; ++i) {
        destroy_collections(w_ctx);

        w_ctx->input = collection_create((size_t)log2n);
        if (!w_ctx->input || (uint64_t)w_ctx->input->size != real_size) {
            if (logger && logger->error) {
                snprintf(log_buffer, sizeof(log_buffer),
                         "OOM o dimensione errata per log2N: %u (Attesi: %lu, Allocati: %lu)",
                         log2n,
                         (unsigned long)real_size,
                         (unsigned long)(w_ctx->input ? w_ctx->input->size : 0));
                logger->error(log_buffer);
            }
            destroy_collections(w_ctx);
            return 1;
        }

        DataGenerator *generator = generator_random_create(0.0, (double)real_size);
        if (!generator || !generator_fill(generator, w_ctx->input)) {
            if (logger && logger->error) {
                logger->error("Data generator problem.");
            }
            if (generator) generator_destroy(generator);
            destroy_collections(w_ctx);
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

                w_ctx->threadnumber = threadnumber[k];
                w_ctx->chunksize = chunksize[l];
                w_ctx->warmup_iterations = WARMUP_REPS;
                w_ctx->work_iterations = WORK_REPS;

                if (logger && logger->log) {
                    snprintf(log_buffer, sizeof(log_buffer),
                             "[%s] Routine: %s | log2N: %ld | Threads: %u | Chunk: %u",
                             sample.meta.type,
                             sample.meta.benchname,
                             sample.config.log2n,
                             sample.config.thread_number,
                             sample.config.chunksize);
                    logger->log(log_buffer);
                }

                benchmark_routine(w_ctx, microroutines[i].run, writer, &sample);
            }
        }

        destroy_collections(w_ctx);
        writer->operations.flush(writer);
    }

    return 0;
}
