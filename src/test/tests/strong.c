#include <stdio.h>
#include <stdlib.h>

#include "config/sizes.h"
#include "config/iterations.h"
#include "config/schedule.h"
#include "config/thread.h"
#include "config/chunksize.h"

#include "core/context.h"
#include "data/collection.h"
#include "data/writers/writer.h"
#include "micro/microroutines.h"
#include "test/test.h"
#include "test/tests/strong.h"

int strongScalingTest(GeneralContext *gen_ctx) {
    if (!gen_ctx || !gen_ctx->work_ctx || !gen_ctx->work_ctx->input || !gen_ctx->file_ctx || !gen_ctx->file_ctx->writer) {
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
    uint64_t sub_size = (uint64_t)1 << log2n;

    // Imposta la dimensione di calcolo del subset
    w_ctx->input->size = (size_t)sub_size;
    collection_update_matrix_dimensions(w_ctx->input); // <--- Aggiorna rows e cols

    char log_buffer[256];

    for (size_t i = 0; i < numRoutines; ++i) {
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
        writer->operations.flush(writer);
    }

    return 0;
}
