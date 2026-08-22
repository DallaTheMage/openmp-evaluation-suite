#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "test/stress.h"
#include "config/types.h"
#include "core/microroutines.h"
#include "data/generator.h"
#include "data/collection.h"
#include "config/configuration.h"

int stressTest(void) {
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;
    uint32 sizes[] = STRESS_PROBLEM_SIZES;
    FILE *file;
    Routine routines[] = {
        { "Parallel for Arithmetic scale", routine_micro_scale },
        { "Parallel for best cache", routine_matrix_row_best },
        { "Parallel for worst cache", routine_matrix_col_worst },
        { "Parallel for reduction", routine_reduction_sum },
        { "Parallel inclusive scan", routine_scan_inclusive },
        { "Task", routine_task_divide_conquer },
        { "Atomic reduction", routine_atomic_reduction },
        { "Taskloop", routine_taskloop_scale }
    };

    uint32 realsize;

    unsigned short numRoutines;
    unsigned short numSizes;
    unsigned short numThreads;
    unsigned short numChunks;

    unsigned short i;
    unsigned short j;
    unsigned short k;
    unsigned short l;
    unsigned short r;

    double start;
    double total;
    double time;

    WorkContext *ctx;
    DataGenerator *generator;

    void (*run)(WorkContext *);
    const char *name;

    numRoutines = sizeof(routines) / sizeof(routines[0]);
    numSizes = sizeof(sizes) / sizeof(sizes[0]);
    numThreads = sizeof(threadnumber) / sizeof(threadnumber[0]);
    numChunks = sizeof(chunksize) / sizeof(chunksize[0]);

    ctx = (WorkContext *)malloc(sizeof(WorkContext));

    file = fopen("results.csv", "a");
    if (file == NULL) {
        return 1;
    }

    if (ctx == NULL) {
        printf("Context problem.\n");
        return 1;
    }

    ctx->input = NULL;
    ctx->output = NULL;

    for (i = 0; i < numRoutines; ++i) {

        run = routines[i].run;
        name = routines[i].name;

        for (j = 0; j < numSizes; ++j) {

            /*
             * sizes[j] rappresenta log2(N).
             * collection_create() riceve log2(N).
             */
            ctx->input = collection_create((size_t)sizes[j]);

            if (ctx->input == NULL) {
                printf("Input collection creation problem.\n");
                free(ctx);
                return 1;
            }

            /*
             * Ogni routine utilizza una Collection di output
             * della stessa dimensione dell'input.
             */
            ctx->output = collection_create((size_t)sizes[j]);

            if (ctx->output == NULL) {
                printf("Output collection creation problem.\n");
                collection_destroy(ctx->input);
                ctx->input = NULL;
                free(ctx);
                return 1;
            }

            /*
             * Numero effettivo di elementi:
             *
             * N = 2^log2(N)
             */
            realsize = ((uint32)1 << sizes[j]);

            /*
             * Creazione del generatore.
             */
            generator = generator_xoshiro256_create(
                0.0,
                (double)realsize
            );

            if (generator == NULL) {
                printf("Generator problem.\n");

                collection_destroy(ctx->input);
                collection_destroy(ctx->output);

                ctx->input = NULL;
                ctx->output = NULL;

                free(ctx);
                return 1;
            }

            /*
             * Il generator riempie direttamente la Collection
             * di input.
             */
            if (!generator->operations.fill(generator, ctx->input)) {
                printf("Collection generation problem.\n");

                generator->operations.clean(generator);

                collection_destroy(ctx->input);
                collection_destroy(ctx->output);

                ctx->input = NULL;
                ctx->output = NULL;

                free(ctx);
                return 1;
            }

            /*
             * Il generatore non serve più dopo il fill().
             */
            generator->operations.clean(generator);
            generator = NULL;

            /*
             * Configurazione del benchmark.
             */
            for (k = 0; k < numThreads; ++k) {

                ctx->threadnumber = threadnumber[k];

                for (l = 0; l < numChunks; ++l) {

                    printf(
                        "Testing %s with log2N=%ld, threadnumber=%d, chunksize=%d\n",
                        name,
                        (long)sizes[j],
                        threadnumber[k],
                        chunksize[l]
                    );

                    ctx->chunksize = chunksize[l];

                    ctx->warmup_iterations = WARMUP_REPS;
                    ctx->work_iterations = WORK_REPS;

                    /*
                     * Warmup
                     */
                    for (r = 0; r < ctx->warmup_iterations; ++r) {
                        run(ctx);
                    }

                    /*
                     * Benchmark
                     */
                    total = 0.0;

                    for (r = 0; r < ctx->work_iterations; ++r) {

                        start = omp_get_wtime();

                        run(ctx);

                        total += omp_get_wtime() - start;
                    }

                    time = total / (double)ctx->work_iterations;

                    fprintf(
                        file,
                        "%s, log2N=%ld, threadnumber=%d, chunksize=%d, time=%f\n",
                        name,
                        (long)sizes[j],
                        threadnumber[k],
                        chunksize[l],
                        time
                    );
                }
            }

            /*
             * Fine della configurazione per questa dimensione.
             */
            collection_destroy(ctx->input);
            collection_destroy(ctx->output);

            ctx->input = NULL;
            ctx->output = NULL;
        }
    }

    free(ctx);
    fclose(file);
    return 0;
}