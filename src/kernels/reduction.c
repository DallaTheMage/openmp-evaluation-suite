#include "kernels/reduction/reduction.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_2_0


/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

void kernel_2d_reduction(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const double *restrict pool =
        view->buffer->pool;

    const uint64_t rows =
        view->meta.v2d.rows;

    const uint64_t cols =
        view->meta.v2d.cols;

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t r = 0; r < rows; ++r) {

        const uint64_t offset = r * cols;

        for (uint64_t c = 0; c < cols; ++c) {
            sum += pool[offset + c];
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sum;
    (void)result;
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

void kernel_3d_reduction(DataView *view,
                         const TestCase *test_case,
                         Profiler *profiler,
                         PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const double *restrict pool =
        view->buffer->pool;

    const uint64_t rows =
        view->meta.v3d.height;

    const uint64_t cols =
        view->meta.v3d.cols;

    const uint64_t depth =
        view->meta.v3d.depth;

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t r = 0; r < rows; ++r) {

        for (uint64_t c = 0; c < cols; ++c) {

            const uint64_t offset =
                (r * cols + c) * depth;

            for (uint64_t d = 0; d < depth; ++d) {
                sum += pool[offset + d];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sum;
    (void)result;
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

void kernel_aos_reduction(DataView *view,
                          const TestCase *test_case,
                          Profiler *profiler,
                          PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aos.num_structs;

    const uint64_t struct_size =
        view->meta.aos.struct_size;

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t i = 0; i < num_elems; ++i) {

        const uint64_t base =
            i * struct_size;

        for (uint64_t f = 0; f < struct_size; ++f) {
            sum += pool[base + f];
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sum;
    (void)result;
}


/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */

void kernel_soa_reduction(DataView *view,
                          const TestCase *test_case,
                          Profiler *profiler,
                          PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.soa.field_length;

    const uint64_t num_fields =
        view->meta.soa.num_fields;

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t f = 0; f < num_fields; ++f) {

        const uint64_t field_offset =
            f * num_elems;

        for (uint64_t i = 0; i < num_elems; ++i) {
            sum += pool[field_offset + i];
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sum;
    (void)result;
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */

void kernel_aosoa_reduction(DataView *view,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;

    const double *restrict pool =
        view->buffer->pool;

    const uint64_t num_elems =
        view->meta.aosoa.num_blocks;

    const uint64_t num_fields =
        view->meta.aosoa.num_fields;

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    const uint64_t num_tiles = num_elems;

    const uint64_t tile_size =
        num_fields * vl;

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        reduction(+:sum)
    for (uint64_t t = 0; t < num_tiles; ++t) {

        const uint64_t tile_base =
            t * tile_size;

        for (uint64_t f = 0; f < num_fields; ++f) {

            const uint64_t field_base =
                tile_base + f * vl;

            for (uint64_t v = 0; v < vl; ++v) {
                sum += pool[field_base + v];
            }
        }
    }

    profiler_stop(profiler, metric);

    volatile double result = sum;
    (void)result;
}

#endif /* OPENMP_HAS_2_0 */
