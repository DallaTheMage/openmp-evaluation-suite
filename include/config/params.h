#ifndef CONFIG_PARAMS_H
#define CONFIG_PARAMS_H

#include <stddef.h>
#include <stdint.h>


/* ========================================================================= */
/* Benchmark repetitions                                                    */
/* ========================================================================= */

#ifndef WARMUP_REPS
    #define WARMUP_REPS 0U
#endif

#ifndef WORK_REPS
    #define WORK_REPS 1U
#endif

#ifndef SLOWDOWN_FACTOR
    #define SLOWDOWN_FACTOR 0U
#endif


/* ========================================================================= */
/* OpenMP scheduling                                                        */
/* ========================================================================= */

#define SCHED_STATIC   1U
#define SCHED_DYNAMIC  2U
#define SCHED_GUIDED   3U
#define SCHED_RUNTIME  4U

#ifndef CHOSEN_SCHEDULE_ID
    #define CHOSEN_SCHEDULE_ID SCHED_STATIC
#endif

#if CHOSEN_SCHEDULE_ID == SCHED_STATIC

    #define CHOSEN_SCHEDULE static

#elif CHOSEN_SCHEDULE_ID == SCHED_DYNAMIC

    #define CHOSEN_SCHEDULE dynamic

#elif CHOSEN_SCHEDULE_ID == SCHED_GUIDED

    #define CHOSEN_SCHEDULE guided

#elif CHOSEN_SCHEDULE_ID == SCHED_RUNTIME

    #define CHOSEN_SCHEDULE runtime

#else

    #error "CHOSEN_SCHEDULE_ID non valido"

#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x)  STRINGIFY2(x)

#define CHOSEN_SCHEDULE_STR STRINGIFY(CHOSEN_SCHEDULE)


/* ========================================================================= */
/* Compiler / toolchain metadata                                            */
/* ========================================================================= */

#ifndef OES_COMPILER_NAME
    #define OES_COMPILER_NAME ""
#endif

#ifndef OES_COMPILER_VERSION
    #define OES_COMPILER_VERSION ""
#endif

#ifndef OES_COMPILER_FLAGS
    #define OES_COMPILER_FLAGS ""
#endif

#ifndef OES_OPENMP_RUNTIME
    #define OES_OPENMP_RUNTIME ""
#endif


/* ========================================================================= */
/* Thread & problem size                                                    */
/* ========================================================================= */

#ifndef THREAD_LIST
    #define THREAD_LIST 1U, 2U, 4U, 8U
#endif

#ifndef PROBLEM_LOG2_SIZE
    #define PROBLEM_LOG2_SIZE 28U
#endif

#ifndef MEMORY_ALIGNMENT
    #define MEMORY_ALIGNMENT 16U
#endif


/* ========================================================================= */
/* Window configuration                                                     */
/* ========================================================================= */

#ifndef WINDOWS_LOG2_LIST
    #define WINDOWS_LOG2_LIST 28U, 26U, 24U, 22U
#endif

#ifndef CHUNK_SIZE_LIST
    #define CHUNK_SIZE_LIST 32U, 64U, 128U
#endif


/* ========================================================================= */
/* CSR configuration                                                        */
/* ========================================================================= */

/*
 * Fraction of elements represented by the CSR.
 *
 *     1.0  -> 100%
 *     0.1  -> 10%
 *     0.01 -> 1%
 *
 * The actual number of non-zero entries is derived from the
 * current window size.
 */
#ifndef CSR_DENSITY
    #define CSR_DENSITY 0.01
#endif


/* ========================================================================= */
/* View shape configuration                                                 */
/* ========================================================================= */

#ifndef CONFIG_VIEW_2D_COLS
    #define CONFIG_VIEW_2D_COLS \
        (UINT64_C(1) << ((PROBLEM_LOG2_SIZE) / 2U))
#endif


#ifndef CONFIG_VIEW_3D_DEPTH
    #define CONFIG_VIEW_3D_DEPTH \
        (UINT64_C(1) << ((PROBLEM_LOG2_SIZE) / 3U))
#endif

#ifndef CONFIG_VIEW_3D_COLS
    #define CONFIG_VIEW_3D_COLS \
        (UINT64_C(1) << ((PROBLEM_LOG2_SIZE) / 3U))
#endif


#ifndef CONFIG_VIEW_AOS_STRUCT_SIZE
    #define CONFIG_VIEW_AOS_STRUCT_SIZE 4ULL
#endif


#ifndef CONFIG_VIEW_SOA_NUM_FIELDS
    #define CONFIG_VIEW_SOA_NUM_FIELDS 4ULL
#endif


#ifndef CONFIG_VIEW_AOSOA_VECTOR_LEN
    #define CONFIG_VIEW_AOSOA_VECTOR_LEN 8ULL
#endif

#ifndef CONFIG_VIEW_AOSOA_NUM_FIELDS
    #define CONFIG_VIEW_AOSOA_NUM_FIELDS 4ULL
#endif


/* ========================================================================= */
/* Compile-time validation                                                  */
/* ========================================================================= */

#if PROBLEM_LOG2_SIZE >= 64U
    #error "PROBLEM_LOG2_SIZE deve essere < 64"
#endif

#if CONFIG_VIEW_2D_COLS == 0
    #error "[Config Error] CONFIG_VIEW_2D_COLS deve essere > 0"
#endif

#if CONFIG_VIEW_3D_DEPTH == 0
    #error "[Config Error] CONFIG_VIEW_3D_DEPTH deve essere > 0"
#endif

#if CONFIG_VIEW_3D_COLS == 0
    #error "[Config Error] CONFIG_VIEW_3D_COLS deve essere > 0"
#endif

#if CONFIG_VIEW_AOS_STRUCT_SIZE == 0
    #error "[Config Error] CONFIG_VIEW_AOS_STRUCT_SIZE deve essere > 0"
#endif

#if CONFIG_VIEW_SOA_NUM_FIELDS == 0
    #error "[Config Error] CONFIG_VIEW_SOA_NUM_FIELDS deve essere > 0"
#endif

#if CONFIG_VIEW_AOSOA_VECTOR_LEN == 0
    #error "[Config Error] CONFIG_VIEW_AOSOA_VECTOR_LEN deve essere > 0"
#endif

#if CONFIG_VIEW_AOSOA_NUM_FIELDS == 0
    #error "[Config Error] CONFIG_VIEW_AOSOA_NUM_FIELDS deve essere > 0"
#endif


/* ========================================================================= */
/* Utilities                                                                */
/* ========================================================================= */

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(arr) \
        (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef GENERATION_SEED
    #define GENERATION_SEED 12345ULL
#endif


/* ========================================================================= */
/* Runtime conversion helpers                                               */
/* ========================================================================= */

static inline uint64_t window_size_from_log2(uint32_t log2)
{
    if (log2 >= 64U) {
        return 0ULL;
    }

    return UINT64_C(1) << log2;
}


static const uint32_t CONFIG_WINDOW_LOG2[] = {
    WINDOWS_LOG2_LIST
};


#define CONFIG_WINDOWS_NUMBER \
    ARRAY_SIZE(CONFIG_WINDOW_LOG2)


#endif /* CONFIG_PARAMS_H */
