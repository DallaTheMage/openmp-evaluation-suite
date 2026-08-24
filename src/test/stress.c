#include <stdio.h>
#include <stdlib.h>

#include <omp.h>

#include "test/stress.h"

#include "config/types.h"
#include "config/configuration.h"

#include "core/microroutines.h"

#include "data/collection.h"
#include "data/generator.h"
#include "data/writers/writer.h"


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

    size_t numRoutines;
    size_t numSizes;
    size_t numThreads;
    size_t numChunks;

    size_t i;
    size_t j;
    size_t k;
    size_t l;
    size_t r;

    uint64 real_size;

    double start;
    double total;
    double time;

    WorkContext *ctx;
    DataGenerator *generator;
    ResultWriter *writer;

    Result result;

    void (*run)(WorkContext *);
    const char *name;


    /*
     * --------------------------------------------------------
     * Number of configurations
     * --------------------------------------------------------
     */

    numRoutines =
        sizeof(routines) /
        sizeof(routines[0]);

    numSizes =
        sizeof(sizes) /
        sizeof(sizes[0]);

    numThreads =
        sizeof(threadnumber) /
        sizeof(threadnumber[0]);

    numChunks =
        sizeof(chunksize) /
        sizeof(chunksize[0]);


    /*
     * --------------------------------------------------------
     * Work context
     * --------------------------------------------------------
     */

    ctx = malloc(sizeof(*ctx));

    if (ctx == NULL) {
        printf("Context problem.\n");
        return 1;
    }

    ctx->input = NULL;
    ctx->output = NULL;


    /*
     * --------------------------------------------------------
     * Result writer
     * --------------------------------------------------------
     */

    writer = create_writer();

    if (writer == NULL) {
        printf("ResultWriter creation problem.\n");

        free(ctx);

        return 1;
    }


    /*
     * --------------------------------------------------------
     * Open results file
     * --------------------------------------------------------
     */

    if (!writer->operations.open(
            writer,
            "results.csv",
            "a")) {

        printf("File opening problem.\n");

        free(writer);
        free(ctx);

        return 1;
    }


    /*
     * ========================================================
     * Benchmark
     * ========================================================
     */

    for (i = 0; i < numRoutines; ++i) {

        run = routines[i].run;
        name = routines[i].name;


        /*
         * ----------------------------------------------------
         * Problem sizes
         * ----------------------------------------------------
         */

        for (j = 0; j < numSizes; ++j) {

            /*
             * sizes[j] = log2(N)
             */
            real_size =
                ((uint64)1 << sizes[j]);


            /*
             * ------------------------------------------------
             * Input collection
             * ------------------------------------------------
             */

            ctx->input =
                collection_create(
                    (size_t)sizes[j]
                );

            if (ctx->input == NULL) {

                printf(
                    "Input collection creation problem.\n"
                );

                writer->operations.close(writer);
                free(writer);
                free(ctx);

                return 1;
            }


            /*
             * ------------------------------------------------
             * Output collection
             * ------------------------------------------------
             */

            ctx->output =
                collection_create(
                    (size_t)sizes[j]
                );

            if (ctx->output == NULL) {

                printf(
                    "Output collection creation problem.\n"
                );

                collection_destroy(ctx->input);

                writer->operations.close(writer);
                free(writer);
                free(ctx);

                return 1;
            }


            /*
             * ------------------------------------------------
             * Random data generator
             * ------------------------------------------------
             *
             * Il tipo concreto dell'RNG viene scelto
             * attraverso RNG_TYPE.
             *
             * stress.c non conosce xoshiro/splitmix.
             * ------------------------------------------------
             */

            generator =
                generator_random_create(
                    (datatype)0,
                    (datatype)real_size
                );

            if (generator == NULL) {

                printf(
                    "Generator creation problem.\n"
                );

                collection_destroy(ctx->input);
                collection_destroy(ctx->output);

                writer->operations.close(writer);
                free(writer);
                free(ctx);

                return 1;
            }


            /*
             * ------------------------------------------------
             * Fill input collection
             * ------------------------------------------------
             */

            if (!generator_fill(
                    generator,
                    ctx->input)) {

                printf(
                    "Collection generation problem.\n"
                );

                generator_destroy(generator);

                collection_destroy(ctx->input);
                collection_destroy(ctx->output);

                writer->operations.close(writer);
                free(writer);
                free(ctx);

                return 1;
            }


            /*
             * Il generatore non serve più dopo il fill.
             */
            generator_destroy(generator);
            generator = NULL;


            /*
             * =================================================
             * Thread configurations
             * =================================================
             */

            for (k = 0; k < numThreads; ++k) {

                ctx->threadnumber =
                    threadnumber[k];


                /*
                 * ------------------------------------------------
                 * Chunk configurations
                 * ------------------------------------------------
                 */

                for (l = 0; l < numChunks; ++l) {

                    printf(
                        "Testing %s with log2N=%u, "
                        "threadnumber=%hu, chunksize=%hu\n",
                        name,
                        (unsigned int)sizes[j],
                        threadnumber[k],
                        chunksize[l]
                    );


                    ctx->chunksize =
                        chunksize[l];

                    ctx->warmup_iterations =
                        WARMUP_REPS;

                    ctx->work_iterations =
                        WORK_REPS;


                    /*
                     * =============================================
                     * Warmup
                     * =============================================
                     */

                    for (
                        r = 0;
                        r < ctx->warmup_iterations;
                        ++r
                    ) {
                        run(ctx);
                    }


                    /*
                     * =============================================
                     * Benchmark
                     * =============================================
                     */

                    total = 0.0;

                    for (
                        r = 0;
                        r < ctx->work_iterations;
                        ++r
                    ) {

                        start =
                            omp_get_wtime();

                        run(ctx);

                        total +=
                            omp_get_wtime() -
                            start;
                    }


                    time =
                        total /
                        (double)ctx->work_iterations;


                    /*
                     * =============================================
                     * Result
                     * =============================================
                     */

                    result.benchname = name;

                    result.log2n =
                        (long)sizes[j];

                    result.threadnumber =
                        threadnumber[k];

                    result.chunksize =
                        chunksize[l];

                    result.time =
                        time;


                    /*
                     * =============================================
                     * Write result
                     * =============================================
                     */

                    if (!writer->operations.write(
                            writer,
                            &result)) {

                        printf(
                            "Result writing problem.\n"
                        );

                        collection_destroy(
                            ctx->input
                        );

                        collection_destroy(
                            ctx->output
                        );

                        writer->operations.close(
                            writer
                        );

                        free(writer);
                        free(ctx);

                        return 1;
                    }
                }
            }


            /*
             * ------------------------------------------------
             * Destroy collections
             * ------------------------------------------------
             */

            collection_destroy(ctx->input);
            collection_destroy(ctx->output);

            ctx->input = NULL;
            ctx->output = NULL;
        }


        /*
         * ----------------------------------------------------
         * Flush results after each routine
         * ----------------------------------------------------
         */

        writer->operations.flush(writer);
    }


    /*
     * ========================================================
     * Cleanup
     * ========================================================
     */

    writer->operations.close(writer);

    free(writer);
    free(ctx);

    return 0;
}