#ifndef CORE_CONFIGURATION_H
#define CORE_CONFIGURATION_H

#include <stddef.h>
#include <stdint.h>

#include "data/views/ViewType.h"


/* ========================================================================= */
/* Dynamic arrays                                                           */
/* ========================================================================= */

typedef struct {
    uint32_t *data;
    size_t    count;
} IntArray32;

typedef struct {
    uint64_t *data;
    size_t    count;
} IntArray64;


/* ========================================================================= */
/* DataView shape configuration                                             */
/* ========================================================================= */

typedef struct {
    uint64_t cols;
} ViewShape2DConfig;

typedef struct {
    uint64_t depth;
    uint64_t cols;
} ViewShape3DConfig;

typedef struct {
    uint64_t struct_size;
} ViewShapeAoSConfig;

typedef struct {
    uint64_t num_fields;
} ViewShapeSoAConfig;

typedef struct {
    uint64_t vector_length;
    uint64_t num_fields;
} ViewShapeAoSoAConfig;

typedef struct {
    double density;
} ViewCSRConfig;

typedef union {
    ViewShape2DConfig    v2d;
    ViewShape3DConfig    v3d;
    ViewCSRConfig        csr;
    ViewShapeAoSConfig   aos;
    ViewShapeSoAConfig   soa;
    ViewShapeAoSoAConfig aosoa;
} ViewShapeConfig;


/* ========================================================================= */
/* Build configuration                                                      */
/* ========================================================================= */

typedef struct {
    char compiler_family[32];
    char compiler_name[32];
    char compiler_version[32];
    char compiler_flags[256];
    char openmp_version[16];
} BuildConfig;


/* ========================================================================= */
/* Execution configuration                                                  */
/* ========================================================================= */

typedef struct {
    uint64_t problem_size_exponent;

    IntArray32 threads;
    IntArray64 window_sizes;
    IntArray64 chunk_sizes;

    int schedule_id;
} ExecutionConfig;


/* ========================================================================= */
/* Benchmark configuration                                                  */
/* ========================================================================= */

typedef struct {
    uint32_t warmup_reps;
    uint32_t work_reps;
    uint32_t slowdown_factor;

    uint64_t generation_seed;
} BenchmarkConfig;


/* ========================================================================= */
/* Memory configuration                                                     */
/* ========================================================================= */

typedef struct {
    uint16_t alignment;
} MemoryConfig;

/* ========================================================================= */
/* Derived metrics                                                          */
/* ========================================================================= */

typedef struct {
    uint64_t real_size_bytes;
    uint64_t double_count;
    uint64_t size_per_thread;
} DerivedMetrics;


/* ========================================================================= */
/* Complete configuration                                                   */
/* ========================================================================= */

typedef struct {
    BuildConfig      build;
    ExecutionConfig  execution;
    BenchmarkConfig  benchmark;
    MemoryConfig     memory;

    ViewShapeConfig  shapes[VIEW_TYPE_COUNT];

    DerivedMetrics   derived;
} Configuration;


/* ========================================================================= */
/* Lifecycle                                                                */
/* ========================================================================= */

Configuration load_configuration(void);

void finalize_configuration_metrics(
    Configuration *config
);

void destroy_configuration(
    Configuration *config
);

#endif /* CORE_CONFIGURATION_H */
