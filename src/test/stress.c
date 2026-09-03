#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "test/stress.h"
#include "config/types.h"
#include "config/sizes.h"
#include "config/iterations.h"
#include "config/schedule.h"
#include "config/thread.h"
#include "config/chunksize.h"
#include "core/microroutines.h"
#include "data/collection.h"
#include "data/generator.h"
#include "data/writers/writer.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static void destroy_collections(WorkContext *ctx) {
    if (ctx->input != NULL) {
        collection_destroy(ctx->input);
        ctx->input = NULL;
    }

    if (ctx->output != NULL) {
        collection_destroy(ctx->output);
        ctx->output = NULL;
    }
}

static void cleanup_stress_test(WorkContext *ctx, ResultWriter *writer) {
    destroy_collections(ctx);

    if (writer != NULL) {
        writer->operations.close(writer);
        free(writer);
    }

    free(ctx);
}

static double benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *)) {
    double start;
    double total = 0.0;
    size_t i;

    for (i = 0; i < ctx->warmup_iterations; ++i) {
        run(ctx);
    }

    for (i = 0; i < ctx->work_iterations; ++i) {
        start = omp_get_wtime();
        run(ctx);
        total += omp_get_wtime() - start;
    }

    return total / (double)ctx->work_iterations;
}

int stressTest(const char *output_file) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;
    uint32 sizes[] = STRESS_PROBLEM_SIZES;

    const char *filename = output_file;
    const char *writing_mode = "a";
    const char *header = "benchname,Problem size (log2n),threadnumber,chunksize,time,speedup,overhead";

    const MicroRoutine* microroutines = get_microroutines();
    size_t numRoutines;
    size_t numSizes;
    size_t numThreads;
    size_t numChunks;

    size_t i;
    size_t j;
    size_t k;
    size_t l;

    uint64 real_size;

    double time;
    double speedup;
    double overhead;
    double baseline;

    WorkContext *ctx;
    DataGenerator *generator;
    ResultWriter *writer;

    Result result;

    void (*run)(WorkContext *);
    const char *name;

    numRoutines = get_microroutines_count();
    numSizes = ARRAY_SIZE(sizes);
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
        cleanup_stress_test(ctx, writer);
        return 1;
    }

    for (i = 0; i < numRoutines; ++i) {
        run = microroutines[i].run;
        name = microroutines[i].name;

        for (j = 0; j < numSizes; ++j) {
            real_size = (uint64)1 << sizes[j];

            ctx->input = collection_create((size_t)sizes[j]);
            ctx->output = collection_create((size_t)sizes[j]);

            if (ctx->input == NULL || ctx->output == NULL) {
                printf("Collection creation problem.\n");
                cleanup_stress_test(ctx, writer);
                return 1;
            }

            generator = generator_random_create(
                (datatype)0,
                (datatype)real_size
            );

            if (generator == NULL) {
                printf("Generator creation problem.\n");
                cleanup_stress_test(ctx, writer);
                return 1;
            }

            if (!generator_fill(generator, ctx->input)) {
                printf("Collection generation problem.\n");
                generator_destroy(generator);
                cleanup_stress_test(ctx, writer);
                return 1;
            }

            generator_destroy(generator);
            generator = NULL;

            baseline = 0.0;

            for (k = 0; k < numThreads; ++k) {
                ctx->threadnumber = threadnumber[k];

                for (l = 0; l < numChunks; ++l) {
                    ctx->chunksize = chunksize[l];
                    ctx->warmup_iterations = WARMUP_REPS;
                    ctx->work_iterations = WORK_REPS;

                    printf(
                        "Testing %s with log2N=%u, "
                        "threadnumber=%hu, chunksize=%hu\n",
                        name,
                        (unsigned int)sizes[j],
                        threadnumber[k],
                        chunksize[l]
                    );

                    time = benchmark_routine(ctx, run);

                    if (threadnumber[k] == 1) {
                        baseline = time;
                        speedup = 1.0;
                        overhead = 0.0;
                    } else {
                        speedup = baseline / time;
                        overhead =
                            time - (baseline / threadnumber[k]);
                    }

                    result.benchname = name;
                    result.log2n = (long)sizes[j];
                    result.threadnumber = threadnumber[k];
                    result.chunksize = chunksize[l];
                    result.time = time;
                    result.speedup = speedup;
                    result.overhead = overhead;

                    if (!writer->operations.write(writer, &result)) {
                        printf("Result writing problem.\n");
                        cleanup_stress_test(ctx, writer);
                        return 1;
                    }
                }
            }
            destroy_collections(ctx);
        }
        writer->operations.flush(writer);
    }
    writer->operations.close(writer);
    free(writer);
    free(ctx);
    return 0;
}
