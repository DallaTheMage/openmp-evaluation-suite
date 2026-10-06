#include "kernels/tasking/taskloop.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_TASKLOOP


/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

void kernel_2d_taskloop(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows = view->meta.v2d.rows;
    const uint64_t cols = view->meta.v2d.cols;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(chunk)
            for (uint64_t r = 0; r < rows; ++r) {

                const uint64_t offset =
                    r * cols;

                for (uint64_t c = 0; c < cols; ++c) {
                    pool[offset + c] =
                        pool[offset + c] * 2.0 + 1.0;
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

void kernel_3d_taskloop(DataView *view,
                        const TestCase *test_case,
                        Profiler *profiler,
                        PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t rows  = view->meta.v3d.height;
    const uint64_t cols  = view->meta.v3d.cols;
    const uint64_t depth = view->meta.v3d.depth;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(chunk)
            for (uint64_t r = 0; r < rows; ++r) {

                for (uint64_t c = 0; c < cols; ++c) {

                    const uint64_t offset =
                        (r * cols + c) * depth;

                    for (uint64_t d = 0; d < depth; ++d) {
                        pool[offset + d] =
                            pool[offset + d] * 2.0 + 1.0;
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

void kernel_aos_taskloop(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aos.num_structs;

    const uint64_t struct_size =
        view->meta.aos.struct_size;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(chunk)
            for (uint64_t i = 0; i < num_elems; ++i) {

                const uint64_t base =
                    i * struct_size;

                for (uint64_t f = 0; f < struct_size; ++f) {
                    pool[base + f] =
                        pool[base + f] * 2.0 + 1.0;
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */

void kernel_soa_taskloop(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.soa.field_length;

    const uint64_t num_fields =
        view->meta.soa.num_fields;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(chunk)
            for (uint64_t f = 0; f < num_fields; ++f) {

                const uint64_t field_offset =
                    f * num_elems;

                for (uint64_t i = 0; i < num_elems; ++i) {
                    pool[field_offset + i] =
                        pool[field_offset + i] * 2.0 + 1.0;
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */

void kernel_aosoa_taskloop(DataView *view,
                           const TestCase *test_case,
                           Profiler *profiler,
                           PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    double *restrict pool = view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aosoa.num_blocks;

    const uint64_t num_fields =
        view->meta.aosoa.num_fields;

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    const uint64_t num_tiles = num_elems;

    const uint64_t tile_size =
        num_fields * vl;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(chunk)
            for (uint64_t t = 0; t < num_tiles; ++t) {

                const uint64_t tile_base =
                    t * tile_size;

                for (uint64_t f = 0; f < num_fields; ++f) {

                    const uint64_t field_base =
                        tile_base + f * vl;

                    for (uint64_t v = 0; v < vl; ++v) {
                        pool[field_base + v] =
                            pool[field_base + v] * 2.0 + 1.0;
                    }
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}

#endif /* OPENMP_HAS_TASKLOOP */
