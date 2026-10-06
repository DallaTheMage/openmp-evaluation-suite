#include "kernels/synchronization/master_masked.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_2_0

void kernel_2d_master(DataView *view,
                      const TestCase *test_case,
                      Profiler *profiler,
                      PerformanceMetric *metric) {

    const uint16_t num_threads =
        test_case->num_threads;

    double *restrict pool =
        view->buffer->pool;

    const uint64_t rows =
        view->meta.v2d.rows;

    const uint64_t cols =
        view->meta.v2d.cols;

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, rows, cols)
    {
        #pragma omp master
        {
            /*
             * Operazione eseguita esclusivamente dal master thread.
             */
            if (rows > 0 && cols > 0) {
                pool[0] =
                    pool[0] * 2.0 + 1.0;
            }
        }
    }

    profiler_stop(profiler, metric);
}

#endif
