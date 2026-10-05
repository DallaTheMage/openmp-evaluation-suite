#include "kernels/critical.h"
#include "profiling/Profiler.h"

#if OPENMP_HAS_2_0


/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

void kernel_2d_critical(DataView *view,
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

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        default(none) shared(pool, rows, cols, sum)
    for (uint64_t r = 0; r < rows; ++r) {

        const uint64_t offset =
            r * cols;

        for (uint64_t c = 0; c < cols; ++c) {

            const double value =
                pool[offset + c];

            #pragma omp critical
            {
                sum += value;
            }
        }
    }

    profiler_stop(profiler, metric);
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

void kernel_3d_critical(DataView *view,
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

    double sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        default(none) \
        shared(pool, rows, cols, depth, sum)
    for (uint64_t r = 0; r < rows; ++r) {

        for (uint64_t c = 0; c < cols; ++c) {

            const uint64_t offset =
                (r * cols + c) * depth;

            for (uint64_t d = 0; d < depth; ++d) {

                const double value =
                    pool[offset + d];

                #pragma omp critical
                {
                    sum += value;
                }
            }
        }
    }

    profiler_stop(profiler, metric);
}


#endif /* OPENMP_HAS_2_0 */
