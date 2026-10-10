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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file params.h
 * @brief Compile-time configuration for benchmark builds.
 *
 * This header is the single source of compile-time benchmark parameters.
 *
 * The configuration is intentionally divided from the runtime
 * Configuration object:
 *
 * - this file selects the benchmark variant at compile time;
 * - Configuration materializes those values for runtime components;
 * - kernels consume the selected OpenMP schedule through CHOSEN_SCHEDULE.
 *
 * OpenMP scheduling is deliberately resolved at compile time.
 * The benchmark does not use omp_set_schedule() or schedule(runtime).
 */

#ifndef CONFIG_PARAMS_H
#define CONFIG_PARAMS_H

#include <stdint.h>
#include <stddef.h>


/* ========================================================================= */
/* Benchmark repetitions                                                     */
/* ========================================================================= */

/**
 * @brief Number of warm-up executions.
 */
#ifndef WARMUP_REPS
    #define WARMUP_REPS 3U
#endif


/**
 * @brief Number of measured executions.
 */
#ifndef WORK_REPS
    #define WORK_REPS 5U
#endif


/**
 * @brief Optional work amplification factor.
 */
#ifndef SLOWDOWN_FACTOR
    #define SLOWDOWN_FACTOR 0U
#endif


/* ========================================================================= */
/* OpenMP scheduling                                                         */
/* ========================================================================= */

/**
 * @brief Compile-time identifiers for supported OpenMP schedules.
 *
 * These identifiers are metadata used by the build configuration and
 * capability logic. They are not runtime scheduling state.
 */
#define SCHED_STATIC   1U
#define SCHED_DYNAMIC  2U
#define SCHED_GUIDED   3U


/**
 * @brief Selected OpenMP scheduling variant.
 *
 * The default benchmark variant uses static scheduling.
 */
#ifndef CHOSEN_SCHEDULE_ID
    #define CHOSEN_SCHEDULE_ID SCHED_STATIC
#endif


/*
 * CHOSEN_SCHEDULE contains only the scheduling policy token. Kernels append
 * the test-case chunk explicitly so every supported schedule uses the same
 * interface:
 *
 *     schedule(CHOSEN_SCHEDULE, chunk)
 *
 * The scheduling policy remains a compile-time property while chunk remains
 * a per-test execution parameter.
 */

#if CHOSEN_SCHEDULE_ID == SCHED_STATIC

    #define CHOSEN_SCHEDULE static

#elif CHOSEN_SCHEDULE_ID == SCHED_DYNAMIC

    #define CHOSEN_SCHEDULE dynamic

#elif CHOSEN_SCHEDULE_ID == SCHED_GUIDED

    #define CHOSEN_SCHEDULE guided

#else

    #error "Invalid CHOSEN_SCHEDULE_ID"

#endif


/* ========================================================================= */
/* Schedule metadata                                                         */
/* ========================================================================= */

/**
 * @brief Convert a preprocessor token sequence to a string literal.
 */
#define OES_STRINGIFY_IMPL(x) #x
#define OES_STRINGIFY(x)      OES_STRINGIFY_IMPL(x)


#if CHOSEN_SCHEDULE_ID == SCHED_STATIC

    #define CHOSEN_SCHEDULE_NAME "static"

#elif CHOSEN_SCHEDULE_ID == SCHED_DYNAMIC

    #define CHOSEN_SCHEDULE_NAME "dynamic"

#elif CHOSEN_SCHEDULE_ID == SCHED_GUIDED

    #define CHOSEN_SCHEDULE_NAME "guided"

#endif


/* ========================================================================= */
/* Benchmark suite selection                                                 */
/* ========================================================================= */

/**
 * @brief Enable the fixed-workload full-scale benchmark suite.
 *
 * Set to 1 to execute the full-scale test plan. Set to 0 to skip it.
 * This flag is independent from RUN_PROPORTIONAL.
 */
#ifndef RUN_FULL_SCALE
    #define RUN_FULL_SCALE 1
#endif


/**
 * @brief Enable the proportional/weak-scaling benchmark suite.
 *
 * Set to 1 to execute the proportional test plan. Set to 0 to skip it.
 * This flag is independent from RUN_FULL_SCALE.
 */
#ifndef RUN_PROPORTIONAL
    #define RUN_PROPORTIONAL 1
#endif


#if (RUN_FULL_SCALE != 0) && (RUN_FULL_SCALE != 1)
    #error "RUN_FULL_SCALE must be 0 or 1"
#endif


#if (RUN_PROPORTIONAL != 0) && (RUN_PROPORTIONAL != 1)
    #error "RUN_PROPORTIONAL must be 0 or 1"
#endif


#if (RUN_FULL_SCALE == 0) && (RUN_PROPORTIONAL == 0)
    #error "At least one benchmark suite must be enabled"
#endif


/* ========================================================================= */
/* Compiler / toolchain metadata                                             */
/* ========================================================================= */

/**
 * @brief Compiler family supplied by the build system.
 */
#ifndef OES_COMPILER_NAME
    #define OES_COMPILER_NAME "unknown"
#endif


/**
 * @brief Compiler version supplied by the build system.
 */
#ifndef OES_COMPILER_VERSION
    #define OES_COMPILER_VERSION "unknown"
#endif


/**
 * @brief Compiler flags supplied by the build system.
 */
#ifndef OES_COMPILER_FLAGS
    #define OES_COMPILER_FLAGS "unknown"
#endif


/**
 * @brief OpenMP runtime implementation supplied by the build system.
 */
#ifndef OES_OPENMP_RUNTIME
    #define OES_OPENMP_RUNTIME "unknown"
#endif


/* ========================================================================= */
/* Thread and problem-size configuration                                     */
/* ========================================================================= */

/**
 * @brief Comma-separated list of thread counts to benchmark.
 */
#ifndef THREAD_LIST
    #define THREAD_LIST 1U, 2U
#endif


/**
 * @brief Base-2 logarithm of the benchmark problem size.
 *
 * The actual number of elements is 2^PROBLEM_LOG2_SIZE.
 */
#ifndef PROBLEM_LOG2_SIZE
    #define PROBLEM_LOG2_SIZE 24U
#endif


/**
 * @brief Number of elements assigned to each thread in proportional
 *        scaling experiments.
 *
 * The total logical workload for N threads is:
 *
 *     SIZE_PER_THREAD * N
 *
 * The value is expressed in number of double-precision elements.
 *
 * The default value is consistent with the default problem size and
 * thread list:
 *
 *     2^25 elements/thread
 *     2^28 elements at 8 threads
 */
#ifndef SIZE_PER_THREAD
    #define SIZE_PER_THREAD UINT64_C(1024)
#endif


/**
 * @brief Alignment requested for benchmark data allocations.
 */
#ifndef MEMORY_ALIGNMENT
    #define MEMORY_ALIGNMENT 16U
#endif


/* ========================================================================= */
/* Window configuration                                                      */
/* ========================================================================= */

/**
 * @brief Comma-separated list of window sizes expressed as log2 values.
 *
 * Each value N represents a window of 2^N elements.
 */
#ifndef WINDOWS_LOG2_LIST
    #define WINDOWS_LOG2_LIST 20U
#endif


/**
 * @brief Comma-separated list of chunk sizes.
 *
 * These values are relevant to scheduling variants that accept a chunk
 * size, such as dynamic and guided scheduling.
 */
#ifndef CHUNK_SIZE_LIST
    #define CHUNK_SIZE_LIST 32U
#endif


/* ========================================================================= */
/* DataView shape configuration                                              */
/* ========================================================================= */

/**
 * @brief Number of columns for two-dimensional views.
 */
#ifndef CONFIG_VIEW_2D_COLS
    #define CONFIG_VIEW_2D_COLS \
        (UINT64_C(1) << (PROBLEM_LOG2_SIZE / 2U))
#endif


/**
 * @brief Depth of three-dimensional views.
 */
#ifndef CONFIG_VIEW_3D_DEPTH
    #define CONFIG_VIEW_3D_DEPTH \
        (UINT64_C(1) << (PROBLEM_LOG2_SIZE / 3U))
#endif


/**
 * @brief Number of columns for three-dimensional views.
 */
#ifndef CONFIG_VIEW_3D_COLS
    #define CONFIG_VIEW_3D_COLS \
        (UINT64_C(1) << (PROBLEM_LOG2_SIZE / 3U))
#endif


/**
 * @brief Size in bytes of an AoS structure.
 */
#ifndef CONFIG_VIEW_AOS_STRUCT_SIZE
    #define CONFIG_VIEW_AOS_STRUCT_SIZE 4ULL
#endif


/**
 * @brief Number of fields in an SoA view.
 */
#ifndef CONFIG_VIEW_SOA_NUM_FIELDS
    #define CONFIG_VIEW_SOA_NUM_FIELDS 4ULL
#endif


/**
 * @brief Vector length of an AoSoA view.
 */
#ifndef CONFIG_VIEW_AOSOA_VECTOR_LEN
    #define CONFIG_VIEW_AOSOA_VECTOR_LEN 8ULL
#endif


/**
 * @brief Number of fields in an AoSoA view.
 */
#ifndef CONFIG_VIEW_AOSOA_NUM_FIELDS
    #define CONFIG_VIEW_AOSOA_NUM_FIELDS 4ULL
#endif


/* ========================================================================= */
/* Compile-time validation                                                    */
/* ========================================================================= */

#if PROBLEM_LOG2_SIZE >= 64U
    #error "PROBLEM_LOG2_SIZE must be less than 64"
#endif


#if MEMORY_ALIGNMENT == 0U
    #error "MEMORY_ALIGNMENT must be greater than zero"
#endif


#if SIZE_PER_THREAD == 0U
    #error "SIZE_PER_THREAD must be greater than zero"
#endif


#if CONFIG_VIEW_2D_COLS == 0
    #error "CONFIG_VIEW_2D_COLS must be greater than zero"
#endif


#if CONFIG_VIEW_3D_DEPTH == 0
    #error "CONFIG_VIEW_3D_DEPTH must be greater than zero"
#endif


#if CONFIG_VIEW_3D_COLS == 0
    #error "CONFIG_VIEW_3D_COLS must be greater than zero"
#endif


#if CONFIG_VIEW_AOS_STRUCT_SIZE == 0
    #error "CONFIG_VIEW_AOS_STRUCT_SIZE must be greater than zero"
#endif


#if CONFIG_VIEW_SOA_NUM_FIELDS == 0
    #error "CONFIG_VIEW_SOA_NUM_FIELDS must be greater than zero"
#endif


#if CONFIG_VIEW_AOSOA_VECTOR_LEN == 0
    #error "CONFIG_VIEW_AOSOA_VECTOR_LEN must be greater than zero"
#endif


#if CONFIG_VIEW_AOSOA_NUM_FIELDS == 0
    #error "CONFIG_VIEW_AOSOA_NUM_FIELDS must be greater than zero"
#endif


/* ========================================================================= */
/* Utilities                                                                 */
/* ========================================================================= */

/**
 * @brief Number of elements in a statically defined array.
 */
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(array) \
        (sizeof(array) / sizeof((array)[0]))
#endif


/**
 * @brief Default deterministic seed for generated benchmark data.
 */
#ifndef GENERATION_SEED
    #define GENERATION_SEED UINT64_C(12345)
#endif


/**
 * @brief Convert a log2 window size to its actual element count.
 *
 * @param log2 Base-2 logarithm of the desired window size.
 *
 * @return 2^log2, or zero if log2 is outside the uint64_t range.
 */
static inline uint64_t
window_size_from_log2(uint32_t log2)
{
    if (log2 >= 64U) {
        return UINT64_C(0);
    }

    return UINT64_C(1) << log2;
}


/* ========================================================================= */
/* Materialized compile-time lists                                           */
/* ========================================================================= */

static const uint32_t CONFIG_WINDOW_LOG2[] = {
    WINDOWS_LOG2_LIST
};


#define CONFIG_WINDOWS_NUMBER \
    ARRAY_SIZE(CONFIG_WINDOW_LOG2)


#endif /* CONFIG_PARAMS_H */