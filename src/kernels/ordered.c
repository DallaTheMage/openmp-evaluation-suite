#include "kernels/ordered.h"
#include "profiling/Profiler.h"

#if OPENMP_HAS_2_0

void kernel_2d_ordered(DataView *view,
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

    double ordered_sum = 0.0;

    profiler_start(profiler);

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        ordered \
        default(none) \
        shared(pool, rows, cols, ordered_sum)
    for (uint64_t r = 0; r < rows; ++r) {

        const uint64_t offset =
            r * cols;

        double local_sum = 0.0;

        for (uint64_t c = 0; c < cols; ++c) {

            const double value =
                pool[offset + c];

            pool[offset + c] =
                value * 2.0 + 1.0;

            local_sum += value;
        }

        /*
         * Questa regione deve essere eseguita nell'ordine
         * delle iterazioni del loop.
         */
        #pragma omp ordered
        {
            ordered_sum += local_sum;
        }
    }

    profiler_stop(profiler, metric);
}

#endif
