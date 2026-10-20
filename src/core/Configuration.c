/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2026 Lorenzo Dall'Ara
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite,
 * developed as part of a Bachelor's thesis at the
 * Alma Mater Studiorum - University of Bologna.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
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
#include "utils/ArrayCopy.h"


/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static inline void copy_string(char *target, size_t capacity, const char *string) {
    (void)snprintf(target, capacity, "%s", string);
}

/**
 * @brief Copy a uint32_t configuration list into owned storage.
 *
 * @param destination Destination configuration list.
 * @param source Source array.
 * @param count Number of elements in source.
 *
 * @return 1 on success, 0 on invalid input or allocation failure.
 */
static int copy_u32_list(ConfigU32List *destination, const uint32_t *source, size_t count) {
    if (destination == NULL) {
        return 0;
    }
    uint32_t *data = utils_array_copy(source, count, sizeof(*data));
    if (data == NULL) {
        return 0;
    }
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
static int copy_u64_list(ConfigU64List *destination, const uint64_t *source, size_t count) {
    if (destination == NULL) {
        return 0;
    }
    uint64_t *data = utils_array_copy(source, count, sizeof(*data));
    if (data == NULL) {
        return 0;
    }
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
static int materialize_window_sizes(ConfigU64List *destination) {
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

    destination->data  = data;
    destination->count = count;

    return 1;
}


/* ========================================================================= */
/* Derived metrics                                                           */
/* ========================================================================= */

void finalize_configuration_metrics(Configuration *config) {
    uint64_t elements;

    if (config == NULL) {
        return;
    }

    /* PROBLEM_LOG2_SIZE is validated to be strictly smaller than 64. */
    elements = UINT64_C(1) << config->execution.problem_size_log2;

    config->derived.problem_size_elements = elements;

    config->derived.problem_size_bytes =
        elements > (UINT64_MAX / (uint64_t)sizeof(double))
        ? UINT64_MAX
        : elements * (uint64_t)sizeof(double);

    config->derived.size_per_thread = (uint64_t)SIZE_PER_THREAD;
}


/* ========================================================================= */
/* Configuration loading                                                     */
/* ========================================================================= */

Configuration load_configuration(void) {
    Configuration config = {0};

    /* --------------------------------------------------------------------- */
    /* Build metadata                                                        */
    /* --------------------------------------------------------------------- */

    copy_string(config.build.compiler_family,
        sizeof(config.build.compiler_family),
        OES_COMPILER_FAMILY
    );
    copy_string(config.build.compiler_name,
        sizeof(config.build.compiler_name),
        OES_COMPILER_NAME
    );
    copy_string(config.build.compiler_version,
        sizeof(config.build.compiler_version),
        OES_COMPILER_VERSION
    );
    copy_string(config.build.compiler_flags,
        sizeof(config.build.compiler_flags),
        OES_COMPILER_FLAGS
    );
    copy_string(config.build.openmp_version,
        sizeof(config.build.openmp_version),
        OPENMP_VERSION_STRING
    );

    /* --------------------------------------------------------------------- */
    /* Execution configuration                                               */
    /* --------------------------------------------------------------------- */

    config.execution.problem_size_log2 = (uint64_t)PROBLEM_LOG2_SIZE;

    /* --------------------------------------------------------------------- */
    /* Benchmark configuration                                               */
    /* --------------------------------------------------------------------- */

    config.benchmark.warmup_reps     = (uint32_t)WARMUP_REPS;
    config.benchmark.work_reps       = (uint32_t)WORK_REPS;
    config.benchmark.slowdown_factor = (uint32_t)SLOWDOWN_FACTOR;
    config.benchmark.generation_seed = (uint64_t)GENERATION_SEED;


    /* --------------------------------------------------------------------- */
    /* Memory configuration                                                  */
    /* --------------------------------------------------------------------- */

    config.memory.alignment               = (size_t)MEMORY_ALIGNMENT;
    config.memory.detected_llc_size_bytes = (uint64_t)DETECTED_LLC_SIZE_BYTES;

    /*
     * AUX_BUFFER_SIZE is expressed in double elements.
     * Check that the value is representable by size_t before storing it.
     */
    if ((uint64_t)AUX_BUFFER_SIZE > (uint64_t)SIZE_MAX) {
        destroy_configuration(&config);
        return (Configuration){0};
    }

    config.memory.aux_buffer_size = (size_t)AUX_BUFFER_SIZE;

    /* --------------------------------------------------------------------- */
    /* Cache eviction configuration                                          */
    /* --------------------------------------------------------------------- */

    config.memory.cache_eviction.enabled      = (uint32_t)CACHE_EVICTION_ENABLED;
    config.memory.cache_eviction.multiplier   = (uint32_t)CACHE_EVICTION_MULTIPLIER;
    config.memory.cache_eviction.stride_bytes = (size_t)CACHE_EVICTION_STRIDE_BYTES;

    /*
     * Validate cache eviction parameters.
     *
     * enabled must be either 0 or 1.
     * multiplier and stride must be strictly positive.
     */
    if (config.memory.cache_eviction.enabled > 1U
        || config.memory.cache_eviction.multiplier == 0U
        || config.memory.cache_eviction.stride_bytes == 0U) {

        fprintf(
            stderr,
            "[Configuration] Invalid cache eviction parameters\n"
        );

        destroy_configuration(&config);
        return (Configuration){0};
    }

    /*
     * When cache eviction is enabled, verify that:
     *
     * 1. The detected LLC size is available.
     * 2. The eviction buffer size does not overflow uint64_t.
     * 3. The resulting size is representable by size_t.
     */
    if (config.memory.cache_eviction.enabled != 0U) {
        const uint64_t llc_bytes  = config.memory.detected_llc_size_bytes;
        const uint64_t multiplier = (uint64_t)config.memory.cache_eviction.multiplier;

        if ((llc_bytes == 0U)
            || (llc_bytes > (UINT64_MAX / multiplier))
            || ((llc_bytes * multiplier) > (uint64_t)SIZE_MAX)) {

            fprintf(
                stderr,
                "[Configuration] Invalid cache eviction buffer size\n"
            );

            destroy_configuration(&config);
            return (Configuration){0};
        }
    }


    /* --------------------------------------------------------------------- */
    /* DataView shape configuration                                          */
    /* --------------------------------------------------------------------- */

    config.shapes[VIEW_2D].v2d.cols               = (uint64_t)CONFIG_VIEW_2D_COLS;
    config.shapes[VIEW_3D].v3d.depth              = (uint64_t)CONFIG_VIEW_3D_DEPTH;
    config.shapes[VIEW_3D].v3d.cols               = (uint64_t)CONFIG_VIEW_3D_COLS;
    config.shapes[VIEW_AOS].aos.struct_size       = (uint64_t)CONFIG_VIEW_AOS_STRUCT_SIZE;
    config.shapes[VIEW_SOA].soa.num_fields        = (uint64_t)CONFIG_VIEW_SOA_NUM_FIELDS;
    config.shapes[VIEW_AOSOA].aosoa.vector_length = (uint64_t)CONFIG_VIEW_AOSOA_VECTOR_LEN;
    config.shapes[VIEW_AOSOA].aosoa.num_fields    = (uint64_t)CONFIG_VIEW_AOSOA_NUM_FIELDS;

    /* --------------------------------------------------------------------- */
    /* Thread configuration                                                  */
    /* --------------------------------------------------------------------- */
    {
        static const uint32_t default_threads[] = { THREAD_LIST };
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
        static const uint64_t default_chunks[] = { CHUNK_SIZE_LIST };
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

void destroy_configuration(Configuration *config) {
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