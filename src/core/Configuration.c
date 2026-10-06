/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file Configuration.c
 * @brief Configuration materialization and lifecycle management.
 *
 * This translation unit converts the compile-time parameters exposed by
 * config/params.h into the runtime Configuration object.
 *
 * Configuration owns its dynamically allocated execution lists. OpenMP
 * scheduling remains a compile-time property and is therefore not stored
 * in the runtime configuration.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/compilers.h"
#include "config/openmp.h"
#include "config/params.h"
#include "core/Configuration.h"


/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

/**
 * @brief Copy a uint32_t configuration list into owned storage.
 *
 * @param destination Destination configuration list.
 * @param source Source array.
 * @param count Number of elements in source.
 *
 * @return 1 on success, 0 on invalid input or allocation failure.
 */
static int copy_u32_list(
    ConfigU32List *destination,
    const uint32_t *source,
    size_t count
)
{
    uint32_t *data;

    if (destination == NULL || source == NULL || count == 0U) {
        return 0;
    }

    if (count > SIZE_MAX / sizeof(*data)) {
        return 0;
    }

    data = malloc(count * sizeof(*data));
    if (data == NULL) {
        return 0;
    }

    memcpy(data, source, count * sizeof(*data));

    destination->data = data;
    destination->count = count;

    return 1;
}


/**
 * @brief Copy a uint64_t configuration list into owned storage.
 *
 * @param destination Destination configuration list.
 * @param source Source array.
 * @param count Number of elements in source.
 *
 * @return 1 on success, 0 on invalid input or allocation failure.
 */
static int copy_u64_list(
    ConfigU64List *destination,
    const uint64_t *source,
    size_t count
)
{
    uint64_t *data;

    if (destination == NULL || source == NULL || count == 0U) {
        return 0;
    }

    if (count > SIZE_MAX / sizeof(*data)) {
        return 0;
    }

    data = malloc(count * sizeof(*data));
    if (data == NULL) {
        return 0;
    }

    memcpy(data, source, count * sizeof(*data));

    destination->data = data;
    destination->count = count;

    return 1;
}


/**
 * @brief Materialize the configured window sizes.
 *
 * Window sizes are specified in params.h as base-2 logarithms.
 *
 * @param destination Destination configuration list.
 *
 * @return 1 on success, 0 if the configuration cannot be materialized.
 */
static int materialize_window_sizes(ConfigU64List *destination)
{
    const size_t count = CONFIG_WINDOWS_NUMBER;
    uint64_t *data;

    if (destination == NULL || count == 0U) {
        return 0;
    }

    if (count > SIZE_MAX / sizeof(*data)) {
        return 0;
    }

    data = malloc(count * sizeof(*data));
    if (data == NULL) {
        return 0;
    }

    for (size_t i = 0U; i < count; ++i) {
        const uint64_t window_size =
            window_size_from_log2(CONFIG_WINDOW_LOG2[i]);

        if (window_size == UINT64_C(0)) {
            free(data);
            return 0;
        }

        data[i] = window_size;
    }

    destination->data = data;
    destination->count = count;

    return 1;
}


/* ========================================================================= */
/* Derived metrics                                                           */
/* ========================================================================= */

void finalize_configuration_metrics(Configuration *config)
{
    uint64_t elements;

    if (config == NULL) {
        return;
    }

    /* PROBLEM_LOG2_SIZE is validated to be strictly smaller than 64. */
    elements = UINT64_C(1) << config->execution.problem_size_log2;

    config->derived.problem_size_elements = elements;

    if (elements > UINT64_MAX / (uint64_t)sizeof(double)) {
        config->derived.problem_size_bytes = UINT64_MAX;
    } else {
        config->derived.problem_size_bytes =
            elements * (uint64_t)sizeof(double);
    }

    config->derived.size_per_thread =
        (uint64_t)SIZE_PER_THREAD;
}


/* ========================================================================= */
/* Configuration loading                                                     */
/* ========================================================================= */

Configuration load_configuration(void)
{
    Configuration config = {0};

    /* --------------------------------------------------------------------- */
    /* Build metadata                                                        */
    /* --------------------------------------------------------------------- */

    (void)snprintf(
        config.build.compiler_family,
        sizeof(config.build.compiler_family),
        "%s",
        OES_COMPILER_FAMILY
    );

    (void)snprintf(
        config.build.compiler_name,
        sizeof(config.build.compiler_name),
        "%s",
        OES_COMPILER_NAME
    );

    (void)snprintf(
        config.build.compiler_version,
        sizeof(config.build.compiler_version),
        "%s",
        OES_COMPILER_VERSION
    );

    (void)snprintf(
        config.build.compiler_flags,
        sizeof(config.build.compiler_flags),
        "%s",
        OES_COMPILER_FLAGS
    );

    (void)snprintf(
        config.build.openmp_version,
        sizeof(config.build.openmp_version),
        "%s",
        OPENMP_VERSION_STRING
    );

    /* --------------------------------------------------------------------- */
    /* Execution configuration                                              */
    /* --------------------------------------------------------------------- */

    config.execution.problem_size_log2 =
        (uint64_t)PROBLEM_LOG2_SIZE;

    /* --------------------------------------------------------------------- */
    /* Benchmark configuration                                               */
    /* --------------------------------------------------------------------- */

    config.benchmark.warmup_reps =
        (uint32_t)WARMUP_REPS;
    config.benchmark.work_reps =
        (uint32_t)WORK_REPS;
    config.benchmark.slowdown_factor =
        (uint32_t)SLOWDOWN_FACTOR;
    config.benchmark.generation_seed =
        (uint64_t)GENERATION_SEED;

    /* --------------------------------------------------------------------- */
    /* Memory configuration                                                  */
    /* --------------------------------------------------------------------- */

    config.memory.alignment =
        (size_t)MEMORY_ALIGNMENT;

    /* --------------------------------------------------------------------- */
    /* DataView shape configuration                                          */
    /* --------------------------------------------------------------------- */

    config.shapes[VIEW_2D].v2d.cols =
        (uint64_t)CONFIG_VIEW_2D_COLS;

    config.shapes[VIEW_3D].v3d.depth =
        (uint64_t)CONFIG_VIEW_3D_DEPTH;
    config.shapes[VIEW_3D].v3d.cols =
        (uint64_t)CONFIG_VIEW_3D_COLS;

    config.shapes[VIEW_AOS].aos.struct_size =
        (uint64_t)CONFIG_VIEW_AOS_STRUCT_SIZE;

    config.shapes[VIEW_SOA].soa.num_fields =
        (uint64_t)CONFIG_VIEW_SOA_NUM_FIELDS;

    config.shapes[VIEW_AOSOA].aosoa.vector_length =
        (uint64_t)CONFIG_VIEW_AOSOA_VECTOR_LEN;
    config.shapes[VIEW_AOSOA].aosoa.num_fields =
        (uint64_t)CONFIG_VIEW_AOSOA_NUM_FIELDS;

    /* --------------------------------------------------------------------- */
    /* Thread configuration                                                  */
    /* --------------------------------------------------------------------- */

    {
        static const uint32_t default_threads[] = {
            THREAD_LIST
        };

        if (!copy_u32_list(
                &config.execution.threads,
                default_threads,
                ARRAY_SIZE(default_threads))) {
            destroy_configuration(&config);
            return (Configuration){0};
        }
    }

    /* --------------------------------------------------------------------- */
    /* Chunk configuration                                                   */
    /* --------------------------------------------------------------------- */

    {
        static const uint64_t default_chunks[] = {
            CHUNK_SIZE_LIST
        };

        if (!copy_u64_list(
                &config.execution.chunk_sizes,
                default_chunks,
                ARRAY_SIZE(default_chunks))) {
            destroy_configuration(&config);
            return (Configuration){0};
        }
    }

    /* --------------------------------------------------------------------- */
    /* Window configuration                                                  */
    /* --------------------------------------------------------------------- */

    if (!materialize_window_sizes(&config.execution.window_sizes)) {
        destroy_configuration(&config);
        return (Configuration){0};
    }

    /* --------------------------------------------------------------------- */
    /* Derived metrics                                                       */
    /* --------------------------------------------------------------------- */

    finalize_configuration_metrics(&config);

    return config;
}


/* ========================================================================= */
/* Configuration destruction                                                */
/* ========================================================================= */

void destroy_configuration(Configuration *config)
{
    if (config == NULL) {
        return;
    }

    free(config->execution.threads.data);
    free(config->execution.window_sizes.data);
    free(config->execution.chunk_sizes.data);

    config->execution.threads.data = NULL;
    config->execution.threads.count = 0U;

    config->execution.window_sizes.data = NULL;
    config->execution.window_sizes.count = 0U;

    config->execution.chunk_sizes.data = NULL;
    config->execution.chunk_sizes.count = 0U;
}