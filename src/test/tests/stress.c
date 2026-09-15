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
#include "test/tests/stress.h"

int stressTest(GeneralContext *gen_ctx) {
    if (!gen_ctx || !gen_ctx->work_ctx || !gen_ctx->work_ctx->input || !gen_ctx->file_ctx || !gen_ctx->file_ctx->writer) {
        return 1;
    }

    WorkContext *w_ctx = gen_ctx->work_ctx;
    ResultWriter *writer = gen_ctx->file_ctx->writer;
    Logger *logger = gen_ctx->logger;

    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;

    uint32_t sizes[WORK_SIZE_SLICES + 1];
    const uint32_t step = MAX_PROBLEM_SIZE / WORK_SIZE_SLICES;

    for (size_t i = 0; i < WORK_SIZE_SLICES; i++) {
        sizes[i] = (uint32_t)((i + 1) * step);
    }
    sizes[WORK_SIZE_SLICES] = MAX_PROBLEM_SIZE;

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines = get_microroutines_count();
    size_t numSizes = ARRAY_SIZE(sizes);
    size_t numThreads = ARRAY_SIZE(threadnumber);
    size_t numChunks = ARRAY_SIZE(chunksize);

    char log_buffer[256];

    for (size_t i = 0; i < numRoutines; ++i) {
        for (size_t j = 0; j < numSizes; ++j) {
            uint64_t sub_size = (uint64_t)1 << sizes[j];

            // Ridimensionamento logico senza riallocazione di memoria RAM
            w_ctx->input->size = (size_t)sub_size;
            collection_update_matrix_dimensions(w_ctx->input); // <--- Aggiorna rows e cols

            for (size_t l = 0; l < numChunks; ++l) {
                for (size_t k = 0; k < numThreads; ++k) {
                    RawSample sample = {0};
                    sample.meta.id = (int)i;
                    sample.meta.type = "Stress Test";
                    sample.meta.benchname = microroutines[i].name;

                    sample.config.log2n = (long)sizes[j];
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
        }
        writer->operations.flush(writer);
    }

    return 0;
}
