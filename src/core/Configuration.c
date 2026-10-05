#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/Configuration.h"

#include "config/params.h"
#include "config/compilers.h"
#include "config/openmp.h"


/* ========================================================================= */
/* --- Derived metrics ---------------------------------------------------- */
/* ========================================================================= */

void finalize_configuration_metrics(
    Configuration *config
)
{
    if (!config)
        return;


    /* --------------------------------------------------------------------- */
    /* Real problem size                                                      */
    /* --------------------------------------------------------------------- */

    config->derived.real_size_bytes =
        UINT64_C(1) << config->execution.problem_size_exponent;


    /* --------------------------------------------------------------------- */
    /* Number of double elements                                              */
    /* --------------------------------------------------------------------- */

    config->derived.double_count =
        config->derived.real_size_bytes / sizeof(double);


    /* --------------------------------------------------------------------- */
    /* Maximum configured thread count                                        */
    /* --------------------------------------------------------------------- */

    uint32_t max_threads = 1U;

    if (config->execution.threads.data &&
        config->execution.threads.count > 0) {

        for (size_t i = 0;
             i < config->execution.threads.count;
             ++i) {

            if (config->execution.threads.data[i] >
                max_threads) {

                max_threads =
                    config->execution.threads.data[i];
            }
        }
    }


    /* --------------------------------------------------------------------- */
    /* Size assigned to each thread                                          */
    /* --------------------------------------------------------------------- */

    config->derived.size_per_thread =
        (max_threads > 0U)
            ? config->derived.real_size_bytes / max_threads
            : config->derived.real_size_bytes;
}


/* ========================================================================= */
/* --- Configuration loading ---------------------------------------------- */
/* ========================================================================= */

Configuration load_configuration(void)
{
    Configuration config = {0};


    /* ===================================================================== */
    /* Build information                                                     */
    /* ===================================================================== */

    snprintf(
        config.build.compiler_family,
        sizeof(config.build.compiler_family),
        "%s",
        OES_COMPILER_FAMILY
    );

    snprintf(
        config.build.compiler_name,
        sizeof(config.build.compiler_name),
        "%s",
        OES_COMPILER_NAME
    );

    snprintf(
        config.build.compiler_version,
        sizeof(config.build.compiler_version),
        "%s",
        OES_COMPILER_VERSION
    );

    snprintf(
        config.build.compiler_flags,
        sizeof(config.build.compiler_flags),
        "%s",
        OES_COMPILER_FLAGS
    );

    snprintf(
        config.build.openmp_version,
        sizeof(config.build.openmp_version),
        "%s",
        OPENMP_VERSION_STRING
    );


    /* ===================================================================== */
    /* Execution configuration                                               */
    /* ===================================================================== */

    config.execution.problem_size_exponent =
        PROBLEM_LOG2_SIZE;

    config.execution.schedule_id =
        CHOSEN_SCHEDULE_ID;


    /* ===================================================================== */
    /* Benchmark parameters                                                  */
    /* ===================================================================== */

    config.benchmark.warmup_reps =
        WARMUP_REPS;

    config.benchmark.work_reps =
        WORK_REPS;

    config.benchmark.slowdown_factor =
        SLOWDOWN_FACTOR;

    config.benchmark.generation_seed =
        GENERATION_SEED;


    /* ===================================================================== */
    /* Memory configuration                                                  */
    /* ===================================================================== */

    config.memory.alignment =
        MEMORY_ALIGNMENT;


    /* ===================================================================== */
    /* DataView shapes                                                       */
    /* ===================================================================== */

    config.shapes[VIEW_2D].v2d.cols =
        CONFIG_VIEW_2D_COLS;

    config.shapes[VIEW_3D].v3d.depth =
        CONFIG_VIEW_3D_DEPTH;

    config.shapes[VIEW_3D].v3d.cols =
        CONFIG_VIEW_3D_COLS;

    config.shapes[VIEW_AOS].aos.struct_size =
        CONFIG_VIEW_AOS_STRUCT_SIZE;

    config.shapes[VIEW_SOA].soa.num_fields =
        CONFIG_VIEW_SOA_NUM_FIELDS;

    config.shapes[VIEW_AOSOA].aosoa.vector_length =
        CONFIG_VIEW_AOSOA_VECTOR_LEN;

    config.shapes[VIEW_AOSOA].aosoa.num_fields =
        CONFIG_VIEW_AOSOA_NUM_FIELDS;


    /* ===================================================================== */
    /* Thread configurations                                                 */
    /* ===================================================================== */

    {
        const uint32_t default_threads[] = {
            THREAD_LIST
        };

        const size_t count =
            ARRAY_SIZE(default_threads);

        config.execution.threads.data =
            malloc(count * sizeof(uint32_t));

        if (config.execution.threads.data) {

            config.execution.threads.count =
                count;

            memcpy(
                config.execution.threads.data,
                default_threads,
                sizeof(default_threads)
            );
        }
    }


    /* ===================================================================== */
    /* Chunk configurations                                                  */
    /* ===================================================================== */

    {
        const uint64_t default_chunks[] = {
            CHUNK_SIZE_LIST
        };

        const size_t count =
            ARRAY_SIZE(default_chunks);

        config.execution.chunk_sizes.data =
            malloc(count * sizeof(uint64_t));

        if (config.execution.chunk_sizes.data) {

            config.execution.chunk_sizes.count =
                count;

            memcpy(
                config.execution.chunk_sizes.data,
                default_chunks,
                sizeof(default_chunks)
            );
        }
    }


    /* ===================================================================== */
    /* Window configurations                                                 */
    /* ===================================================================== */

    {
        config.execution.window_sizes.data =
            malloc(
                CONFIG_WINDOWS_NUMBER *
                sizeof(uint64_t)
            );

        if (config.execution.window_sizes.data) {

            config.execution.window_sizes.count =
                CONFIG_WINDOWS_NUMBER;

            for (size_t i = 0;
                 i < CONFIG_WINDOWS_NUMBER;
                 ++i) {

                config.execution.window_sizes.data[i] =
                    window_size_from_log2(
                        CONFIG_WINDOW_LOG2[i]
                    );
            }
        }
    }


    /* ===================================================================== */
    /* Derived metrics                                                       */
    /* ===================================================================== */

    finalize_configuration_metrics(&config);

    return config;
}


/* ========================================================================= */
/* --- Configuration destruction ------------------------------------------ */
/* ========================================================================= */

void destroy_configuration(
    Configuration *config
)
{
    if (!config)
        return;


    free(config->execution.threads.data);
    free(config->execution.window_sizes.data);
    free(config->execution.chunk_sizes.data);


    config->execution.threads.data = NULL;
    config->execution.threads.count = 0;

    config->execution.window_sizes.data = NULL;
    config->execution.window_sizes.count = 0;

    config->execution.chunk_sizes.data = NULL;
    config->execution.chunk_sizes.count = 0;
}
