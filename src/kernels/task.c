#include "kernels/task.h"
#include "profiling/Profiler.h"

#if OPENMP_HAS_3_0


/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

void kernel_2d_task(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t rows =
        view->meta.v2d.rows;

    const uint64_t cols =
        view->meta.v2d.cols;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, rows, cols, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < rows;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < rows)
                        ? begin + chunk
                        : rows;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, cols)
                {
                    for (uint64_t r = begin;
                         r < end;
                         ++r) {

                        const uint64_t offset =
                            r * cols;

                        for (uint64_t c = 0;
                             c < cols;
                             ++c) {

                            pool[offset + c] =
                                pool[offset + c] * 2.0 + 1.0;
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

void kernel_3d_task(DataView *view,
                    const TestCase *test_case,
                    Profiler *profiler,
                    PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t rows =
        view->meta.v3d.rows;

    const uint64_t cols =
        view->meta.v3d.cols;

    const uint64_t depth =
        view->meta.v3d.depth;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, rows, cols, depth, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < rows;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < rows)
                        ? begin + chunk
                        : rows;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, cols, depth)
                {
                    for (uint64_t r = begin;
                         r < end;
                         ++r) {

                        for (uint64_t c = 0;
                             c < cols;
                             ++c) {

                            const uint64_t offset =
                                (r * cols + c) * depth;

                            for (uint64_t d = 0;
                                 d < depth;
                                 ++d) {

                                pool[offset + d] =
                                    pool[offset + d] * 2.0 + 1.0;
                            }
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

void kernel_aos_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aos.num_elements;

    const uint64_t struct_size =
        view->meta.aos.struct_size;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, num_elems, struct_size, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < num_elems;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < num_elems)
                        ? begin + chunk
                        : num_elems;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, struct_size)
                {
                    for (uint64_t i = begin;
                         i < end;
                         ++i) {

                        const uint64_t base =
                            i * struct_size;

                        for (uint64_t f = 0;
                             f < struct_size;
                             ++f) {

                            pool[base + f] =
                                pool[base + f] * 2.0 + 1.0;
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */

void kernel_soa_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.soa.num_elements;

    const uint64_t num_fields =
        view->meta.soa.num_fields;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, num_elems, num_fields, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < num_fields;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < num_fields)
                        ? begin + chunk
                        : num_fields;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, num_elems)
                {
                    for (uint64_t f = begin;
                         f < end;
                         ++f) {

                        const uint64_t offset =
                            f * num_elems;

                        for (uint64_t i = 0;
                             i < num_elems;
                             ++i) {

                            pool[offset + i] =
                                pool[offset + i] * 2.0 + 1.0;
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */

void kernel_aosoa_task(DataView *view,
                       const TestCase *test_case,
                       Profiler *profiler,
                       PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aosoa.num_elements;

    const uint64_t num_fields =
        view->meta.aosoa.num_fields;

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    if (vl == 0) {
        return;
    }

    const uint64_t num_tiles =
        num_elems / vl;

    const uint64_t tile_size =
        num_fields * vl;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) \
        shared(pool, num_tiles, num_fields, vl, tile_size, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < num_tiles;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < num_tiles)
                        ? begin + chunk
                        : num_tiles;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, num_fields, vl, tile_size)
                {
                    for (uint64_t t = begin;
                         t < end;
                         ++t) {

                        const uint64_t tile_base =
                            t * tile_size;

                        for (uint64_t f = 0;
                             f < num_fields;
                             ++f) {

                            const uint64_t field_base =
                                tile_base + f * vl;

                            for (uint64_t v = 0;
                                 v < vl;
                                 ++v) {

                                pool[field_base + v] =
                                    pool[field_base + v] * 2.0 + 1.0;
                            }
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_CSR
 * ========================================================================== */

void kernel_csr_task(DataView *view,
                     const TestCase *test_case,
                     Profiler *profiler,
                     PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t nrows =
        view->meta.csr.nrows;

    const uint64_t *restrict row_ptr =
        view->meta.csr.row_ptr;

    const uint64_t *restrict col_ind =
        view->meta.csr.col_ind;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, nrows, row_ptr, col_ind, chunk)
    {
        #pragma omp single
        {
            for (uint64_t begin = 0;
                 begin < nrows;
                 begin += chunk) {

                const uint64_t end =
                    (begin + chunk < nrows)
                        ? begin + chunk
                        : nrows;

                #pragma omp task firstprivate(begin, end) \
                    shared(pool, row_ptr, col_ind)
                {
                    for (uint64_t r = begin;
                         r < end;
                         ++r) {

                        const uint64_t start =
                            row_ptr[r];

                        const uint64_t row_end =
                            row_ptr[r + 1];

                        for (uint64_t idx = start;
                             idx < row_end;
                             ++idx) {

                            const uint64_t col =
                                col_ind[idx];

                            pool[idx] =
                                pool[idx] * 2.0 + pool[col];
                        }
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


#endif /* OPENMP_HAS_3_0 */
