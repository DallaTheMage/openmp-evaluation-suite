#include "kernels/loop.h"

#if OPENMP_HAS_2_0

void kernel_loop_run(
DataView *view,
const TestPoint *point,
Profiler *profiler,
PerformanceMetric *metric
) {
if (!view ||
!view->buffer ||
!point) {

    return;
}

const uint32_t threads = point->num_threads;
const uint64_t chunk   = point->chunk_size;

double *restrict pool =
    view->buffer->pool;

switch (view->type) {

    case VIEW_1D: {
        const uint64_t n =
            view->meta.v1d.dim0;

        if (profiler) {
            profiler_start(profiler);
        }

        #pragma omp parallel num_threads(threads) \
            default(none) shared(pool, n, chunk)
        {
            #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
            for (uint64_t i = 0; i < n; ++i) {
                pool[i] = pool[i] * 2.0 + 1.0;
            }
        }

        if (profiler) {
            profiler_stop(profiler, metric);
        }

        break;
    }

    case VIEW_2D: {
        const uint64_t rows =
            view->meta.v2d.rows;

        const uint64_t cols =
            view->meta.v2d.cols;

        if (profiler) {
            profiler_start(profiler);
        }

        #pragma omp parallel num_threads(threads) \
            default(none) shared(pool, rows, cols, chunk)
        {
            #pragma omp for schedule(CHOSEN_SCHEDULE, chunk)
            for (uint64_t r = 0; r < rows; ++r) {

                const uint64_t offset =
                    r * cols;

                for (uint64_t c = 0; c < cols; ++c) {
                    pool[offset + c] =
                        pool[offset + c] * 2.0 + 1.0;
                }
            }
        }

        if (profiler) {
            profiler_stop(profiler, metric);
        }

        break;
    }

    /* ... altre ViewType ... */

    default:
        break;
}


}

#endif /* OPENMP_HAS_2_0 */