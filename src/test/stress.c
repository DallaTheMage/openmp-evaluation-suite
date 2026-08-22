#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "test/stress.h"
#include "config/types.h"
#include "core/microroutines.h"
#include "data/generator.h"
#include "data/collection.h"
#include "config/configuration.h"
#include "data/writer.h"

int stressTest(void)
{
    unsigned short threadnumber[] = STRESS_THREADS;
    unsigned short chunksize[] = STRESS_CHUNKS;
    uint32 sizes[] = STRESS_PROBLEM_SIZES;

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

    size_t numRoutines;
    size_t numSizes;
    size_t numThreads;
    size_t numChunks;

    size_t i;
    size_t j;
    size_t k;
    size_t l;
    size_t r;

    double start;
    double total;
    double time;

    WorkContext *ctx;
    DataGenerator *generator;
    Writer *writer;
    Result result;

    void (*run)(WorkContext *);
    const char *name;

    numRoutines = sizeof(routines) / sizeof(routines[0]);
    numSizes = sizeof(sizes) / sizeof(sizes[0]);
    numThreads = sizeof(threadnumber) / sizeof(threadnumber[0]);
    numChunks = sizeof(chunksize) / sizeof(chunksize[0]);

    ctx = malloc(sizeof *ctx);

    if (!ctx) {
        printf("Context problem.\n");
        return 1;
    }

    ctx->input = NULL;
    ctx->output = NULL;

    /*
     * Creazione del writer.
     */
    writer = create_csv_writer();

    if (!writer) {
        printf("Writer creation problem.\n");
        free(ctx);
        return 1;
    }

    /*
     * Apertura del file.
     */
    if (!writer->operations.open(writer, "results.csv", "a")) {
        printf("File opening problem.\n");
        free(writer);
        free(ctx);
        return 1;
    }

    for (i = 0; i < numRoutines; ++i) {

        run = routines[i].run;
        name = routines[i].name;

        for (j = 0; j < numSizes; ++j) {

            /*
             * sizes[j] rappresenta log2(N).
             * collection_create() riceve log2(N).
             */
            ctx->input = collection_create((size_t)sizes[j]);

            if (!ctx->input) {
                printf("Input collection creation problem.\n");

                writer->operations.close(writer);
                free(writer);
                free(ctx);

                return 1;
            }

            /*
             * Ogni routine utilizza una Collection di output
             * della stessa dimensione dell'input.
             */
            ctx->output = collection_create((size_t)sizes[j]);

            if (!ctx->output) {
                printf("Output collection creation problem.\n");

                collection_destroy(ctx->input);

                writer->operations.close(writer);
                free(writer);
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

            if (!generator) {
                printf("Generator problem.\n");

                collection_destroy(ctx->input);
                collection_destroy(ctx->output);

                writer->operations.close(writer);
                free(writer);
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

                writer->operations.close(writer);
                free(writer);
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
                        "Testing %s with log2N=%ld, "
                        "threadnumber=%d, chunksize=%d\n",
                        name,
                        (long)sizes[j],
                        threadnumber[k],
                        chunksize[l]
                    );

                    ctx->chunksize = chunksize[l];

                    ctx->warmup_iterations = WARMUP_REPS;
                    ctx->work_iterations = WORK_REPS;

                    /*
                     * Warmup.
                     */
                    for (r = 0; r < ctx->warmup_iterations; ++r) {
                        run(ctx);
                    }

                    /*
                     * Benchmark.
                     */
                    total = 0.0;

                    for (r = 0; r < ctx->work_iterations; ++r) {

                        start = omp_get_wtime();

                        run(ctx);

                        total += omp_get_wtime() - start;
                    }

                    time = total / (double)ctx->work_iterations;

                    /*
                     * Aggiornamento del Result.
                     *
                     * Lo stesso record viene riutilizzato
                     * per ogni configurazione.
                     */
                    result.benchname = name;
                    result.log2n = (long)sizes[j];
                    result.threadnumber = threadnumber[k];
                    result.chunksize = chunksize[l];
                    result.time = time;

                    /*
                     * Scrittura del risultato.
                     */
                    if (!writer->operations.write(writer, &result)) {
                        printf("Result writing problem.\n");

                        collection_destroy(ctx->input);
                        collection_destroy(ctx->output);

                        writer->operations.close(writer);
                        free(writer);
                        free(ctx);

                        return 1;
                    }
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
        writer->operations.flush(writer);
    }

    /*
     * Chiusura del writer.
     */
    writer->operations.close(writer);
    free(writer);

    free(ctx);

    return 0;
}