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
 * @file Configuration.h
 * @brief Central configuration contract for the benchmark suite.
 *
 * This header defines the immutable-at-runtime description of a benchmark
 * configuration:
 *
 * (+) Compiler and OpenMP build metadata;
 * (+) Problem and execution parameters;
 * (+) Benchmark repetition parameters;
 * (+) Memory allocation parameters;
 * (+) DataView shape parameters;
 * (+) Metrics derived from the configured problem size.
 *
 * The configuration does not contain OpenMP scheduling state.
 * The OpenMP schedule is a compile-time property of a benchmark build and
 * is therefore defined through config/params.h and encoded directly in
 * OpenMP pragmas.
 *
 * In particular, the benchmark must not rely on:
 *
 *     omp_set_schedule()
 *     schedule(runtime)
 *
 * for selecting the scheduling policy.
 *
 * This design keeps the hot execution path independent from runtime
 * scheduling configuration and makes benchmark variants reproducible.
 */

#ifndef CORE_CONFIGURATION_H
#define CORE_CONFIGURATION_H

#include <stddef.h>
#include <stdint.h>

#include "data/views/ViewType.h"


/* ========================================================================= */
/* Configuration-owned dynamic arrays                                       */
/* ========================================================================= */

/**
 * @brief Dynamically allocated array of 32-bit unsigned integers.
 *
 * Ownership belongs to Configuration.
 */
typedef struct {
    uint32_t *data;
    size_t    count;
} ConfigU32List;


/**
 * @brief Dynamically allocated array of 64-bit unsigned integers.
 *
 * Ownership belongs to Configuration.
 */
typedef struct {
    uint64_t *data;
    size_t    count;
} ConfigU64List;


/* ========================================================================= */
/* DataView shape configuration                                             */
/* ========================================================================= */

/**
 * @brief Configuration parameters for a two-dimensional view.
 */
typedef struct {
    uint64_t cols;
} ViewShape2DConfig;


/**
 * @brief Configuration parameters for a three-dimensional view.
 */
typedef struct {
    uint64_t depth;
    uint64_t cols;
} ViewShape3DConfig;


/**
 * @brief Configuration parameters for an Array-of-Structures view.
 */
typedef struct {
    uint64_t struct_size;
} ViewShapeAoSConfig;


/**
 * @brief Configuration parameters for a Structure-of-Arrays view.
 */
typedef struct {
    uint64_t num_fields;
} ViewShapeSoAConfig;


/**
 * @brief Configuration parameters for an Array-of-Structures-of-Arrays view.
 */
typedef struct {
    uint64_t vector_length;
    uint64_t num_fields;
} ViewShapeAoSoAConfig;


/**
 * @brief Configuration parameters for a compressed sparse row view.
 */
typedef struct {
    double density;
} ViewCSRConfig;


/**
 * @brief Shape-specific configuration.
 *
 * VIEW_1D does not require additional shape parameters.
 *
 * The active member is selected according to the corresponding ViewType.
 */
typedef union {
    ViewShape2DConfig    v2d;
    ViewShape3DConfig    v3d;
    ViewCSRConfig        csr;
    ViewShapeAoSConfig   aos;
    ViewShapeSoAConfig   soa;
    ViewShapeAoSoAConfig aosoa;
} ViewShapeConfig;


/* ========================================================================= */
/* Build metadata                                                            */
/* ========================================================================= */

/**
 * @brief Metadata describing the binary that executes the benchmark.
 *
 * These fields are descriptive metadata only. They do not control the
 * OpenMP runtime during benchmark execution.
 */
typedef struct {
    char compiler_family[32];
    char compiler_name[32];
    char compiler_version[32];
    char compiler_flags[256];
    char openmp_version[16];
} BuildConfig;


/* ========================================================================= */
/* Execution configuration                                                   */
/* ========================================================================= */

/**
 * @brief Parameters defining the benchmark execution space.
 *
 * The OpenMP scheduling policy is intentionally absent from this structure.
 * Scheduling is a compile-time property selected through config/params.h.
 */
typedef struct {
    /**
     * @brief Base-2 logarithm of the configured problem size.
     *
     * The actual number of elements is 2^problem_size_log2.
     */
    uint64_t problem_size_log2;

    /**
     * @brief Thread counts to benchmark.
     */
    ConfigU32List threads;

    /**
     * @brief Data-window sizes to benchmark.
     */
    ConfigU64List window_sizes;

    /**
     * @brief Chunk sizes used by scheduling policies that accept a chunk.
     *
     * For example, dynamic and guided scheduling variants may use these
     * values. The list is harmless for static scheduling and allows the
     * same configuration model to describe multiple build variants.
     */
    ConfigU64List chunk_sizes;
} ExecutionConfig;


/* ========================================================================= */
/* Benchmark configuration                                                   */
/* ========================================================================= */

/**
 * @brief Parameters controlling benchmark repetition and data generation.
 */
typedef struct {
    /**
     * @brief Number of warm-up executions.
     */
    uint32_t warmup_reps;

    /**
     * @brief Number of measured executions.
     */
    uint32_t work_reps;

    /**
     * @brief Work amplification factor used by selected kernels.
     */
    uint32_t slowdown_factor;

    /**
     * @brief Seed used for deterministic data generation.
     */
    uint64_t generation_seed;
} BenchmarkConfig;


/* ========================================================================= */
/* Memory configuration                                                      */
/* ========================================================================= */

/**
 * @brief Parameters controlling benchmark data allocation.
 */
typedef struct {
    /**
     * @brief Required alignment in bytes.
     *
     * size_t is used because allocation APIs represent alignment-related
     * quantities using size_t-compatible types.
     */
    size_t alignment;
} MemoryConfig;


/* ========================================================================= */
/* Derived configuration metrics                                             */
/* ========================================================================= */

/**
 * @brief Values derived from the configured problem size.
 *
 * These values depend only on the global problem size. Quantities that
 * depend on a particular thread count or DataView window do not belong here.
 */
typedef struct {
    /**
     * @brief Total number of elements in the benchmark data pool.
     */
    uint64_t problem_size_elements;

    /**
     * @brief Total size of the benchmark data pool in bytes.
     */
    uint64_t problem_size_bytes;
} DerivedMetrics;


/* ========================================================================= */
/* Complete configuration                                                    */
/* ========================================================================= */

/**
 * @brief Complete benchmark configuration.
 *
 * Configuration owns the dynamic arrays contained in:
 *
 * - execution.threads;
 * - execution.window_sizes;
 * - execution.chunk_sizes.
 *
 * All other members are stored directly inside the structure.
 */
typedef struct {
    BuildConfig     build;
    ExecutionConfig execution;
    BenchmarkConfig benchmark;
    MemoryConfig    memory;

    /**
     * @brief Shape parameters indexed by ViewType.
     *
     * VIEW_TYPE_COUNT is used only as the array dimension and is not itself
     * a valid view type.
     */
    ViewShapeConfig shapes[VIEW_TYPE_COUNT];

    DerivedMetrics derived;
} Configuration;


/* ========================================================================= */
/* Configuration lifecycle                                                   */
/* ========================================================================= */

/**
 * @brief Load the benchmark configuration from compile-time parameters.
 *
 * The returned Configuration owns all dynamically allocated arrays.
 *
 * @return A fully initialized configuration object.
 */
Configuration load_configuration(void);


/**
 * @brief Compute metrics derived from the configured problem size.
 *
 * This function must be called after load_configuration() if the loader does
 * not already finalize the derived values.
 *
 * @param config Configuration to finalize.
 */
void finalize_configuration_metrics(Configuration *config);


/**
 * @brief Release resources owned by a configuration.
 *
 * Passing NULL is allowed and has no effect.
 *
 * @param config Configuration whose dynamically allocated members are freed.
 */
void destroy_configuration(Configuration *config);


#endif /* CORE_CONFIGURATION_H */