#include "kernels/simd.h"
#include "profiling/Profiler.h"

#if OPENMP_HAS_SIMD


/* ============================================================================
 * SIMD MEMORY BOUND
 * ============================================================================
 *
 * Obiettivo:
 *   Evidenziare il comportamento della memoria e del pattern di accesso.
 *
 * Il lavoro computazionale per elemento è volutamente minimo:
 *
 *     load -> reduction
 *
 * In questo modo le differenze tra 2D, 3D, AoS, SoA, AoSoA e CSR
 * sono maggiormente influenzate dal layout e dalla località degli accessi.
 * ========================================================================== */


/* --------------------------------------------------------------------------
 * VIEW_2D
 * -------------------------------------------------------------------------- */

void kernel_2d_simd_memory(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows = view->meta.v2d.rows;
    const uint64_t cols = view->meta.v2d.cols;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {

            const uint64_t offset = r * cols;

            #pragma omp simd reduction(+:sink)
            for (uint64_t c = 0; c < cols; ++c) {
                sink += pool[offset + c];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_3D
 * -------------------------------------------------------------------------- */

void kernel_3d_simd_memory(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows  = view->meta.v3d.rows;
    const uint64_t cols  = view->meta.v3d.cols;
    const uint64_t depth = view->meta.v3d.depth;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, depth, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {

            for (uint64_t c = 0; c < cols; ++c) {

                const uint64_t offset =
                    (r * cols + c) * depth;

                #pragma omp simd reduction(+:sink)
                for (uint64_t d = 0; d < depth; ++d) {
                    sink += pool[offset + d];
                }
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_AOS
 * -------------------------------------------------------------------------- */

void kernel_aos_simd_memory(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aos.num_elements;

    const uint64_t struct_size =
        view->meta.aos.struct_size;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, struct_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t i = 0; i < num_elems; ++i) {

            const uint64_t base = i * struct_size;

            #pragma omp simd reduction(+:sink)
            for (uint64_t f = 0; f < struct_size; ++f) {
                sink += pool[base + f];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_SOA
 * -------------------------------------------------------------------------- */

void kernel_soa_simd_memory(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.soa.num_elements;

    const uint64_t num_fields =
        view->meta.soa.num_fields;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, num_fields, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t f = 0; f < num_fields; ++f) {

            const uint64_t field_offset =
                f * num_elems;

            #pragma omp simd reduction(+:sink)
            for (uint64_t i = 0; i < num_elems; ++i) {
                sink += pool[field_offset + i];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_AOSOA
 * -------------------------------------------------------------------------- */

void kernel_aosoa_simd_memory(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aosoa.num_elements;

    const uint64_t num_fields =
        view->meta.aosoa.num_fields;

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    const uint64_t num_tiles =
        (vl > 0) ? (num_elems / vl) : 0;

    const uint64_t tile_size =
        num_fields * vl;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_tiles, num_fields, vl, tile_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t t = 0; t < num_tiles; ++t) {

            const uint64_t tile_base =
                t * tile_size;

            for (uint64_t f = 0; f < num_fields; ++f) {

                const uint64_t field_base =
                    tile_base + f * vl;

                #pragma omp simd reduction(+:sink)
                for (uint64_t v = 0; v < vl; ++v) {
                    sink += pool[field_base + v];
                }
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_CSR
 * -------------------------------------------------------------------------- */

void kernel_csr_simd_memory(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const uint64_t nrows =
        view->meta.csr.nrows;

    const uint64_t *restrict row_ptr =
        view->meta.csr.row_ptr;

    const uint64_t *restrict col_ind =
        view->meta.csr.col_ind;

    const double *restrict val =
        view->buffer->pool;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(nrows, row_ptr, col_ind, val, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < nrows; ++r) {

            const uint64_t start = row_ptr[r];
            const uint64_t end   = row_ptr[r + 1];

            #pragma omp simd reduction(+:sink)
            for (uint64_t idx = start; idx < end; ++idx) {

                const uint64_t col =
                    col_ind[idx];

                sink += val[idx] * val[col];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* ============================================================================
 * SIMD COMPUTE BOUND
 * ============================================================================
 *
 * Obiettivo:
 *   Aumentare il lavoro aritmetico per ogni elemento caricato.
 *
 * Il pattern di accesso rimane identico alla versione MEMORY.
 * Cambia solamente l'intensità computazionale.
 *
 * Questo permette di osservare quanto le differenze di layout/accesso
 * vengano attenuate quando il collo di bottiglia passa dalla memoria
 * alle unità di calcolo SIMD.
 * ========================================================================== */


/* --------------------------------------------------------------------------
 * VIEW_2D
 * -------------------------------------------------------------------------- */

void kernel_2d_simd_compute(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows = view->meta.v2d.rows;
    const uint64_t cols = view->meta.v2d.cols;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {

            const uint64_t offset = r * cols;

            #pragma omp simd reduction(+:sink)
            for (uint64_t c = 0; c < cols; ++c) {

                double x = pool[offset + c];

                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;

                sink += x;
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_3D
 * -------------------------------------------------------------------------- */

void kernel_3d_simd_compute(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows  = view->meta.v3d.rows;
    const uint64_t cols  = view->meta.v3d.cols;
    const uint64_t depth = view->meta.v3d.depth;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, rows, cols, depth, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < rows; ++r) {

            for (uint64_t c = 0; c < cols; ++c) {

                const uint64_t offset =
                    (r * cols + c) * depth;

                #pragma omp simd reduction(+:sink)
                for (uint64_t d = 0; d < depth; ++d) {

                    double x = pool[offset + d];

                    x = x * 1.000001 + 0.999999;
                    x = x * 0.999999 + 1.000001;
                    x = x * 1.000003 + 0.999997;
                    x = x * 0.999997 + 1.000003;
                    x = x * 1.000007 + 0.999993;
                    x = x * 0.999993 + 1.000007;

                    sink += x;
                }
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_AOS
 * -------------------------------------------------------------------------- */

void kernel_aos_simd_compute(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aos.num_elements;

    const uint64_t struct_size =
        view->meta.aos.struct_size;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, struct_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t i = 0; i < num_elems; ++i) {

            const uint64_t base =
                i * struct_size;

            #pragma omp simd reduction(+:sink)
            for (uint64_t f = 0; f < struct_size; ++f) {

                double x = pool[base + f];

                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;

                sink += x;
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_SOA
 * -------------------------------------------------------------------------- */

void kernel_soa_simd_compute(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.soa.num_elements;

    const uint64_t num_fields =
        view->meta.soa.num_fields;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_elems, num_fields, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t f = 0; f < num_fields; ++f) {

            const uint64_t field_offset =
                f * num_elems;

            #pragma omp simd reduction(+:sink)
            for (uint64_t i = 0; i < num_elems; ++i) {

                double x =
                    pool[field_offset + i];

                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;

                sink += x;
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_AOSOA
 * -------------------------------------------------------------------------- */

void kernel_aosoa_simd_compute(DataView *view,
                               const TestCase *test_case,
                               Profiler *profiler,
                               PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aosoa.num_elements;

    const uint64_t num_fields =
        view->meta.aosoa.num_fields;

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    const uint64_t num_tiles =
        (vl > 0) ? (num_elems / vl) : 0;

    const uint64_t tile_size =
        num_fields * vl;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(pool, num_tiles, num_fields, vl, tile_size, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t t = 0; t < num_tiles; ++t) {

            const uint64_t tile_base =
                t * tile_size;

            for (uint64_t f = 0; f < num_fields; ++f) {

                const uint64_t field_base =
                    tile_base + f * vl;

                #pragma omp simd reduction(+:sink)
                for (uint64_t v = 0; v < vl; ++v) {

                    double x =
                        pool[field_base + v];

                    x = x * 1.000001 + 0.999999;
                    x = x * 0.999999 + 1.000001;
                    x = x * 1.000003 + 0.999997;
                    x = x * 0.999997 + 1.000003;
                    x = x * 1.000007 + 0.999993;
                    x = x * 0.999993 + 1.000007;

                    sink += x;
                }
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


/* --------------------------------------------------------------------------
 * VIEW_CSR
 * -------------------------------------------------------------------------- */

void kernel_csr_simd_compute(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const uint64_t nrows =
        view->meta.csr.nrows;

    const uint64_t *restrict row_ptr =
        view->meta.csr.row_ptr;

    const uint64_t *restrict col_ind =
        view->meta.csr.col_ind;

    const double *restrict val =
        view->buffer->pool;

    double sink = 0.0;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) default(none) \
        shared(nrows, row_ptr, col_ind, val, chunk) \
        reduction(+:sink)
    {
        #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
        for (uint64_t r = 0; r < nrows; ++r) {

            const uint64_t start = row_ptr[r];
            const uint64_t end   = row_ptr[r + 1];

            #pragma omp simd reduction(+:sink)
            for (uint64_t idx = start; idx < end; ++idx) {

                const uint64_t col =
                    col_ind[idx];

                double x =
                    val[idx] * val[col];

                x = x * 1.000001 + 0.999999;
                x = x * 0.999999 + 1.000001;
                x = x * 1.000003 + 0.999997;
                x = x * 0.999997 + 1.000003;
                x = x * 1.000007 + 0.999993;
                x = x * 0.999993 + 1.000007;

                sink += x;
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sink;
    (void)result;
}


#endif /* OPENMP_HAS_SIMD */
